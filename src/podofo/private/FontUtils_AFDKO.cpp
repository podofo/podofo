/**
 * SPDX-FileCopyrightText: (C) 2024 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "PdfDeclarationsPrivate.h"
#include "FontUtils.h"
#include <afdko/include/cffwrite.h>
#include <afdko/include/t1read.h>
#include <afdko/include/cffread.h>

#include <podofo/private/FreetypePrivate.h>

 // The following functions include software developed by
 // the Adobe Font Development Kit for OpenType (https://github.com/adobe-type-tools/afdko)
 // License: https://github.com/adobe-type-tools/afdko?tab=License-1-ov-file#readme

using namespace std;
using namespace PoDoFo;

#define sig_PostScript0 CTL_TAG('%', '!', 0x00, 0x00)
#define sig_PostScript1 CTL_TAG('%', 'A', 0x00, 0x00) // %ADO...
#define sig_PostScript2 CTL_TAG('%', '%', 0x00, 0x00) // %%...
#define sig_PFB ((ctlTag)0x80010000)
#define sig_CFF ((ctlTag)0x01000000)
#define sfr_OTTO_tag CTL_TAG('O', 'T', 'T', 'O') // OTF

namespace
{
    enum class StreamType : uint8_t
    {
        ReadBuffer = 1,
        ReadWriteButter = 2,
        AppendBuffer = 3,
    };

    struct AppendBuffer
    {
        StreamType type = StreamType::AppendBuffer;
        charbuff* buff = nullptr;
    };

    struct ReadBuffer
    {
        StreamType type = StreamType::ReadBuffer;
        bufferview buff;
        size_t pos = 0;
    };

    struct ReadWriteBuffer
    {
        StreamType type = StreamType::ReadWriteButter;
        bool eof = false;
        charbuff* buff = nullptr;
        size_t pos = 0;
        char rtmp[BUFSIZ]; // Read cache
    };

    typedef struct ConvCtx* ConvCtxPtr;

    typedef size_t(*SegRefillFunc)(ConvCtxPtr h, char** ptr);

    enum GlyphSelector
    {
        sel_by_tag,
        sel_by_cid,
    };

    typedef void(*SubsetCallback)(ConvCtxPtr h, GlyphSelector type, unsigned short id);

    struct GlyphSubsetCtx
    {
        // The original callbacks
        void (*width)(abfGlyphCallbacks* cb, float hAdv) = nullptr;
        int (*beg)(abfGlyphCallbacks* cb, abfGlyphInfo* info);

        vector<abfGlyphInfo> glyphInfos;
        unsigned short cid = 0;
        float hAdv = 0;
    };

    struct ConvCtx
    {
        ConvCtx(const bufferview& src, charbuff& dst);
        ConvCtx(const bufferview& src, charbuff& dst,
            const cspan<PdfCharGIDInfo>& subsetInfos,
            const PdfFontMetrics& metrics,
            const PdfCIDSystemInfo& cidInfo);
        ~ConvCtx();

        abfTopDict* top = nullptr;        // Top dictionary
        ReadBuffer src;                   // Src data
        struct // Destination data
        {
            AppendBuffer stm;
            void (*begset)(ConvCtxPtr h) = nullptr;
            void (*begfont)(ConvCtxPtr h, abfTopDict* top) = nullptr;
            void (*endfont)(ConvCtxPtr h) = nullptr;
            void (*endset)(ConvCtxPtr h) = nullptr;
        } dst;
        const PdfFontMetrics* metrics = nullptr;
        cspan<PdfCharGIDInfo> subsetInfos;
        const PdfCIDSystemInfo* cidInfo = nullptr;
        GlyphSubsetCtx subsetCtx;
        struct // Font data segment
        {
            SegRefillFunc refill = nullptr; // Format-specific refill
            size_t left = 0;          // Bytes remaining in segment
        } seg;
        struct // t1read library
        {
            t1rCtx ctx{ };
            ReadWriteBuffer tmp;
            charbuff buff;

        } t1r;
        struct // cffread library
        {
            cfrCtx ctx{ };
            ReadWriteBuffer tmp;
            charbuff buff;
            long flags = 0;
        } cfr;
        struct // cffwrite library
        {
            cfwCtx ctx{ };
            ReadWriteBuffer tmp;
            charbuff buff;
        } cfw;
        struct // Callbacks
        {
            ctlMemoryCallbacks mem{ };
            ctlStreamCallbacks stm{ };
            abfGlyphCallbacks glyph{ };
        } cb;
    };
}

static unsigned char read1(ConvCtxPtr h)
{
    if (h->src.pos == h->src.buff.size())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Unexpected end of file while reading font");

    unsigned char ret = (unsigned char)*(h->src.buff.data() + h->src.pos);
    h->src.pos++;
    return ret;
}

static size_t PFBRefill(ConvCtxPtr h, char** ptr)
{
    while (h->seg.left == 0)
    {
        // New segment; read segment header
        int escape = read1(h);
        int type = read1(h);

        // Check segment header
        if (escape != 128 || (type != 1 && type != 2 && type != 3))
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Bad PFB segment type");

        if (type == 3)
        {
            // EOF
            *ptr = nullptr;
            return 0;
        }
        else
        {
            // Read segment length (little endian)
            h->seg.left = read1(h);
            h->seg.left |= (size_t)read1(h) << 8;
            h->seg.left |= (size_t)read1(h) << 16;
            h->seg.left |= (size_t)read1(h) << 24;
        }
    }

    *ptr = const_cast<char*>(h->src.buff.data()) + h->src.pos;
    size_t srcleft = h->src.buff.size() - h->src.pos;
    if (srcleft <= h->seg.left)
    {
        // Return full buffer
        h->seg.left -= srcleft;
        h->src.pos += h->src.buff.size();
    }
    else
    {
        // Return partial buffer
        srcleft = h->seg.left;
        h->src.pos += h->seg.left;
        h->seg.left = 0;
    }

    return srcleft;
}

// Begin font set.
static void cff_BegSet(ConvCtxPtr h)
{
    if (cfwBegSet(h->cfw.ctx, CFW_PRESERVE_GLYPH_ORDER))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "cff_BegSet");
}

// Begin font.
static void cff_BegFont(ConvCtxPtr h, abfTopDict* top)
{
    (void)top;
    h->cb.glyph = cfwGlyphCallbacks;
    h->cb.glyph.direct_ctx = h->cfw.ctx;

    // This keeps these callbacks from being used when writing a
    // regular CFF, and avoids the overhead of processing the source
    // CFF2 blend args
    h->cb.glyph.moveVF = nullptr;
    h->cb.glyph.lineVF = nullptr;
    h->cb.glyph.curveVF = nullptr;
    h->cb.glyph.stemVF = nullptr;

    if (cfwBegFont(h->cfw.ctx, nullptr, 0))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "cfwBegFont");
}

// End font.
static void cff_EndFont(ConvCtxPtr h)
{
    if (cfwEndFont(h->cfw.ctx, h->top))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "cfwEndFont");
}

// End font set.
static void cff_EndSet(ConvCtxPtr h)
{
    if (cfwEndSet(h->cfw.ctx))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "cfwEndSet");
}

// Setup cff mode.
static void setModeCFF(ConvCtxPtr h)
{
    // Initialize control data

    // Set library functions
    h->dst.begset = cff_BegSet;
    h->dst.begfont = cff_BegFont;
    h->dst.endfont = cff_EndFont;
    h->dst.endset = cff_EndSet;

    if (h->cfw.ctx == nullptr)
    {
        // Create library context
        h->cfw.ctx = cfwNew(&h->cb.mem, &h->cb.stm, CFW_CHECK_ARGS);
        if (h->cfw.ctx == nullptr)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "cfw: can't init lib");
    }

    // The default callbacks. These get reset in cff_BegFont() and
    // cff_EndFont() as some options play the font data through a different
    // library on a first pass, before writing to cff on a second pass
    h->cb.glyph = cfwGlyphCallbacks;
    h->cb.glyph.direct_ctx = h->cfw.ctx;

    // This keeps these callbacks from being used when writing a regular
    // CFF, and avoids the overhead of processing the source CFF2 blend
    // args
    h->cb.glyph.moveVF = nullptr;
    h->cb.glyph.lineVF = nullptr;
    h->cb.glyph.curveVF = nullptr;
    h->cb.glyph.stemVF = nullptr;
}

static int subsetBegCallback(abfGlyphCallbacks* cb, abfGlyphInfo* info)
{
    auto ctx = (GlyphSubsetCtx*)cb->indirect_ctx;

    // Substitute the glyph info. Force the CID to be incremental
    // and specify the glyph has not been previously inserted
    auto& newinfo = ctx->glyphInfos[ctx->cid];
    newinfo = *info;
    newinfo.cid = ctx->cid;
    newinfo.flags |= ABF_GLYPH_CID;
    newinfo.flags &= ~ABF_GLYPH_SEEN;

    // Remove legacy properties for Type1 fonts
    newinfo.gname.ptr = { };
    newinfo.encoding = { };

    return ctx->beg(cb, &newinfo);
}

static void subsetWidthCallback(abfGlyphCallbacks* cb, float hAdv)
{
    (void)hAdv;
    auto ctx = (GlyphSubsetCtx*)cb->indirect_ctx;
    // Override the width. Must be set outside
    ctx->width(cb, ctx->hAdv);
}

// Filter glyphs using the glyph list parameter.
static void doSubset(ConvCtxPtr h, SubsetCallback callback)
{
    // Enforce CID font
    h->top->sup.flags |= ABF_CID_FONT;
    h->top->cid.Registry.ptr = const_cast<char*>(h->cidInfo->Registry.GetString().data());
    h->top->cid.Ordering.ptr = const_cast<char*>(h->cidInfo->Ordering.GetString().data());
    h->top->cid.Supplement = h->cidInfo->Supplement;

    // Override some callbacks with our implementation
    h->subsetCtx.beg = h->cb.glyph.beg;
    h->subsetCtx.width = h->cb.glyph.width;
    h->cb.glyph.beg = subsetBegCallback;
    h->cb.glyph.width = subsetWidthCallback;
    h->cb.glyph.indirect_ctx = &h->subsetCtx;

    // Prepare afdko glyph infos that will be substituted in subsetBegCallback
    h->subsetCtx.glyphInfos.resize(h->subsetInfos.size() +  1);

    GlyphSelector selector = h->metrics->GetFontFileType() == PdfFontFileType::CIDKeyedCFF
        ? GlyphSelector::sel_by_cid : GlyphSelector::sel_by_tag;

    // Ensure the first glyph is always the first one
    h->subsetCtx.cid = 0;
    h->subsetCtx.hAdv = (float)(h->metrics->GetGlyphWidth(0) * h->top->sup.UnitsPerEm);
    callback(h, selector, 0);

    for (unsigned i = 0; i < h->subsetInfos.size(); i++)
    {
        auto& info = h->subsetInfos[i];
        // Compute the overridden width and set it to the context
        h->subsetCtx.cid = (unsigned short)(i + 1);
        h->subsetCtx.hAdv = (float)(h->metrics->GetGlyphWidth(info.Gid.MetricsId) * h->top->sup.UnitsPerEm);
        callback(h, selector, (unsigned short)info.Gid.Id);
    }
}

static void callbackGlyphT1(ConvCtxPtr h, GlyphSelector type, unsigned short id)
{
    switch (type)
    {
        case sel_by_tag:
            (void)t1rGetGlyphByTag(h->t1r.ctx, id, &h->cb.glyph);
            break;
        case sel_by_cid:
            (void)t1rGetGlyphByCID(h->t1r.ctx, id, &h->cb.glyph);
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

// Read font with t1read library.
static void t1rReadFont(ConvCtxPtr h, long origin)
{
    if (h->t1r.ctx == nullptr)
    {
        // Initialize library
        h->t1r.ctx = t1rNew(&h->cb.mem, &h->cb.stm, T1R_CHECK_ARGS);
        if (h->t1r.ctx == nullptr)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "t1r: can't init lib");
    }

    if (t1rBegFont(h->t1r.ctx, 0, origin, &h->top, nullptr))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "t1r: t1rBegFont");

    h->dst.begfont(h, h->top);

    if (h->subsetInfos.size() == 0)
    {
        if (t1rIterateGlyphs(h->t1r.ctx, &h->cb.glyph))
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "t1r: t1rIterateGlyphs");
    }
    else
    {
        doSubset(h, callbackGlyphT1);
    }

    h->dst.endfont(h);

    if (t1rEndFont(h->t1r.ctx))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "t1r: t1rEndFont");
}

static void callbackGlyphCFF(ConvCtxPtr h, GlyphSelector type, unsigned short id)
{
    switch (type)
    {
        case sel_by_tag:
            (void)cfrGetGlyphByTag(h->cfr.ctx, id, &h->cb.glyph);
            break;
        case sel_by_cid:
            (void)cfrGetGlyphByCID(h->cfr.ctx, id, &h->cb.glyph);
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

// Read font with cffread library.
static void cfrReadFont(ConvCtxPtr h, long origin, int ttcIndex)
{
    if (h->cfr.ctx == NULL)
    {
        h->cfr.ctx = cfrNew(&h->cb.mem, &h->cb.stm, CFR_CHECK_ARGS);
        if (h->cfr.ctx == NULL)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "cfr: can't init lib");
    }

    // Convert seac for subsets
    long flags = h->subsetInfos.size() == 0 ? h->cfr.flags : h->cfr.flags | CFR_UPDATE_OPS;
    if (cfrBegFont(h->cfr.ctx, flags, origin, ttcIndex, &h->top, nullptr))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "cfr: cfrBegFont");

    h->dst.begfont(h, h->top);

    if (h->subsetInfos.size() == 0)
    {
        if (cfrIterateGlyphs(h->cfr.ctx, &h->cb.glyph))
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "cfr: cfrIterateGlyphs");
    }
    else
    {
        doSubset(h, callbackGlyphCFF);
    }

    h->dst.endfont(h);

    if (cfrEndFont(h->cfr.ctx))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "cfr: cfrEndFont");
}

// Manage memory
static void* mem_manage(ctlMemoryCallbacks* cb, void* old, size_t size)
{
    (void)cb;
    if (size > 0)
    {
        if (old == nullptr)
            return std::malloc(size); // size != 0, old == nullptr
        else
            return std::realloc(old, size); // size != 0, old != nullptr
    }
    else
    {
        if (old == nullptr)
            return nullptr; // size == 0, old == nullptr
        else
        {
            std::free(old); // size == 0, old != nullptr
            return nullptr;
        }
    }
}

static void* stm_open(ctlStreamCallbacks* cb, int id, size_t size)
{
    (void)size;
    ConvCtxPtr h = (ConvCtxPtr)cb->direct_ctx;
    switch (id)
    {
        case T1R_SRC_STREAM_ID:
            return &h->src;
        case CFR_SRC_STREAM_ID:
            return &h->src;
        case CFW_DST_STREAM_ID:
            return &h->dst.stm;
        case T1R_TMP_STREAM_ID:
            return &h->t1r.tmp;
        case CFW_TMP_STREAM_ID:
            return &h->cfw.tmp;
        case T1R_DBG_STREAM_ID:
        case CFR_DBG_STREAM_ID:
        case CFW_DBG_STREAM_ID:
            // Return null stream
            return nullptr;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

// Seek to stream position
static int stm_seek(ctlStreamCallbacks* cb, void* stream_, long offset)
{
    (void)cb;
    if (offset < 0)
    {
        // Ignored negative offset
        // https://github.com/adobe-type-tools/afdko/blob/0b588588a46e2e107cd5f93d9a6e80caab52c58e/c/shared/source/tx_shared/tx_shared.c#L365
        return -1;
    }

    auto type = *(StreamType*)stream_;
    switch (type)
    {
        case StreamType::ReadBuffer:
        {
            auto& stream = *(ReadBuffer*)stream_;
            if ((size_t)offset > stream.buff.size())
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Invalid seek out of bounds");

            stream.pos = (size_t)offset;
            return 0;
        }
        case StreamType::ReadWriteButter:
        {
            auto& stream = *(ReadWriteBuffer*)stream_;
            if ((size_t)offset > stream.buff->size())
                stream.buff->resize((size_t)offset);

            stream.pos = (size_t)offset;
            stream.eof = false;
            return 0;
        }
        case StreamType::AppendBuffer:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unsupported seek");
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

// Return stream position
static long stm_tell(ctlStreamCallbacks* cb, void* stream_)
{
    (void)cb;
    auto type = *(StreamType*)stream_;
    switch (type)
    {
        case StreamType::ReadBuffer:
        {
            auto& stream = *(ReadBuffer*)stream_;
            return (long)stream.pos;
        }
        case StreamType::ReadWriteButter:
        {
            auto& stream = *(ReadWriteBuffer*)stream_;
            return (long)stream.pos;
        }
        case StreamType::AppendBuffer:
        {
            auto& stream = *(AppendBuffer*)stream_;
            return (long)stream.buff->size();
        }
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

// Read from stream
static size_t stm_read(ctlStreamCallbacks* cb, void* stream_, char** ptr)
{
    (void)cb;
    auto type = *(StreamType*)stream_;
    switch (type)
    {
        case StreamType::ReadBuffer:
        {
            auto& stream = *(ReadBuffer*)stream_;
            ConvCtxPtr h = (ConvCtxPtr)cb->direct_ctx;
            if (h->seg.refill != nullptr)
                return h->seg.refill(h, ptr);

            size_t readCount = stream.buff.size() - stream.pos;
            *ptr = const_cast<char*>(stream.buff.data()) + stream.pos;
            stream.pos = stream.buff.size(); // Just put it at the end of the buffer
            return readCount;
        }
        case StreamType::ReadWriteButter:
        {
            auto& stream = *(ReadWriteBuffer*)stream_;
            *ptr = stream.rtmp;
            if (stream.eof)
                return 0;

            size_t readCount = std::min((size_t)sizeof(stream.rtmp), stream.buff->size() - stream.pos);
            std::memcpy(stream.rtmp, stream.buff->data() + stream.pos, readCount);
            stream.pos += readCount;
            if (stream.pos == stream.buff->size())
                stream.eof = true;

            return readCount;
        }
        case StreamType::AppendBuffer:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unsupported read");
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

// Write to stream
static size_t stm_write(ctlStreamCallbacks* cb, void* stream_,
    size_t count, char* ptr)
{
    (void)cb;
    auto type = *(StreamType*)stream_;
    switch (type)
    {
        case StreamType::ReadWriteButter:
        {
            auto& stream = *(ReadWriteBuffer*)stream_;
            if (stream.pos + count > stream.buff->size())
                stream.buff->resize(stream.pos + count);

            std::memcpy(stream.buff->data() + stream.pos, ptr, count);
            stream.pos += count;
            stream.eof = false;
            return count;
        }
        case StreamType::AppendBuffer:
        {
            auto& stream = *(AppendBuffer*)stream_;
            stream.buff->append(ptr, count);
            return count;
        }
        case StreamType::ReadBuffer:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unsupported write");
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

// Return stream status
static int stm_status(ctlStreamCallbacks* cb, void* stream_)
{
    (void)cb;
    auto type = *(StreamType*)stream_;
    switch (type)
    {
        case StreamType::ReadBuffer:
        {
            auto& stream = *(ReadBuffer*)stream_;
            return stream.pos == stream.buff.size() ? CTL_STREAM_END : CTL_STREAM_OK;
        }
        case StreamType::AppendBuffer:
        {
            return CTL_STREAM_OK;
        }
        case StreamType::ReadWriteButter:
        {
            auto& stream = *(ReadWriteBuffer*)stream_;
            return (int)stream.eof;
        }
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

// Close stream
static int stm_close(ctlStreamCallbacks* cb, void* stream)
{
    (void)cb;
    (void)stream;
    // Nothing to do
    return 0;
}

ConvCtx::ConvCtx(const bufferview& src, charbuff& dst, const cspan<PdfCharGIDInfo>& subsetInfos,
    const PdfFontMetrics& metrics, const PdfCIDSystemInfo& cidInfo) : ConvCtx(src, dst)
{
    this->subsetInfos = subsetInfos;
    this->metrics = &metrics;
    this->cidInfo = &cidInfo;
}

ConvCtx::ConvCtx(const bufferview& src, charbuff& dst)
{
    cb.mem.ctx = this;
    cb.mem.manage = mem_manage;

    cb.stm.direct_ctx = this;
    cb.stm.open = stm_open;
    cb.stm.seek = stm_seek;
    cb.stm.tell = stm_tell;
    cb.stm.read = stm_read;
    cb.stm.write = stm_write;
    cb.stm.status = stm_status;
    cb.stm.close = stm_close;

    t1r.tmp.buff = &t1r.buff;
    cfw.tmp.buff = &cfw.buff;

    this->src.buff = src;
    this->dst.stm.buff = &dst;
}

ConvCtx::~ConvCtx()
{
    t1rFree(t1r.ctx);
    cfwFree(cfw.ctx);
}

static void doConversion(ConvCtxPtr h)
{
    ctlTag sig;

    // Initialize segment
    h->seg.refill = nullptr;

    // Make 2-byte signature
    sig = (ctlTag)read1(h) << 24;
    sig |= (ctlTag)read1(h) << 16;

    switch (sig)
    {
        case sig_PFB:
        {
            // PFB files have segment interleaving
            h->seg.refill = PFBRefill;
            h->seg.left = 0;
            goto Type1;
        }
        case sig_PostScript0:
        case sig_PostScript1:
        case sig_PostScript2:
        {
        Type1:
            // Reset source position
            h->src.pos = 0;
            t1rReadFont(h, 0);
            break;
        }
        case sig_CFF:
        {
            if (read1(h) != 0x04) // header size
                goto Unsupported;

        CFF:
            // Reset source position
            h->src.pos = 0;
            cfrReadFont(h, 0, 0);
            break;
        }
        default:
        {
            // Make a 4-byte signature
            sig |= read1(h) << 8;
            sig |= read1(h);

            if (sig == sfr_OTTO_tag)
            {
                h->cfr.flags |= CFR_NO_ENCODING;
                goto CFF;
            }

        Unsupported:
            PODOFO_RAISE_ERROR(PdfErrorCode::UnsupportedFontFormat);
        }
    }
}

void PoDoFo::ConvertFontType1ToCFF(const bufferview& src, charbuff& dst)
{
    ConvCtx ctx(src, dst);
    setModeCFF(&ctx);

    ctx.dst.begset(&ctx);
    doConversion(&ctx);
    ctx.dst.endset(&ctx);
}

void PoDoFo::SubsetFontCFF(const PdfFontMetrics& metrics, const cspan<PdfCharGIDInfo>& subsetInfos,
    const PdfCIDSystemInfo& cidInfo, charbuff& dst)
{
    PODOFO_ASSERT(metrics.GetFontFileType() == PdfFontFileType::Type1CFF
        || metrics.GetFontFileType() == PdfFontFileType::CIDKeyedCFF
        || metrics.GetFontFileType() == PdfFontFileType::OpenTypeCFF);

    ConvCtx ctx(metrics.GetOrLoadFontFileData(), dst, subsetInfos, metrics, cidInfo);
    setModeCFF(&ctx);

    ctx.dst.begset(&ctx);
    doConversion(&ctx);
    ctx.dst.endset(&ctx);
}

// Configure AFDKO error handler
extern "C"
{
    struct _t_Exc_buf;

// NOTE: Ignore MSVC warning C4297 ("function assumed not to throw
// an exception but does") because of extern "C": we will catch
// the exception in a C++ method
#pragma warning(push)
#pragma warning(disable: 4297)
    void os_raise(_t_Exc_buf* buf, int code, char* msg)
    {
        (void)buf;
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "AFDKO error {}: {}", code, msg);
    }
#pragma warning(pop)
}

/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include "PdfDeclarationsPrivate.h"
#include "ImageUtils.h"

using namespace std;
using namespace PoDoFo;

#ifdef PODOFO_IS_LITTLE_ENDIAN
#define FETCH_BIT(bytes, idx) ((bytes[idx / 8] >> (7 - (idx % 8))) & 1)
#else // PODOFO_IS_BIG_ENDIAN
#define FETCH_BIT(bytes, idx) ((bytes[idx / 8] >> (idx % 8)) & 1)
#endif

template <int bpp>
static void fetchScanLineRGB(unsigned char* dstScanLine, PdfPixelFormat format,
    const unsigned char* srcScanLine, unsigned width);
template <int bpp>
static void fetchScanLineRGB(unsigned char* dstScanLine, PdfPixelFormat format,
    const unsigned char* srcScanLine, unsigned width, const unsigned char* srcAphaLine);
static void fetchScanLineGrayScale(unsigned char* dstScanLine, PdfPixelFormat format,
    const unsigned char* srcScanLine, unsigned width);
static void fetchScanLineGrayScale(unsigned char* dstScanLine, PdfPixelFormat format,
    const unsigned char* srcScanLine, unsigned width,
    const unsigned char* srcAphaLine);
static void fetchScanLineBW(unsigned char* dstScanLine, PdfPixelFormat format,
    const unsigned char* srcScanLine, unsigned width);
static void fetchScanLineBW(unsigned char* dstScanLine, PdfPixelFormat format,
    const unsigned char* srcScanLine, unsigned width,
    const unsigned char* srcAphaLine);

static charbuff initScanLine(PdfPixelFormat format, unsigned width, int scanLineSizeHint);

void utls::FetchImage(OutputStream& stream, PdfPixelFormat format, int scanLineSize,
    const unsigned char* imageData, unsigned width, unsigned heigth, unsigned bitsPerComponent,
    const PdfColorSpaceFilter& map, const charbuff& smaskData)
{
    // TODO: Add support for non-trivial /BitsPerComponent. This could be done
    // by keeping existing optimized fecthScanLine* methods and add other overloads
    // that take bitsPerComponent as an argument
    if (bitsPerComponent != 8)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Unsupported /BitsPerComponent");

    charbuff scanLine = initScanLine(format, width, scanLineSize);
    if (map.IsRawEncoded())
    {
        switch (map.GetPixelFormat())
        {
            case PdfColorSpacePixelFormat::Grayscale:
            {
                unsigned srcScanLineSize = width;
                if (smaskData.size() == 0)
                {
                    for (unsigned i = 0; i < heigth; i++)
                    {
                        fetchScanLineGrayScale((unsigned char*)scanLine.data(),
                            format, imageData + i * srcScanLineSize, width);
                        stream.Write(scanLine.data(), scanLine.size());
                    }
                }
                else
                {
                    for (unsigned i = 0; i < heigth; i++)
                    {
                        fetchScanLineGrayScale((unsigned char*)scanLine.data(),
                            format, imageData + i * srcScanLineSize, width,
                            (const unsigned char*)smaskData.data() + i * width);
                        stream.Write(scanLine.data(), scanLine.size());
                    }
                }
                break;
            }
            case PdfColorSpacePixelFormat::RGB:
            {
                unsigned srcScanLineSize = width * 3;
                if (smaskData.size() == 0)
                {
                    for (unsigned i = 0; i < heigth; i++)
                    {
                        fetchScanLineRGB<3>((unsigned char*)scanLine.data(),
                            format, imageData + i * srcScanLineSize, width);
                        stream.Write(scanLine.data(), scanLine.size());
                    }
                }
                else
                {
                    for (unsigned i = 0; i < heigth; i++)
                    {
                        fetchScanLineRGB<3>((unsigned char*)scanLine.data(),
                            format, imageData + i * srcScanLineSize, width,
                            (const unsigned char*)smaskData.data() + i * width);
                        stream.Write(scanLine.data(), scanLine.size());
                    }
                }
                break;
            }
            default:
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Unsupported color space pixel output format");
        }
    }
    else
    {
        charbuff midwaySourceScanLine(map.GetScanLineSize(width, bitsPerComponent));
        unsigned srcScanLineSize = map.GetSourceScanLineSize(width, bitsPerComponent);
        switch (map.GetPixelFormat())
        {
            case PdfColorSpacePixelFormat::Grayscale:
            {
                if (smaskData.size() == 0)
                {
                    for (unsigned i = 0; i < heigth; i++)
                    {
                        map.FetchScanLine((unsigned char*)midwaySourceScanLine.data(),
                            imageData + i * srcScanLineSize, width, bitsPerComponent);
                        fetchScanLineGrayScale((unsigned char*)scanLine.data(),
                            format, (const unsigned char*)midwaySourceScanLine.data(), width);
                        stream.Write(scanLine.data(), scanLine.size());
                    }
                }
                else
                {
                    for (unsigned i = 0; i < heigth; i++)
                    {
                        map.FetchScanLine((unsigned char*)midwaySourceScanLine.data(),
                            imageData + i * srcScanLineSize, width, bitsPerComponent);
                        fetchScanLineGrayScale((unsigned char*)scanLine.data(),
                            format, (unsigned char*)midwaySourceScanLine.data(), width,
                            (const unsigned char*)smaskData.data() + i * width);
                        stream.Write(scanLine.data(), scanLine.size());
                    }
                }
                break;
            }
            case PdfColorSpacePixelFormat::RGB:
            {
                if (smaskData.size() == 0)
                {
                    for (unsigned i = 0; i < heigth; i++)
                    {
                        map.FetchScanLine((unsigned char*)midwaySourceScanLine.data(),
                            imageData + i * srcScanLineSize, width, bitsPerComponent);
                        fetchScanLineRGB<3>((unsigned char*)scanLine.data(),
                            format, (const unsigned char*)midwaySourceScanLine.data(), width);
                        stream.Write(scanLine.data(), scanLine.size());
                    }
                }
                else
                {
                    for (unsigned i = 0; i < heigth; i++)
                    {
                        map.FetchScanLine((unsigned char*)midwaySourceScanLine.data(),
                            imageData + i * srcScanLineSize, width, bitsPerComponent);
                        fetchScanLineRGB<3>((unsigned char*)scanLine.data(),
                            format, (unsigned char*)midwaySourceScanLine.data(), width,
                            (const unsigned char*)smaskData.data() + i * width);
                        stream.Write(scanLine.data(), scanLine.size());
                    }
                }
                break;
            }
            default:
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Unsupported color space pixel output format");
        }
    }
}

void utls::FetchImageCCITT(OutputStream& stream, PdfPixelFormat format, int scanLineSize,
    fxcodec::ScanlineDecoder& decoder, unsigned width, unsigned heigth, const charbuff& smaskData)
{
    charbuff scanLine = initScanLine(format, width, scanLineSize);

    if (smaskData.size() == 0)
    {
        for (unsigned i = 0; i < heigth; i++)
        {
            auto scanLineBW = decoder.GetScanline(i);
            fetchScanLineBW((unsigned char*)scanLine.data(),
                format, (const unsigned char*)scanLineBW.data(), width);
            stream.Write(scanLine.data(), scanLine.size());
        }
    }
    else
    {
        for (unsigned i = 0; i < heigth; i++)
        {
            auto scanLineBW = decoder.GetScanline(i);
            fetchScanLineBW((unsigned char*)scanLine.data(),
                format, scanLineBW.data(), width,
                (const unsigned char*)smaskData.data() + i * width);
            stream.Write(scanLine.data(), scanLine.size());
        }
    }
}

#ifdef PODOFO_HAVE_JPEG_LIB

void utls::FetchImageJPEG(OutputStream& stream, PdfPixelFormat format, int scanLineSize,
    jpeg_decompress_struct* ctx, unsigned width, unsigned heigth, const charbuff& smaskData)
{
    (void)heigth;
    charbuff scanLine = initScanLine(format, width, scanLineSize);

    unsigned rowBytes = (unsigned)(ctx->output_width * ctx->output_components);

    // buffer will be deleted by jpeg_destroy_decompress
    JSAMPARRAY jScanLine = (*ctx->mem->alloc_sarray)(reinterpret_cast<j_common_ptr>(ctx), JPOOL_IMAGE, rowBytes, 1);

    switch (ctx->out_color_space)
    {
        case JCS_RGB:
        {
            if (smaskData.size() == 0)
            {
                for (unsigned i = 0; i < ctx->output_height; i++)
                {
                    jpeg_read_scanlines(ctx, jScanLine, 1);
                    fetchScanLineRGB<3>((unsigned char*)scanLine.data(),
                        format, jScanLine[0], ctx->output_width);
                    stream.Write(scanLine.data(), scanLine.size());
                }
            }
            else
            {
                for (unsigned i = 0; i < ctx->output_height; i++)
                {
                    jpeg_read_scanlines(ctx, jScanLine, 1);
                    fetchScanLineRGB<3>((unsigned char*)scanLine.data(), format,
                        jScanLine[0], ctx->output_width, (const unsigned char*)smaskData.data()
                        + i * ctx->output_width);
                    stream.Write(scanLine.data(), scanLine.size());
                }
            }
            break;
        }
        case JCS_GRAYSCALE:
        {
            if (smaskData.size() == 0)
            {
                for (unsigned i = 0; i < ctx->output_height; i++)
                {
                    jpeg_read_scanlines(ctx, jScanLine, 1);
                    fetchScanLineGrayScale((unsigned char*)scanLine.data(),
                        format, jScanLine[0], ctx->output_width);
                    stream.Write(scanLine.data(), scanLine.size());
                }
            }
            else
            {
                for (unsigned i = 0; i < ctx->output_height; i++)
                {
                    jpeg_read_scanlines(ctx, jScanLine, 1);
                    fetchScanLineGrayScale((unsigned char*)scanLine.data(), format,
                        jScanLine[0], ctx->output_width, (const unsigned char*)smaskData.data()
                        + i * ctx->output_width);
                    stream.Write(scanLine.data(), scanLine.size());
                }
            }
            break;
        }
        case JCS_CMYK:
        {
            if (smaskData.size() == 0)
            {
                for (unsigned i = 0; i < ctx->output_height; i++)
                {
                    jpeg_read_scanlines(ctx, jScanLine, 1);
                    ConvertScanlineCYMKToRGB(ctx, jScanLine[0]);
                    fetchScanLineRGB<4>((unsigned char*)scanLine.data(),
                        format, jScanLine[0], ctx->output_width);
                    stream.Write(scanLine.data(), scanLine.size());
                }
            }
            else
            {
                for (unsigned i = 0; i < ctx->output_height; i++)
                {
                    jpeg_read_scanlines(ctx, jScanLine, 1);
                    ConvertScanlineCYMKToRGB(ctx, jScanLine[0]);
                    fetchScanLineRGB<4>((unsigned char*)scanLine.data(), format,
                        jScanLine[0], ctx->output_width, (const unsigned char*)smaskData.data()
                        + i * ctx->output_width);
                    stream.Write(scanLine.data(), scanLine.size());
                }
            }
            break;
        }
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
    }
}

#endif // PODOFO_HAVE_JPEG_LIB

template <int bpp>
void fetchScanLineRGB(unsigned char* dstScanLine, PdfPixelFormat format,
    const unsigned char* srcScanLine, unsigned width)
{
    switch (format)
    {
        case PdfPixelFormat::RGB24:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 3 + 0] = srcScanLine[i * bpp + 0];
                dstScanLine[i * 3 + 1] = srcScanLine[i * bpp + 1];
                dstScanLine[i * 3 + 2] = srcScanLine[i * bpp + 2];
            }
            break;
        }
        case PdfPixelFormat::BGR24:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 3 + 0] = srcScanLine[i * bpp + 2];
                dstScanLine[i * 3 + 1] = srcScanLine[i * bpp + 1];
                dstScanLine[i * 3 + 2] = srcScanLine[i * bpp + 0];
            }
            break;
        }
        case PdfPixelFormat::RGBA:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 4 + 0] = srcScanLine[i * bpp + 0];
                dstScanLine[i * 4 + 1] = srcScanLine[i * bpp + 1];
                dstScanLine[i * 4 + 2] = srcScanLine[i * bpp + 2];
                dstScanLine[i * 4 + 3] = 255;
            }
            break;
        }
        case PdfPixelFormat::BGRA:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 4 + 0] = srcScanLine[i * bpp + 2];
                dstScanLine[i * 4 + 1] = srcScanLine[i * bpp + 1];
                dstScanLine[i * 4 + 2] = srcScanLine[i * bpp + 0];
                dstScanLine[i * 4 + 3] = 255;
            }
            break;
        }
        case PdfPixelFormat::ARGB:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 4 + 0] = 255;
                dstScanLine[i * 4 + 1] = srcScanLine[i * bpp + 0];
                dstScanLine[i * 4 + 2] = srcScanLine[i * bpp + 1];
                dstScanLine[i * 4 + 3] = srcScanLine[i * bpp + 2];
            }
            break;
        }
        case PdfPixelFormat::ABGR:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 4 + 0] = 255;
                dstScanLine[i * 4 + 1] = srcScanLine[i * bpp + 2];
                dstScanLine[i * 4 + 2] = srcScanLine[i * bpp + 1];
                dstScanLine[i * 4 + 3] = srcScanLine[i * bpp + 0];
            }
            break;
        }
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedPixelFormat, "Unsupported pixel format");
    }
}

template <int bpp>
void fetchScanLineRGB(unsigned char* dstScanLine, PdfPixelFormat format,
    const unsigned char* srcScanLine, unsigned width, const unsigned char* srcAphaLine)
{
    switch (format)
    {
        // TODO: Handle alpha?
        case PdfPixelFormat::RGB24:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 3 + 0] = srcScanLine[i * bpp + 0];
                dstScanLine[i * 3 + 1] = srcScanLine[i * bpp + 1];
                dstScanLine[i * 3 + 2] = srcScanLine[i * bpp + 2];
            }
            break;
        }
        // TODO: Handle alpha?
        case PdfPixelFormat::BGR24:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 3 + 0] = srcScanLine[i * bpp + 2];
                dstScanLine[i * 3 + 1] = srcScanLine[i * bpp + 1];
                dstScanLine[i * 3 + 2] = srcScanLine[i * bpp + 0];
            }
            break;
        }
        case PdfPixelFormat::RGBA:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 4 + 0] = srcScanLine[i * bpp + 0];
                dstScanLine[i * 4 + 1] = srcScanLine[i * bpp + 1];
                dstScanLine[i * 4 + 2] = srcScanLine[i * bpp + 2];
                dstScanLine[i * 4 + 3] = srcAphaLine[i];
            }
            break;
        }
        case PdfPixelFormat::BGRA:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 4 + 0] = srcScanLine[i * bpp + 2];
                dstScanLine[i * 4 + 1] = srcScanLine[i * bpp + 1];
                dstScanLine[i * 4 + 2] = srcScanLine[i * bpp + 0];
                dstScanLine[i * 4 + 3] = srcAphaLine[i];
            }
            break;
        }
        case PdfPixelFormat::ARGB:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 4 + 0] = srcAphaLine[i];
                dstScanLine[i * 4 + 1] = srcScanLine[i * bpp + 0];
                dstScanLine[i * 4 + 2] = srcScanLine[i * bpp + 1];
                dstScanLine[i * 4 + 3] = srcScanLine[i * bpp + 2];
            }
            break;
        }
        case PdfPixelFormat::ABGR:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 4 + 0] = srcAphaLine[i];
                dstScanLine[i * 4 + 1] = srcScanLine[i * bpp + 2];
                dstScanLine[i * 4 + 2] = srcScanLine[i * bpp + 1];
                dstScanLine[i * 4 + 3] = srcScanLine[i * bpp + 0];
            }
            break;
        }
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedPixelFormat, "Unsupported pixel format");
    }
}

void fetchScanLineGrayScale(unsigned char* dstScanLine, PdfPixelFormat format,
    const unsigned char* srcScanLine, unsigned width)
{
    switch (format)
    {
        case PdfPixelFormat::Grayscale:
        {
            for (unsigned i = 0; i < width; i++)
                dstScanLine[i] = srcScanLine[i];
            break;
        }
        case PdfPixelFormat::RGB24:
        case PdfPixelFormat::BGR24:
        {
            for (unsigned i = 0; i < width; i++)
            {
                unsigned char gray = srcScanLine[i];
                dstScanLine[i * 3 + 0] = gray;
                dstScanLine[i * 3 + 1] = gray;
                dstScanLine[i * 3 + 2] = gray;
            }
            break;
        }
        case PdfPixelFormat::RGBA:
        case PdfPixelFormat::BGRA:
        {
            for (unsigned i = 0; i < width; i++)
            {
                unsigned char gray = srcScanLine[i];
                dstScanLine[i * 4 + 0] = gray;
                dstScanLine[i * 4 + 1] = gray;
                dstScanLine[i * 4 + 2] = gray;
                dstScanLine[i * 4 + 3] = 255;
            }
            break;
        }
        case PdfPixelFormat::ARGB:
        case PdfPixelFormat::ABGR:
        {
            for (unsigned i = 0; i < width; i++)
            {
                unsigned char gray = srcScanLine[i];
                dstScanLine[i * 4 + 0] = 255;
                dstScanLine[i * 4 + 1] = gray;
                dstScanLine[i * 4 + 2] = gray;
                dstScanLine[i * 4 + 3] = gray;
            }
            break;
        }
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedPixelFormat, "Unsupported pixel format");
    }
}

void fetchScanLineGrayScale(unsigned char* dstScanLine, PdfPixelFormat format,
    const unsigned char* srcScanLine, unsigned width, const unsigned char* srcAphaLine)
{
    switch (format)
    {
        // TODO: Handle alpha?
        case PdfPixelFormat::Grayscale:
        {
            for (unsigned i = 0; i < width; i++)
                dstScanLine[i] = srcScanLine[i];
            break;
        }
        // TODO: Handle alpha?
        case PdfPixelFormat::RGB24:
        case PdfPixelFormat::BGR24:
        {
            for (unsigned i = 0; i < width; i++)
            {
                unsigned char gray = srcScanLine[i];
                dstScanLine[i * 3 + 0] = gray;
                dstScanLine[i * 3 + 1] = gray;
                dstScanLine[i * 3 + 2] = gray;
            }
            break;
        }
        case PdfPixelFormat::RGBA:
        case PdfPixelFormat::BGRA:
        {
            for (unsigned i = 0; i < width; i++)
            {
                unsigned char gray = srcScanLine[i];
                dstScanLine[i * 4 + 0] = gray;
                dstScanLine[i * 4 + 1] = gray;
                dstScanLine[i * 4 + 2] = gray;
                dstScanLine[i * 4 + 3] = srcAphaLine[i];
            }
            break;
        }
        case PdfPixelFormat::ARGB:
        case PdfPixelFormat::ABGR:
        {
            for (unsigned i = 0; i < width; i++)
            {
                unsigned char gray = srcScanLine[i];
                dstScanLine[i * 4 + 0] = srcAphaLine[i];
                dstScanLine[i * 4 + 1] = gray;
                dstScanLine[i * 4 + 2] = gray;
                dstScanLine[i * 4 + 3] = gray;
            }
            break;
        }
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedPixelFormat, "Unsupported pixel format");
    }
}

void fetchScanLineBW(unsigned char* dstScanLine, PdfPixelFormat format,
    const unsigned char* srcScanLine, unsigned width)
{
    switch (format)
    {
        case PdfPixelFormat::Grayscale:
        {
            for (unsigned i = 0; i < width; i++)
                dstScanLine[i] = (unsigned char)(FETCH_BIT(srcScanLine, i) * 255);
            break;
        }
        case PdfPixelFormat::RGB24:
        case PdfPixelFormat::BGR24:
        {
            for (unsigned i = 0; i < width; i++)
            {
                unsigned char value = (unsigned char)(FETCH_BIT(srcScanLine, i) * 255);
                dstScanLine[i * 3 + 0] = value;
                dstScanLine[i * 3 + 1] = value;
                dstScanLine[i * 3 + 2] = value;
            }
            break;
        }
        case PdfPixelFormat::RGBA:
        case PdfPixelFormat::BGRA:
        {
            for (unsigned i = 0; i < width; i++)
            {
                unsigned char value = (unsigned char)(FETCH_BIT(srcScanLine, i) * 255);
                dstScanLine[i * 4 + 0] = value;
                dstScanLine[i * 4 + 1] = value;
                dstScanLine[i * 4 + 2] = value;
                dstScanLine[i * 4 + 3] = 255;
            }
            break;
        }
        case PdfPixelFormat::ARGB:
        case PdfPixelFormat::ABGR:
        {
            for (unsigned i = 0; i < width; i++)
            {
                unsigned char value = (unsigned char)(FETCH_BIT(srcScanLine, i) * 255);
                dstScanLine[i * 4 + 0] = 255;
                dstScanLine[i * 4 + 1] = value;
                dstScanLine[i * 4 + 2] = value;
                dstScanLine[i * 4 + 3] = value;
            }
            break;
        }
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedPixelFormat, "Unsupported pixel format");
    }
}

void fetchScanLineBW(unsigned char* dstScanLine, PdfPixelFormat format,
    const unsigned char* srcScanLine, unsigned width,
    const unsigned char* srcAphaLine)
{
    switch (format)
    {
        // TODO: Handle alpha?
        case PdfPixelFormat::Grayscale:
        {
            for (unsigned i = 0; i < width; i++)
                dstScanLine[i] = (unsigned char)(FETCH_BIT(srcScanLine, i) * 255);
            break;
        }
        // TODO: Handle alpha?
        case PdfPixelFormat::RGB24:
        case PdfPixelFormat::BGR24:
        {
            for (unsigned i = 0; i < width; i++)
            {
                unsigned char value = (unsigned char)(FETCH_BIT(srcScanLine, i) * 255);
                dstScanLine[i * 3 + 0] = value;
                dstScanLine[i * 3 + 1] = value;
                dstScanLine[i * 3 + 2] = value;
            }
            break;
        }
        case PdfPixelFormat::RGBA:
        case PdfPixelFormat::BGRA:
        {
            for (unsigned i = 0; i < width; i++)
            {
                unsigned char value = (unsigned char)(FETCH_BIT(srcScanLine, i) * 255);
                dstScanLine[i * 4 + 0] = value;
                dstScanLine[i * 4 + 1] = value;
                dstScanLine[i * 4 + 2] = value;
                dstScanLine[i * 4 + 3] = srcAphaLine[i];
            }
            break;
        }
        case PdfPixelFormat::ARGB:
        case PdfPixelFormat::ABGR:
        {
            for (unsigned i = 0; i < width; i++)
            {
                unsigned char value = (unsigned char)(FETCH_BIT(srcScanLine, i) * 255);
                dstScanLine[i * 4 + 0] = srcAphaLine[i];
                dstScanLine[i * 4 + 1] = value;
                dstScanLine[i * 4 + 2] = value;
                dstScanLine[i * 4 + 3] = value;
            }
            break;
        }
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedPixelFormat, "Unsupported pixel format");
    }
}

charbuff initScanLine(PdfPixelFormat format, unsigned width, int scanLineSizeHint)
{
    unsigned defaultScanLineSize;
    switch (format)
    {
        case PdfPixelFormat::Grayscale:
        {
            defaultScanLineSize = 4 * ((width + 3) / 4);;
            break;
        }
        case PdfPixelFormat::RGB24:
        case PdfPixelFormat::BGR24:
        {
            defaultScanLineSize = 4 * ((3 * width + 3) / 4);
            break;
        }
        case PdfPixelFormat::RGBA:
        case PdfPixelFormat::BGRA:
        case PdfPixelFormat::ARGB:
        case PdfPixelFormat::ABGR:
        {
            defaultScanLineSize = 4 * width;
            break;
        }
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    if (scanLineSizeHint < 0)
    {
        return charbuff(defaultScanLineSize);
    }
    else
    {
        if (scanLineSizeHint < (int)defaultScanLineSize)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedImageFormat, "The buffer row size is too small");

        return charbuff((size_t)scanLineSizeHint);
    }
}

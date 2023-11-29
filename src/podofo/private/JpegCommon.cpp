#include <podofo/private/PdfDeclarationsPrivate.h>
#include "JpegCommon.h"
#include <string>

extern "C" {
#include <jerror.h>
}

using namespace PoDoFo;
using namespace std;

constexpr unsigned BLOCK_SIZE = 4096;

static void setErrorHandler(jpeg_common_struct& ctx, JpegErrorHandler& handler);

// Handlers for JPeg library
extern "C"
{
// NOTE: Ignore MSVC warning C4297 ("function assumed not to throw
// an exception but does") because of extern "C": we will catch
// the exception in a C++ method
#pragma warning(push)
#pragma warning(disable: 4297)
    void cust_error_exit(j_common_ptr ctx)
    {
        string error;
        error.resize(JMSG_LENGTH_MAX);
        (*ctx->err->format_message)(ctx, error.data());
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, error);
    }
#pragma warning(pop)

    void cust_emit_message(j_common_ptr, int)
    {
        // Do nothing
    }

    void buff_dest_init(j_compress_ptr ctx)
    {
        auto& dest = *(JpegBufferDestination*)(ctx->dest);

        // Some room in the buffer must be available at this point
        dest.buff->resize(BLOCK_SIZE);

        // Point to allocated data
        ctx->dest->next_output_byte = (unsigned char*)dest.buff->data();
        ctx->dest->free_in_buffer = BLOCK_SIZE;
    }

    boolean buff_dest_empty_buffer(j_compress_ptr ctx)
    {
        auto& dest = *(JpegBufferDestination*)(ctx->dest);

        size_t oldsize = dest.buff->size();
        dest.buff->resize(oldsize + BLOCK_SIZE);

        // Point to newly allocated data
        ctx->dest->next_output_byte = (unsigned char*)(dest.buff->data() + oldsize);
        ctx->dest->free_in_buffer = dest.buff->size() - oldsize;
        return TRUE;
    }

    void buff_dest_term(j_compress_ptr ctx)
    {
        auto& dest = *(JpegBufferDestination*)(ctx->dest);

        // Resize vector to number of bytes actually used
        dest.buff->resize(dest.buff->size() - ctx->dest->free_in_buffer);
    }
};

void PoDoFo::InitJpegDecompressContext(jpeg_decompress_struct& ctx, JpegErrorHandler& handler)
{
    setErrorHandler((jpeg_common_struct&)ctx, handler);
    jpeg_create_decompress(&ctx);
}

void PoDoFo::ConvertScanlineCYMKToRGB(j_decompress_ptr info, JSAMPROW scanLine)
{
    int c;
    int m;
    int y;
    int k;

    // As found in https://github.com/petewarden/tensorflow_makefile/blob/49c08e4d4ff3b6e7d99374dc2fbf8b358150ef9c/tensorflow/core/lib/jpeg/jpeg_mem.cc#L199
    if (info->saw_Adobe_marker)
    {
        for (unsigned i = 0; i < info->image_width; i++)
        {
            c = scanLine[i * 4 + 0];
            m = scanLine[i * 4 + 1];
            y = scanLine[i * 4 + 2];
            k = scanLine[i * 4 + 3];

            scanLine[i * 4 + 0] = (unsigned char)std::clamp((k * c) / 255, 0, 255);
            scanLine[i * 4 + 1] = (unsigned char)std::clamp((k * m) / 255, 0, 255);
            scanLine[i * 4 + 2] = (unsigned char)std::clamp((k * y) / 255, 0, 255);
        }
    }
    else
    {
        for (unsigned i = 0; i < info->image_width; i++)
        {
            c = scanLine[i * 4 + 0];
            m = scanLine[i * 4 + 1];
            y = scanLine[i * 4 + 2];
            k = scanLine[i * 4 + 3];

            scanLine[i * 4 + 0] = (unsigned char)std::clamp((255 - k) * (255 - c) / 255, 0, 255);
            scanLine[i * 4 + 1] = (unsigned char)std::clamp((255 - k) * (255 - m) / 255, 0, 255);
            scanLine[i * 4 + 2] = (unsigned char)std::clamp((255 - k) * (255 - y) / 255, 0, 255);
        }
    }
}

void PoDoFo::InitJpegCompressContext(jpeg_compress_struct& ctx, JpegErrorHandler& handler)
{
    setErrorHandler((jpeg_common_struct&)ctx, handler);
    jpeg_create_compress(&ctx);
}

void PoDoFo::SetJpegBufferDestination(jpeg_compress_struct& ctx, charbuff& buff, JpegBufferDestination& handler)
{
    handler.pub.init_destination = buff_dest_init;
    handler.pub.empty_output_buffer = buff_dest_empty_buffer;
    handler.pub.term_destination = buff_dest_term;
    handler.buff = &buff;
    ctx.dest = (jpeg_destination_mgr*)&handler;
}

void setErrorHandler(jpeg_common_struct& ctx, JpegErrorHandler& handler)
{
    jpeg_std_error(&handler);
    handler.error_exit = &cust_error_exit;
    handler.emit_message = &cust_emit_message;
    ctx.err = (jpeg_error_mgr*)&handler;
}

/*
 * memsrc.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains decompression data source routines for the case of
 * reading JPEG data from a memory buffer that is preloaded with the entire
 * JPEG file. This would not seem especially useful at first sight, but
 * a number of people have asked for it.
 * This is really just a stripped-down version of jdatasrc.c. Comparison
 * of this code with jdatasrc.c may be helpful in seeing how to make
 * custom source managers for other purposes.
*/

/* Expanded data source object for memory input */
struct my_source_mgr
{
    struct jpeg_source_mgr pub; /* public fields */
    JOCTET eoi_buffer[2]; /* a place to put a dummy EOI */
};

using my_src_ptr = my_source_mgr*;

/*
 * Initialize source, called by jpeg_read_header
 * before any data is actually read.
 */

METHODDEF(void) init_source(j_decompress_ptr)
{
    /* No work, since jpeg_memory_src set up the buffer pointer and count.
     * Indeed, if we want to read multiple JPEG images from one buffer,
     * this *must* not do anything to the pointer.
     */
}

/*
 * Fill the input buffer, called whenever buffer is emptied.
 *
 * In this application, this routine should never be called; if it is called,
 * the decompressor has overrun the end of the input buffer, implying we
 * supplied an incomplete or corrupt JPEG datastream. A simple error exit
 * might be the most appropriate response.
 *
 * But what we choose to do in this code is to supply dummy EOI markers
 * in order to force the decompressor to finish processing and supply
 * some sort of output image, no matter how corrupted.
 */

METHODDEF(boolean) fill_input_buffer(j_decompress_ptr ctx)
{
    my_src_ptr src = reinterpret_cast<my_src_ptr>(ctx->src);
    WARNMS(ctx, JWRN_JPEG_EOF);

    /* Create a fake EOI marker */
    src->eoi_buffer[0] = static_cast<JOCTET>(0xFF);
    src->eoi_buffer[1] = static_cast<JOCTET>(JPEG_EOI);
    src->pub.next_input_byte = src->eoi_buffer;
    src->pub.bytes_in_buffer = 2;

    return TRUE;
}

/*
 * Skip data, used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 *
 * If we overrun the end of the buffer, we let fill_input_buffer deal with
 * it. An extremely large skip could cause some time-wasting here, but
 * it really isn't supposed to happen ... and the decompressor will never
 * skip more than 64K anyway.
 */
METHODDEF(void) skip_input_data(j_decompress_ptr ctx, long num_bytes)
{
    my_src_ptr src = reinterpret_cast<my_src_ptr>(ctx->src);

    if (num_bytes > 0)
    {
        while (num_bytes > static_cast<long>(src->pub.bytes_in_buffer))
        {
            num_bytes -= static_cast<long>(src->pub.bytes_in_buffer);
            fill_input_buffer(ctx);
            /* note we assume that fill_input_buffer will never return FALSE,
             * so suspension need not be handled.
             */
        }

        src->pub.next_input_byte += static_cast<size_t>(num_bytes);
        src->pub.bytes_in_buffer -= static_cast<size_t>(num_bytes);
    }
}

/*
 * An additional method that can be provided by data source modules is the
 * resync_to_restart method for error recovery in the presence of RST markers.
 * For the moment, this source module just uses the default resync method
 * provided by the JPEG library. That method assumes that no backtracking
 * is possible.
 */

 /*
  * Terminate source, called by jpeg_finish_decompress
  * after all data has been read. Often a no-op.
  *
  * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
  * application must deal with any cleanup that should happen even
  * for error exit.
  */
METHODDEF(void) term_source(j_decompress_ptr)
{
    /* no work necessary here */
}

/*
 * Prepare for input from a memory buffer.
 */
void PoDoFo::jpeg_memory_src(j_decompress_ptr ctx, const JOCTET* buffer, size_t bufsize)
{
    my_src_ptr src;

    /* The source object is made permanent so that a series of JPEG images
     * can be read from a single buffer by calling jpeg_memory_src
     * only before the first one.
     * This makes it unsafe to use this manager and a different source
     * manager serially with the same JPEG object. Caveat programmer.
     */

    if (ctx->src == nullptr)
    {
        // first time for this JPEG object?
        ctx->src = static_cast<struct jpeg_source_mgr*>(
            (*ctx->mem->alloc_small) (reinterpret_cast<j_common_ptr>(ctx), JPOOL_PERMANENT,
                sizeof(my_source_mgr)));
    }

    src = reinterpret_cast<my_src_ptr>(ctx->src);
    src->pub.init_source = init_source;
    src->pub.fill_input_buffer = fill_input_buffer;
    src->pub.skip_input_data = skip_input_data;
    src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
    src->pub.term_source = term_source;

    src->pub.next_input_byte = buffer;
    src->pub.bytes_in_buffer = bufsize;
}

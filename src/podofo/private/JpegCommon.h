#ifndef JPEG_COMMON_H
#define JPEG_COMMON_H

#include <podofo/main/PdfDeclarations.h>

extern "C" {
#include <jpeglib.h>
}

namespace PoDoFo
{
    using JpegErrorHandler = jpeg_error_mgr;

    struct JpegBufferDestination
    {
        jpeg_destination_mgr pub = { };
        PoDoFo::charbuff* buff = nullptr;
    };

    // NOTE: Don't use directly, use INIT_JPEG_COMPRESS_CONTEXT
    void InitJpegCompressContext(jpeg_compress_struct& ctx, JpegErrorHandler& jerr);
    // NOTE: Don't use directly, use INIT_JPEG_DECOMPRESS_CONTEXT
    void InitJpegDecompressContext(jpeg_decompress_struct& ctx, JpegErrorHandler& jerr);
    void SetJpegBufferDestination(jpeg_compress_struct& ctx, charbuff& buff, JpegBufferDestination& jdest);
    void jpeg_memory_src(j_decompress_ptr cinfo, const JOCTET* buffer, size_t bufsize);
    void ConvertScanlineCYMKToRGB(j_decompress_ptr info, JSAMPROW scanLine);
}

#endif // JPEG_COMMON_H

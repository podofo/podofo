/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef IMAGE_UTILS_H
#define IMAGE_UTILS_H

#include <podofo/main/PdfOutputStream.h>

#ifdef PODOFO_HAVE_JPEG_LIB
#include <podofo/private/JpegCommon.h>
#endif // PODOFO_HAVE_JPEG_LIB

#include <pdfium/core/fxcodec/scanlinedecoder.h>

namespace utls
{
    /** Fetch a RGB image and write it to the stream
     */
    void FetchImageRGB(PoDoFo::OutputStream& stream, unsigned width, unsigned heigth, PoDoFo::PdfPixelFormat format,
        const unsigned char* imageData, const PoDoFo::charbuff& smaskData, PoDoFo::charbuff& scanLine);

    /** Fetch a GrayScale image and write it to the stream
     */
    void FetchImageGrayScale(PoDoFo::OutputStream& stream, unsigned width, unsigned heigth, PoDoFo::PdfPixelFormat format,
        const unsigned char* imageData, const PoDoFo::charbuff& smaskData, PoDoFo::charbuff& scanLine);

    /** Fetch a black and white image and write it to the stream
     */
    void FetchImageBW(PoDoFo::OutputStream& stream, unsigned width, unsigned heigth, PoDoFo::PdfPixelFormat format,
        fxcodec::ScanlineDecoder& decoder, const PoDoFo::charbuff& smaskData, PoDoFo::charbuff& scanLine);

#ifdef PODOFO_HAVE_JPEG_LIB
    void FetchImageJPEG(PoDoFo::OutputStream& stream, PoDoFo::PdfPixelFormat format, jpeg_decompress_struct* ctx,
        JSAMPARRAY jScanLine, const PoDoFo::charbuff& smaskData, PoDoFo::charbuff& scanLine);
#endif // PODOFO_HAVE_JPEG_LIB
}

#endif // IMAGE_UTILS_H

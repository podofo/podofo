/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef IMAGE_UTILS_H
#define IMAGE_UTILS_H

#include <podofo/auxiliary/OutputStream.h>
#include <podofo/main/PdfColorSpaceFilter.h>

#ifdef PODOFO_HAVE_JPEG_LIB
#include <podofo/private/JpegCommon.h>
#endif // PODOFO_HAVE_JPEG_LIB

#include <pdfium/core/fxcodec/scanlinedecoder.h>

namespace utls
{
    /** Fetch a RGB image and write it to the stream
     */
    void FetchImage(PoDoFo::OutputStream& stream, PoDoFo::PdfPixelFormat format, int scanLineSize,
        const unsigned char* imageData, unsigned width, unsigned heigth, unsigned bitsPerComponent,
        const PoDoFo::PdfColorSpaceFilter& filter, const PoDoFo::charbuff& smaskData);

    /** Fetch a Black and White image and write it to the stream
     */
    void FetchImageCCITT(PoDoFo::OutputStream& stream, PoDoFo::PdfPixelFormat format, int scanLineSize,
        fxcodec::ScanlineDecoder& decoder, unsigned width, unsigned heigth, const PoDoFo::charbuff& smaskData);

#ifdef PODOFO_HAVE_JPEG_LIB
    void FetchImageJPEG(PoDoFo::OutputStream& stream, PoDoFo::PdfPixelFormat format, int scanLineSize,
        jpeg_decompress_struct* ctx, unsigned width, unsigned heigth, const PoDoFo::charbuff& smaskData);
#endif // PODOFO_HAVE_JPEG_LIB
}

#endif // IMAGE_UTILS_H

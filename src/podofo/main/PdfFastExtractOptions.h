/**
 * SPDX-FileCopyrightText: (C) 2026 PoDoFo Project
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FAST_EXTRACT_OPTIONS_H
#define PDF_FAST_EXTRACT_OPTIONS_H

#include <podofo/main/PdfDeclarations.h>
#include <podofo/main/PdfTextExtract.h>

namespace PoDoFo {

/**
 * Options for fast PDF text and image extraction
 */
struct PODOFO_API PdfFastExtractOptions final
{
    /** Whether to extract text (default: true) */
    bool ExtractText = true;

    /** Whether to extract images (default: true) */
    bool ExtractImages = true;

    /** Whether to use parallel processing for multiple pages (default: false) */
    bool ParallelProcessing = false;

    /** Maximum number of threads for parallel processing (default: 4) */
    int MaxThreads = 4;

    /** Whether to use stream processing to reduce memory usage (default: true) */
    bool StreamProcessing = true;

    /** Text extraction flags (default: None) */
    PdfTextExtractFlags TextFlags = PdfTextExtractFlags::None;

    /** Clip rectangle for text extraction (optional) */
    nullable<Rect> ClipRect;

    /** Callback for early interruption of extraction */
    std::function<bool(int pageNum, size_t processedBytes)> ProgressCallback = nullptr;

    /** Maximum memory usage in bytes (0 for unlimited, default: 0) */
    size_t MaxMemoryUsage = 0;

    /** Whether to cache font information between pages (default: true) */
    bool CacheFonts = true;

    /** Whether to skip invalid or corrupted pages (default: true) */
    bool SkipInvalidPages = true;
};

} // namespace PoDoFo

#endif // PDF_FAST_EXTRACT_OPTIONS_H
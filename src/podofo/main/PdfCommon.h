// SPDX-FileCopyrightText: 2022 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#ifndef PDF_COMMON_H

#include "PdfDeclarations.h"

namespace PoDoFo {

using LogMessageCallback = std::function<void(PdfLogSeverity logSeverity, const std::string_view& msg)>;

class PODOFO_API PdfCommon final
{
    PdfCommon() = delete;

public:
    /// Determine if the library was built with debug semantics, as
    /// determined by the standard NDEBUG macro when it was compiled.
    static bool IsDebugBuild();

    static void AddFontDirectory(const std::string_view& path);

    /// Set a global static LogMessageCallback functor to replace stderr output in LogMessageInternal.
    /// @param logMessageCallback the pointer to the new callback functor object
    /// @returns the pointer to the previous callback functor object
    static void SetLogMessageCallback(const LogMessageCallback& logMessageCallback);

    /// Set the maximum logging severity.
    /// The higher the maximum (enum integral value), the more is logged
    static void SetMaxLoggingSeverity(PdfLogSeverity logSeverity);

    /// Get the maximum logging severity
    /// The higher the maximum (enum integral value), the more is logged
    static PdfLogSeverity GetMaxLoggingSeverity();

    /// Set the maximum nesting depth of the recursive operations guarded against
    /// stack overflow, such as parsing nested objects. The default is 256
    /// @remarks The check can't be disabled: exceeding the depth raises
    /// PdfErrorCode::MaxRecursionReached, so 0 makes every guarded operation fail
    static void SetMaxRecursionDepth(unsigned maxRecursionDepth);

    static unsigned GetMaxRecursionDepth();

    /// The if the given logging severity enabled or not
    static bool IsLoggingSeverityEnabled(PdfLogSeverity logSeverity);

    static unsigned GetMaxObjectCount();

    /// @remarks Values above 4294967294 are capped, so the object
    /// number following the last one never wraps to 0
    static void SetMaxObjectCount(unsigned maxObjectCount);

    /// Set the maximum count of bytes the filters of a stream may produce while
    /// decoding it, summed across the filter chain. The default is 512 MiB
    /// @remarks Decoding a stream that exceeds it raises PdfErrorCode::ValueOutOfRange
    static void SetMaxDecodedStreamSize(size_t maxDecodedStreamSize);

    static size_t GetMaxDecodedStreamSize();
};

}

#define PDF_COMMON_H

#endif // PDF_COMMON_H

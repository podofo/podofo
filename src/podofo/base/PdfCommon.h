/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_COMMON_H

#include "PdfDeclarations.h"

namespace PoDoFo {

class PODOFO_API PdfCommon final
{
    PdfCommon() = delete;

public:
    static void AddFontDirectory(const std::string_view& path);

    /** Set a global static LogMessageCallback functor to replace stderr output in LogMessageInternal.
     *  \param logMessageCallback the pointer to the new callback functor object
     *  \returns the pointer to the previous callback functor object
     */
    static void SetLogMessageCallback(const LogMessageCallback& logMessageCallback);

    /** Set the maximum logging severity.
     * The higher the maximum (enum integral value), the more is logged
     */
    static void SetMaxLoggingSeverity(PdfLogSeverity logSeverity);

    /** Get the maximum logging severity
     * The higher the maximum (enum integral value), the more is logged
     */
    static PdfLogSeverity GetMaxLoggingSeverity();

    /** The if the given logging severity enabled or not
     */
    static bool IsLoggingSeverityEnabled(PdfLogSeverity logSeverity);
};

}

#define PDF_COMMON_H

#endif // PDF_COMMON_H

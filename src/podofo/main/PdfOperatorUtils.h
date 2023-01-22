/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PODOFO_OPERATOR_UTILS_H
#define PODOFO_OPERATOR_UTILS_H

#include <podofo/main/PdfDeclarations.h>

namespace PoDoFo
{
    PODOFO_API PdfOperator GetPdfOperator(const std::string_view& opstr);
    PODOFO_API bool TryGetPdfOperator(const std::string_view& opstr, PdfOperator& op);

    /** Get the operands count of the operator
     * \returns count the number of operand, -1 means variadic number of operands
     */
    PODOFO_API int GetOperandCount(PdfOperator op);

    /** Get the operands count of the operator
     * \param count the number of operand, -1 means variadic number of operands
     */
    PODOFO_API bool TryGetOperandCount(PdfOperator op, int& count);

    PODOFO_API std::string_view GetPdfOperatorName(PdfOperator op);
    PODOFO_API bool TryGetPdfOperatorName(PdfOperator op, std::string_view& opstr);
}

#endif // PODOFO_OPERATOR_UTILS_H

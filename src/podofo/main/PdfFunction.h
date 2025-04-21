/**
 * SPDX-FileCopyrightText: (C) 2025 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_FUNCTION_H
#define PDF_FUNCTION_H

#include "PdfElement.h"
#include "PdfFunctionDefinition.h"

namespace PoDoFo
{
    class PODOFO_API PdfFunction final : public PdfDictionaryElement
    {
        friend class PdfDocument;

    private:
        PdfFunction(PdfDocument& doc, PdfFunctionDefinitionPtr&& definition);

    public:
        const PdfFunctionDefinition& GetDefinition() const { return *m_Definition; }
        PdfFunctionDefinitionPtr GetDefinitionPtr() const { return m_Definition; }

    public:
        PdfFunctionDefinitionPtr m_Definition;
    };
}

#endif // PDF_FUNCTION_H

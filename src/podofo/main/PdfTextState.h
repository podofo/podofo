/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_TEXT_STATE_H
#define PDF_TEXT_STATE_H

#include "PdfDeclarations.h"

namespace PoDoFo
{
    class PdfFont;

    // TODO: Add missing properties ISO 32000-1:2008 "9.3 Text State Parameters and Operators"
    struct PODOFO_API PdfTextState final
    {
    public:
        PdfTextState();
    public:
        const PdfFont* Font;
        double FontSize;
        double FontScale;
        double CharSpacing;
        double WordSpacing;
        PdfTextRenderingMode RenderingMode;
    };
}

#endif // PDF_TEXT_STATE_H

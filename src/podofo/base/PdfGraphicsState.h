/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_GRAPHICS_STATE_H
#define PDF_GRAPHICS_STATE_H

#include "PdfColor.h"
#include "PdfMath.h"

namespace PoDoFo
{
    // TODO: Add missing properties ISO 32000-1:2008 "8.4 Graphics State"
    struct PODOFO_API PdfGraphicsState final
    {
    public:
        PdfGraphicsState();
    public:
        Matrix CTM;
        double LineWidth;
        double MiterLimit;
        PdfLineCapStyle LineCapStyle;
        PdfLineJoinStyle LineJoinStyle;
        std::string RenderingIntent;
        PdfColor FillColor;
        PdfColor StrokeColor;
    };
}

#endif // PDF_GRAPHICS_STATE_H

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
        Matrix CTM;
        double LineWidth = 0;
        double MiterLimit = 10;
        PdfLineCapStyle LineCapStyle = PdfLineCapStyle::Square;
        PdfLineJoinStyle LineJoinStyle = PdfLineJoinStyle::Miter;
        std::string RenderingIntent;
        PdfColor FillColor;
        PdfColor StrokeColor;
    };
}

#endif // PDF_GRAPHICS_STATE_H

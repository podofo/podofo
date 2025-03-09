/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_MATH_H
#define PDF_MATH_H

#include "PdfDeclarations.h"
#include <podofo/auxiliary/Rect.h>
#include <podofo/auxiliary/Matrix.h>

namespace PoDoFo
{
    class PdfPage;

    /**
     * Get a rotation transformation that aligns the rectangle to the axis after the rotation
     * \param teta rotation in radians
     */
    Matrix PODOFO_API GetFrameRotationTransform(const Rect& rect, double teta);

    /**
     * Get an inverse rotation transformation that aligns the rectangle to the axis after the rotation
     * \param teta rotation in radians
     */
    Matrix PODOFO_API GetFrameRotationTransformInverse(const Rect& rect, double teta);

    /**
     * Transform the given rect accordingly to the page rotation
     * \param rect a normalized rect in the canonical PDF coordinate system
     * \returns a normalized rect in the canonical PDF coordinate system
     */
    Rect PODOFO_API TransformRectPage(const Rect& rect, const PdfPage& page);
}

#endif // PDF_MATH_H

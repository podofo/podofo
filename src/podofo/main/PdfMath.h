/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_MATH_H
#define PDF_MATH_H

#include "PdfDeclarations.h"
#include <podofo/auxiliary/Matrix.h>

namespace PoDoFo
{
    class PdfRect;

    /** Get a rotation trasformation that aligns the rectangle to the axis after the rotation
     */
    Matrix PODOFO_API GetFrameRotationTransform(const PdfRect& rect, double teta);

    /** Get an inverse rotation trasformation that aligns the rectangle to the axis after the rotation
     */
    Matrix PODOFO_API GetFrameRotationTransformInverse(const PdfRect& rect, double teta);
}

#endif // PDF_MATH_H

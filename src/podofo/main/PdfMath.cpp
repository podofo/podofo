/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfMath.h"

using namespace PoDoFo;

Matrix PoDoFo::GetFrameRotationTransform(const Rect& rect, double teta)
{
    auto R = Matrix::CreateRotation(teta);

    Vector2 leftBottom(rect.GetLeft(), rect.GetBottom());
    Vector2 rightTop(rect.GetRight(), rect.GetTop());

    // Rotate the rect
    auto corner1 = leftBottom * R;
    auto corner2 = rightTop * R;
    auto rect_1 = Rect::FromCorners(corner1.X, corner1.Y, corner2.X, corner2.Y);

    // Find the axis align translation
    Vector2 leftBottom_1(rect_1.X, rect_1.Y);
    Vector2 alignTx_1 = leftBottom - leftBottom_1;
    return  R * Matrix::CreateTranslation(alignTx_1);
}

Matrix PoDoFo::GetFrameRotationTransformInverse(const Rect& rect, double teta)
{
    auto R = Matrix::CreateRotation(teta);
    auto R_inv = Matrix::CreateRotation(-teta);

    Vector2 leftBottom(rect.GetLeft(), rect.GetBottom());
    Vector2 rightTop(rect.GetRight(), rect.GetTop());

    // Rotate rect to the canonical frame
    auto corner1 = leftBottom * R;
    auto corner2 = rightTop * R;
    auto rect_1 = Rect::FromCorners(corner1.X, corner1.Y, corner2.X, corner2.Y);

    // Find the axis align translation in the canonical frame
    Vector2 leftBottom_1(rect_1.X, rect_1.Y);
    Vector2 alignTx_1 = leftBottom_1 - leftBottom;
    return Matrix::CreateTranslation(alignTx_1) * R_inv;
}

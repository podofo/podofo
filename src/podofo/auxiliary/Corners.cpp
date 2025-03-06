/**
 * SPDX-FileCopyrightText: (C) 2025 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "Corners.h"

#include <podofo/main/PdfArray.h>

using namespace std;
using namespace PoDoFo;

Corners::Corners()
    : X1(0), Y1(0), X2(0), Y2(0) { }

Corners::Corners(double x1, double y1, double x2, double y2) :
    X1(x1), Y1(y1), X2(x2), Y2(y2) { }

Corners Corners::FromCorners(const Vector2& corner1, const Vector2& corner2)
{
    return Corners(corner1.X, corner1.Y, corner2.X, corner2.Y);
}

Corners Corners::FromArray(const PdfArray& arr)
{
    if (arr.size() != 4)
        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

    double x1 = arr[0].GetReal();
    double y1 = arr[1].GetReal();
    double x2 = arr[2].GetReal();
    double y2 = arr[3].GetReal();

    return Corners(x1, y1, x2, y2);
}

Vector2 Corners::GetCorner1() const
{
    return Vector2(X1, Y1);
}

Vector2 Corners::GetCorner2() const
{
    return Vector2(X2, Y2);
}

double Corners::GetWidth() const
{
    return std::abs(X1 - X2);
}

double Corners::GetHeight() const
{
    return std::abs(Y1 - Y2);
}

Rect Corners::GetNormalized() const
{
    return Rect::FromCorners(*this);
}

void Corners::ToArray(PdfArray& arr) const
{
    arr.Clear();
    arr.Add(PdfObject(X1));
    arr.Add(PdfObject(Y1));
    arr.Add(PdfObject(X2));
    arr.Add(PdfObject(Y2));
}

PdfArray Corners::ToArray() const
{
    PdfArray arr;
    arr.Add(PdfObject(X1));
    arr.Add(PdfObject(Y1));
    arr.Add(PdfObject(X2));
    arr.Add(PdfObject(Y2));
    return arr;
}

bool Corners::operator==(const Corners& rect) const
{
    return X1 == rect.X1 && Y1 == rect.Y1 && X2 == rect.X2 && Y2 == rect.Y2;
}

bool Corners::operator!=(const Corners& rect) const
{
    return X1 != rect.X1 || Y1 != rect.Y1 || X2 != rect.X2 || Y2 != rect.Y2;
}

Corners::operator Rect() const
{
    return Rect::FromCorners(*this);
}

/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "Rect.h"

#include "Corners.h"

#include <podofo/main/PdfArray.h>
#include <podofo/main/PdfVariant.h>
#include <podofo/auxiliary/Matrix.h>

using namespace std;
using namespace PoDoFo;

static Rect CreateRect(double x1, double y1, double x2, double y2);

Rect::Rect() :
    X(0),
    Y(0),
    Width(0),
    Height(0)
{
}

Rect::Rect(double x, double y, double width, double height) :
    X(x),
    Y(y),
    Width(width),
    Height(height)
{
}

Rect Rect::FromCorners(double x1, double y1, double x2, double y2)
{
    return CreateRect(x1, y1, x2, y2);
}

Rect Rect::FromCorners(const Vector2& corner1, const Vector2& corner2)
{
    return CreateRect(corner1.X, corner1.Y, corner2.X, corner2.Y);
}

Rect Rect::FromCorners(const Corners& corners)
{
    return CreateRect(corners.X1, corners.Y1, corners.X2, corners.Y2);
}

void Rect::ToArray(PdfArray& arr) const
{
    arr.Clear();
    arr.Add(PdfObject(X));
    arr.Add(PdfObject(Y));
    arr.Add(PdfObject((Width + X)));
    arr.Add(PdfObject((Height + Y)));
}

PdfArray Rect::ToArray() const
{
    PdfArray arr;
    arr.Add(PdfObject(X));
    arr.Add(PdfObject(Y));
    arr.Add(PdfObject((Width + X)));
    arr.Add(PdfObject((Height + Y)));
    return arr;
}

string Rect::ToString() const
{
    PdfArray arr;
    string str;
    this->ToArray(arr);
    PdfVariant(arr).ToString(str);
    return str;
}

bool Rect::Contains(double x, double y) const
{
	return x >= X && x <= X + Width
		&& y >= Y && y <= Y + Height;
}

Rect Rect::FromArray(const PdfArray& arr)
{
    if (arr.size() != 4)
        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

    double x1 = arr[0].GetReal();
    double y1 = arr[1].GetReal();
    double x2 = arr[2].GetReal();
    double y2 = arr[3].GetReal();

    return CreateRect(x1, y1, x2, y2);
}

double Rect::GetRight() const
{
    return X + Width;
}

double Rect::GetTop() const
{
    return Y + Height;
}

bool Rect::operator==(const Rect& rect) const
{
    return X == rect.X && Y == rect.Y && Width == rect.Width && Height == rect.Height;
}

bool Rect::operator!=(const Rect& rect) const
{
    return X != rect.X || Y != rect.Y || Width != rect.Width || Height != rect.Height;
}

void Rect::Intersect(const Rect& rect)
{
    if (rect.Y != 0 || rect.Height != 0 || rect.X != 0 || rect.Width != 0)
    {
        double diff;

        diff = rect.X - X;
        if (diff > 0.0)
        {
            X += diff;
            Width -= diff;
        }

        diff = (X + Width) - (rect.X + rect.Width);
        if (diff > 0.0)
        {
            Width -= diff;
        }

        diff = rect.Y - Y;
        if (diff > 0.0)
        {
            Y += diff;
            Height -= diff;
        }

        diff = (Y + Height) - (rect.Y + rect.Height);
        if (diff > 0.0)
        {
            Height -= diff;
        }
    }
}

bool Rect::IsValid() const
{
    return Width != 0 && Height != 0;
}

Corners Rect::ToCorners() const
{
    return Corners(X, Y, X + Width, Y + Height);
}

Vector2 Rect::GetLeftBottom() const
{
    return Vector2(X, Y);
}

Vector2 Rect::GetRightTop() const
{
    return Vector2(X + Width, Y + Height);
}

Rect Rect::operator*(const Matrix& m) const
{
    Vector2 corner1(X, Y);
    Vector2 corner2(GetRight(), GetTop());
    corner1 = corner1 * m;
    corner2 = corner2 * m;
    return Rect::FromCorners(corner1.X, corner1.Y, corner2.X, corner2.Y);
}

Rect::operator Corners() const
{
    return Corners(X, Y, X + Width, Y + Height);
}

Rect CreateRect(double x1, double y1, double x2, double y2)
{
    // See Pdf Reference 1.7, 3.8.4 Rectangles
    utls::NormalizeCoordinates(x1, y1, x2, y2);
    return Rect(x1, y1, x2 - x1, y2 - y1);
}

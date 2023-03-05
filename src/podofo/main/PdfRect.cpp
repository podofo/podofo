/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfRect.h"

#include "PdfArray.h"
#include "PdfVariant.h"
#include "PdfMath.h"

#include <iomanip>

static void CreateRect(double x1, double y1, double x2, double y2,
    double& left, double& bottom, double& width, double& height);
static void NormalizeCoordinates(double& coord1, double& coord2);

using namespace std;
using namespace PoDoFo;

PdfRect::PdfRect() :
    X(0),
    Y(0),
    Width(0),
    Height(0)
{
}

PdfRect::PdfRect(double x, double y, double width, double height) :
    X(x),
    Y(y),
    Width(width),
    Height(height)
{
}

PdfRect PdfRect::FromCorners(double x1, double y1, double x2, double y2)
{
    PdfRect rect;
    CreateRect(x1, y1, x2, y2, rect.X, rect.Y, rect.Width, rect.Height);
    return rect;
}

PdfRect::PdfRect(const PdfArray& arr)
{
    Y = X = Width = Height = 0;
    FromArray(arr);
}

void PdfRect::ToArray(PdfArray& arr) const
{
    arr.Clear();
    arr.Add(PdfObject(X));
    arr.Add(PdfObject(Y));
    arr.Add(PdfObject((Width + X)));
    arr.Add(PdfObject((Height + Y)));
}

string PdfRect::ToString() const
{
    PdfArray arr;
    string str;
    this->ToArray(arr);
    PdfVariant(arr).ToString(str);
    return str;
}

bool PdfRect::Contains(double x, double y) const
{
	return x >= X && x <= X + Width
		&& y >= Y && y <= Y + Height;
}

void PdfRect::FromArray(const PdfArray& arr)
{
    if (arr.size() == 4)
    {
        double x1 = arr[0].GetReal();
        double y1 = arr[1].GetReal();
        double x2 = arr[2].GetReal();
        double y2 = arr[3].GetReal();

        CreateRect(x1, y1, x2, y2, X, Y, Width, Height);
    }
    else
    {
        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);
    }
}

double PdfRect::GetRight() const
{
    return X + Width;
}

double PdfRect::GetTop() const
{
    return Y + Height;
}

void PdfRect::Intersect(const PdfRect& rect)
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

PdfRect PdfRect::operator*(const Matrix& m) const
{
    Vector2 corner1(X, Y);
    Vector2 corner2(GetRight(), GetTop());
    corner1 = corner1 * m;
    corner2 = corner2 * m;
    return PdfRect::FromCorners(corner1.X, corner1.Y, corner2.X, corner2.Y);
}

void CreateRect(double x1, double y1, double x2, double y2, double& left, double& bottom, double& width, double& height)
{
    // See Pdf Reference 1.7, 3.8.4 Rectangles
    NormalizeCoordinates(x1, x2);
    NormalizeCoordinates(y1, y2);

    left = x1;
    bottom = y1;
    width = x2 - x1;
    height = y2 - y1;
}

void NormalizeCoordinates(double& coord1, double& coord2)
{
    if (coord1 > coord2)
    {
        double temp = coord1;
        coord1 = coord2;
        coord2 = temp;
    }
}

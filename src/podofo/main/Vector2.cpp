/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "Vector2.h"
#include "Matrix.h"

using namespace PoDoFo;

Vector2::Vector2()
    : X(0), Y(0) { }

Vector2::Vector2(double x, double y)
    : X(x), Y(y) { }

double Vector2::GetLength() const
{
    return std::sqrt(X * X + Y * Y);
}

double Vector2::GetSquaredLength() const
{
    return X * X + Y * Y;
}

Vector2 Vector2::operator+(const Vector2& v) const
{
    return Vector2(X + v.X, Y + v.Y);
}

Vector2 Vector2::operator-(const Vector2& v) const
{
    return Vector2(X - v.X, Y - v.Y);
}

Vector2 Vector2::operator*(const Matrix& m) const
{
    return Vector2(
        m[0] * X + m[2] * Y + m[4],
        m[1] * X + m[3] * Y + m[5]
    );
}

Vector2& Vector2::operator+=(const Vector2& v)
{
    X += v.X;
    Y += v.Y;
    return *this;
}

Vector2& Vector2::operator-=(const Vector2& v)
{
    X -= v.X;
    Y -= v.Y;
    return *this;
}

double Vector2::Dot(const Vector2& v) const
{
	return X * v.X + Y * v.Y;
}

bool Vector2::operator==(const Vector2& v) const
{
    return X == v.X && Y == v.Y;
}

bool Vector2::operator!=(const Vector2& v) const
{
    return X != v.X || Y != v.Y;
}

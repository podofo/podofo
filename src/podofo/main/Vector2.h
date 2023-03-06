/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_VECTOR2_H
#define PDF_VECTOR2_H

#include "PdfDeclarations.h"

namespace PoDoFo
{
    class Matrix;

    class PODOFO_API Vector2 final
    {
    public:
        Vector2();
        Vector2(double x, double y);

    public:
        double GetLength() const;
        double GetSquaredLength() const;

        Vector2 operator+(const Vector2& v) const;
        Vector2 operator-(const Vector2& v) const;
        Vector2 operator*(const Matrix& m) const;

        Vector2& operator+=(const Vector2& v);
        Vector2& operator-=(const Vector2& v);

        double Dot(const Vector2& v) const;

        bool operator==(const Vector2& v) const;
        bool operator!=(const Vector2& v) const;

    public:
        Vector2(const Vector2&) = default;
        Vector2& operator=(const Vector2&) = default;

    public:
        double X;
        double Y;
    };
}

#endif // PDF_VECTOR2_H

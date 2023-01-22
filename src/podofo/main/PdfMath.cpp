/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfMath.h"
#include "PdfArray.h"
#include "PdfRect.h"

using namespace PoDoFo;

// 4.2.3 Transformation Matrices
//  Convention: 1) Row-major vectors (as opposite to column vectors)
//              2) Row-major matrix storage
//
//  | x' y' 1 | = | x y 1 | * | a b 0 |
//                            | c d 0 |
//                            | e f 1 |
//

Matrix::Matrix()
    : Matrix(1, 0, 0, 1, 0, 0) { }

Matrix Matrix::FromArray(const double arr[6])
{
    return Matrix(arr);
}

Matrix Matrix::FromArray(const PdfArray& arr)
{
    return Matrix(arr[0].GetReal(), arr[1].GetReal(), arr[2].GetReal(), arr[3].GetReal(), arr[4].GetReal(), arr[5].GetReal());
}

Matrix Matrix::FromCoefficients(double a, double b, double c, double d, double e, double f)
{
    return Matrix(a, b, c, d, e, f);
}

Matrix Matrix::CreateTranslation(const Vector2& tx)
{
    // NOTE: Pdf treats vectors as row. See Pdf Reference  1.7 p.205
    return Matrix(1, 0, 0, 1, tx.X, tx.Y);
}

Matrix Matrix::CreateScale(const Vector2& scale)
{
    return Matrix(scale.X, 0, 0, scale.Y, 0, 0);
}

Matrix Matrix::CreateRotation(double teta)
{
    return CreateRotation(Vector2(), teta);
}

//
//  | alpha                                       beta                 0 |
//  | -beta                                       alpha                0 |
//  |  -Cx * alpha + Cy * beta + Cx   -Cx * beta - Cy * alpha + Cy     1 |
//
//  alpha = cos(teta)
//  beta = sin(teta)
//
Matrix Matrix::CreateRotation(const Vector2& c, double teta)
{
    // NOTE: Pdf treats vectors as row. See Pdf Reference  1.7 p.205
    double alpha = cos(teta);
    double beta = sin(teta);
    return Matrix(alpha, beta, -beta, alpha, -c.X * alpha + c.Y * beta + c.X, -c.X * beta - c.Y * alpha + c.Y);
}

Matrix& Matrix::Translate(const Vector2& tx)
{
    m_mat[4] = tx.X * m_mat[0] + tx.Y * m_mat[2] + m_mat[4];
    m_mat[5] = tx.X * m_mat[1] + tx.Y * m_mat[3] + m_mat[5];
    return *this;
}

Matrix Matrix::Translated(const Vector2& tx) const
{
    auto ret = *this;
    ret.Translate(tx);
    return ret;
}

Matrix Matrix::operator*(const Matrix& m2) const
{
    auto m1 = m_mat;
    return Matrix(
        m1[0] * m2[0] + m1[1] * m2[2],
        m1[0] * m2[1] + m1[1] * m2[3],
        m1[2] * m2[0] + m1[3] * m2[2],
        m1[2] * m2[1] + m1[3] * m2[3],
        m1[4] * m2[0] + m1[5] * m2[2] + m2[4],
        m1[4] * m2[1] + m1[5] * m2[3] + m2[5]);
}

Matrix Matrix::GetScalingRotation() const
{
    return Matrix(m_mat[0], m_mat[1], m_mat[2], m_mat[3], 0, 0);
}

Matrix Matrix::GetRotation() const
{
    double scalex = std::sqrt(m_mat[0] * m_mat[0] + m_mat[2] * m_mat[2]);
    double scaley = std::sqrt(m_mat[1] * m_mat[1] + m_mat[3] * m_mat[3]);
    return Matrix(m_mat[0] / scalex, m_mat[1] / scaley, m_mat[2] / scalex, m_mat[3] / scaley, 0, 0);
}

Vector2 Matrix::GetScaleVector() const
{
    return Vector2(
        std::sqrt(m_mat[0] * m_mat[0] + m_mat[2] * m_mat[2]),
        std::sqrt(m_mat[1] * m_mat[1] + m_mat[3] * m_mat[3])
    );
}

Vector2 Matrix::GetTranslationVector() const
{
    return Vector2(m_mat[4], m_mat[5]);
}

void Matrix::ToArray(double arr[6]) const
{
    arr[0] = m_mat[0];
    arr[1] = m_mat[1];
    arr[2] = m_mat[2];
    arr[3] = m_mat[3];
    arr[4] = m_mat[4];
    arr[5] = m_mat[5];
}

void Matrix::ToArray(PdfArray& arr) const
{
    arr.Clear();
    arr.Add(m_mat[0]);
    arr.Add(m_mat[1]);
    arr.Add(m_mat[2]);
    arr.Add(m_mat[3]);
    arr.Add(m_mat[4]);
    arr.Add(m_mat[5]);
}

bool Matrix::operator==(const Matrix& m) const
{
    return m_mat[0] == m.m_mat[0]
        && m_mat[1] == m.m_mat[1]
        && m_mat[2] == m.m_mat[2]
        && m_mat[3] == m.m_mat[3]
        && m_mat[4] == m.m_mat[4]
        && m_mat[5] == m.m_mat[5];
}

bool Matrix::operator!=(const Matrix& m) const
{
    return m_mat[0] != m.m_mat[0]
        || m_mat[1] != m.m_mat[1]
        || m_mat[2] != m.m_mat[2]
        || m_mat[3] != m.m_mat[3]
        || m_mat[4] != m.m_mat[4]
        || m_mat[5] != m.m_mat[5];
}

const double& Matrix::operator[](unsigned idx) const
{
    return m_mat[idx];
}

Matrix::Matrix(const double arr[6])
{
    std::memcpy(m_mat, arr, sizeof(double) * 6);
}

Matrix::Matrix(double a, double b, double c, double d, double e, double f)
{
    m_mat[0] = a;
    m_mat[1] = b;
    m_mat[2] = c;
    m_mat[3] = d;
    m_mat[4] = e;
    m_mat[5] = f;
}

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

double PoDoFo::Vector2::Dot(const Vector2& v) const
{
	return X * v.X + Y * v.Y;
}

Matrix PoDoFo::GetFrameRotationTransform(const PdfRect& rect, double teta)
{
    auto R = Matrix::CreateRotation(teta);

    Vector2 leftBottom(rect.GetLeft(), rect.GetBottom());
    Vector2 rightTop(rect.GetRight(), rect.GetTop());

    // Rotate the rect
    auto corner1 = leftBottom * R;
    auto corner2 = rightTop * R;
    auto rect_1 = PdfRect::FromCorners(corner1.X, corner1.Y, corner2.X, corner2.Y);

    // Find the axis align translation
    Vector2 leftBottom_1(rect_1.GetLeft(), rect_1.GetBottom());
    Vector2 alignTx_1 = leftBottom - leftBottom_1;
    return  R * Matrix::CreateTranslation(alignTx_1);
}

Matrix PoDoFo::GetFrameRotationTransformInverse(const PdfRect& rect, double teta)
{
    auto R = Matrix::CreateRotation(teta);
    auto R_inv = Matrix::CreateRotation(-teta);

    Vector2 leftBottom(rect.GetLeft(), rect.GetBottom());
    Vector2 rightTop(rect.GetRight(), rect.GetTop());

    // Rotate rect to the canonical frame
    auto corner1 = leftBottom * R;
    auto corner2 = rightTop * R;
    auto rect_1 = PdfRect::FromCorners(corner1.X, corner1.Y, corner2.X, corner2.Y);

    // Find the axis align translation in the canonical frame
    Vector2 leftBottom_1(rect_1.GetLeft(), rect_1.GetBottom());
    Vector2 alignTx_1 = leftBottom_1 - leftBottom;
    return Matrix::CreateTranslation(alignTx_1) * R_inv;
}

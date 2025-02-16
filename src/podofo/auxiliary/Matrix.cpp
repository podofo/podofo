/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "Matrix.h"
#include <podofo/main/PdfArray.h>

using namespace PoDoFo;

// 4.2.3 Transformation Matrices
//  Convention: 1) Row-major vectors (as opposite to column vectors)
//              2) Row-major matrix storage
//
//  | x' y' 1 | = | x y 1 | * | a b 0 |
//                            | c d 0 |
//                            | e f 1 |
//

Matrix Matrix::Identity;

Matrix::Matrix()
    : m_mat{ 1, 0, 0, 1, 0, 0 } { }

Matrix Matrix::FromArray(const double arr[6])
{
    return Matrix(arr);
}

Matrix Matrix::FromArray(const PdfArray& arr)
{
    return Matrix(arr[0].GetReal(), arr[1].GetReal(), arr[2].GetReal(), arr[3].GetReal(), arr[4].GetReal(), arr[5].GetReal());
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

PdfArray Matrix::ToArray() const
{
    PdfArray arr;
    arr.Add(m_mat[0]);
    arr.Add(m_mat[1]);
    arr.Add(m_mat[2]);
    arr.Add(m_mat[3]);
    arr.Add(m_mat[4]);
    arr.Add(m_mat[5]);
    return arr;
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

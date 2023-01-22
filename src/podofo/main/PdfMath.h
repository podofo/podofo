/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_MATH_H
#define PDF_MATH_H

#include "PdfDeclarations.h"
#include "PdfMathBase.h"

namespace PoDoFo
{
    class PdfRect;
    class PdfArray;

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

    public:
        Vector2(const Vector2&) = default;
        Vector2& operator=(const Vector2&) = default;

    public:
        double X;
        double Y;
    };

    class PODOFO_API Matrix final
    {
    public:
        /** Constructs an identity matrix
         */
        Matrix();

    public:
        static Matrix FromArray(const double arr[6]);
        static Matrix FromArray(const PdfArray& arr);
        static Matrix FromCoefficients(double a, double b, double c, double d, double e, double f);
        static Matrix CreateTranslation(const Vector2& tx);
        static Matrix CreateScale(const Vector2& scale);
        static Matrix CreateRotation(double teta);
        static Matrix CreateRotation(const Vector2& center, double teta);

    public:
        // Prepend the given translation to the current matrix
        Matrix& Translate(const Vector2& tx);

        // Return the matrix with the given translation prepended
        Matrix Translated(const Vector2& tx) const;

        // TODO: Rotate/Scale

    public:
        template <AlgebraicTrait trait>
        double Get() const
        {
            return MatrixTraits<trait>::Get(m_mat);
        }

        template <AlgebraicTrait trait>
        void Set(double value)
        {
            MatrixTraits<trait>::Set(m_mat, value);
        }

        // Apply (prepend) the given operation
        template <AlgebraicTrait trait>
        Matrix& Apply(double value)
        {
            MatrixTraits<trait>::Apply(m_mat, value);
            return *this;
        }

    public:
        Matrix operator*(const Matrix& m) const;

    public:
        Matrix GetScalingRotation() const;
        Matrix GetRotation() const;
        Vector2 GetScaleVector() const;
        Vector2 GetTranslationVector() const;
        void ToArray(double arr[6]) const;
        void ToArray(PdfArray& arr) const;

    public:
        Matrix(const Matrix&) = default;
        Matrix& operator=(const Matrix&) = default;
        bool operator==(const Matrix& m) const;
        bool operator!=(const Matrix& m) const;

    public:
        const double& operator[](unsigned idx) const;

    private:
        Matrix(const double arr[6]);
        Matrix(double a, double b, double c, double d, double e, double f);

    private:
        double m_mat[6];
    };

    /** Get a rotation trasformation that aligns the rectangle to the axis after the rotation
     */
    Matrix PODOFO_API GetFrameRotationTransform(const PdfRect& rect, double teta);

    /** Get an inverse rotation trasformation that aligns the rectangle to the axis after the rotation
     */
    Matrix PODOFO_API GetFrameRotationTransformInverse(const PdfRect& rect, double teta);
}

#endif // PDF_MATH_H

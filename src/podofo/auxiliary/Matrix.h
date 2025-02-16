/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef AUX_MATRIX_H
#define AUX_MATRIX_H

#include "MathBase.h"
#include "Vector2.h"

namespace PoDoFo
{
    class PdfArray;

    class PODOFO_API Matrix final
    {
    public:
        static Matrix Identity;

    public:
        /** Constructs an identity matrix
         */
        Matrix();
        Matrix(double a, double b, double c, double d, double e, double f);

    public:
        static Matrix FromArray(const double arr[6]);
        static Matrix FromArray(const PdfArray& arr);
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
        PdfArray ToArray() const;

    public:
        Matrix(const Matrix&) = default;
        Matrix& operator=(const Matrix&) = default;
        bool operator==(const Matrix& m) const;
        bool operator!=(const Matrix& m) const;

    public:
        const double& operator[](unsigned idx) const;

    private:
        Matrix(const double arr[6]);

    private:
        double m_mat[6];
    };
}

#endif // AUX_MATRIX_H

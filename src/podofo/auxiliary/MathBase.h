/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef AUX_MATH_BASE_H
#define AUX_MATH_BASE_H

namespace PoDoFo
{
    enum class AlgebraicTrait
    {
        Tx, ///< X Translation
        Ty, ///< Y Translation
        // TODO: Scale/Rotation traits
    };

    constexpr AlgebraicTrait Tx = AlgebraicTrait::Tx;   ///< X Translation trait
    constexpr AlgebraicTrait Ty = AlgebraicTrait::Ty;   ///< X Translation trait

    template <AlgebraicTrait>
    struct MatrixTraits;

    template <>
    struct MatrixTraits<Tx>
    {
        static double Get(const double m[6])
        {
            return m[4];
        }

        static void Set(double m[6], double value)
        {
            m[4] = value;
        }

        static void Apply(double m[6], double value)
        {
            m[4] = value * m[0] + m[4];
        }
    };

    template <>
    struct MatrixTraits<Ty>
    {
        static double Get(const double m[6])
        {
            return m[5];
        }

        static void Set(double m[6], double value)
        {
            m[5] = value;
        }

        static void Apply(double m[6], double value)
        {
            m[5] = value * m[3] + m[5];
        }
    };
}

#endif // AUX_MATH_BASE_H

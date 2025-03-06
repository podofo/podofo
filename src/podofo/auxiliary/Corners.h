/**
 * SPDX-FileCopyrightText: (C) 2025 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef AUX_CORNERS_H
#define AUX_CORNERS_H

#include "Vector2.h"

namespace PoDoFo {

class PdfArray;
class Rect;

/** An unoriented rectangle defined by 2 points
 */
class PODOFO_API Corners final
{
public:
    double X1;
    double Y1;
    double X2;
    double Y2;

public:
    /** Create an empty rectangle
     */
    Corners();

    /** Create a rectangle from 2 points
     */
    Corners(double x1, double y1, double x2, double y2);

    Corners(const Corners& rhs) = default;

public:
    static Corners FromCorners(const Vector2& corner1, const Vector2& corner2);

    /** Create a Corners instance from a the 4 values in the array
     *  \param arr the array to load the values from
     */
    static Corners FromArray(const PdfArray& arr);

    Vector2 GetCorner1() const;

    Vector2 GetCorner2() const;

    double GetWidth() const;

    double GetHeight() const;

    /** Get the normalized rectangle defined by position (left-bottom) and size
     */
    Rect GetNormalized() const;

    /** Converts the rectangle into an array
     */
    void ToArray(PdfArray& arr) const;
    PdfArray ToArray() const;

public:
    bool operator==(const Corners& rect) const;
    bool operator!=(const Corners& rect) const;
    Corners& operator=(const Corners& rhs) = default;
    explicit operator Rect() const;
};

};

#endif // AUX_CORNERS_H

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
    Vector2 GetCorner1() const;

    Vector2 GetCorner2() const;

    /** Converts the rectangle into an array
     */
    void ToArray(PdfArray& arr) const;
    PdfArray ToArray() const;

public:
    bool operator==(const Corners& rect) const;
    bool operator!=(const Corners& rect) const;
    Corners& operator=(const Corners& rhs) = default;
};

};

#endif // AUX_CORNERS_H

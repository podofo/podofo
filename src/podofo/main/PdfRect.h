/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_RECT_H
#define PDF_RECT_H

#include "PdfDeclarations.h"

namespace PoDoFo {

class PdfArray;
class Matrix;

/** A rectangle as defined by the PDF reference
 */
class PODOFO_API PdfRect final
{
public:
    double X;
    double Y;
    double Width;
    double Height;

public:
    /** Create an empty rectangle with bottom=left=with=height=0
     */
    PdfRect();

    /** Create a rectangle with a given size and position
     *  All values are in PDF units
     *	NOTE: PDF rect is normally bottom-left origined
     */
    PdfRect(double x, double y, double width, double height);

    /** Copy constructor
     */
    PdfRect(const PdfRect& rhs) = default;

public:
    /** Create a PdfRect from a couple of arbitrary points
     * \returns the created PdfRect
     */
    static PdfRect FromCorners(double x1, double y1, double x2, double y2);

    /** Create a PdfRect from a the 4 values in the array
     *  \param arr the array to load the values from
     */
    static PdfRect FromArray(const PdfArray& arr);

    /** Converts the rectangle into an array
     *  based on PDF units
     *  \param var the array to store the Rect
     */
    void ToArray(PdfArray& arr) const;

    /** Returns a string representation of the PdfRect
     * \returns std::string representation as [ left bottom right top ]
     */
    std::string ToString() const;

    bool Contains(double x, double y) const;

    // REMOVE-ME: The name of this method is bad and it's also
    /** Intersect with another rect
     *  \param rect the rect to intersect with
     */
    void Intersect(const PdfRect& rect);

public:
    /** Get the left coordinate of the rectangle
     */
    double GetLeft() const { return X; }

    /** Get the bottom coordinate of the rectangle
     */
    double GetBottom() const { return Y; }

    /** Get the right coordinate of the rectangle
     */
    double GetRight() const;

    /** Get the top coordinate of the rectangle
     */
    double GetTop() const;

public:
    PdfRect operator*(const Matrix& m) const;
    PdfRect& operator=(const PdfRect& rhs) = default;
};

};

#endif // PDF_RECT_H

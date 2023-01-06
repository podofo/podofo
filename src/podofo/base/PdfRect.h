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
    /** Create an empty rectangle with bottom=left=with=height=0
     */
    PdfRect();

    /** Create a rectangle with a given size and position
     *  All values are in PDF units
     *	NOTE: since PDF is bottom-left origined, we pass the bottom instead of the top
     */
    PdfRect(double left, double bottom, double width, double height);

    /** Create a rectangle from an array
     *  All values are in PDF units
     */
    PdfRect(const PdfArray& inArray);

    /** Copy constructor
     */
    PdfRect(const PdfRect& rhs) = default;

public:
    /** Create a PdfRect from a couple of arbitrary points
     * \returns the created PdfRect
     */
    static PdfRect FromCorners(double x1, double y1, double x2, double y2);

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

    /** Assigns the values of this PdfRect from the 4 values in the array
     *  \param inArray the array to load the values from
     */
    void FromArray(const PdfArray& arr);

    // REMOVE-ME: The name of this method is bad and it's also
    /** Intersect with another rect
     *  \param rect the rect to intersect with
     */
    void Intersect(const PdfRect& rect);

    PdfRect operator*(const Matrix& m) const;

public:
    /** Get the right coordinate of the rectangle
     *  \returns bottom
     */
    double GetRight() const;

    /** Get the top coordinate of the rectangle
     *  \returns bottom
     */
    double GetTop() const;

    /** Get the bottom coordinate of the rectangle
     *  \returns bottom
     */
    inline double GetBottom() const { return m_Bottom; }

    /** Set the bottom coordinate of the rectangle
     *  \param bottom
     */
    inline void SetBottom(double bottom) { m_Bottom = bottom; }

    /** Get the left coordinate of the rectangle
     *  \returns left in PDF units
     */
    inline double GetLeft() const { return m_Left; }

    /** Set the left coordinate of the rectangle
     *  \param left in PDF units
     */
    inline void SetLeft(double left) { m_Left = left; }

    /** Get the width of the rectangle
     *  \returns width in PDF units
     */
    inline double GetWidth() const { return m_Width; }

    /** Set the width of the rectangle
     *  \param lWidth in PDF units
     */
    inline void SetWidth(double width) { m_Width = width; }

    /** Get the height of the rectangle
     *  \returns height in PDF units
     */
    inline double GetHeight() const { return m_Height; }

    /** Set the height of the rectangle
     *  \param lHeight in PDF units
     */
    inline void SetHeight(double height) { m_Height = height; }

public:
    PdfRect& operator=(const PdfRect& rhs) = default;

private:
    double m_Left;
    double m_Bottom;
    double m_Width;
    double m_Height;
};

};

#endif // PDF_RECT_H

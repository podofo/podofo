/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _PDF_RECT_H_
#define _PDF_RECT_H_

#include "PdfDefines.h"


namespace PoDoFo {

class PdfPage;
class PdfVariant;
   
/** A rectangle as defined by the PDF reference
 */
class PdfRect {
 public:
    /** Create an empty rectangle with top=left=with=height=0
     */
    PdfRect();

    /** Create a rectangle with a given size and position
     *  All values are in 1/1000th mm
     */
    PdfRect( long left, long top, long width, long height );

    /** Copy constructor 
     */
    PdfRect( const PdfRect & rhs );

    /** Converts the rectangle into an array
     *  with all coordinates converted from 1/1000th mm to
     *  PDF units and adds the array into an variant.
     *  \param var the variant to store the Rect
     *  \param pPage if a page is passed the y coordinate is transformed correctly
     */
    void ToVariant( PdfVariant & var, PdfPage* pPage = NULL ) const;

    /** Get the top coordinate of the rectangle
     *  \returns top in 1/1000th mm
     */
    inline long Top() const;

    /** Set the top coordinate of the rectangle
     *  \param lTop in 1/1000th mm
     */
    inline void SetTop( long lTop );

    /** Get the left coordinate of the rectangle
     *  \returns left in 1/1000th mm
     */
    inline long Left() const;

    /** Set the left coordinate of the rectangle
     *  \param lLeft in 1/1000th mm
     */
    inline void SetLeft( long lLeft );

    /** Get the width of the rectangle
     *  \returns width in 1/1000th mm
     */
    inline long Width() const;

    /** Set the width of the rectangle
     *  \param lWidth in 1/1000th mm
     */
    inline void SetWidth( long lWidth );

    /** Get the height of the rectangle
     *  \returns height in 1/1000th mm
     */
    inline long Height() const;

    /** Set the height of the rectangle
     *  \param lHeight in 1/1000th mm
     */
    inline void SetHeight( long lHeight );

    PdfRect & operator=( const PdfRect & rhs );

 private:
    long m_lTop;
    long m_lLeft;
    long m_lWidth;
    long m_lHeight;
};

long PdfRect::Top() const
{
    return m_lTop;
}

void PdfRect::SetTop( long lTop )
{
    m_lTop = lTop;
}

long PdfRect::Left() const
{
    return m_lLeft;
}

void PdfRect::SetLeft( long lLeft )
{
    m_lLeft = lLeft;
}

long PdfRect::Width() const
{
    return m_lWidth;
}

void PdfRect::SetWidth( long lWidth )
{
    m_lWidth = lWidth;
}

long PdfRect::Height() const
{
    return m_lHeight;
}

void PdfRect::SetHeight( long lHeight )
{
    m_lHeight = lHeight;
}

};

#endif /* _PDF_RECT_H_ */

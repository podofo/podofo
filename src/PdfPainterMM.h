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

#ifndef _PDF_PAINTER_MM_H_
#define _PDF_PAINTER_MM_H_

#include "PdfDefines.h"
#include "PdfPainter.h"

namespace PoDoFo {

class PdfCanvas;
class PdfFont;
class PdfImage;
class PdfName;
class PdfObject;
class PdfReference;
class PdfStream;
class PdfString;
class PdfXObject;

#ifndef CONVERSION_CONSTANT
/** \def CONVERSION_CONSTANT
 *  Conversation constant to convert 1/1000th mm to 1/72 inch
 *  Internal use only.
 */
#define CONVERSION_CONSTANT 0.002834645669291339
#endif // CONVERSION_CONSTANT

/**
 * This class provides an easy to use painter object which allows you to draw on a PDF page
 * object.
 * 
 * During all drawing operations, you are still able to access the stream of the object you are
 * drawing on directly. 
 * 
 * This painter takes all coordinates in 1/1000th mm instead of PDF units.
 *
 * Developer note: we use ownership rather than inheritance here, so as to use the same
 * methods names a PdfPainter AND avoid compiler confusion on picking the right one.
 *
 * \see PdfPainter 
 */
class PdfPainterMM : public PdfPainter {
 public:
    /** Create a new PdfPainterMM object.
     */
    PdfPainterMM() {}
    virtual ~PdfPainterMM() {}
    
    /** Set the line width for all stroking operations.
     *  \param lWidth in 1/1000th mm
     */
    inline void SetStrokeWidthMM( long lWidth );

    /** Draw a line with the current color and line settings.
     *  \param lStartX x coordinate of the starting point
     *  \param lStartY y coordinate of the starting point
     *  \param lEndX x coordinate of the ending point
     *  \param lEndY y coordinate of the ending point
     */
    inline void DrawLineMM( long lStartX, long lStartY, long lEndX, long lEndY );

    /** Draw a rectangle with the current stroking settings
     *  \param lX x coordinate of the rectangle
     *  \param lY y coordinate of the rectangle
     *  \param lWidth width of the rectangle
     *  \param lHeight absolute height of the rectangle
     */
    inline void DrawRectMM( long lX, long lY, long lWidth, long lHeight );

    /** Fill a rectangle with the current fill settings
     *  \param lX x coordinate of the rectangle
     *  \param lY y coordinate of the rectangle
     *  \param lWidth width of the rectangle 
     *  \param lHeight absolute height of the rectangle
     */
    inline void FillRectMM( long lX, long lY, long lWidth, long lHeight );

    /** Draw an ellipse with the current stroking settings
     *  \param lX x coordinate of the ellipse (left coordinate)
     *  \param lY y coordinate of the ellipse (top coordinate)
     *  \param lWidth width of the ellipse
     *  \param lHeight absolute height of the ellipse
     */
    inline void DrawEllipseMM( long lX, long lY, long lWidth, long lHeight ); 

    /** Fill an ellipse with the current fill settings
     *  \param lX x coordinate of the ellipse (left coordinate)
     *  \param lY y coordinate of the ellipse (top coordinate)
     *  \param lWidth width of the ellipse 
     *  \param lHeight absolute height of the ellipse
     */
    inline void FillEllipseMM( long lX, long lY, long lWidth, long lHeight ); 

    /** Draw a text string on a page using a given font object.
     *  You have to call SetFont before calling this function.
     *  \param lX the x coordinate
     *  \param lY the y coordinate
     *  \param sText the text string which should be printed 
     *
     *  \see PdfPainter::SetFont()
     */
    inline void DrawTextMM( long lX, long lY, const PdfString & sText);

    /** Draw a text string on a page using a given font object.
     *  You have to call SetFont before calling this function.
     *  \param lX the x coordinate
     *  \param lY the y coordinate
     *  \param sText the text string which should be printed (is not allowed to be NULL!)
     *  \param lLen draw only lLen characters of pszText
     *
     *  \see PdfPainter::SetFont()
     */
    inline void DrawTextMM( long lX, long lY, const PdfString & sText, long lLen );

    /** Draw an image on the current page.
     *  \param lX the x coordinate (bottom left position of the image)
     *  \param lY the y coordinate (bottom position of the image)
     *  \param pObject an PdfXObject
     *  \param dScaleX option scaling factor in x direction
     *  \param dScaleY option scaling factor in y direction
     */
    inline void DrawImageMM( long lX, long lY, PdfImage* pObject, double dScaleX = 1.0, double dScaleY = 1.0);

    /** Draw an XObject on the current page.
     *  \param lX the x coordinate (bottom left position of the XObject)
     *  \param lY the y coordinate (bottom position of the XObject)
     *  \param pObject an PdfXObject
     *  \param dScaleX option scaling factor in x direction
     *  \param dScaleY option scaling factor in y direction
     */
    inline void DrawXObjectMM( long lX, long lY, PdfXObject* pObject, double dScaleX = 1.0, double dScaleY = 1.0);

    /** Append a line segment to the current path. Matches the PDF 'l' operator.
     *  This function is useful to construct an own path
     *  for drawing or clipping.
     *  \param lX x position
     *  \param lY y position
     */
    inline void LineToMM( long lX, long lY );

    /** Begin a new path. Matches the PDF 'm' operator. 
     *  This function is useful to construct an own path
     *  for drawing or clipping.
     *  \param lX x position
     *  \param lY y position
     */
    inline void MoveToMM( long lX, long lY );
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::SetStrokeWidthMM( long lWidth )
{
    this->SetStrokeWidth( (double)lWidth * CONVERSION_CONSTANT );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::DrawLineMM( long lStartX, long lStartY, long lEndX, long lEndY )
{
    this->DrawLine( (double)lStartX * CONVERSION_CONSTANT,
                       (double)lStartY * CONVERSION_CONSTANT,
                       (double)lEndX   * CONVERSION_CONSTANT,
                       (double)lEndY   * CONVERSION_CONSTANT );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::DrawRectMM( long lX, long lY, long lWidth, long lHeight )
{
    this->DrawRect( (double)lX      * CONVERSION_CONSTANT,
                       (double)lY      * CONVERSION_CONSTANT,
                       (double)lWidth  * CONVERSION_CONSTANT,
                       (double)lHeight * CONVERSION_CONSTANT );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::FillRectMM( long lX, long lY, long lWidth, long lHeight )
{
    this->FillRect( (double)lX      * CONVERSION_CONSTANT,
                       (double)lY      * CONVERSION_CONSTANT,
                       (double)lWidth  * CONVERSION_CONSTANT,
                       (double)lHeight * CONVERSION_CONSTANT );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::DrawEllipseMM( long lX, long lY, long lWidth, long lHeight )
{
    this->DrawEllipse( (double)lX      * CONVERSION_CONSTANT,
                          (double)lY      * CONVERSION_CONSTANT,
                          (double)lWidth  * CONVERSION_CONSTANT,
                          (double)lHeight * CONVERSION_CONSTANT );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::FillEllipseMM( long lX, long lY, long lWidth, long lHeight )
{
    this->FillEllipse( (double)lX      * CONVERSION_CONSTANT,
                          (double)lY      * CONVERSION_CONSTANT,
                          (double)lWidth  * CONVERSION_CONSTANT,
                          (double)lHeight * CONVERSION_CONSTANT );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::DrawTextMM( long lX, long lY, const PdfString & sText)
{
    this->DrawText( (double)lX * CONVERSION_CONSTANT,
                       (double)lY * CONVERSION_CONSTANT,
                       sText );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::DrawTextMM( long lX, long lY, const PdfString & sText, long lLen )
{
   this->DrawText( (double)lX * CONVERSION_CONSTANT,
                      (double)lY * CONVERSION_CONSTANT,
                      sText, lLen );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::DrawImageMM( long lX, long lY, PdfImage* pObject, double dScaleX, double dScaleY )
{
   this->DrawImage( (double)lX * CONVERSION_CONSTANT,
                       (double)lY * CONVERSION_CONSTANT,
                       pObject, dScaleX, dScaleY );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::DrawXObjectMM( long lX, long lY, PdfXObject* pObject, double dScaleX, double dScaleY )
{
   this->DrawXObject( (double)lX * CONVERSION_CONSTANT,
                         (double)lY * CONVERSION_CONSTANT,
                         pObject, dScaleX, dScaleY );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::LineToMM( long lX, long lY )
{
    this->LineTo( (double)lX * CONVERSION_CONSTANT,
                     (double)lY * CONVERSION_CONSTANT );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::MoveToMM( long lX, long lY )
{
    this->MoveTo( (double)lX * CONVERSION_CONSTANT,
                     (double)lY * CONVERSION_CONSTANT );
}


};

#endif // _PDF_PAINTER_MM_H_

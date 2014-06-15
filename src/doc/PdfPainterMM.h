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
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#ifndef _PDF_PAINTER_MM_H_
#define _PDF_PAINTER_MM_H_

#include "podofo/base/PdfDefines.h"
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
class PODOFO_DOC_API PdfPainterMM : public PdfPainter {
 public:
    /** Create a new PdfPainterMM object.
     */
    PdfPainterMM() {}

    virtual ~PdfPainterMM();
    
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

    /** Add a rectangle into the current path
     *  \param lX x coordinate of the rectangle
     *  \param lY y coordinate of the rectangle
     *  \param lWidth width of the rectangle
     *  \param lHeight absolute height of the rectangle
     */
    inline void RectangleMM( long lX, long lY, long lWidth, long lHeight );

    /** Add an ellipse into the current path
     *  \param lX x coordinate of the ellipse (left coordinate)
     *  \param lY y coordinate of the ellipse (top coordinate)
     *  \param lWidth width of the ellipse
     *  \param lHeight absolute height of the ellipse
     */
    inline void EllipseMM( long lX, long lY, long lWidth, long lHeight ); 

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
    this->SetStrokeWidth( static_cast<double>(lWidth) * CONVERSION_CONSTANT );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::DrawLineMM( long lStartX, long lStartY, long lEndX, long lEndY )
{
    this->DrawLine( static_cast<double>(lStartX) * CONVERSION_CONSTANT,
                    static_cast<double>(lStartY) * CONVERSION_CONSTANT,
                    static_cast<double>(lEndX)   * CONVERSION_CONSTANT,
                    static_cast<double>(lEndY)   * CONVERSION_CONSTANT );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::RectangleMM( long lX, long lY, long lWidth, long lHeight )
{
    this->Rectangle( static_cast<double>(lX)      * CONVERSION_CONSTANT,
                       static_cast<double>(lY)      * CONVERSION_CONSTANT,
                       static_cast<double>(lWidth)  * CONVERSION_CONSTANT,
                       static_cast<double>(lHeight) * CONVERSION_CONSTANT );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::EllipseMM( long lX, long lY, long lWidth, long lHeight )
{
    this->Ellipse( static_cast<double>(lX)      * CONVERSION_CONSTANT,
                       static_cast<double>(lY)      * CONVERSION_CONSTANT,
                       static_cast<double>(lWidth)  * CONVERSION_CONSTANT,
                       static_cast<double>(lHeight) * CONVERSION_CONSTANT );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::DrawTextMM( long lX, long lY, const PdfString & sText)
{
    this->DrawText( static_cast<double>(lX) * CONVERSION_CONSTANT,
                    static_cast<double>(lY) * CONVERSION_CONSTANT,
                    sText );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::DrawTextMM( long lX, long lY, const PdfString & sText, long lLen )
{
   this->DrawText( static_cast<double>(lX) * CONVERSION_CONSTANT,
                   static_cast<double>(lY) * CONVERSION_CONSTANT,
                   sText, lLen );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::DrawImageMM( long lX, long lY, PdfImage* pObject, double dScaleX, double dScaleY )
{
   this->DrawImage( static_cast<double>(lX) * CONVERSION_CONSTANT,
                    static_cast<double>(lY) * CONVERSION_CONSTANT,
                    pObject, dScaleX, dScaleY );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::DrawXObjectMM( long lX, long lY, PdfXObject* pObject, double dScaleX, double dScaleY )
{
   this->DrawXObject( static_cast<double>(lX) * CONVERSION_CONSTANT,
                      static_cast<double>(lY) * CONVERSION_CONSTANT,
                      pObject, dScaleX, dScaleY );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::LineToMM( long lX, long lY )
{
    this->LineTo( static_cast<double>(lX) * CONVERSION_CONSTANT,
                  static_cast<double>(lY) * CONVERSION_CONSTANT );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPainterMM::MoveToMM( long lX, long lY )
{
    this->MoveTo( static_cast<double>(lX) * CONVERSION_CONSTANT,
                  static_cast<double>(lY) * CONVERSION_CONSTANT );
}


};

#endif // _PDF_PAINTER_MM_H_

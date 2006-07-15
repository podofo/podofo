/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
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

#ifndef _PDF_PAINTER_H_
#define _PDF_PAINTER_H_

#include "PdfDefines.h"

#define PDF_PAINTER_BUFFER 512

namespace PoDoFo {

class PdfCanvas;
class PdfFont;
class PdfImageRef;
class PdfName;
class PdfObject;
class PdfReference;
class PdfStream;
class PdfString;


/**
 * This class provides an easy to use painter object which allows you to draw on a PDF page
 * object.
 * 
 * During all drawing operations, you are still able to access the stream of the object you are
 * drawing on directly. 
 * 
 * All functions that take coordinates expect these to be in 1/1000mm. Keep in mind that PDF has
 * its coordinate system origin at the bottom left corner.
 */
class PdfPainter {
 public:
    /** Create a new PdfPainter object.
     */
    PdfPainter();
    virtual ~PdfPainter();

    /** Set the page on which the painter should draw.
     *  The painter will draw of course on the pages
     *  contents object.
     *  \param pPage a PdfCanvas object (most likely a PdfPage or PdfXObject).
     *
     *  \see PdfPage \see PdfXObject
     */
    void SetPage( PdfCanvas* pPage );

    /** Set the color for all following stroking operations
     *  in grayscale colorspace. This operation used the 'G'
     *  PDF operator.
     *  \param g gray scale value in the range 0.0 - 1.0
     */
    void SetStrokingGray( double g );

    /** Set the color for all following non-stroking operations
     *  in grayscale colorspace. This operation used the 'g'
     *  PDF operator.
     *  \param g gray scale value in the range 0.0 - 1.0
     */
    void SetGray( double g );

    /** Set the color for all following stroking operations
     *  in rgb colorspace. This operation used the 'RG'
     *  PDF operator.
     *  \param r red value in the range 0.0 - 1.0
     *  \param g green value in the range 0.0 - 1.0
     *  \param b blue value in the range 0.0 - 1.0
     */
    void SetStrokingColor( double r, double g, double b );

    /** Set the color for all following non-stroking operations
     *  in rgb colorspace. This operation used the 'rg'
     *  PDF operator.
     *
     *  This color is also used when drawing text.
     *
     *  \param r red value in the range 0.0 - 1.0
     *  \param g green value in the range 0.0 - 1.0
     *  \param b blue value in the range 0.0 - 1.0
     */
    void SetColor( double r, double g, double b );

    /** Set the color for all following stroking operations
     *  in cmyk colorspace. This operation used the 'K'
     *  PDF operator.
     *  \param c cyan value in the range 0.0 - 1.0
     *  \param m magenta value in the range 0.0 - 1.0
     *  \param y yellow value in the range 0.0 - 1.0
     *  \param k black value in the range 0.0 - 1.0
     */
    void SetStrokingColorCMYK( double c, double m, double y, double k );

    /** Set the color for all following non-stroking operations
     *  in cmyk colorspace. This operation used the 'k'
     *  PDF operator.
     *  \param c cyan value in the range 0.0 - 1.0
     *  \param m magenta value in the range 0.0 - 1.0
     *  \param y yellow value in the range 0.0 - 1.0
     *  \param k black value in the range 0.0 - 1.0
     */
    void SetColorCMYK( double c, double m, double y, double k );

    /** Set the line width for all stroking operations.
     *  \param lWidth in 1/1000th mm.
     */
    void SetStrokeWidth( long lWidth );

    /** Set the stoke style for all stroking operations.
     *  \param eStyle style of the stroking operations
     *  \param pszCustom a custom stroking style which is used when
     *                   eStyle == ePdfStrokeStyle_Custom.
     *
     *  Possible values:
     *    ePdfStrokeStyle_None
     *    ePdfStrokeStyle_Solid
     *    ePdfStrokeStyle_Dash
     *    ePdfStrokeStyle_Dot
     *    ePdfStrokeStyle_DashDot
     *    ePdfStrokeStyle_DashDotDot
     *    ePdfStrokeStyle_Custom
     *
     */
    void SetStrokeStyle( EPdfStrokeStyle eStyle, const char* pszCustom = NULL );

    /** Set the line cap style for all stroking operations.
     *  \param eCapStyle the cap style. 
     *
     *  Possible values:
     *    ePdfLineCapStyle_Butt,
     *    ePdfLineCapStyle_Round,
     *    ePdfLineCapStyle_Square
     */
    void SetLineCapStyle( EPdfLineCapStyle eCapStyle );

    /** Set the line join style for all stroking operations.
     *  \param eJoinStyle the join style. 
     *
     *  Possible values:
     *    ePdfLineJoinStyle_Miter
     *    ePdfLineJoinStyle_Round
     *    ePdfLineJoinStyle_Bevel
     */
    void SetLineJoinStyle( EPdfLineJoinStyle eJoinStyle );

    /** Set the font for all text drawing operations
     *  \param pFont a handle to a valid PdfFont object
     *
     *  \see DrawText
     */
    void SetFont( PdfFont* pFont );

    /** Get the current font:
     *  \returns a font object or NULL if no font was set.
     */
    inline PdfFont* Font() const;

    /** Draw a line with the current color and line settings.
     *  \param lStartX x coordinate of the starting point
     *  \param lStartY y coordinate of the starting point
     *  \param lEndX x coordinate of the ending point
     *  \param lEndY y coordinate of the ending point
     */
    void DrawLine( long lStartX, long lStartY, long lEndX, long lEndY );

    /** Draw a rectangle with the current stroking settings
     *  \param lX x coordinate of the rectangle
     *  \param lY y coordinate of the rectangle
     *  \param lWidth width of the rectangle
     *  \param lHeight absolute height of the rectangle
     */
    void DrawRect( long lX, long lY, long lWidth, long lHeight );

    /** Fill a rectangle with the current fill settings
     *  \param lX x coordinate of the rectangle
     *  \param lY y coordinate of the rectangle
     *  \param lWidth width of the rectangle 
     *  \param lHeight absolute height of the rectangle
     */
    void FillRect( long lX, long lY, long lWidth, long lHeight );

    /** Draw an ellipse with the current stroking settings
     *  \param lX x coordinate of the ellipse (left coordinate)
     *  \param lY y coordinate of the ellipse (top coordinate)
     *  \param lWidth width of the ellipse
     *  \param lHeight absolute height of the ellipse
     */
    void DrawEllipse( long lX, long lY, long lWidth, long lHeight ); 

    /** Fill an ellipse with the current fill settings
     *  \param lX x coordinate of the ellipse (left coordinate)
     *  \param lY y coordinate of the ellipse (top coordinate)
     *  \param lWidth width of the ellipse 
     *  \param lHeight absolute height of the ellipse
     */
    void FillEllipse( long lX, long lY, long lWidth, long lHeight ); 

    /** Draw a text string on a page using a given font object.
     *  You have to call SetFont before calling this function.
     *  \param lX the x coordinate
     *  \param lY the y coordinate
     *  \param sText the text string which should be printed 
     *
     *  \see SetFont()
     */
    void DrawText( long lX, long lY, const PdfString & sText);

    /** Draw a text string on a page using a given font object.
     *  You have to call SetFont before calling this function.
     *  \param lX the x coordinate
     *  \param lY the y coordinate
     *  \param sText the text string which should be printed (is not allowed to be NULL!)
     *  \param lLen draw only lLen characters of pszText
     *
     *  \see SetFont()
     */
    void DrawText( long lX, long lY, const PdfString & sText, long lLen );

    /** Draw an image on the current page.
     *  \param lX the x coordinate (bottom left position of the image)
     *  \param lY the y coordinate (bottom position of the image)
     *  \param pImageRef an image reference object
     *  \param dScaleX option scaling factor in x direction
     *  \param dScaleY option scaling factor in y direction
     */
    inline void DrawImage( long lX, long lY, PdfImageRef* pImageRef, double dScaleX = 1.0, double dScaleY = 1.0);

    /** Draw an XObject on the current page.
     *  \param lX the x coordinate (bottom left position of the XObject)
     *  \param lY the y coordinate (bottom position of the XObject)
     *  \param pImageRef an image reference object (an image reference is used
     *                   as XObjects and images are handled the same internally).
     *  \param dScaleX option scaling factor in x direction
     *  \param dScaleY option scaling factor in y direction
     */
    void DrawXObject( long lX, long lY, PdfImageRef* pImageRef, double dScaleX = 1.0, double dScaleY = 1.0);

    /** Closes the current path by drawing a line from the current point
     *  to the starting point of the path. Matches the PDF 'h' operator.
     *  This function is useful to construct an own path
     *  for drawing or clipping.
     */
    void ClosePath();

    /** Append a line segment to the current path. Matches the PDF 'l' operator.
     *  This function is useful to construct an own path
     *  for drawing or clipping.
     *  \param lX x position
     *  \param lY y position
     */
    void LineTo( long  lX, long lY );

    /** Begin a new path. Matches the PDF 'm' operator. 
     *  This function is useful to construct an own path
     *  for drawing or clipping.
     *  \param lX x position
     *  \param lY y position
     */
    void MoveTo( long  lX, long lY );

    /** Stroke the current path. Matches the PDF 'S' operator.
     *  This function is useful to construct an own path
     *  for drawing or clipping.
     */
    void Stroke();

    /** Fill the current path. Matches the PDF 'f' operator.
     *  This function is useful to construct an own path
     *  for drawing or clipping.
     */
    void Fill();

    /** Save the current graphics settings onto the graphics
     *  stack. Operator 'q' in PDF.
     *  This call has to be balanced with a corresponding call 
     *  to Restore()!
     *
     *  \see Restore
     */
    void Save();

    /** Restore the current graphics settings from the graphics
     *  stack. Operator 'Q' in PDF.
     *  This call has to be balanced with a corresponding call 
     *  to Save()!
     *
     *  \see Save
     */
    void Restore();

 private:
    /** Register an object in the resource dictionary of this page
     *  so that it can be used for any following drawing operations.
     *  
     *  \param rIdentifier identifier of this object, e.g. /Ft0
     *  \param rRef reference to the object you want to register
     *  \param rName register under this key in the resource dictionary
     */
    void AddToPageResources( const PdfName & rIdentifier, const PdfReference & rRef, const PdfName & rName );

    /** Coverts a rectangle to an array of points which can be used 
     *  to draw an ellipse using 4 bezier curves.
     * 
     *  The arrays plPointX and plPointY need space for at least 12 longs 
     *  to be stored.
     *
     *  \param lX x position of the bounding rectangle
     *  \param lY y position of the bounding rectangle
     *  \param lWidth width of the bounding rectangle
     *  \param lHeight height of the bounding rectangle
     *  \param plPointX pointer to an array were the x coordinates 
     *                  of the resulting points will be stored
     *  \param plPointY pointer to an array were the y coordinates 
     *                  of the resulting points will be stored
     */
    void ConvertRectToBezier( long lX, long lY, long lWidth, long lHeight, long plPointX[], long plPointY[] );

    /** Set the tab width for the DrawText operation.
     *  Every tab '\\t' is replaced with nTabWidth 
     *  spaces before drawing text. Default is a value of 4
     *
     *  \param nTabWidth replace every tabulator by this much spaces
     *
     *  \see DrawText
     *  \see TabWidth
     */
    inline void SetTabWidth( unsigned short nTabWidth );

    /** Get the currently set tab width
     *  \returns by how many spaces a tabulator will be replaced
     *  
     *  \see DrawText
     *  \see TabWidth
     */
    inline unsigned short TabWidth() const;

 protected:
    /** Sets the color that was last set by the user as the current stroking color.
     *  You should always enclose this function by Save() and Restore()
     *
     *  \see Save() \see Restore()
     */
    void SetCurrentStrokingColor();

 protected:
    /** All drawing operations work on this stream.
     *  This object may not be NULL. If it is NULL any function accessing it should
     *  return ERROR_PDF_INVALID_HANDLE
     */
    PdfStream* m_pCanvas;

    /** The page object is needed so that fonts etc. can be added
     *  to the page resource dictionary as appropriate.
     */
    PdfCanvas* m_pPage;

    /** The internal buffer which is used to create the strings for the
     *  drawing operations which are added to the stream later
     */
    char m_szBuffer[PDF_PAINTER_BUFFER];

    /** Font for all drawing operations
     */
    PdfFont* m_pFont;

    /** Every tab '\\t' is replaced with m_nTabWidth 
     *  spaces before drawing text. Default is a value of 4
     */
    unsigned short m_nTabWidth;

    /** The current color space for non stroking colors
     */
    EPdfColorSpace m_eCurColorSpace;

    /** Save the current color
     */
    double m_curColor1, m_curColor2, m_curColor3, m_curColor4;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfPainter::DrawImage( long lX, long lY, PdfImageRef* pImageRef, double dScaleX, double dScaleY )
{
    this->DrawXObject( lX, lY, pImageRef, dScaleX, dScaleY );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfFont* PdfPainter::Font() const
{
    return m_pFont;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfPainter::SetTabWidth( unsigned short nTabWidth )
{
    m_nTabWidth = nTabWidth;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned short PdfPainter::TabWidth() const
{
    return m_nTabWidth;
}

};

#endif // _PDF_PAINTER_H_

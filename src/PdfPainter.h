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

#include "PdfRect.h"

#include <sstream>

namespace PoDoFo {

class PdfCanvas;
class PdfColor;
class PdfExtGState;
class PdfFont;
class PdfImage;
class PdfName;
class PdfObject;
class PdfReference;
class PdfShadingPattern;
class PdfStream;
class PdfString;
class PdfXObject;

/**
 * This class provides an easy to use painter object which allows you to draw on a PDF page
 * object.
 * 
 * During all drawing operations, you are still able to access the stream of the object you are
 * drawing on directly. 
 * 
 * All functions that take coordinates expect these to be in PDF User Units. Keep in mind that PDF has
 * its coordinate system origin at the bottom left corner.
 */
class PODOFO_API PdfPainter {
 public:
    /** Create a new PdfPainter object.
     */
    PdfPainter();

    virtual ~PdfPainter();

    /** Set the page on which the painter should draw.
     *  The painter will draw of course on the pages
     *  contents object.
     *
     *  Calls FinishPage() on the last page if it was not yet called.
     *
     *  \param pPage a PdfCanvas object (most likely a PdfPage or PdfXObject).
     *
     *  \see PdfPage \see PdfXObject
     *  \see FinishPage()
     *  \see GetPage()
     */
    void SetPage( PdfCanvas* pPage );

    /** Return the current page that is that on the painter.
     *
     *  \returns the current page of the painter or NULL if none is set
     */
    inline const PdfCanvas* GetPage() const;

    /** Finish drawing onto a page.
     * 
     *  This has to be called whenever a page has been drawn complete.
     */
    void FinishPage();

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

    /** Set the shading pattern for all following stroking operations.
     *  This operation uses the 'SCN' PDF operator.
     *
     *  \param rPattern a shading pattern
     */
    void SetStrokingShadingPattern( const PdfShadingPattern & rPattern );

    /** Set the shading pattern for all following non-stroking operations.
     *  This operation uses the 'scn' PDF operator.
     *
     *  \param rPattern a shading pattern
     */
    void SetShadingPattern( const PdfShadingPattern & rPattern );

    /** Set the color for all following stroking operations. 
     * 
     *  \param rColor a PdfColor object
     */
    void SetStrokingColor( const PdfColor & rColor );

    /** Set the color for all following non-stroking operations. 
     * 
     *  \param rColor a PdfColor object
     */
    void SetColor( const PdfColor & rColor );

    /** Set the line width for all stroking operations.
     *  \param dWidth in PDF User Units.
     */
    void SetStrokeWidth( double dWidth );

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
    inline PdfFont* GetFont() const;

    /** Set a clipping rectangle
     *
     *  \param dX x coordinate of the rectangle (left coordinate)
     *  \param dY y coordinate of the rectangle (bottom coordinate)
     *  \param dWidth width of the rectangle
     *  \param dHeight absolute height of the rectangle
     */
    void SetClipRect( double dX, double dY, double dWidth, double dHeight );

	/** Set a clipping rectangle
     *
     *  \param rRect rectangle
     */
    inline void SetClipRect( const PdfRect & rRect );

    /** Draw a line with the current color and line settings.
     *  \param dStartX x coordinate of the starting point
     *  \param dStartY y coordinate of the starting point
     *  \param dEndX x coordinate of the ending point
     *  \param dEndY y coordinate of the ending point
     */
    void DrawLine( double dStartX, double dStartY, double dEndX, double dEndY );

    /** Draw a rectangle with the current stroking settings
     *  \param dX x coordinate of the rectangle (left coordinate)
     *  \param dY y coordinate of the rectangle (bottom coordinate)
     *  \param dWidth width of the rectangle
     *  \param dHeight absolute height of the rectangle
     *  \param dRoundX rounding factor, x direction
     *  \param dRoundY rounding factor, y direction
     */
    void DrawRect( double dX, double dY, double dWidth, double dHeight,
                   double dRoundX=0.0, double dRoundY=0.0 );

    /** Draw a rectangle with the current stroking settings
     *	
     *  \param rRect the rectangle area
     *  \param dRoundX rounding factor, x direction
     *  \param dRoundY rounding factor, y direction
     *
     *  \see DrawRect
     */
	inline void DrawRect( const PdfRect & rRect, double dRoundX=0.0, double dRoundY=0.0 );

    /** Fill a rectangle with the current fill settings
     *  \param dX x coordinate of the rectangle (left coordinate)
     *  \param dY y coordinate of the rectangle (bottom coordinate) 
     *  \param dWidth width of the rectangle 
     *  \param dHeight absolute height of the rectangle
     *  \param dRoundX rounding factor, x direction
     *  \param dRoundY rounding factor, y direction
     */
    void FillRect( double dX, double dY, double dWidth, double dHeight,
                   double dRoundX=0.0, double dRoundY=0.0 );

    /** Fill a rectangle with the current fill settings
     *	
     *  \param rRect the rectangle area
     *  \param dRoundX rounding factor, x direction
     *  \param dRoundY rounding factor, y direction
     *
     *  \see FillRect
     */
    inline void FillRect( const PdfRect & rRect, double dRoundX=0.0, double dRoundY=0.0 );

    /** Draw an ellipse with the current stroking settings
     *  \param dX x coordinate of the ellipse (left coordinate)
     *  \param dY y coordinate of the ellipse (top coordinate)
     *  \param dWidth width of the ellipse
     *  \param dHeight absolute height of the ellipse
     */
    void DrawEllipse( double dX, double dY, double dWidth, double dHeight ); 

    /** Fill an ellipse with the current fill settings
     *  \param dX x coordinate of the ellipse (left coordinate)
     *  \param dY y coordinate of the ellipse (top coordinate)
     *  \param dWidth width of the ellipse 
     *  \param dHeight absolute height of the ellipse
     */
    void FillEllipse( double dX, double dY, double dWidth, double dHeight ); 

    /** Draw a circle with the current stroking settings
     *  \param dX x center coordinate of the circle
     *  \param dY y coordinate of the circle
     *  \param dRadius radius of the circle
     */
    void DrawCircle( double dX, double dY, double dRadius );

    /** Fill a Circle with the current fill settings
     *  \param dX x center coordinate of the circle
     *  \param dY y coordinate of the circle
     *  \param dRadius radius of the circle
     */
    void FillCircle( double dX, double dY, double dRadius );

    /** Draw a single-line text string on a page using a given font object.
     *  You have to call SetFont before calling this function.
     *  \param dX the x coordinate
     *  \param dY the y coordinate
     *  \param sText the text string which should be printed 
     *
     *  \see SetFont()
     */
    void DrawText( double dX, double dY, const PdfString & sText);

    /** Draw a single-line text string on a page using a given font object.
     *  You have to call SetFont before calling this function.
     *  \param dX the x coordinate
     *  \param dY the y coordinate
     *  \param sText the text string which should be printed (is not allowed to be NULL!)
     *  \param lLen draw only lLen characters of pszText
     *
     *  \see SetFont()
     */
    void DrawText( double dX, double dY, const PdfString & sText, long lLen );

    /** Draw multiline text into a rectangle doing automatic wordwrapping.
     *  The current font is used and SetFont has to be called at least once
     *  before using this function
     *
     *  \param dX the x coordinate of the text area (left)
     *  \param dY the y coordinate of the text area (bottom)
     *  \param dWidth width of the text area
     *  \param dHeight height of the text area
     *  \param rsText the text which should be drawn
     *  \param eAlignment alignment of the individual text lines in the given bounding box
     *  \param eVertical vertical alignment of the text in the given bounding box
     */
    void DrawMultiLineText( double dX, double dY, double dWidth, double dHeight, 
                            const PdfString & rsText, EPdfAlignment eAlignment = ePdfAlignment_Left,
                            EPdfVerticalAlignment eVertical = ePdfVerticalAlignment_Top);

    /** Draw multiline text into a rectangle doing automatic wordwrapping.
     *  The current font is used and SetFont has to be called at least once
     *  before using this function
     *
     *  \param rRect bounding rectangle of the text
     *  \param rsText the text which should be drawn
     *  \param eAlignment alignment of the individual text lines in the given bounding box
     *  \param eVertical vertical alignment of the text in the given bounding box
     */
    inline void DrawMultiLineText( const PdfRect & rRect, const PdfString & rsText, EPdfAlignment eAlignment = ePdfAlignment_Left,
                                   EPdfVerticalAlignment eVertical = ePdfVerticalAlignment_Top);

    /** Draw a single line of text horizontally aligned.
     *  \param dX the x coordinate of the text line
     *  \param dY the y coordinate of the text line
     *  \param dWidth the width of the text line
     *  \param rsText the text to draw
     *  \param eAlignmet alignment of the text line
     */
    void DrawTextAligned( double dX, double dY, double dWidth, const PdfString & rsText, EPdfAlignment eAlignment );

    /** Draw an image on the current page.
     *  \param dX the x coordinate (bottom left position of the image)
     *  \param dY the y coordinate (bottom position of the image)
     *  \param pObject an PdfXObject
     *  \param dScaleX option scaling factor in x direction
     *  \param dScaleY option scaling factor in y direction
     */
    void DrawImage( double dX, double dY, PdfImage* pObject, double dScaleX = 1.0, double dScaleY = 1.0);

    /** Draw an XObject on the current page. For PdfImage use DrawImage.
     *
     *  \param dX the x coordinate (bottom left position of the XObject)
     *  \param dY the y coordinate (bottom position of the XObject)
     *  \param pObject an PdfXObject
     *  \param dScaleX option scaling factor in x direction
     *  \param dScaleY option scaling factor in y direction
     *
     *  \see DrawImage
     */
    void DrawXObject( double dX, double dY, PdfXObject* pObject, double dScaleX = 1.0, double dScaleY = 1.0);

    /** Closes the current path by drawing a line from the current point
     *  to the starting point of the path. Matches the PDF 'h' operator.
     *  This function is useful to construct an own path
     *  for drawing or clipping.
     */
    void ClosePath();

    /** Append a line segment to the current path. Matches the PDF 'l' operator.
     *  This function is useful to construct an own path
     *  for drawing or clipping.
     *  \param dX x position
     *  \param dY y position
     */
    void LineTo( double  dX, double dY );

    /** Begin a new path. Matches the PDF 'm' operator. 
     *  This function is useful to construct an own path
     *  for drawing or clipping.
     *  \param dX x position
     *  \param dY y position
     */
    void MoveTo( double dX, double dY );

    /** Append a cubic bezier curve to the current path
     *  Matches the PDF 'c' operator.
     *
     *  \param dX1 x coordinate of the first control point
     *  \param dY1 y coordinate of the first control point
     *  \param dX2 x coordinate of the second control point
     *  \param dY2 y coordinate of the second control point
     *  \param dX3 x coordinate of the end point, which is the new current point
     *  \param dY3 y coordinate of the end point, which is the new current point
     */
    void CubicBezierTo( double dX1, double dY1, double dX2, double dY2, double dX3, double dY3 );

    /** Append a horizontal line to the current path
     *  Matches the SVG 'H' operator
     *
     *  \param dX x coordinate to draw the line to
     */
    void HorizonalLineTo( double dX );

    /** Append a vertical line to the current path
     *  Matches the SVG 'V' operator
     *
     *  \param dY y coordinate to draw the line to
     */
    void VerticalLineTo( double dY );
    
    /** Append a smooth bezier curve to the current path
     *  Matches the SVG 'S' operator.
     *
     *  \param dX2 x coordinate of the second control point
     *  \param dY2 y coordinate of the second control point
     *  \param dX3 x coordinate of the end point, which is the new current point
     *  \param dY3 y coordinate of the end point, which is the new current point
     */
    void SmoothCurveTo( double dX2, double dY2, double dX3, double dY3 );

    /** Append a quadratic bezier curve to the current path
     *  Matches the SVG 'Q' operator.
     *
     *  \param dX1 x coordinate of the first control point
     *  \param dY1 y coordinate of the first control point
     *  \param dX3 x coordinate of the end point, which is the new current point
     *  \param dY3 y coordinate of the end point, which is the new current point
     */
    void QuadCurveTo( double dX1, double dY1, double dX3, double dY3 );

    /** Append a smooth quadratic bezier curve to the current path
     *  Matches the SVG 'T' operator.
     *
     *  \param dX3 x coordinate of the end point, which is the new current point
     *  \param dY3 y coordinate of the end point, which is the new current point
     */
    void SmoothQuadCurveTo( double dX3, double dY3 );

    /** Append a Arc to the current path
     *  Matches the SVG 'A' operator.
     *
     *  \param dX x coordinate of the start point
     *  \param dY y coordinate of the start point
     *  \param dRadiusX x coordinate of the end point, which is the new current point
     *  \param dRadiusY y coordinate of the end point, which is the new current point
     *	\param dRotation degree of rotation in radians
     *	\param bLarge large or small portion of the arc
     *	\param bSweep sweep?
     */
    void ArcTo( double dX, double dY, double dRadiusX, double dRadiusY,
                double	dRotation, bool bLarge, bool bSweep);
    
    /** Close the current path. Matches the PDF 'h' operator.
     */
    void Close();

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

    /** Clip the current path. Matches the PDF 'W' operator.
     *  This function is useful to construct an own path
     *  for drawing or clipping.
     */
    void Clip();

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

    /** Set the transformation matrix for the current coordinate system
     *  See the operator 'cm' in PDF.
     *
     *  The six parameters are a standard 3x3 transformation matrix
     *  where the 3 left parameters are 0 0 1.
     *
     *  \param a scale in x direction
     *  \param b rotation
     *  \param c rotation
     *  \param d scale in y direction
     *  \param e translate in x direction
     *  \param f translate in y direction
     * 
     *  \see Save()
     *  \see Restore()
     */
    void SetTransformationMatrix( double a, double b, double c, double d, double e, double f );

    /** Sets a specific PdfExtGState as being active
     *	\param inGState the specific ExtGState to set
     */
    void SetExtGState( PdfExtGState* inGState );
    
    /** Sets a specific rendering intent
     *	\param intent the specific intent to set
     */
    void SetRenderingIntent( char* intent );

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
    inline unsigned short GetTabWidth() const;

    /** Set the floating point precision.
     *
     *  \param inPrec write this many decimal places
     */
    inline void SetPrecision( unsigned short inPrec );

    /** Get the currently set floating point precision
     *  \returns how many decimal places will be written out for any floating point value
     */
    inline unsigned short GetPrecision() const;

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
     *  \param dX x position of the bounding rectangle
     *  \param dY y position of the bounding rectangle
     *  \param dWidth width of the bounding rectangle
     *  \param dHeight height of the bounding rectangle
     *  \param pdPointX pointer to an array were the x coordinates 
     *                  of the resulting points will be stored
     *  \param pdPointY pointer to an array were the y coordinates 
     *                  of the resulting points will be stored
     */
    void ConvertRectToBezier( double dX, double dY, double dWidth, double dHeight, double pdPointX[], double pdPointY[] );

 protected:
    /** Sets the color that was last set by the user as the current stroking color.
     *  You should always enclose this function by Save() and Restore()
     *
     *  \see Save() \see Restore()
     */
    void SetCurrentStrokingColor();

    /** Expand all tab characters in a string
     *  using spaces.
     *
     *  \param rsString expand all tabs in this string using spaces
     *  \param lLen use only lLen characters of rsString
     *  \returns an expanded copy of the passed string
     *  \see SetTabWidth
     */
    PdfString ExpandTabs( const PdfString & rsString, long lLen ) const;
    
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200	// MSC 6.0 has a template-bug 
    PdfString ExpandTabs_char( const char* pszText, long lStringLen, int nTabCnt, const char cTab, const char cSpace ) const;
    PdfString ExpandTabs_pdf_utf16be( const pdf_utf16be* pszText, long lStringLen, int nTabCnt, const pdf_utf16be cTab, const pdf_utf16be cSpace ) const;
#else
    template<typename C>
        PdfString ExpandTabsPrivate( const C* pszText, long lStringLen, int nTabCnt, const C cTab, const C cSpace ) const;
#endif

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

    /** temporary stream buffer 
     */
    std::ostringstream  m_oss;

    double		lpx, lpy, lpx2, lpy2, lpx3, lpy3, 	// points for this operation
        lcx, lcy, 							// last "current" point
        lrx, lry;							// "reflect points"
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfCanvas* PdfPainter::GetPage() const
{
    return m_pPage;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfFont* PdfPainter::GetFont() const
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
unsigned short PdfPainter::GetTabWidth() const
{
    return m_nTabWidth;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfPainter::SetPrecision( unsigned short inPrec )
{
    m_oss.precision( inPrec );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned short PdfPainter::GetPrecision() const
{
    return m_oss.precision();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfPainter::SetClipRect( const PdfRect & rRect )
{
    this->SetClipRect( rRect.GetLeft(), rRect.GetBottom(), rRect.GetWidth(), rRect.GetHeight() );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfPainter::DrawRect( const PdfRect & rRect, double dRoundX, double dRoundY )
{
    this->DrawRect( rRect.GetLeft(), rRect.GetBottom(), 
                    rRect.GetWidth(), rRect.GetHeight(), 
                    dRoundX, dRoundY );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfPainter::FillRect( const PdfRect & rRect, double dRoundX, double dRoundY )
{
    this->FillRect( rRect.GetLeft(), rRect.GetBottom(), 
                    rRect.GetWidth(), rRect.GetHeight(), 
                    dRoundX, dRoundY );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfPainter::DrawMultiLineText( const PdfRect & rRect, const PdfString & rsText, 
                                    EPdfAlignment eAlignment, EPdfVerticalAlignment eVertical)
{
    this->DrawMultiLineText( rRect.GetLeft(), rRect.GetBottom(), rRect.GetWidth(), rRect.GetHeight(), 
                             rsText, eAlignment, eVertical );
}

};

#endif // _PDF_PAINTER_H_

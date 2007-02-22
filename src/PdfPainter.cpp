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

#include <iostream>
#include <iomanip>

#include "PdfPainter.h"

#include "PdfContents.h"
#include "PdfDictionary.h"
#include "PdfExtGState.h"
#include "PdfFilter.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfImage.h"
#include "PdfName.h"
#include "PdfRect.h"
#include "PdfStream.h"
#include "PdfString.h"
#include "PdfXObject.h"

#define BEZIER_POINTS 13

/* 4/3 * (1-cos 45ƒ)/sin 45ƒ = 4/3 * sqrt(2) - 1 */
#define ARC_MAGIC    0.552284749f
#define PI           3.141592654f

namespace PoDoFo {

static inline void CheckDoubleRange( double val, double min, double max )
{
    if( val < min || val > max )
    {
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }
}

PdfPainter::PdfPainter()
: m_pCanvas( NULL ), m_pPage( NULL ), m_pFont( NULL ), m_nTabWidth( 4 ),
  m_eCurColorSpace( ePdfColorSpace_DeviceRGB ), 
  m_curColor1( 0.0 ), m_curColor2( 0.0 ), m_curColor3( 0.0 ), m_curColor4( 0.0 )
{
    m_oss.flags( std::ios_base::fixed );
    m_oss.precision( 3 );
}

PdfPainter::~PdfPainter()
{
    PODOFO_RAISE_LOGIC_IF( m_pCanvas, "FinishPage() has to be called after a page is completed!" );
}

void PdfPainter::SetPage( PdfCanvas* pPage )
{
    if( m_pCanvas )
        m_pCanvas->EndAppend();

    m_pPage   = pPage;

    m_pCanvas = pPage ? pPage->GetContentsForAppending()->GetStream() : NULL;
    if ( m_pCanvas ) 
    {
        m_pCanvas->BeginAppend( false );
        if ( m_pCanvas->GetLength() ) 
        {    
            // there is already content here - so let's assume we are appending
            // as such, we MUST put in a "space" to separate whatever we do.
            m_pCanvas->Append( " " );
        }
    } 
    else 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
}

void PdfPainter::FinishPage()
{
    if( m_pCanvas )
        m_pCanvas->EndAppend();

    m_pCanvas = NULL;
    m_pPage   = NULL;
}

void PdfPainter::SetStrokingGray( double g )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );
    CheckDoubleRange( g, 0.0, 1.0 );

    m_oss.str("");
    m_oss << g << " G" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetGray( double g )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );
    CheckDoubleRange( g, 0.0, 1.0 );
    
    m_oss.str("");
    m_oss << g << " g" << std::endl;
    m_pCanvas->Append( m_oss.str() );
        
    m_curColor1      = g;
    m_eCurColorSpace = ePdfColorSpace_DeviceGray;
}

void PdfPainter::SetStrokingColor( double r, double g, double b )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    CheckDoubleRange( r, 0.0, 1.0 );
    CheckDoubleRange( g, 0.0, 1.0 );
    CheckDoubleRange( b, 0.0, 1.0 );

    m_oss.str("");
    m_oss << r << " "
          << g << " "
          << b 
          << " RG" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetColor( double r, double g, double b )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );
    CheckDoubleRange( r, 0.0, 1.0 );
    CheckDoubleRange( g, 0.0, 1.0 );
    CheckDoubleRange( b, 0.0, 1.0 );
    
    m_oss.str("");
    m_oss << r << " " 
          << g << " " 
          << b 
          << " rg" << std::endl;
    m_pCanvas->Append( m_oss.str() );

    m_curColor1      = r;
    m_curColor2      = g;
    m_curColor3      = b;
    m_eCurColorSpace = ePdfColorSpace_DeviceRGB;
}

void PdfPainter::SetStrokingColorCMYK( double c, double m, double y, double k )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );
    CheckDoubleRange( c, 0.0, 1.0 );
    CheckDoubleRange( m, 0.0, 1.0 );
    CheckDoubleRange( y, 0.0, 1.0 );
    CheckDoubleRange( k, 0.0, 1.0 );

    m_oss.str("");
    m_oss << c << " " 
          << m << " " 
          << y << " " 
          << k 
          << " K" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetColorCMYK( double c, double m, double y, double k )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    
    CheckDoubleRange( c, 0.0, 1.0 );
    CheckDoubleRange( m, 0.0, 1.0 );
    CheckDoubleRange( y, 0.0, 1.0 );
    CheckDoubleRange( k, 0.0, 1.0 );
    
    m_oss.str("");
    m_oss << c << " " 
          << m << " " 
          << y << " " 
          << k 
          << " k" << std::endl;
    m_pCanvas->Append( m_oss.str() );
    
    m_curColor1      = c;
    m_curColor2      = m;
    m_curColor3      = y;
    m_curColor4      = k;
    m_eCurColorSpace = ePdfColorSpace_DeviceCMYK;
}

void PdfPainter::SetStrokeWidth( double dWidth )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

    m_oss.str("");
    m_oss << dWidth << " w" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetStrokeStyle( EPdfStrokeStyle eStyle, const char* pszCustom )
{
    const char* pszCurStroke = NULL;

    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

    // TODO: fix the stroking styles
    switch( eStyle )
    {
        case ePdfStrokeStyle_Solid:
            pszCurStroke = "[] 0";
            break;
        case ePdfStrokeStyle_Dash:
            pszCurStroke = "[3] 0";
            break;
        case ePdfStrokeStyle_Dot:
            pszCurStroke = "[1] 0";
            break;
        case ePdfStrokeStyle_DashDot:
            pszCurStroke = "[3 1 1] 0";
            break;
        case ePdfStrokeStyle_DashDotDot:
            pszCurStroke = "[3 1 1 1 1] 0";
            break;
        case ePdfStrokeStyle_Custom:
            pszCurStroke = pszCustom;
            break;
        case ePdfStrokeStyle_Unknown:
        default:
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidStrokeStyle );
        }
    }

    if( !pszCurStroke )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidStrokeStyle );
    }
    
    m_oss.str("");
    m_oss << pszCurStroke << " d" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetLineCapStyle( EPdfLineCapStyle eCapStyle )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

    m_oss.str("");
    m_oss << static_cast<int>(eCapStyle) << " J" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetLineJoinStyle( EPdfLineJoinStyle eJoinStyle )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

    m_oss.str("");
    m_oss << static_cast<int>(eJoinStyle) << "j" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetFont( PdfFont* pFont )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

    if( !pFont )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pFont = pFont;
}

void PdfPainter::DrawLine( double dStartX, double dStartY, double dEndX, double dEndY )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

    m_oss.str("");
    m_oss << dStartX << " "
          << dStartY
          << " m "
          << dEndX << " "
          << dEndY        
          << " l S" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::DrawRect( double dX, double dY, double dWidth, double dHeight,
                           double dRoundX, double dRoundY )
{ 
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

    if ( static_cast<int>(dRoundX) || static_cast<int>(dRoundY) ) {
        double x = dX, y = dY, 
               w = dWidth, h = dHeight,
               rx = dRoundX, ry = dRoundY;
        double b = 0.4477f;

        MoveTo(x + rx, y);
        LineTo(x + w - rx, y);
        CubicBezierTo(x + w - rx * b, y, x + w, y + ry * b, x + w, y + ry);
        LineTo(x + w, y + h - ry);
        CubicBezierTo(x + w, y + h - ry * b, x + w - rx * b, y + h, x + w - rx, y + h);
        LineTo(x + rx, y + h);
        CubicBezierTo(x + rx * b, y + h, x, y + h - ry * b, x, y + h - ry);
        LineTo(x, y + ry);
        CubicBezierTo(x, y + ry * b, x + rx * b, y, x + rx, y);
        m_pCanvas->Append( "S\n" );
    } else {
        m_oss.str("");
        m_oss << dX << " "
            << dY << " "
            << dWidth << " "
            << -dHeight        
            << " re S" << std::endl;
        m_pCanvas->Append( m_oss.str() );
    }
}

void PdfPainter::FillRect( double dX, double dY, double dWidth, double dHeight,
                           double dRoundX, double dRoundY )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    m_oss.str("");

    if ( static_cast<int>(dRoundX) || static_cast<int>(dRoundY) ) {
         double    x = dX, y = dY, 
                w = dWidth, h = dHeight,
                rx = dRoundX, ry = dRoundY;
        double b = 0.4477f;

        MoveTo(x + rx, y);
        LineTo(x + w - rx, y);
        CubicBezierTo(x + w - rx * b, y, x + w, y + ry * b, x + w, y + ry);
        LineTo(x + w, y + h - ry);
        CubicBezierTo(x + w, y + h - ry * b, x + w - rx * b, y + h, x + w - rx, y + h);
        LineTo(x + rx, y + h);
        CubicBezierTo(x + rx * b, y + h, x, y + h - ry * b, x, y + h - ry);
        LineTo(x, y + ry);
        CubicBezierTo(x, y + ry * b, x + rx * b, y, x + rx, y);
        m_pCanvas->Append( "f\n" );
    } else {
        m_oss << dX << " "
            << dY << " "
            << dWidth << " "
            << -dHeight        
            << " re f" << std::endl;
    }

    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::DrawEllipse( double dX, double dY, double dWidth, double dHeight )
{
    double dPointX[BEZIER_POINTS];
    double dPointY[BEZIER_POINTS];
    int    i;

    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    ConvertRectToBezier( dX, dY, dWidth, dHeight, dPointX, dPointY );


    m_oss.str("");
    m_oss << dPointX[0] << " "
          << dPointY[0]
          << " m" << std::endl;

    for( i=1;i<BEZIER_POINTS; i+=3 )
    {
        m_oss << dPointX[i] << " "
              << dPointY[i] << " "
              << dPointX[i+1] << " "
              << dPointY[i+1] << " "
              << dPointX[i+2] << " "
              << dPointY[i+2]    
              << " c" << std::endl;
    }

    m_pCanvas->Append( m_oss.str() );
    m_pCanvas->Append( "S\n" );
}

void PdfPainter::FillEllipse( double dX, double dY, double dWidth, double dHeight )
{
    double dPointX[BEZIER_POINTS];
    double dPointY[BEZIER_POINTS];
    int    i;

    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    ConvertRectToBezier( dX, dY, dWidth, dHeight, dPointX, dPointY );

    m_oss.str("");
    m_oss << dPointX[0] << " "
          << dPointY[0]
          << " m" << std::endl;

    for( i=1;i<BEZIER_POINTS; i+=3 )
    {
        m_oss << dPointX[i] << " "
              << dPointY[i] << " "
              << dPointX[i+1] << " "
              << dPointY[i+1] << " "
              << dPointX[i+2] << " "
              << dPointY[i+2]    
              << " c" << std::endl;
    }

    m_pCanvas->Append( m_oss.str() );
    m_pCanvas->Append( "f\n" );
}

void PdfPainter::FillCircle( double dX, double dY, double dRadius )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    /* draw four Bezier curves to approximate a circle */
    MoveTo( dX + dRadius, dY );
    CubicBezierTo( dX + dRadius, dY + dRadius*ARC_MAGIC,
             dX + dRadius*ARC_MAGIC, dY + dRadius,
             dX, dY + dRadius );
    CubicBezierTo( dX - dRadius*ARC_MAGIC, dY + dRadius,
            dX - dRadius, dY + dRadius*ARC_MAGIC,
            dX - dRadius, dY );
    CubicBezierTo( dX - dRadius, dY - dRadius*ARC_MAGIC,
            dX - dRadius*ARC_MAGIC, dY - dRadius,
            dX, dY - dRadius );
    CubicBezierTo( dX + dRadius*ARC_MAGIC, dY - dRadius,
            dX + dRadius, dY - dRadius*ARC_MAGIC,
            dX + dRadius, dY );
    Close();

    m_pCanvas->Append( "f\n" );
}

void PdfPainter::DrawCircle( double dX, double dY, double dRadius )
{
    if( !m_pCanvas )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    /* draw four Bezier curves to approximate a circle */
    MoveTo( dX + dRadius, dY );
    CubicBezierTo( dX + dRadius, dY + dRadius*ARC_MAGIC,
             dX + dRadius*ARC_MAGIC, dY + dRadius,
             dX, dY + dRadius );
    CubicBezierTo( dX - dRadius*ARC_MAGIC, dY + dRadius,
            dX - dRadius, dY + dRadius*ARC_MAGIC,
            dX - dRadius, dY );
    CubicBezierTo( dX - dRadius, dY - dRadius*ARC_MAGIC,
            dX - dRadius*ARC_MAGIC, dY - dRadius,
            dX, dY - dRadius );
    CubicBezierTo( dX + dRadius*ARC_MAGIC, dY - dRadius,
            dX + dRadius, dY - dRadius*ARC_MAGIC,
            dX + dRadius, dY );
    Close();

    m_pCanvas->Append( "S\n" );
}

void PdfPainter::DrawText( double dX, double dY, const PdfString & sText )
{
    this->DrawText( dX, dY, sText, sText.GetLength() );
}

void PdfPainter::DrawText( double dX, double dY, const PdfString & sText, long lStringLen )
{
    int         nTabCnt = 0;
    int         i,z;
    char*       pszTab;
    char*       pBuffer;
    const char* pszText;
    long        lLen;

    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    if( !m_pFont || !m_pPage || !sText.IsValid() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // replace anytabs in pszText by enough spaces
    if( !sText.IsHex() )
        for( i=0;i<=lStringLen;i++ )
            if( sText.GetString()[i] == '\t' )
                ++nTabCnt;

    if( nTabCnt )
    {
        pszText = sText.GetString();
        lLen    = lStringLen + nTabCnt*(m_nTabWidth-1) + 1;

        pszTab = static_cast<char*>(malloc( sizeof( char ) * lLen ));
        if( !pszTab )
        {
            PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
        }

        i = 0;
        while( lStringLen-- )
        {
            if( *pszText == '\t' )
            {
                for( z=0;z<m_nTabWidth; z++ )
                    pszTab[i+z] += ' ';
                
                i+=m_nTabWidth;
            }
            else
                pszTab[i++] = *pszText;

            ++pszText;
        }
        
        lStringLen = lLen;
        pszTab[i]  = '\0';
    }
    else 
        // pszTab is accessed only readonly beyond this point
        // so this cast is ok
        pszTab = const_cast<char*>(sText.GetString());

    this->AddToPageResources( m_pFont->GetIdentifier(), m_pFont->GetObject()->Reference(), PdfName("Font") );

    if( m_pFont->IsUnderlined() )
    {
        this->Save();
        this->SetCurrentStrokingColor();
        this->SetStrokeWidth( m_pFont->GetFontMetrics()->GetUnderlineThickness() );
        this->DrawLine( dX, 
                        dY + m_pFont->GetFontMetrics()->GetUnderlinePosition(), 
                        dX + m_pFont->GetFontMetrics()->StringWidth( pszTab ),
                        dY + m_pFont->GetFontMetrics()->GetUnderlinePosition() );
        this->Restore();
    }

    if( !sText.IsHex() )
    {
        std::auto_ptr<PdfFilter> pFilter = PdfFilterFactory::Create( ePdfFilter_ASCIIHexDecode );
        pFilter->Encode( pszTab, lStringLen, &pBuffer, &lLen );
    }

    m_oss.str("");
    m_oss << "BT" << std::endl << "/" << m_pFont->GetIdentifier().GetName()
          << " "  << m_pFont->GetFontSize()
          << " Tf" << std::endl
          << dX << std::endl
          << dY << std::endl << " Td <";

    m_pCanvas->Append( m_oss.str() );

    if( !sText.IsHex() )
    {
        m_pCanvas->Append( pBuffer, lLen );
        free( pBuffer );
    }
    else
        m_pCanvas->Append( sText.GetString(), sText.GetLength() );

    m_pCanvas->Append( ">Tj\nET\n" );

    if( nTabCnt )
        free( pszTab );
}

void PdfPainter::DrawXObject( double dX, double dY, PdfXObject* pObject, double dScaleX, double dScaleY )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    if( !pObject )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // use OriginalReference() as the XObject might have been written to disk
    // already and is not in memory anymore in this case.
    this->AddToPageResources( pObject->GetIdentifier(), pObject->GetObjectReference(), "XObject" );

    m_oss.str("");
    m_oss << "q" << std::endl
          << pObject->GetPageSize().GetWidth() * dScaleX << " 0 0 "
          << pObject->GetPageSize().GetHeight() * dScaleY << " "
          << dX << " " 
          << dY << " cm" << std::endl
          << "/" << pObject->GetIdentifier().GetName() << " Do" << std::endl << "Q" << std::endl;
    
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::ClosePath()
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    m_pCanvas->Append( "h\n" );
}

void PdfPainter::LineTo( double dX, double dY )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );
    
    m_oss.str("");
    m_oss << dX << " "
          << dY
          << " l" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::MoveTo( double dX, double dY )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );
    
    m_oss.str("");
    m_oss << dX << " "
          << dY
          << " m" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::CubicBezierTo( double dX1, double dY1, double dX2, double dY2, double dX3, double dY3 )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    m_oss.str("");
    m_oss << dX1 << " "
          << dY1 << " "
          << dX2 << " "
          << dY2 << " "
          << dX3 << " "
          << dY3 
          << " c" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::HorizonalLineTo( double inX )
{
    LineTo( inX, lpy3 );
}

void PdfPainter::VerticalLineTo( double inY )
{
    LineTo( lpx3, inY );
}

void PdfPainter::SmoothCurveTo( double inX2, double inY2, double inX3, double inY3 )
{
    double    px, py, px2 = inX2, 
            py2 = inY2, 
            px3 = inX3, py3 = inY3;

    // compute the reflective points (thanks Raph!)
    px = 2 * lcx - lrx;
    py = 2 * lcy - lry;

    lpx = px; lpy = py; lpx2 = px2; lpy2 = py2; lpx3 = px3; lpy3 = py3;
    lcx = px3;    lcy = py3;    lrx = px2;    lry = py2;    // thanks Raph!

    CubicBezierTo( px, py, px2, py2, px3, py3 );
}

void PdfPainter::QuadCurveTo( double inX1, double inY1, double inX3, double inY3 )
{
    double px = inX1, py = inY1, 
           px2, py2, 
           px3 = inX3, py3 = inY3;

    /* raise quadratic bezier to cubic    - thanks Raph!
        http://www.icce.rug.nl/erikjan/bluefuzz/beziers/beziers/beziers.html
    */
    px = (lcx + 2 * px) * (1.0 / 3.0);
    py = (lcy + 2 * py) * (1.0 / 3.0);
    px2 = (px3 + 2 * px) * (1.0 / 3.0);
    py2 = (py3 + 2 * py) * (1.0 / 3.0);

    lpx = px; lpy = py; lpx2 = px2; lpy2 = py2; lpx3 = px3; lpy3 = py3;
    lcx = px3;    lcy = py3;    lrx = px2;    lry = py2;    // thanks Raph!

    CubicBezierTo( px, py, px2, py2, px3, py3 );
}

void PdfPainter::SmoothQuadCurveTo( double inX3, double inY3 )
{
    double px, py, px2, py2, 
           px3 = inX3, py3 = inY3;

    double xc, yc; /* quadratic control point */
    xc = 2 * lcx - lrx;
    yc = 2 * lcy - lry;

    /* generate a quadratic bezier with control point = xc, yc */
    px = (lcx + 2 * xc) * (1.0 / 3.0);
    py = (lcy + 2 * yc) * (1.0 / 3.0);
    px2 = (px3 + 2 * xc) * (1.0 / 3.0);
    py2 = (py3 + 2 * yc) * (1.0 / 3.0);

    lpx = px; lpy = py; lpx2 = px2; lpy2 = py2; lpx3 = px3; lpy3 = py3;
    lcx = px3;    lcy = py3;    lrx = xc;    lry = yc;    // thanks Raph!

    CubicBezierTo( px, py, px2, py2, px3, py3 );
}

void PdfPainter::ArcTo( double inX, double inY, double inRadiusX, double inRadiusY,
                       double    inRotation, bool inLarge, bool inSweep)
{
    double    px = inX, py = inY;
    double rx = inRadiusX, ry = inRadiusY, rot = inRotation;
    int        large = ( inLarge ? 1 : 0 ),
            sweep = ( inSweep ? 1 : 0 );

    double sin_th, cos_th;
    double a00, a01, a10, a11;
    double x0, y0, x1, y1, xc, yc;
    double d, sfactor, sfactor_sq;
    double th0, th1, th_arc;
    int i, n_segs;

    sin_th     = sin (rot * (PI / 180.0));
    cos_th     = cos (rot * (PI / 180.0));
    a00     = cos_th / rx;
    a01     = sin_th / rx;
    a10     = -sin_th / ry;
    a11     = cos_th / ry;
    x0         = a00 * lcx + a01 * lcy;
    y0         = a10 * lcx + a11 * lcy;
    x1         = a00 * px + a01 * py;
    y1         = a10 * px + a11 * py;
    /* (x0, y0) is current point in transformed coordinate space.
     (x1, y1) is new point in transformed coordinate space.

     The arc fits a unit-radius circle in this space.
    */
    d = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
    sfactor_sq = 1.0 / d - 0.25;
    if (sfactor_sq < 0) sfactor_sq = 0;
    sfactor = sqrt (sfactor_sq);
    if (sweep == large) sfactor = -sfactor;
    xc = 0.5 * (x0 + x1) - sfactor * (y1 - y0);
    yc = 0.5 * (y0 + y1) + sfactor * (x1 - x0);
    /* (xc, yc) is center of the circle. */

    th0 = atan2 (y0 - yc, x0 - xc);
    th1 = atan2 (y1 - yc, x1 - xc);

    th_arc = th1 - th0;
    if (th_arc < 0 && sweep)        th_arc += 2 * PI;
    else if (th_arc > 0 && !sweep)    th_arc -= 2 * PI;

    n_segs = static_cast<int>(ceil (fabs (th_arc / (PI * 0.5 + 0.001))));

    for (i = 0; i < n_segs; i++) {
        double nth0 = th0 + static_cast<double>(i) * th_arc / n_segs,
                nth1 = th0 + static_cast<double>(i + 1) * th_arc / n_segs;
        double nsin_th, ncos_th;
        double na00, na01, na10, na11;
        double nx1, ny1, nx2, ny2, nx3, ny3;
        double t;
        double th_half;

        nsin_th = sin (rot * (PI / 180.0));
        ncos_th = cos (rot * (PI / 180.0)); 
        /* inverse transform compared with rsvg_path_arc */
        na00 = ncos_th * rx;
        na01 = -nsin_th * ry;
        na10 = nsin_th * rx;
        na11 = ncos_th * ry;

        th_half = 0.5 * (nth1 - nth0);
        t = (8.0 / 3.0) * sin (th_half * 0.5) * sin (th_half * 0.5) / sin (th_half);
        nx1 = xc + cos (nth0) - t * sin (nth0);
        ny1 = yc + sin (nth0) + t * cos (nth0);
        nx3 = xc + cos (nth1);
        ny3 = yc + sin (nth1);
        nx2 = nx3 + t * sin (nth1);
        ny2 = ny3 - t * cos (nth1);
        nx1 = na00 * nx1 + na01 * ny1;
        ny1 = na10 * nx1 + na11 * ny1;
        nx2 = na00 * nx2 + na01 * ny2;
        ny2 = na10 * nx2 + na11 * ny2;
        nx3 = na00 * nx3 + na01 * ny3;
        ny3 = na10 * nx3 + na11 * ny3;
        CubicBezierTo( nx1, ny1, nx2, ny2, nx3, ny3 );
    }

    lpx = lpx2 = lpx3 = px; lpy = lpy2 = lpy3 = py;
    lcx = px;    lcy = py;    lrx = px;    lry = py;    // thanks Raph!
}

void PdfPainter::Close()
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );
    
    m_pCanvas->Append( "h\n" );
}

void PdfPainter::Stroke()
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );
    
    m_pCanvas->Append( "S\n" );
}

void PdfPainter::Fill()
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );
    
    m_pCanvas->Append( "f\n" );
}

void PdfPainter::Save()
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    m_pCanvas->Append( "q\n" );
}

void PdfPainter::Restore()
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    m_pCanvas->Append( "q\n" );
}

void PdfPainter::AddToPageResources( const PdfName & rIdentifier, const PdfReference & rRef, const PdfName & rName )
{
    if( !m_pPage || !rName.GetLength() || !rIdentifier.GetLength() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    PdfObject* pResource = m_pPage->GetResources();
    
    if( !pResource )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( !pResource->GetDictionary().HasKey( rName ) )
    {
        pResource->GetDictionary().AddKey( rName, PdfDictionary() );
    }

    if( !pResource->GetDictionary().GetKey( rName )->GetDictionary().HasKey( rIdentifier ) )
        pResource->GetDictionary().GetKey( rName )->GetDictionary().AddKey( rIdentifier, rRef );
}

void PdfPainter::ConvertRectToBezier( double dX, double dY, double dWidth, double dHeight, double pdPointX[], double pdPointY[] )
{
    // this function is based on code from:
    // http://www.codeguru.com/Cpp/G-M/gdi/article.php/c131/
    // (Llew Goodstadt)

    // MAGICAL CONSTANT to map ellipse to beziers
    //                          2/3*(sqrt(2)-1) 
    const double dConvert =     0.2761423749154;

    double dOffX    = dWidth  * dConvert;
    double dOffY    = dHeight * dConvert;
    double dCenterX = dX + (dWidth / 2.0); 
    double dCenterY = dY + (dHeight / 2.0); 

    pdPointX[0]  =                            //------------------------//
    pdPointX[1]  =                            //                        //
    pdPointX[11] =                            //        2___3___4       //
    pdPointX[12] = dX;                        //     1             5    //
    pdPointX[5]  =                            //     |             |    //
    pdPointX[6]  =                            //     |             |    //
    pdPointX[7]  = dX + dWidth;               //     0,12          6    //
    pdPointX[2]  =                            //     |             |    //
    pdPointX[10] = dCenterX - dOffX;          //     |             |    //
    pdPointX[4]  =                            //    11             7    //
    pdPointX[8]  = dCenterX + dOffX;          //       10___9___8       //
    pdPointX[3]  =                            //                        //
    pdPointX[9]  = dCenterX;                  //------------------------//

    pdPointY[2]  =
    pdPointY[3]  =
    pdPointY[4]  = dY;
    pdPointY[8]  =
    pdPointY[9]  =
    pdPointY[10] = dY + dHeight;
    pdPointY[7]  =
    pdPointY[11] = dCenterY + dOffY;
    pdPointY[1]  =
    pdPointY[5]  = dCenterY - dOffY;
    pdPointY[0]  =
    pdPointY[12] =
    pdPointY[6]  = dCenterY;
}

void PdfPainter::SetCurrentStrokingColor()
{
    switch( m_eCurColorSpace )
    {
        case ePdfColorSpace_DeviceGray:
            this->SetStrokingGray( m_curColor1 );
        break;
        case ePdfColorSpace_DeviceRGB:
            this->SetStrokingColor( m_curColor1, m_curColor2, m_curColor3 );
        break;
        case ePdfColorSpace_DeviceCMYK:
            this->SetStrokingColorCMYK( m_curColor1, m_curColor2, m_curColor3, m_curColor4 );
        break;
        case ePdfColorSpace_Unknown:
        default:
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, 
                                     "The color space for the current text drawing operation is invalid. "
                                     "Please set a correct color."  ); 
        }
        break;
    }
}

void PdfPainter::SetTransformationMatrix( double a, double b, double c, double d, double e, double f )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    m_oss.str("");
    m_oss << a << " "
          << b << " "
          << c << " "
          << d << " "
          << e << " "
          << f << " cm" << std::endl;

    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetExtGState( PdfExtGState* inGState )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    this->AddToPageResources( inGState->GetIdentifier(), inGState->GetObject()->Reference(), PdfName("ExtGState") );
    
    m_oss.str("");
    m_oss << "/" << inGState->GetIdentifier().GetName()
          << " gs" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetRenderingIntent( char* intent )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    m_oss.str("");
    m_oss << "/" << intent
          << " ri" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

}


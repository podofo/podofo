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

#ifdef _MSC_VER
#pragma warning(disable: 4786)
#endif

#include <iostream>
#include <iomanip>

#include "PdfPainter.h"

#include "PdfColor.h"
#include "PdfContents.h"
#include "PdfDictionary.h"
#include "PdfExtGState.h"
#include "PdfFilter.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfImage.h"
#include "PdfName.h"
#include "PdfRect.h"
#include "PdfShadingPattern.h"
#include "PdfStream.h"
#include "PdfString.h"
#include "PdfXObject.h"
#include "PdfLocale.h"

#define BEZIER_POINTS 13

/* 4/3 * (1-cos 45ƒ)/sin 45ƒ = 4/3 * sqrt(2) - 1 */
#define ARC_MAGIC    0.552284749f
#define PI           3.141592654f

namespace PoDoFo {

struct TLineElement 
{
	TLineElement()
		: pszStart( NULL ), lLen( 0L )
	{
	}

	const char* pszStart;
	long        lLen;
};

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
    PdfLocaleImbue(m_oss);
}

PdfPainter::~PdfPainter()
{
    PODOFO_RAISE_LOGIC_IF( m_pCanvas, "FinishPage() has to be called after a page is completed!" );
}

void PdfPainter::SetPage( PdfCanvas* pPage )
{
    // Ignore setting the same page twice
    if( m_pPage == pPage )
        return;

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

    this->SetStrokingColor( PdfColor( g ) );
}

void PdfPainter::SetGray( double g )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );
    CheckDoubleRange( g, 0.0, 1.0 );

    this->SetColor( PdfColor( g ) );
}

void PdfPainter::SetStrokingColor( double r, double g, double b )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    CheckDoubleRange( r, 0.0, 1.0 );
    CheckDoubleRange( g, 0.0, 1.0 );
    CheckDoubleRange( b, 0.0, 1.0 );

    this->SetStrokingColor( PdfColor( r, g, b ) );
}

void PdfPainter::SetColor( double r, double g, double b )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );
    CheckDoubleRange( r, 0.0, 1.0 );
    CheckDoubleRange( g, 0.0, 1.0 );
    CheckDoubleRange( b, 0.0, 1.0 );

    this->SetColor( PdfColor( r, g, b ) );
}

void PdfPainter::SetStrokingColorCMYK( double c, double m, double y, double k )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );
    CheckDoubleRange( c, 0.0, 1.0 );
    CheckDoubleRange( m, 0.0, 1.0 );
    CheckDoubleRange( y, 0.0, 1.0 );
    CheckDoubleRange( k, 0.0, 1.0 );

    this->SetStrokingColor( PdfColor( c, m, y, k ) );
}

void PdfPainter::SetColorCMYK( double c, double m, double y, double k )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    
    CheckDoubleRange( c, 0.0, 1.0 );
    CheckDoubleRange( m, 0.0, 1.0 );
    CheckDoubleRange( y, 0.0, 1.0 );
    CheckDoubleRange( k, 0.0, 1.0 );

    this->SetColor( PdfColor( c, m, y, k ) );
}

void PdfPainter::SetStrokingShadingPattern( const PdfShadingPattern & rPattern )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

    this->AddToPageResources( rPattern.GetIdentifier(), rPattern.GetObject()->Reference(), PdfName("Pattern") );

    m_oss.str("");
    m_oss << "/Pattern CS /" << rPattern.GetIdentifier().GetName() << " SCN" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetShadingPattern( const PdfShadingPattern & rPattern )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

    this->AddToPageResources( rPattern.GetIdentifier(), rPattern.GetObject()->Reference(), PdfName("Pattern") );

    m_oss.str("");
    m_oss << "/Pattern cs /" << rPattern.GetIdentifier().GetName() << " scn" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetStrokingColor( const PdfColor & rColor )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

    m_oss.str("");

    switch( rColor.GetColorSpace() ) 
    {
        default: 
        case ePdfColorSpace_DeviceRGB:
            m_oss << rColor.GetRed()   << " "
                  << rColor.GetGreen() << " "
                  << rColor.GetBlue() 
                  << " RG" << std::endl;
            break;
        case ePdfColorSpace_DeviceCMYK:
            m_oss << rColor.GetCyan()    << " " 
                  << rColor.GetMagenta() << " " 
                  << rColor.GetYellow()  << " " 
                  << rColor.GetBlack() 
                  << " K" << std::endl;
            break;
        case ePdfColorSpace_DeviceGray:
            m_oss << rColor.GetGrayScale() << " G" << std::endl;
            break;
    }

    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetColor( const PdfColor & rColor )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

    m_oss.str("");

    switch( rColor.GetColorSpace() ) 
    {
        default: 
        case ePdfColorSpace_DeviceRGB:
            m_eCurColorSpace = ePdfColorSpace_DeviceRGB;
            m_curColor1      = rColor.GetRed();
            m_curColor2      = rColor.GetGreen();
            m_curColor3      = rColor.GetBlue();

            m_oss << rColor.GetRed()   << " "
                  << rColor.GetGreen() << " "
                  << rColor.GetBlue() 
                  << " rg" << std::endl;
            break;
        case ePdfColorSpace_DeviceCMYK:
            m_eCurColorSpace = ePdfColorSpace_DeviceCMYK;
            m_curColor1      = rColor.GetCyan();
            m_curColor2      = rColor.GetMagenta();
            m_curColor3      = rColor.GetYellow();
            m_curColor4      = rColor.GetBlack();

            m_oss << rColor.GetCyan()    << " " 
                  << rColor.GetMagenta() << " " 
                  << rColor.GetYellow()  << " " 
                  << rColor.GetBlack() 
                  << " k" << std::endl;
            break;
        case ePdfColorSpace_DeviceGray:
            m_eCurColorSpace = ePdfColorSpace_DeviceGray;
            m_curColor1      = rColor.GetGrayScale();

            m_oss << rColor.GetGrayScale() << " g" << std::endl;
            break;
    }

    m_pCanvas->Append( m_oss.str() );
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

void PdfPainter::SetClipRect( double dX, double dY, double dWidth, double dHeight )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

    m_oss.str("");
    m_oss << dX << " "
          << dY << " "
          << dWidth << " "
          << dHeight        
          << " re W n" << std::endl;
    m_pCanvas->Append( m_oss.str() );
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

    if ( static_cast<int>(dRoundX) || static_cast<int>(dRoundY) ) 
    {
        double x = dX, y = dY, 
               w = dWidth, h = dHeight,
               rx= dRoundX, ry = dRoundY;
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
    } 
    else 
    {
        m_oss.str("");
        m_oss << dX << " "
              << dY << " "
              << dWidth << " "
              << dHeight        
              << " re S" << std::endl;
        m_pCanvas->Append( m_oss.str() );
    }
}

void PdfPainter::FillRect( double dX, double dY, double dWidth, double dHeight,
                           double dRoundX, double dRoundY )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    m_oss.str("");

    if ( static_cast<int>(dRoundX) || static_cast<int>(dRoundY) ) 
    {
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
    }
    else 
    {
        m_oss << dX << " "
            << dY << " "
            << dWidth << " "
            << dHeight        
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
    this->DrawText( dX, dY, sText, sText.GetCharacterLength() );
}

void PdfPainter::DrawText( double dX, double dY, const PdfString & sText, long lStringLen )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    if( !m_pFont || !m_pPage || !sText.IsValid() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    PdfString sString = this->ExpandTabs( sText, lStringLen );
    this->AddToPageResources( m_pFont->GetIdentifier(), m_pFont->GetObject()->Reference(), PdfName("Font") );

    if( m_pFont->IsUnderlined() || m_pFont->IsStrikeOut())
    {
        this->Save();
        this->SetCurrentStrokingColor();
		
        // Draw underline
        this->SetStrokeWidth( m_pFont->GetFontMetrics()->GetUnderlineThickness() );
        if( m_pFont->IsUnderlined() )
            this->DrawLine( dX, 
                            dY + m_pFont->GetFontMetrics()->GetUnderlinePosition(), 
                            dX + m_pFont->GetFontMetrics()->StringWidth( sString.GetString() ),
                            dY + m_pFont->GetFontMetrics()->GetUnderlinePosition() );
        
        // Draw strikeout
        this->SetStrokeWidth( m_pFont->GetFontMetrics()->GetStrikeoutThickness() );
        if( m_pFont->IsStrikeOut() )
            this->DrawLine( dX, 
                            dY + m_pFont->GetFontMetrics()->GetStrikeOutPosition(), 
                            dX + m_pFont->GetFontMetrics()->StringWidth( sString.GetString() ),
                            dY + m_pFont->GetFontMetrics()->GetStrikeOutPosition() );
        

        this->Restore();
    }
    
    m_oss.str("");
    m_oss << "BT" << std::endl << "/" << m_pFont->GetIdentifier().GetName()
          << " "  << m_pFont->GetFontSize()
          << " Tf" << std::endl;

    //if( m_pFont->GetFontScale() != 100.0F ) - this value is kept between text blocks
    m_oss << m_pFont->GetFontScale() << " Tz" << std::endl;

    //if( m_pFont->GetFontCharSpace() != 0.0F )  - this value is kept between text blocks
    m_oss << m_pFont->GetFontCharSpace() * m_pFont->GetFontSize() / 100.0 << " Tc" << std::endl;

    m_oss << dX << std::endl
          << dY << std::endl << "Td ";

    m_pCanvas->Append( m_oss.str() );
    m_pFont->WriteStringToStream( sString, m_pCanvas );

    /*
    char* pBuffer;
    std::auto_ptr<PdfFilter> pFilter = PdfFilterFactory::Create( ePdfFilter_ASCIIHexDecode );
    pFilter->Encode( sString.GetString(), sString.GetLength(), &pBuffer, &lLen );

    m_pCanvas->Append( pBuffer, lLen );
    free( pBuffer );
    */

    m_pCanvas->Append( " Tj\nET\n" );
}

void PdfPainter::DrawMultiLineText( double dX, double dY, double dWidth, double dHeight, const PdfString & rsText, 
                                    EPdfAlignment eAlignment, EPdfVerticalAlignment eVertical )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    if( !m_pFont || !m_pPage || !rsText.IsValid() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
    if( dWidth <= 0.0 || dHeight <= 0.0 ) // nonsense arguments
        return;
    
    TLineElement              tLine;
    std::vector<TLineElement> vecLines;
    this->Save();
    this->SetClipRect( dX, dY, dWidth, dHeight );
    
    PdfString   sString  = this->ExpandTabs( rsText, rsText.GetCharacterLength() );
    tLine.pszStart       = sString.GetString();
    const char* pszEnd   = tLine.pszStart;
    const char* pszWord  = tLine.pszStart;
    
    double dCurWidth = 0.0;

    // do simple word wrapping
    // TODO: Use better algorithm!
    while( *pszEnd ) 
    {
        dCurWidth += m_pFont->GetFontMetrics()->CharWidth( *pszEnd );
        
        if( *pszEnd == '\n' ) // hard-break!
        {
            ++pszEnd; // skip the line feed
            
            tLine.lLen = pszEnd - tLine.pszStart;
            vecLines.push_back( tLine );
            
            tLine.pszStart = pszEnd;
            dCurWidth = 0.0;
        }
        else if( isspace( static_cast<unsigned int>(static_cast<unsigned char>(*pszEnd)) ) || 
                 ispunct( static_cast<unsigned int>(static_cast<unsigned char>(*pszEnd)) ))
            pszWord = pszEnd;
        
        if( dCurWidth > dWidth ) 
        {
            // The last word does not fit anymore in the current line.
            // -> Move it to the next one.
			
            // skip leading whitespaces!
            while( *tLine.pszStart && isspace( static_cast<unsigned int>(static_cast<unsigned char>(*tLine.pszStart)) ) )
                ++tLine.pszStart;

            tLine.lLen = pszEnd - tLine.pszStart;
            vecLines.push_back( tLine );
            tLine.pszStart = pszWord;

            dCurWidth = pszEnd-pszWord > 0 ? 
                m_pFont->GetFontMetrics()->StringWidth( pszWord, pszEnd-pszWord ) : 0.0;
        }
        ++pszEnd;
    }

    if( pszEnd-tLine.pszStart > 0 ) 
    {
        tLine.lLen = pszEnd - tLine.pszStart;
        vecLines.push_back( tLine );
    }

    // Do vertical alignment
    switch( eVertical ) 
    {
        default:
        case ePdfVerticalAlignment_Top:
            dY += dHeight; break;
        case ePdfVerticalAlignment_Bottom:
            dY += m_pFont->GetFontMetrics()->GetLineSpacing() * vecLines.size(); break;
        case ePdfVerticalAlignment_Center:
            dY += (dHeight - 
                   ((dHeight - (m_pFont->GetFontMetrics()->GetLineSpacing() * vecLines.size()))/2.0)); 
            break;
    }

    std::vector<TLineElement>::const_iterator it = vecLines.begin();
    while( it != vecLines.end() )
    {
        dY -= m_pFont->GetFontMetrics()->GetLineSpacing();
        if( (*it).pszStart )
            this->DrawTextAligned( dX, dY, dWidth, PdfString( (*it).pszStart, (*it).lLen ), eAlignment );

        ++it;
    }
    this->Restore();
}

void PdfPainter::DrawTextAligned( double dX, double dY, double dWidth, const PdfString & rsText, EPdfAlignment eAlignment )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    if( !m_pFont || !m_pPage || !rsText.IsValid() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( dWidth <= 0.0 ) // nonsense arguments
        return;

    switch( eAlignment ) 
    {
        default:
        case ePdfAlignment_Left:
            break;
        case ePdfAlignment_Center:
            dX += (dWidth - m_pFont->GetFontMetrics()->StringWidth( rsText ) ) / 2.0;
            break;
        case ePdfAlignment_Right:
            dX += (dWidth - m_pFont->GetFontMetrics()->StringWidth( rsText ) );
            break;
    }

    this->DrawText( dX, dY, rsText );
}

void PdfPainter::DrawImage( double dX, double dY, PdfImage* pObject, double dScaleX, double dScaleY )
{
    this->DrawXObject( dX, dY, reinterpret_cast<PdfXObject*>(pObject), 
                       dScaleX * pObject->GetPageSize().GetWidth(), 
                       dScaleY * pObject->GetPageSize().GetHeight() );
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
          << dScaleX << " 0 0 "
          << dScaleY << " "
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

void PdfPainter::Clip()
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );
    
    m_pCanvas->Append( "W n\n" );
}

void PdfPainter::Save()
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    m_pCanvas->Append( "q\n" );
}

void PdfPainter::Restore()
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    m_pCanvas->Append( "Q\n" );
}

void PdfPainter::AddToPageResources( const PdfName & rIdentifier, const PdfReference & rRef, const PdfName & rName )
{
    if( !m_pPage )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pPage->AddResource( rIdentifier, rRef, rName );
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

template<typename C>
PdfString PdfPainter::ExpandTabsPrivate( const C* pszText, long lStringLen, int nTabCnt, const C cTab, const C cSpace ) const
{
    printf("TABCOUNT=%i size=%i width=%i cSpace=%04x\n", nTabCnt, sizeof(C),m_nTabWidth, cSpace);
    long lLen    = lStringLen + nTabCnt*(m_nTabWidth-1) + sizeof(C);
    C*   pszTab  = static_cast<C*>(malloc( sizeof( C ) * lLen ));

    if( !pszTab )
    {
        PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
    }
    
    int i = 0;
    while( lStringLen-- )
    {
        if( *pszText == cTab )
        {
            for( int z=0;z<m_nTabWidth; z++ )
                pszTab[i+z] = cSpace;
            
            i+=m_nTabWidth;
        }
        else
            pszTab[i++] = *pszText;
        
        ++pszText;
    }
    
    pszTab[i]  = 0;

    PdfString str( pszTab );
    printf("OUT=");
    for(int z=0;z<lLen;z++)
        printf("%04x ", str.GetUnicode()[z] );
    printf("\n\n");
    free( pszTab );
    
    return str;
}

PdfString PdfPainter::ExpandTabs( const PdfString & rsString, long lStringLen ) const
{
    int               nTabCnt  = 0;
    int               i;
    bool              bUnicode = rsString.IsUnicode();
    const pdf_utf16be cTab     = 0x0900;
    const pdf_utf16be cSpace   = 0x2000;

    // count the number of tabs in the string
    if( bUnicode ) 
    {
        for( i=0;i<=lStringLen;i++ )
            if( rsString.GetUnicode()[i] == cTab ) 
                ++nTabCnt;
    }
    else
    {
        for( i=0;i<=lStringLen;i++ )
            if( rsString.GetString()[i] == '\t' )
                ++nTabCnt;
    }

    // if no tabs are found: bail out!
    if( !nTabCnt )
        return rsString;
    
    if( rsString.IsUnicode() )
        return ExpandTabsPrivate<pdf_utf16be>( rsString.GetUnicode(), lStringLen, nTabCnt, cTab, cSpace );
    else
        return ExpandTabsPrivate<char>( rsString.GetString(), lStringLen, nTabCnt, '\t', ' ' );
}

} /* namespace PoDoFo */



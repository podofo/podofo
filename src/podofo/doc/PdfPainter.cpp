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

#if defined(_MSC_VER)  &&  _MSC_VER <= 1200
#pragma warning(disable: 4786)
#endif

#include <iostream>
#include <iomanip>
#include <vector>

#include "PdfPainter.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfColor.h"
#include "base/PdfDictionary.h"
#include "base/PdfFilter.h"
#include "base/PdfName.h"
#include "base/PdfRect.h"
#include "base/PdfStream.h"
#include "base/PdfString.h"
#include "base/PdfLocale.h"

#include "PdfContents.h"
#include "PdfExtGState.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfImage.h"
#include "PdfMemDocument.h"
#include "PdfShadingPattern.h"
#include "PdfTilingPattern.h"
#include "PdfXObject.h"


#include <stdlib.h>

#define BEZIER_POINTS 13

/* 4/3 * (1-cos 45,A0(B)/sin 45,A0(B = 4/3 * sqrt(2) - 1 */
#define ARC_MAGIC    0.552284749f
#define PI           3.141592654f

namespace PoDoFo {

static const long clPainterHighPrecision    = 15L;
static const long clPainterDefaultPrecision = 3L;

static inline void CheckDoubleRange( double val, double min, double max )
{
    if( val < min || val > max )
    {
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }
}

static inline unsigned short SwapBytes(unsigned short val)
{
	return ((val & 0x00FF) << 8) | ((val & 0xFF00) >> 8);
}

static inline unsigned short SwapCharBytesIfRequired(pdf_utf16be ch)
{
#ifdef PODOFO_IS_LITTLE_ENDIAN
	return SwapBytes(ch);
#else
	return ch;
#endif
}

static inline bool IsNewLineChar(pdf_utf16be ch)
{
    return SwapCharBytesIfRequired(ch) == '\n';
}

static inline bool IsSpaceChar(pdf_utf16be ch)
{
	return iswspace( SwapCharBytesIfRequired(ch) ) != 0;
}

PdfPainter::PdfPainter()
: m_pCanvas( NULL ), m_pPage( NULL ), m_pFont( NULL ), m_nTabWidth( 4 ),
  m_curColor( PdfColor( 0.0, 0.0, 0.0 ) ),
  m_isTextOpen( false ), m_oss(), m_curPath(), m_isCurColorICCDepend( false ), m_CSTag()
{
    m_oss.flags( std::ios_base::fixed );
    m_oss.precision( clPainterDefaultPrecision );
    PdfLocaleImbue(m_oss);

    m_curPath.flags( std::ios_base::fixed );
    m_curPath.precision( clPainterDefaultPrecision );
    PdfLocaleImbue(m_curPath);

    lpx  = 
    lpy  = 
    lpx2 = 
    lpy2 = 
    lpx3 = 
    lpy3 = 
    lcx  =
    lcy  = 
    lrx  = 
    lry  = 0.0;
    currentTextRenderingMode = ePdfTextRenderingMode_Fill;
}

PdfPainter::~PdfPainter()
{
	// Throwing exceptions in C++ destructors is not allowed.
	// Just log the error.
    // PODOFO_RAISE_LOGIC_IF( m_pCanvas, "FinishPage() has to be called after a page is completed!" );
    // Note that we can't do this for the user, since FinishPage() might
    // throw and we can't safely have that in a dtor. That also means
    // we can't throw here, but must abort.
    if( m_pCanvas )
        PdfError::LogMessage( eLogSeverity_Error, 
                              "PdfPainter::~PdfPainter(): FinishPage() has to be called after a page is completed!" );

    #ifdef DEBUG
    PODOFO_ASSERT( !m_pCanvas );
    #endif
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
        // GetLength() must be called before BeginAppend()
        if ( m_pCanvas->GetLength() ) 
        {
            m_pCanvas->BeginAppend( false );
            // there is already content here - so let's assume we are appending
            // as such, we MUST put in a "space" to separate whatever we do.
            m_pCanvas->Append( " " );
        }
        else
            m_pCanvas->BeginAppend( false );

        currentTextRenderingMode = ePdfTextRenderingMode_Fill;
    } 
    else 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
}

void PdfPainter::FinishPage()
{
	try { 
		if( m_pCanvas )
			m_pCanvas->EndAppend();
	} catch( PdfError & e ) {
	    // clean up, even in case of error
		m_pCanvas = NULL;
		m_pPage   = NULL;

		throw e;
	}

    m_pCanvas = NULL;
    m_pPage   = NULL;
    currentTextRenderingMode = ePdfTextRenderingMode_Fill;
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

void PdfPainter::SetStrokingTilingPattern( const PdfTilingPattern & rPattern )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

    this->AddToPageResources( rPattern.GetIdentifier(), rPattern.GetObject()->Reference(), PdfName("Pattern") );

    m_oss.str("");
    m_oss << "/Pattern CS /" << rPattern.GetIdentifier().GetName() << " SCN" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetStrokingTilingPattern( const std::string &rPatternName )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

    m_oss.str("");
    m_oss << "/Pattern CS /" << rPatternName << " SCN" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetTilingPattern( const PdfTilingPattern & rPattern )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

    this->AddToPageResources( rPattern.GetIdentifier(), rPattern.GetObject()->Reference(), PdfName("Pattern") );

    m_oss.str("");
    m_oss << "/Pattern cs /" << rPattern.GetIdentifier().GetName() << " scn" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetTilingPattern( const std::string &rPatternName )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

    m_oss.str("");
    m_oss << "/Pattern cs /" << rPatternName << " scn" << std::endl;
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
        case ePdfColorSpace_Separation:
			m_pPage->AddColorResource( rColor );
			m_oss << "/ColorSpace" << PdfName( rColor.GetName() ).GetEscapedName() << " CS " << rColor.GetDensity() << " SCN" << std::endl;
            break;
        case ePdfColorSpace_CieLab:
			m_pPage->AddColorResource( rColor );
			m_oss << "/ColorSpaceCieLab" << " CS " 
				  << rColor.GetCieL() << " " 
                  << rColor.GetCieA() << " " 
                  << rColor.GetCieB() <<
				  " SCN" << std::endl;
            break;
        case ePdfColorSpace_Unknown:
        case ePdfColorSpace_Indexed:
        {
            PODOFO_RAISE_ERROR( ePdfError_CannotConvertColor );
        }
    }

    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetColor( const PdfColor & rColor )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

    m_isCurColorICCDepend = false;

    m_oss.str("");

    m_curColor = rColor;
    switch( rColor.GetColorSpace() ) 
    {
        default: 
        case ePdfColorSpace_DeviceRGB:
            m_oss << rColor.GetRed()   << " "
                  << rColor.GetGreen() << " "
                  << rColor.GetBlue() 
                  << " rg" << std::endl;
            break;
        case ePdfColorSpace_DeviceCMYK:
            m_oss << rColor.GetCyan()    << " " 
                  << rColor.GetMagenta() << " " 
                  << rColor.GetYellow()  << " " 
                  << rColor.GetBlack() 
                  << " k" << std::endl;
            break;
        case ePdfColorSpace_DeviceGray:
            m_oss << rColor.GetGrayScale() << " g" << std::endl;
            break;
        case ePdfColorSpace_Separation:
			m_pPage->AddColorResource( rColor );
            m_oss << "/ColorSpace" << PdfName( rColor.GetName() ).GetEscapedName() << " cs " << rColor.GetDensity() << " scn" << std::endl;
            break;
        case ePdfColorSpace_CieLab:
			m_pPage->AddColorResource( rColor );
			m_oss << "/ColorSpaceCieLab" << " cs " 
				  << rColor.GetCieL() << " " 
                  << rColor.GetCieA() << " " 
                  << rColor.GetCieB() <<
				  " scn" << std::endl;
			break;
        case ePdfColorSpace_Unknown:
        case ePdfColorSpace_Indexed:
        {
            PODOFO_RAISE_ERROR( ePdfError_CannotConvertColor );
        }
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

void PdfPainter::SetStrokeStyle( EPdfStrokeStyle eStyle, const char* pszCustom, bool inverted, double scale, bool subtractJoinCap)
{
    bool have = false;

    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

    m_oss.str("");

    if (eStyle != ePdfStrokeStyle_Custom) {
        m_oss << "[";
    }

    if (inverted && eStyle != ePdfStrokeStyle_Solid && eStyle != ePdfStrokeStyle_Custom) {
       m_oss << "0 ";
    }

    switch( eStyle )
    {
        case ePdfStrokeStyle_Solid:
            have = true;
            break;
        case ePdfStrokeStyle_Dash:
            have = true;
            if (scale >= 1.0 - 1e-5 && scale <= 1.0 + 1e-5) {
                m_oss << "6 2";
            } else {
                if (subtractJoinCap) {
                    m_oss << scale * 2.0 << " " << scale * 2.0;
                } else {
                    m_oss << scale * 3.0 << " " << scale * 1.0;
                }
            }
            break;
        case ePdfStrokeStyle_Dot:
            have = true;
            if (scale >= 1.0 - 1e-5 && scale <= 1.0 + 1e-5) {
                m_oss << "2 2";
            } else {
                if (subtractJoinCap) {
                    // zero length segments are drawn anyway here
                    m_oss << 0.001 << " " << 2.0 * scale << " " << 0 << " " << 2.0 * scale;
                } else {
                   m_oss << scale * 1.0 << " " << scale * 1.0;
                }
            }
            break;
        case ePdfStrokeStyle_DashDot:
            have = true;
            if (scale >= 1.0 - 1e-5 && scale <= 1.0 + 1e-5) {
                m_oss << "3 2 1 2";
            } else {
                if (subtractJoinCap) {
                    // zero length segments are drawn anyway here
                    m_oss << scale * 2.0 << " " << scale * 2.0 << " " << 0 << " " << scale * 2.0;
                } else {
                    m_oss << scale * 3.0 << " " << scale * 1.0 << " " << scale * 1.0 << " " << scale * 1.0;
                }
            }
            break;
        case ePdfStrokeStyle_DashDotDot:
            have = true;
            if (scale >= 1.0 - 1e-5 && scale <= 1.0 + 1e-5) {
                m_oss << "3 1 1 1 1 1";
            } else {
                if (subtractJoinCap) {
                    // zero length segments are drawn anyway here
                    m_oss << scale * 2.0 << " " << scale * 2.0 << " " << 0 << " " << scale * 2.0 << " " << 0 << " " << scale * 2.0;
                } else {
                    m_oss << scale * 3.0 << " " << scale * 1.0 << " " << scale * 1.0 << " " << scale * 1.0 << " " << scale * 1.0 << " " << scale * 1.0;
                }
            }
            break;
        case ePdfStrokeStyle_Custom:
            have = pszCustom != NULL;
            if (have)
                m_oss << pszCustom;
            break;
        default:
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidStrokeStyle );
        }
    }

    if( !have )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidStrokeStyle );
    }
    
    if (inverted && eStyle != ePdfStrokeStyle_Solid && eStyle != ePdfStrokeStyle_Custom) {
        m_oss << " 0";
    }

    if (eStyle != ePdfStrokeStyle_Custom) {
        m_oss << "] 0";
    }

    m_oss << " d" << std::endl;
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
    m_oss << static_cast<int>(eJoinStyle) << " j" << std::endl;
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

void PdfPainter::SetTextRenderingMode( EPdfTextRenderingMode mode )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

    if (mode == currentTextRenderingMode) {
        return;
    }

    currentTextRenderingMode = mode;
    if (m_isTextOpen)
        SetCurrentTextRenderingMode();
}

void PdfPainter::SetCurrentTextRenderingMode( void )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    m_oss << (int) currentTextRenderingMode << " Tr" << std::endl;
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

	 m_curPath
			 << dX << " "
          << dY << " "
          << dWidth << " "
          << dHeight        
          << " re W n" << std::endl;
}

void PdfPainter::SetMiterLimit(double value)
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

    m_oss.str("");
    m_oss << value << " M" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::DrawLine( double dStartX, double dStartY, double dEndX, double dEndY )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );    

	 m_curPath.str("");
    m_curPath
		    << dStartX << " "
          << dStartY
          << " m "
          << dEndX << " "
          << dEndY        
          << " l" << std::endl;

    m_oss.str("");
    m_oss << dStartX << " "
          << dStartY
          << " m "
          << dEndX << " "
          << dEndY        
          << " l S" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::Rectangle( double dX, double dY, double dWidth, double dHeight,
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
    } 
    else 
    {
        m_curPath 
				  << dX << " "
              << dY << " "
              << dWidth << " "
              << dHeight        
              << " re" << std::endl;

    m_oss.str("");
        m_oss << dX << " "
            << dY << " "
            << dWidth << " "
            << dHeight        
              << " re" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}
}

void PdfPainter::Ellipse( double dX, double dY, double dWidth, double dHeight )
{
    double dPointX[BEZIER_POINTS];
    double dPointY[BEZIER_POINTS];
    int    i;

    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    ConvertRectToBezier( dX, dY, dWidth, dHeight, dPointX, dPointY );

    m_curPath
			 << dPointX[0] << " "
          << dPointY[0]
          << " m" << std::endl;

    m_oss.str("");
    m_oss << dPointX[0] << " "
          << dPointY[0]
          << " m" << std::endl;

    for( i=1;i<BEZIER_POINTS; i+=3 )
    {
        m_curPath
				  << dPointX[i] << " "
              << dPointY[i] << " "
              << dPointX[i+1] << " "
              << dPointY[i+1] << " "
              << dPointX[i+2] << " "
              << dPointY[i+2]    
              << " c" << std::endl;

        m_oss << dPointX[i] << " "
              << dPointY[i] << " "
              << dPointX[i+1] << " "
              << dPointY[i+1] << " "
              << dPointX[i+2] << " "
              << dPointY[i+2]    
              << " c" << std::endl;
    }

    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::Circle( double dX, double dY, double dRadius )
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
}

void PdfPainter::DrawText( double dX, double dY, const PdfString & sText )
{
    this->DrawText( dX, dY, sText, static_cast<long>(sText.GetCharacterLength()) );
}

void PdfPainter::DrawText( double dX, double dY, const PdfString & sText, long lStringLen )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    if( !m_pFont || !m_pPage || !sText.IsValid() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // Peter Petrov 25 September 2008
    //m_pFont->EmbedFont();

    PdfString sString = this->ExpandTabs( sText, lStringLen );
    this->AddToPageResources( m_pFont->GetIdentifier(), m_pFont->GetObject()->Reference(), PdfName("Font") );
    if( m_pFont->IsSubsetting() )
    {
        m_pFont->AddUsedSubsettingGlyphs( sText, lStringLen );
    }

    if( m_pFont->IsUnderlined() || m_pFont->IsStrikeOut())
    {
        this->Save();
        this->SetCurrentStrokingColor();
		
        // Draw underline
        this->SetStrokeWidth( m_pFont->GetFontMetrics()->GetUnderlineThickness() );
        if( m_pFont->IsUnderlined() )
        {
            if (sString.IsUnicode())
                this->DrawLine( dX,
                                dY + m_pFont->GetFontMetrics()->GetUnderlinePosition(),
                                dX + m_pFont->GetFontMetrics()->StringWidth( sString.GetUnicode() ),
                                dY + m_pFont->GetFontMetrics()->GetUnderlinePosition() );
            else
                this->DrawLine( dX,
                                dY + m_pFont->GetFontMetrics()->GetUnderlinePosition(),
                                dX + m_pFont->GetFontMetrics()->StringWidth( sString.GetString() ),
                                dY + m_pFont->GetFontMetrics()->GetUnderlinePosition() );
        }

        // Draw strikeout
        this->SetStrokeWidth( m_pFont->GetFontMetrics()->GetStrikeoutThickness() );
        if( m_pFont->IsStrikeOut() )
        {
            if (sString.IsUnicode())
                this->DrawLine( dX,
                                dY + m_pFont->GetFontMetrics()->GetStrikeOutPosition(),
                                dX + m_pFont->GetFontMetrics()->StringWidth( sString.GetUnicode() ),
                                dY + m_pFont->GetFontMetrics()->GetStrikeOutPosition() );
            else
                this->DrawLine( dX,
                                dY + m_pFont->GetFontMetrics()->GetStrikeOutPosition(),
                                dX + m_pFont->GetFontMetrics()->StringWidth( sString.GetString() ),
                                dY + m_pFont->GetFontMetrics()->GetStrikeOutPosition() );
        }

        this->Restore();
    }
    



    m_oss.str("");
    m_oss << "BT" << std::endl << "/" << m_pFont->GetIdentifier().GetName()
          << " "  << m_pFont->GetFontSize()
          << " Tf" << std::endl;

    if (currentTextRenderingMode != ePdfTextRenderingMode_Fill) {
        SetCurrentTextRenderingMode();
    }

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
    PODOFO_UNIQUEU_PTR<PdfFilter> pFilter( PdfFilterFactory::Create( ePdfFilter_ASCIIHexDecode ) );
    pFilter->Encode( sString.GetString(), sString.GetLength(), &pBuffer, &lLen );

    m_pCanvas->Append( pBuffer, lLen );
    podofo_free( pBuffer );
    */

    m_pCanvas->Append( " Tj\nET\n" );
}

void PdfPainter::BeginText( double dX, double dY )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    if( !m_pFont || !m_pPage ||  m_isTextOpen)
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    this->AddToPageResources( m_pFont->GetIdentifier(), m_pFont->GetObject()->Reference(), PdfName("Font") );

    m_oss.str("");
    m_oss << "BT" << std::endl << "/" << m_pFont->GetIdentifier().GetName()
          << " "  << m_pFont->GetFontSize()
          << " Tf" << std::endl;

    if (currentTextRenderingMode != ePdfTextRenderingMode_Fill) {
        SetCurrentTextRenderingMode();
    }

    //if( m_pFont->GetFontScale() != 100.0F ) - this value is kept between text blocks
    m_oss << m_pFont->GetFontScale() << " Tz" << std::endl;

    //if( m_pFont->GetFontCharSpace() != 0.0F )  - this value is kept between text blocks
    m_oss << m_pFont->GetFontCharSpace() * m_pFont->GetFontSize() / 100.0 << " Tc" << std::endl;

    m_oss << dX << " " << dY << " Td" << std::endl ;

    m_pCanvas->Append( m_oss.str() );

	m_isTextOpen = true;
}

void PdfPainter::MoveTextPos( double dX, double dY )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    if( !m_pFont || !m_pPage || !m_isTextOpen )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_oss.str("");
    m_oss << dX << " " << dY << " Td" << std::endl ;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::AddText( const PdfString & sText )
{
	AddText( sText, sText.GetCharacterLength() );
}

void PdfPainter::AddText( const PdfString & sText, pdf_long lStringLen )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    if( !m_pFont || !m_pPage || !sText.IsValid() || !m_isTextOpen )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    PdfString sString = this->ExpandTabs( sText, lStringLen );
    if( m_pFont->IsSubsetting() )
    {
        m_pFont->AddUsedSubsettingGlyphs( sText, lStringLen );
    }

	// TODO: Underline and Strikeout not yet supported
    
	m_pFont->WriteStringToStream( sString, m_pCanvas );

    m_pCanvas->Append( " Tj\n" );
}

void PdfPainter::EndText()
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    if( !m_pFont || !m_pPage || !m_isTextOpen )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pCanvas->Append( "ET\n" );
	m_isTextOpen = false;
}

void PdfPainter::DrawMultiLineText( double dX, double dY, double dWidth, double dHeight, const PdfString & rsText, 
                                    EPdfAlignment eAlignment, EPdfVerticalAlignment eVertical, bool bClip, bool bSkipSpaces )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    if( !m_pFont || !m_pPage || !rsText.IsValid() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // Peter Petrov 25 September 2008
    //m_pFont->EmbedFont();
    
    if( dWidth <= 0.0 || dHeight <= 0.0 ) // nonsense arguments
        return;

    this->Save();
    if( bClip ) 
    {
        this->SetClipRect( dX, dY, dWidth, dHeight );
    }

    PdfString   sString  = this->ExpandTabs( rsText, rsText.GetCharacterLength() );

	std::vector<PdfString> vecLines = GetMultiLineTextAsLines( dWidth, sString, bSkipSpaces );
    double dLineGap = m_pFont->GetFontMetrics()->GetLineSpacing() - m_pFont->GetFontMetrics()->GetAscent() + m_pFont->GetFontMetrics()->GetDescent();
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

    dY -= (m_pFont->GetFontMetrics()->GetAscent() + dLineGap / (2.0));

    std::vector<PdfString>::const_iterator it = vecLines.begin();
    while( it != vecLines.end() )
    {
        if( (*it).GetCharacterLength() )
            this->DrawTextAligned( dX, dY, dWidth, *it, eAlignment );
        dY -= m_pFont->GetFontMetrics()->GetLineSpacing();
        ++it;
    }
    this->Restore();
}

std::vector<PdfString> PdfPainter::GetMultiLineTextAsLines( double dWidth, const PdfString & rsText, bool bSkipSpaces )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    if( !m_pFont || !m_pPage || !rsText.IsValid() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
     
    if( dWidth <= 0.0 ) // nonsense arguments
	    return std::vector<PdfString>();
    
    if( rsText.GetCharacterLength() == 0 ) // empty string
        return std::vector<PdfString>(1, rsText);
	        
    // We will work with utf16 encoded string because it allows us 
    // fast and easy individual characters access    
    const std::string& stringUtf8 = rsText.GetStringUtf8();
    std::vector<pdf_utf16be> stringUtf16(stringUtf8.length() + 1, 0);
    PODOFO_ASSERT( stringUtf16.size() > 0 );
    const pdf_long converted = PdfString::ConvertUTF8toUTF16(
	    reinterpret_cast<const pdf_utf8*>(stringUtf8.c_str()), &stringUtf16[0], stringUtf16.size());
	//const pdf_long len = rsText.GetCharacterLength();
    PODOFO_ASSERT( converted == (rsText.GetCharacterLength() + 1) );

	const pdf_utf16be* const stringUtf16Begin = &stringUtf16[0];
    const pdf_utf16be* pszLineBegin = stringUtf16Begin;
    const pdf_utf16be* pszCurrentCharacter = stringUtf16Begin;
    const pdf_utf16be* pszStartOfCurrentWord  = stringUtf16Begin;
    bool startOfWord = true;
    double dCurWidthOfLine = 0.0;
	std::vector<PdfString> vecLines;

    // do simple word wrapping
    while( *pszCurrentCharacter ) 
    {
        if( IsNewLineChar( *pszCurrentCharacter ) ) // hard-break! 
        {
            vecLines.push_back( PdfString( pszLineBegin, pszCurrentCharacter - pszLineBegin ) );

            pszLineBegin = pszCurrentCharacter + 1;// skip the line feed
            startOfWord = true;
            dCurWidthOfLine = 0.0;
        }
        else if( IsSpaceChar( *pszCurrentCharacter ) )
        {
            if( dCurWidthOfLine > dWidth )
            {
                // The previous word does not fit in the current line.
                // -> Move it to the next one.
                if( pszStartOfCurrentWord > pszLineBegin )
                {
                    vecLines.push_back( PdfString( pszLineBegin, pszStartOfCurrentWord - pszLineBegin ) );
                }
                else
                {
                    vecLines.push_back( PdfString( pszLineBegin, pszCurrentCharacter - pszLineBegin ) );
                    if (bSkipSpaces)
                    {
                        // Skip all spaces at the end of the line
                        while( IsSpaceChar( *( pszCurrentCharacter + 1 ) ) )
                            pszCurrentCharacter++;

                        pszStartOfCurrentWord = pszCurrentCharacter + 1;
                    }
                    else
                    {
                        pszStartOfCurrentWord = pszCurrentCharacter;
                    }
                    startOfWord=true;
                }
                pszLineBegin = pszStartOfCurrentWord;

                if (!startOfWord)
                {
                    dCurWidthOfLine = m_pFont->GetFontMetrics()->StringWidth( 
						pszStartOfCurrentWord, pszCurrentCharacter - pszStartOfCurrentWord );
                }
                else
                {
                    dCurWidthOfLine = 0.0;
                }
            }
            else if( ( dCurWidthOfLine + m_pFont->GetFontMetrics()->UnicodeCharWidth( SwapCharBytesIfRequired( *pszCurrentCharacter ) ) ) > dWidth )
            {
                vecLines.push_back( PdfString( pszLineBegin, pszCurrentCharacter - pszLineBegin ) );
                if( bSkipSpaces )
                {
                    // Skip all spaces at the end of the line
                    while( IsSpaceChar( *( pszCurrentCharacter + 1 ) ) )
                        pszCurrentCharacter++;

                    pszStartOfCurrentWord = pszCurrentCharacter + 1;
                }
                else
                {
                    pszStartOfCurrentWord = pszCurrentCharacter;
                }
                pszLineBegin = pszStartOfCurrentWord;
                startOfWord = true;
                dCurWidthOfLine = 0.0;
            }
            else 
            {           
                dCurWidthOfLine += m_pFont->GetFontMetrics()->UnicodeCharWidth( SwapCharBytesIfRequired( *pszCurrentCharacter ) );
            }

            startOfWord = true;
        }
        else
        {
            if (startOfWord)
            {
                pszStartOfCurrentWord = pszCurrentCharacter;
                startOfWord = false;
            }
            //else do nothing

            if ((dCurWidthOfLine + m_pFont->GetFontMetrics()->UnicodeCharWidth( SwapCharBytesIfRequired( *pszCurrentCharacter ))) > dWidth)
            {
                if ( pszLineBegin == pszStartOfCurrentWord )
                {
                    // This word takes up the whole line.
                    // Put as much as possible on this line.                    
                    if (pszLineBegin == pszCurrentCharacter)
                    {
                        vecLines.push_back( PdfString( pszCurrentCharacter, 1 ) );
                        pszLineBegin = pszCurrentCharacter + 1;
                        pszStartOfCurrentWord = pszCurrentCharacter + 1;
                        dCurWidthOfLine = 0;
                    }
                    else
                    {
                        vecLines.push_back(PdfString(pszLineBegin, pszCurrentCharacter - pszLineBegin));
                        pszLineBegin = pszCurrentCharacter;
                        pszStartOfCurrentWord = pszCurrentCharacter;
                        dCurWidthOfLine = m_pFont->GetFontMetrics()->UnicodeCharWidth( SwapCharBytesIfRequired( *pszCurrentCharacter ) );
                    }
                }
                else
                {
                    // The current word does not fit in the current line.
                    // -> Move it to the next one.                    
                    vecLines.push_back( PdfString( pszLineBegin, pszStartOfCurrentWord - pszLineBegin ) );
                    pszLineBegin = pszStartOfCurrentWord;
                    dCurWidthOfLine = m_pFont->GetFontMetrics()->StringWidth( pszStartOfCurrentWord, (pszCurrentCharacter - pszStartOfCurrentWord) + 1);
                }
            }
            else 
            {
                dCurWidthOfLine += m_pFont->GetFontMetrics()->UnicodeCharWidth( SwapCharBytesIfRequired( *pszCurrentCharacter ) );
            }
        }
        ++pszCurrentCharacter;
    }

    if( (pszCurrentCharacter - pszLineBegin) > 0 ) 
    {
        if( dCurWidthOfLine > dWidth && pszStartOfCurrentWord > pszLineBegin )
        {
            // The previous word does not fit in the current line.
            // -> Move it to the next one.
            vecLines.push_back( PdfString( pszLineBegin, pszStartOfCurrentWord - pszLineBegin) );
            pszLineBegin = pszStartOfCurrentWord;
        }
        //else do nothing

        if( pszCurrentCharacter - pszLineBegin > 0 ) 
        {
            vecLines.push_back( PdfString( pszLineBegin, pszCurrentCharacter - pszLineBegin) );
        }
        //else do nothing
    }

    return vecLines;
}

void PdfPainter::DrawTextAligned( double dX, double dY, double dWidth, const PdfString & rsText, EPdfAlignment eAlignment )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    if( !m_pFont || !m_pPage || !rsText.IsValid() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // Peter Petrov 25 Septemer 2008
    //m_pFont->EmbedFont();

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

void PdfPainter::DrawGlyph( PdfMemDocument* pDocument, double dX, double dY, const char* pszGlyphname)
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );
    
    if( !m_pFont || !m_pPage || !pszGlyphname )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

	PdfFont* pGlyphFont = NULL;
	int code = 32;

	for ( int num = 1; num <= 999; num++ )
	{
		// search for a copy of this font to enter difference-encoding, create a new one if not found
		char suffix[256];
		sprintf( suffix, "Glyph%i", num ); 
		pGlyphFont = pDocument->CreateDuplicateFontType1( m_pFont, suffix );

		PdfObject* pGlyphFontObj = pGlyphFont->GetObject();
		PdfObject* pEncoding = pGlyphFontObj->GetDictionary().GetKey( "Encoding" );

		// first time: create difference-encoding as reference, enter glyph
		if ( pEncoding == NULL  ||  pEncoding->IsReference() == false )
		{

			// get width of glyph to enter in difference-encoding
			int width = static_cast<int>(pGlyphFont->GetFontMetrics()->GetGlyphWidth( pszGlyphname ) );
			pEncoding = pDocument->GetObjects().CreateObject( "Encoding" );
	        
			code++;
	        
			PdfArray diffs;
			diffs.push_back( PdfVariant( static_cast<pdf_int64>( code ) ) );
			diffs.push_back( PdfName( pszGlyphname ) );
	        
			pEncoding->GetDictionary().AddKey( "Differences", diffs );
			pGlyphFontObj->GetDictionary().AddKey("Encoding", pEncoding->Reference() );

			// clear Widths-array and enter width of this glyph
			PdfObject* pWidthObj = pGlyphFontObj->MustGetIndirectKey( "Widths" );
			PdfArray & rWidthArr = pWidthObj->GetArray();
			for ( unsigned int i = 0; i < rWidthArr.size(); i++ )
			{
				rWidthArr[i] = PdfVariant( static_cast<pdf_int64>( 0 ) );
			}
			rWidthArr[code] = PdfVariant( static_cast<pdf_int64>( width ) );

			break;
		}


		// Existing font, search for glyph in existing difference-encoding
		{
			pEncoding = pDocument->GetObjects().GetObject( pEncoding->GetReference() );
	        
			PODOFO_ASSERT( pEncoding != NULL ); // paranoia

			PdfArray diffs;
            diffs = pEncoding->MustGetIndirectKey( "Differences" )->GetArray();

			bool foundIt = false;

			TCIVariantList it = diffs.begin();
			while( it != diffs.end() )
			{
				if( (*it).GetDataType() == ePdfDataType_Name )
				{
					code++;
					if ( (*it).GetName().GetName() == pszGlyphname )
					{
						foundIt = true;
						break;
					}
				}
					
				++it;
			}
			if ( foundIt )	// glyph fount, use it
				break;
		}

		// limit to codes <= 127, make new duplicate font if more
		if ( code+1 >= 127 )
		{
			code = 32;
			continue;
		}

		// add glyph to existing difference-encoding
		{
			// get width of glyph to enter in difference-encoding
			int width = static_cast<int>(pGlyphFont->GetFontMetrics()->GetGlyphWidth( pszGlyphname ) );

			code++;

			PdfArray diffs;
            diffs = pEncoding->MustGetIndirectKey( "Differences" )->GetArray();
			diffs.push_back( PdfName( pszGlyphname ) );

			pEncoding->GetDictionary().AddKey( "Differences", diffs );

			// enter width of glyph
            PdfObject* pWidthObj = pGlyphFontObj->MustGetIndirectKey( "Widths" );
			PdfArray & rWidthArr = pWidthObj->GetArray();
			rWidthArr[code] = PdfVariant( static_cast<pdf_int64>( width ) );

			break;
		}
	}

	// select identical sizes
	pGlyphFont->SetFontSize( m_pFont->GetFontSize() );
	pGlyphFont->SetFontCharSpace( m_pFont->GetFontCharSpace() );
	pGlyphFont->SetFontScale( m_pFont->GetFontScale() );

	PODOFO_ASSERT( code > 32  &&  code <= 127 );

	if( m_pFont->IsSubsetting() ) 
	{
		// mark glyph as used in basefont (needed for subsetting)
		m_pFont->AddUsedGlyphname( pszGlyphname );
	}

    // output
	SetFont( pGlyphFont );
	char temp[2];
	temp[0] = code;
	temp[1] = '\0';
    DrawText( dX, dY, PdfString( temp ) );
	SetFont( m_pFont );
}

void PdfPainter::DrawImage( double dX, double dY, PdfImage* pObject, double dScaleX, double dScaleY )
{
    this->DrawXObject( dX, dY, pObject, 
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

	std::streamsize oldPrecision = m_oss.precision(clPainterHighPrecision);
    m_oss.str("");
    m_oss << "q" << std::endl
          << dScaleX << " 0 0 "
          << dScaleY << " "
          << dX << " " 
          << dY << " cm" << std::endl
          << "/" << pObject->GetIdentifier().GetName() << " Do" << std::endl << "Q" << std::endl;
	m_oss.precision(oldPrecision);
    
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::ClosePath()
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

	 m_curPath << "h" << std::endl;

    m_pCanvas->Append( "h\n" );
}

void PdfPainter::LineTo( double dX, double dY )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );
    
	 m_curPath
			 << dX << " "
          << dY
          << " l" << std::endl;

    m_oss.str("");
    m_oss << dX << " "
          << dY
          << " l" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::MoveTo( double dX, double dY )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );
    
    m_curPath
        << dX << " "
        << dY
        << " m" << std::endl;

    m_oss.str("");
    m_oss << dX << " "
          << dY
          << " m" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::CubicBezierTo( double dX1, double dY1, double dX2, double dY2, double dX3, double dY3 )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

	 m_curPath
         << dX1 << " "
         << dY1 << " "
         << dX2 << " "
         << dY2 << " "
         << dX3 << " "
         << dY3 
         << " c" << std::endl;

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

void PdfPainter::HorizontalLineTo( double inX )
{
    LineTo( inX, lpy3 );
}

void PdfPainter::VerticalLineTo( double inY )
{
    LineTo( lpx3, inY );
}

void PdfPainter::SmoothCurveTo( double inX2, double inY2, double inX3, double inY3 )
{
    double
        px, py, px2 = inX2, 
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
    double px = inX, py = inY;
    double rx = inRadiusX, ry = inRadiusY, rot = inRotation;
    int    large = ( inLarge ? 1 : 0 ),
           sweep = ( inSweep ? 1 : 0 );

    double sin_th, cos_th;
    double a00, a01, a10, a11;
    double x0, y0, x1, y1, xc, yc;
    double d, sfactor, sfactor_sq;
    double th0, th1, th_arc;
    int i, n_segs;

    sin_th     = sin (rot * (PI / 180.0));
    cos_th     = cos (rot * (PI / 180.0));
    a00        = cos_th / rx;
    a01        = sin_th / rx;
    a10        = -sin_th / ry;
    a11        = cos_th / ry;
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
        double nsin_th = 0.0,
                ncos_th = 0.0;
        double na00 = 0.0, 
               na01 = 0.0, 
               na10 = 0.0, 
               na11 = 0.0;
        double nx1 = 0.0, 
               ny1 = 0.0, 
               nx2 = 0.0, 
               ny2 = 0.0, 
               nx3 = 0.0,
               ny3 = 0.0;
        double t   = 0.0;
        double th_half = 0.0;

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
        CubicBezierTo( nx1, 
                       ny1,
                       nx2, 
                       ny2, 
                       nx3, 
                       ny3 );
    }

    lpx = lpx2 = lpx3 = px; lpy = lpy2 = lpy3 = py;
    lcx = px;    lcy = py;    lrx = px;    lry = py;    // thanks Raph!
}

// Peter Petrov 5 January 2009 was delivered from libHaru
bool PdfPainter::Arc(double dX, double dY, double dRadius, double dAngle1, double dAngle2)
{
    bool cont_flg = false;

    bool ret = true;

    if (dAngle1 >= dAngle2 || (dAngle2 - dAngle1) >= 360.0f)
        return false;

    while (dAngle1 < 0.0f || dAngle2 < 0.0f) {
        dAngle1 = dAngle1 + 360.0f;
        dAngle2 = dAngle2 + 360.0f;
    }

    for (;;) {
        if (dAngle2 - dAngle1 <= 90.0f)
            return InternalArc (dX, dY, dRadius, dAngle1, dAngle2, cont_flg);
        else {
            double tmp_ang = dAngle1 + 90.0f;

            ret = InternalArc (dX, dY, dRadius, dAngle1, tmp_ang, cont_flg);
            if (!ret)
                return ret;

            dAngle1 = tmp_ang;
        }

        if (dAngle1 >= dAngle2)
            break;

        cont_flg = true;
    }

    return true;
}

bool PdfPainter::InternalArc(
              double    x,
              double    y,
              double    ray,
              double    ang1,
              double    ang2,
              bool      cont_flg)
{
    bool ret = true;

    double rx0, ry0, rx1, ry1, rx2, ry2, rx3, ry3;
    double x0, y0, x1, y1, x2, y2, x3, y3;
    double delta_angle = (90.0f - static_cast<double>(ang1 + ang2) / 2.0f) / 180.0f * PI;
    double new_angle = static_cast<double>(ang2 - ang1) / 2.0f / 180.0f * PI;

    rx0 = ray * cos (new_angle);
    ry0 = ray * sin (new_angle);
    rx2 = (ray * 4.0f - rx0) / 3.0f;
    ry2 = ((ray * 1.0f - rx0) * (rx0 - ray * 3.0f)) / (3.0 * ry0);
    rx1 = rx2;
    ry1 = -ry2;
    rx3 = rx0;
    ry3 = -ry0;

    x0 = rx0 * cos (delta_angle) - ry0 * sin (delta_angle) + x;
    y0 = rx0 * sin (delta_angle) + ry0 * cos (delta_angle) + y;
    x1 = rx1 * cos (delta_angle) - ry1 * sin (delta_angle) + x;
    y1 = rx1 * sin (delta_angle) + ry1 * cos (delta_angle) + y;
    x2 = rx2 * cos (delta_angle) - ry2 * sin (delta_angle) + x;
    y2 = rx2 * sin (delta_angle) + ry2 * cos (delta_angle) + y;
    x3 = rx3 * cos (delta_angle) - ry3 * sin (delta_angle) + x;
    y3 = rx3 * sin (delta_angle) + ry3 * cos (delta_angle) + y;

    if (!cont_flg) {
        MoveTo(x0,y0);
    }

    CubicBezierTo( x1, 
                   y1,
                   x2, 
                   y2, 
                   x3, 
                   y3 );

    //attr->cur_pos.x = (HPDF_REAL)x3;
    //attr->cur_pos.y = (HPDF_REAL)y3;
    lcx = x3;
    lcy = y3;

    lpx = lpx2 = lpx3 = x3; 
    lpy = lpy2 = lpy3 = y3;
    lcx = x3;   
    lcy = y3;    
    lrx = x3;    
    lry = y3;   

    return ret;
}

void PdfPainter::Close()
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    m_curPath << "h" << std::endl;

    m_pCanvas->Append( "h\n" );
}

void PdfPainter::Stroke()
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    m_curPath.str("");

    m_pCanvas->Append( "S\n" );
}

void PdfPainter::Fill(bool useEvenOddRule)
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    m_curPath.str("");

    if (useEvenOddRule)
        m_pCanvas->Append( "f*\n" );
    else
        m_pCanvas->Append( "f\n" );
}

void PdfPainter::FillAndStroke(bool useEvenOddRule)
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    m_curPath.str("");

    if (useEvenOddRule)
        m_pCanvas->Append( "B*\n" );
    else
        m_pCanvas->Append( "B\n" );
}

void PdfPainter::Clip( bool useEvenOddRule )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );
    
    if ( useEvenOddRule )
        m_pCanvas->Append( "W* n\n" );
    else
        m_pCanvas->Append( "W n\n" );
}

void PdfPainter::EndPath(void)
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

    m_curPath << "n" << std::endl;

    m_pCanvas->Append( "n\n" );
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
    if ( m_isCurColorICCDepend )
    {
        m_oss.str("");
        m_oss << "/" << m_CSTag     << " CS ";
        m_oss << m_curColor.GetRed()   << " "
              << m_curColor.GetGreen() << " "
              << m_curColor.GetBlue()
              << " SC" << std::endl;
        m_pCanvas->Append( m_oss.str() );
    }
    else
    {
        SetStrokingColor( m_curColor );
    }
}

void PdfPainter::SetTransformationMatrix( double a, double b, double c, double d, double e, double f )
{
    PODOFO_RAISE_LOGIC_IF( !m_pCanvas, "Call SetPage() first before doing drawing operations." );

	// Need more precision for transformation-matrix !!
	std::streamsize oldPrecision = m_oss.precision(clPainterHighPrecision);
    m_oss.str("");
    m_oss << a << " "
          << b << " "
          << c << " "
          << d << " "
          << e << " "
          << f << " cm" << std::endl;
	m_oss.precision(oldPrecision);

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

void PdfPainter::SetDependICCProfileColor( const PdfColor &rColor, const std::string &pCSTag )
{
    m_isCurColorICCDepend = true;
    m_curColor = rColor;
    m_CSTag = pCSTag;

    m_oss.str("");
    m_oss << "/" << m_CSTag << " cs ";
    m_oss << rColor.GetRed()   << " "
          << rColor.GetGreen() << " "
          << rColor.GetBlue()
          << " sc" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

#if defined(_MSC_VER)  &&  _MSC_VER <= 1200	// MSC 6.0 has a template-bug
PdfString PdfPainter::ExpandTabs_char( const char* pszText, long lStringLen, int nTabCnt, const char cTab, const char cSpace ) const
{
    long lLen    = lStringLen + nTabCnt*(m_nTabWidth-1) + sizeof(char);
    char*   pszTab  = static_cast<char*>(podofo_calloc( lLen, sizeof(char) ));

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
    podofo_free( pszTab );
    
    return str;
}

PdfString PdfPainter::ExpandTabs_pdf_utf16be( const pdf_utf16be* pszText, long lStringLen, int nTabCnt, const pdf_utf16be cTab, const pdf_utf16be cSpace ) const
{
    long lLen    = lStringLen + nTabCnt*(m_nTabWidth-1) + sizeof(pdf_utf16be);
    pdf_utf16be*   pszTab  = static_cast<pdf_utf16be*>(podofo_calloc( lLen, sizeof(pdf_utf16be) ));

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
    podofo_free( pszTab );
    
    return str;
}
#else
template<typename C>
PdfString PdfPainter::ExpandTabsPrivate( const C* pszText, pdf_long lStringLen, int nTabCnt, const C cTab, const C cSpace ) const
{
    pdf_long lLen    = lStringLen + nTabCnt*(m_nTabWidth-1) + sizeof(C);
    C*   pszTab  = static_cast<C*>(podofo_calloc( lLen, sizeof(C) ));

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
    podofo_free( pszTab );
    
    return str;
}
#endif

PdfString PdfPainter::ExpandTabs( const PdfString & rsString, pdf_long lStringLen ) const
{
    int               nTabCnt  = 0;
    int               i;
    bool              bUnicode = rsString.IsUnicode();
    const pdf_utf16be cTab     = 0x0900;
    const pdf_utf16be cSpace   = 0x2000;

    if( lStringLen == -1 )
        lStringLen = rsString.GetCharacterLength();

    if (lStringLen > rsString.GetCharacterLength())
    {
        PdfError::DebugMessage( "Requested to expand tabs in string of %" PDF_FORMAT_INT64 " chars, while it has only %" PDF_FORMAT_INT64 "; correcting the value\n",
            static_cast<pdf_int64>( lStringLen ), static_cast<pdf_int64>( rsString.GetCharacterLength() ) );

        lStringLen = rsString.GetCharacterLength();
    }

    // count the number of tabs in the string
    if( bUnicode ) 
    {
        for( i=0;i<lStringLen;i++ )
            if( rsString.GetUnicode()[i] == cTab ) 
                ++nTabCnt;
    }
    else
    {
        for( i=0;i<lStringLen;i++ )
            if( rsString.GetString()[i] == '\t' )
                ++nTabCnt;
    }

    // if no tabs are found: bail out!
    if( !nTabCnt )
        return rsString;
    
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200	// MSC 6.0 has a template-bug
    if( rsString.IsUnicode() )
        return ExpandTabs_pdf_utf16be( rsString.GetUnicode(), lStringLen, nTabCnt, cTab, cSpace );
    else
        return ExpandTabs_char( rsString.GetString(), lStringLen, nTabCnt, '\t', ' ' );
#else
    if( rsString.IsUnicode() )
        return ExpandTabsPrivate<pdf_utf16be>( rsString.GetUnicode(), lStringLen, nTabCnt, cTab, cSpace );
    else
        return ExpandTabsPrivate<char>( rsString.GetString(), lStringLen, nTabCnt, '\t', ' ' );
#endif
}

} /* namespace PoDoFo */


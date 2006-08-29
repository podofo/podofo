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

#include "PdfCanvas.h"
#include "PdfDictionary.h"
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

namespace PoDoFo {

static inline void CheckDoubleRange( double val, double min, double max )
{
    if( val < min || val > max )
    {
        RAISE_ERROR( ePdfError_ValueOutOfRange );
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
}

void PdfPainter::SetPage( PdfCanvas* pPage )
{
    m_pPage   = pPage;

    m_pCanvas = pPage ? pPage->Contents()->Stream() : NULL;
    if ( m_pCanvas ) 
    {
        if ( m_pCanvas->Length() ) 
        {	
            // there is already content here - so let's assume we are appending
            // as such, we MUST put in a "space" to separate whatever we do.
            m_pCanvas->Append( " " );
        }
    } 
    else 
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }
}

void PdfPainter::SetStrokingGray( double g )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    CheckDoubleRange( g, 0.0, 1.0 );

    m_oss.str("");
    m_oss << g << " G" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetGray( double g )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
    CheckDoubleRange( g, 0.0, 1.0 );
    
    m_oss.str("");
    m_oss << g << " g" << std::endl;
    m_pCanvas->Append( m_oss.str() );
        
    m_curColor1      = g;
    m_eCurColorSpace = ePdfColorSpace_DeviceGray;
}

void PdfPainter::SetStrokingColor( double r, double g, double b )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

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
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
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
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

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
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
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
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_oss.str("");
    m_oss << dWidth << " w" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetStrokeStyle( EPdfStrokeStyle eStyle, const char* pszCustom )
{
    const char* pszCurStroke = NULL;

    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

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
            RAISE_ERROR( ePdfError_InvalidStrokeStyle );
        }
    }

    if( !pszCurStroke )
    {
        RAISE_ERROR( ePdfError_InvalidStrokeStyle );
    }
    
    m_oss.str("");
    m_oss << pszCurStroke << " d" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetLineCapStyle( EPdfLineCapStyle eCapStyle )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_oss.str("");
    m_oss << (int)eCapStyle << " J" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetLineJoinStyle( EPdfLineJoinStyle eJoinStyle )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_oss.str("");
    m_oss << (int)eJoinStyle << "j" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::SetFont( PdfFont* pFont )
{
    if( !m_pCanvas || !pFont )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pFont = pFont;
}

void PdfPainter::DrawLine( double dStartX, double dStartY, double dEndX, double dEndY )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_oss.str("");
    m_oss << dStartX << " "
          << dStartY
          << " m "
          << dEndX << " "
          << dEndY		
          << " l S" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::DrawRect( double dX, double dY, double dWidth, double dHeight )
{ 
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
    m_oss.str("");
    m_oss << dX << " "
          << dY << " "
          << dWidth << " "
          << -dHeight		
          << " re S" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::FillRect( double dX, double dY, double dWidth, double dHeight )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_oss.str("");
    m_oss << dX << " "
          << dY << " "
          << dWidth << " "
          << -dHeight		
          << " re f" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::DrawEllipse( double dX, double dY, double dWidth, double dHeight )
{
    double dPointX[BEZIER_POINTS];
    double dPointY[BEZIER_POINTS];
    int    i;

    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

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

    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

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

void PdfPainter::DrawText( double dX, double dY, const PdfString & sText )
{
    if( !m_pFont || !m_pCanvas || !m_pPage || !sText.IsValid() )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    this->DrawText( dX, dY, sText, sText.Length() );
}

void PdfPainter::DrawText( double dX, double dY, const PdfString & sText, long lStringLen )
{
    int         nTabCnt = 0;
    int         i,z;
    char*       pszTab;
    char*       pBuffer;
    const char* pszText;
    long        lLen;

    if( !m_pFont || !m_pCanvas || !m_pPage || !sText.IsValid() )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // replace anytabs in pszText by enough spaces
    if( !sText.IsHex() )
        for( i=0;i<=lStringLen;i++ )
            if( sText.String()[i] == '\t' )
                ++nTabCnt;

    if( nTabCnt )
    {
        pszText = sText.String();
        lLen    = lStringLen + nTabCnt*(m_nTabWidth-1) + 1;

        pszTab = (char*)malloc( sizeof( char ) * lLen );
        if( !pszTab )
        {
            RAISE_ERROR( ePdfError_OutOfMemory );
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
        pszTab = const_cast<char*>(sText.String());

    this->AddToPageResources( m_pFont->Identifier(), m_pFont->Object()->Reference(), PdfName("Font") );

    if( m_pFont->IsUnderlined() )
    {
        this->Save();
        this->SetCurrentStrokingColor();
        this->SetStrokeWidth( m_pFont->FontMetrics()->UnderlineThickness() );
        this->DrawLine( dX, 
                        dY + m_pFont->FontMetrics()->UnderlinePosition(), 
                        dX + m_pFont->FontMetrics()->StringWidth( pszTab ),
                        dY + m_pFont->FontMetrics()->UnderlinePosition() );
        this->Restore();
    }

    if( !sText.IsHex() )
    {
        const PdfFilter* pFilter = PdfFilterFactory::Create( ePdfFilter_ASCIIHexDecode );
        pFilter->Encode( pszTab, lStringLen, &pBuffer, &lLen );
    }

    m_oss.str("");
    m_oss << "BT" << std::endl << "/" << m_pFont->Identifier().Name().c_str()
          << " "  << m_pFont->FontSize()
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
        m_pCanvas->Append( sText.String(), sText.Length() );

    m_pCanvas->Append( ">Tj\nET\n" );

    if( nTabCnt )
        free( pszTab );
}

void PdfPainter::DrawXObject( double dX, double dY, PdfXObject* pObject, double dScaleX, double dScaleY )
{
    if( !pObject )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    this->AddToPageResources( pObject->Identifier(), pObject->Object()->Reference(), "XObject" );

    m_oss.str("");
    m_oss << "q" << std::endl
          << pObject->PageSize().Width() * dScaleX << " 0 0 "
          << pObject->PageSize().Height() * dScaleY << " "
          << dX << " " 
          << dY << " cm" << std::endl
          << "/" << pObject->Identifier().Name().c_str() << " Do" << std::endl << "Q" << std::endl;
    
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::ClosePath()
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pCanvas->Append( "h\n" );
}

void PdfPainter::LineTo( double dX, double dY )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
    m_oss.str("");
    m_oss << dX << " "
          << dY
          << " l" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::MoveTo( double dX, double dY )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
    m_oss.str("");
    m_oss << dX << " "
          << dY
          << " m" << std::endl;
    m_pCanvas->Append( m_oss.str() );
}

void PdfPainter::Stroke()
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
    m_pCanvas->Append( "S\n" );
}

void PdfPainter::Fill()
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
    m_pCanvas->Append( "f\n" );
}

void PdfPainter::Save()
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pCanvas->Append( "q\n" );
}

void PdfPainter::Restore()
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pCanvas->Append( "q\n" );
}

void PdfPainter::AddToPageResources( const PdfName & rIdentifier, const PdfReference & rRef, const PdfName & rName )
{
    if( !m_pPage || !rName.Length() || !rIdentifier.Length() )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    PdfObject* pResource = m_pPage->Resources();
    
    if( !pResource )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
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
        default:
        {
            RAISE_ERROR_INFO( ePdfError_InvalidDataType, "The color space for the current text drawing operation is invalid. Please set a correct color."  ); 
        }
        break;
    }
}

void PdfPainter::SetTransformationMatrix( double a, double b, double c, double d, double e, double f )
{
    m_oss.str("");
    m_oss << a << " "
          << b << " "
          << c << " "
          << d << " "
          << e << " "
          << f << " cm" << std::endl;

    m_pCanvas->Append( m_oss.str() );
}

};


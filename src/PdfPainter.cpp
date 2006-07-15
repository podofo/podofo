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

#include "PdfPainter.h"

#include "PdfCanvas.h"
#include "PdfDictionary.h"
#include "PdfFilter.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfImage.h"
#include "PdfName.h"
#include "PdfStream.h"
#include "PdfString.h"

#define BEZIER_POINTS 13

namespace PoDoFo {

PdfPainter::PdfPainter()
{
    m_pCanvas = NULL;
    m_pFont   = NULL;

    m_curColor1 = 
        m_curColor2 = 
        m_curColor3 = 
        m_curColor4 = 0.0;

    m_eCurColorSpace = ePdfColorSpace_DeviceRGB;
}

PdfPainter::~PdfPainter()
{
}

void PdfPainter::SetPage( PdfCanvas* pPage )
{
    m_pCanvas = pPage ? pPage->Contents()->Stream() : NULL;
    m_pPage   = pPage;
}

void PdfPainter::SetStrokingGray( double g )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( g < 0.0 || g > 1.0 )
    {
        RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "%f G\n", g );
    m_pCanvas->Append( m_szBuffer );
}

void PdfPainter::SetGray( double g )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( g < 0.0 || g > 1.0 )
    {
        RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "%f G\n", g );
    m_pCanvas->Append( m_szBuffer );

    m_curColor1      = g;
    m_eCurColorSpace = ePdfColorSpace_DeviceGray;
}

void PdfPainter::SetStrokingColor( double r, double g, double b )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( r < 0.0 || r > 1.0 )
    {
        RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    if( g < 0.0 || g > 1.0 )
    {
        RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    if( b < 0.0 || b > 1.0 )
    {
        RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "%f %f %f RG\n", r, g, b );
    m_pCanvas->Append( m_szBuffer );
}

void PdfPainter::SetColor( double r, double g, double b )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( r < 0.0 || r > 1.0 )
    {
        RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    if( g < 0.0 || g > 1.0 )
    {
        RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    if( b < 0.0 || b > 1.0 )
    {
        RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "%f %f %f rg\n", r, g, b );
    m_pCanvas->Append( m_szBuffer );

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

    if( c < 0.0 || c > 1.0 )
    {
        RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    if( m < 0.0 || m > 1.0 )
    {
        RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    if( y < 0.0 || y > 1.0 )
    {
        RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    if( k < 0.0 || k > 1.0 )
    {
        RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "%f %f %f %f K\n", c, m, y, k );
    m_pCanvas->Append( m_szBuffer );
}

void PdfPainter::SetColorCMYK( double c, double m, double y, double k )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( c < 0.0 || c > 1.0 )
    {
        RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    if( m < 0.0 || m > 1.0 )
    {
        RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    if( y < 0.0 || y > 1.0 )
    {
        RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    if( k < 0.0 || k > 1.0 )
    {
        RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "%f %f %f %f k\n", c, m, y, k );
    m_pCanvas->Append( m_szBuffer );

    m_curColor1      = c;
    m_curColor2      = m;
    m_curColor3      = y;
    m_curColor4      = k;
    m_eCurColorSpace = ePdfColorSpace_DeviceCMYK;
}

void PdfPainter::SetStrokeWidth( long lWidth )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "%.3f w\n", 
              (double)lWidth * CONVERSION_CONSTANT );
    m_pCanvas->Append( m_szBuffer );
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
    
    snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "%s d\n", pszCurStroke );
    m_pCanvas->Append( m_szBuffer );
}

void PdfPainter::SetLineCapStyle( EPdfLineCapStyle eCapStyle )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "%i J\n", (int)eCapStyle );
    m_pCanvas->Append( m_szBuffer );
}

void PdfPainter::SetLineJoinStyle( EPdfLineJoinStyle eJoinStyle )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "%i j\n", (int)eJoinStyle );
    m_pCanvas->Append( m_szBuffer );
}

void PdfPainter::SetFont( PdfFont* pFont )
{
    if( !m_pCanvas || !pFont )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pFont = pFont;
}

void PdfPainter::DrawLine( long lStartX, long lStartY, long lEndX, long lEndY )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "%.3f %.3f m %.3f %.3f l S\n", 
              (double)lStartX * CONVERSION_CONSTANT,
              (double)(m_pPage->PageSize().lHeight - lStartY) * CONVERSION_CONSTANT,
              (double)lEndX * CONVERSION_CONSTANT,
              (double)(m_pPage->PageSize().lHeight - lEndY) * CONVERSION_CONSTANT );
    m_pCanvas->Append( m_szBuffer );
}

void PdfPainter::DrawRect( long lX, long lY, long lWidth, long lHeight )
{ 
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
    lHeight *= -1;

    snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "%.3f %.3f %.3f %.3f re S\n", 
              (double)lX * CONVERSION_CONSTANT,
              (double)(m_pPage->PageSize().lHeight - lY) * CONVERSION_CONSTANT,
              (double)lWidth * CONVERSION_CONSTANT,
              (double)lHeight * CONVERSION_CONSTANT );
    m_pCanvas->Append( m_szBuffer );
}

void PdfPainter::FillRect( long lX, long lY, long lWidth, long lHeight )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    lHeight *= -1;
   
    snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "%.3f %.3f %.3f %.3f re f\n", 
              (double)lX * CONVERSION_CONSTANT,
              (double)(m_pPage->PageSize().lHeight - lY) * CONVERSION_CONSTANT,
              (double)lWidth * CONVERSION_CONSTANT,
             (double)lHeight * CONVERSION_CONSTANT );
    m_pCanvas->Append( m_szBuffer );
}

void PdfPainter::DrawEllipse( long lX, long lY, long lWidth, long lHeight )
{
    long lPointX[BEZIER_POINTS];
    long lPointY[BEZIER_POINTS];
    int  i;

    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    lHeight *= -1;
    lY       = (m_pPage->PageSize().lHeight - lY);

    ConvertRectToBezier( lX, lY, lWidth, lHeight, lPointX, lPointY );

    snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "%.3f %.3f m\n",
              (double)lPointX[0] * CONVERSION_CONSTANT,
              (double)lPointY[0] * CONVERSION_CONSTANT 
        );
    m_pCanvas->Append( m_szBuffer );              

    for( i=1;i<BEZIER_POINTS; i+=3 )
    {
        snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "%.3f %.3f %.3f %.3f %.3f %.3f c\n", 
                  (double)lPointX[i] * CONVERSION_CONSTANT,
                  (double)lPointY[i] * CONVERSION_CONSTANT,
                  (double)lPointX[i+1] * CONVERSION_CONSTANT,
                  (double)lPointY[i+1] * CONVERSION_CONSTANT,
                  (double)lPointX[i+2] * CONVERSION_CONSTANT,
                  (double)lPointY[i+2] * CONVERSION_CONSTANT
                  );
        m_pCanvas->Append( m_szBuffer );
    }
    m_pCanvas->Append( "s\n" );
}

void PdfPainter::FillEllipse( long lX, long lY, long lWidth, long lHeight )
{
    long lPointX[BEZIER_POINTS];
    long lPointY[BEZIER_POINTS];
    int  i;

    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    lHeight *= -1;
    lY       = (m_pPage->PageSize().lHeight - lY);

    ConvertRectToBezier( lX, lY, lWidth, lHeight, lPointX, lPointY );

    snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "%.3f %.3f m\n",
              (double)lPointX[0] * CONVERSION_CONSTANT,
              (double)lPointY[0] * CONVERSION_CONSTANT 
        );
    m_pCanvas->Append( m_szBuffer );              

    for( i=1;i<BEZIER_POINTS; i+=3 )
    {
        snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "%.3f %.3f %.3f %.3f %.3f %.3f c\n", 
                  (double)lPointX[i] * CONVERSION_CONSTANT,
                  (double)lPointY[i] * CONVERSION_CONSTANT,
                  (double)lPointX[i+1] * CONVERSION_CONSTANT,
                  (double)lPointY[i+1] * CONVERSION_CONSTANT,
                  (double)lPointX[i+2] * CONVERSION_CONSTANT,
                  (double)lPointY[i+2] * CONVERSION_CONSTANT
                  );
        m_pCanvas->Append( m_szBuffer );
    }
    m_pCanvas->Append( "f\n" );
}

void PdfPainter::DrawText( long lX, long lY, const PdfString & sText )
{
    if( !m_pFont || !m_pCanvas || !m_pPage || !sText.IsValid() )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    this->DrawText( lX, lY, sText, sText.Length() );
}

void PdfPainter::DrawText( long lX, long lY, const PdfString & sText, long lStringLen )
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

    this->AddToPageResources( m_pFont->Identifier(), m_pFont->Reference(), PdfName("Font") );

    if( m_pFont->IsUnderlined() )
    {
        this->Save();
        this->SetCurrentStrokingColor();
        this->SetStrokeWidth( m_pFont->FontMetrics()->UnderlineThickness() );
        this->DrawLine( lX, 
                        lY - m_pFont->FontMetrics()->UnderlinePosition(), 
                        lX + m_pFont->FontMetrics()->StringWidth( pszTab ),
                        lY - m_pFont->FontMetrics()->UnderlinePosition() );
        this->Restore();
    }

    if( !sText.IsHex() )
    {
        const PdfFilter* pFilter = PdfFilterFactory::Create( ePdfFilter_ASCIIHexDecode );
        pFilter->Encode( pszTab, lStringLen, &pBuffer, &lLen );
    }

    snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "BT\n/%s %.3f Tf\n%.3f %.3f Td\n<", 
              m_pFont->Identifier().Name().c_str(), m_pFont->FontSize(),
              (double)lX * CONVERSION_CONSTANT,
              (double)(m_pPage->PageSize().lHeight - lY) * CONVERSION_CONSTANT );

    m_pCanvas->Append( m_szBuffer );

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

void PdfPainter::DrawXObject( long lX, long lY, PdfImageRef* pImageRef, double dScaleX, double dScaleY )
{
    if( !pImageRef )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    this->AddToPageResources( pImageRef->Identifier(), pImageRef->Reference(), "XObject" );

    snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "q\n%.3f 0 0 %.3f %.3f %.3f cm\n/%s Do\nQ\n",
              (double)pImageRef->Width() * dScaleX,
              (double)pImageRef->Height() * dScaleY,
              (double)lX * CONVERSION_CONSTANT,
              (double)(m_pPage->PageSize().lHeight - lY) * CONVERSION_CONSTANT,
              pImageRef->Identifier().Name().c_str() );
    m_pCanvas->Append( m_szBuffer );
}

void PdfPainter::ClosePath()
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pCanvas->Append( "h\n" );
}

void PdfPainter::LineTo( long  lX, long lY )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
    snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "%.3f %.3f l\n", 
              (double)lX * CONVERSION_CONSTANT,
              (double)(m_pPage->PageSize().lHeight - lY) * CONVERSION_CONSTANT );
    m_pCanvas->Append( m_szBuffer );
}

void PdfPainter::MoveTo( long  lX, long lY )
{
    if( !m_pCanvas )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
    snprintf( m_szBuffer, PDF_PAINTER_BUFFER, "%.3f %.3f m\n", 
              (double)lX * CONVERSION_CONSTANT,
              (double)(m_pPage->PageSize().lHeight - lY) * CONVERSION_CONSTANT );
    m_pCanvas->Append( m_szBuffer );
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

void PdfPainter::ConvertRectToBezier( long lX, long lY, long lWidth, long lHeight, long plPointX[], long plPointY[] )
{
    // this function is based on code from:
    // http://www.codeguru.com/Cpp/G-M/gdi/article.php/c131/
    // (Llew Goodstadt)

    // MAGICAL CONSTANT to map ellipse to beziers
    //                          2/3*(sqrt(2)-1) 
    const double dConvert =     0.2761423749154;

    long lOffX    = (long)(lWidth  * dConvert);
    long lOffY    = (long)(lHeight * dConvert);
    long lCenterX = lX + (lWidth  >> 1); 
    long lCenterY = lY + (lHeight >> 1); 

    plPointX[0]  =                            //------------------------//
    plPointX[1]  =                            //                        //
    plPointX[11] =                            //        2___3___4       //
    plPointX[12] = lX;                        //     1             5    //
    plPointX[5]  =                            //     |             |    //
    plPointX[6]  =                            //     |             |    //
    plPointX[7]  = lX + lWidth;               //     0,12          6    //
    plPointX[2]  =                            //     |             |    //
    plPointX[10] = lCenterX - lOffX;          //     |             |    //
    plPointX[4]  =                            //    11             7    //
    plPointX[8]  = lCenterX + lOffX;          //       10___9___8       //
    plPointX[3]  =                            //                        //
    plPointX[9]  = lCenterX;                  //------------------------//

    plPointY[2]  =
    plPointY[3]  =
    plPointY[4]  = lY;
    plPointY[8]  =
    plPointY[9]  =
    plPointY[10] = lY + lHeight;
    plPointY[7]  =
    plPointY[11] = lCenterY + lOffY;
    plPointY[1]  =
    plPointY[5]  = lCenterY - lOffY;
    plPointY[0]  =
    plPointY[12] =
    plPointY[6]  = lCenterY;
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

};


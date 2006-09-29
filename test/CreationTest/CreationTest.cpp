/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "PdfAction.h"
#include "PdfAnnotation.h"
#include "PdfDestination.h"
#include "PdfDocument.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfImage.h"
#include "PdfOutlines.h"
#include "PdfPage.h"
#include "PdfPainter.h"
#include "PdfPainterMM.h"
#include "PdfRect.h"
#include "PdfString.h"
#include "PdfXObject.h"

#include "../PdfTest.h"

using namespace PoDoFo;

#define CONVERSION_CONSTANT 0.002834645669291339


void LineTest( PdfPainter* pPainter, PdfPage* pPage, PdfDocument* pDocument )
{
    double     x     = 10000 * CONVERSION_CONSTANT;
    double     y     = pPage->GetPageSize().GetHeight() - 10000 * CONVERSION_CONSTANT;
    PdfFont* pFont;

    const double dLineLength = 50000 * CONVERSION_CONSTANT; // 5cm
    double h;
    double w;

    pFont = pDocument->CreateFont( "Arial" );
    if( !pFont )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    pFont->SetFontSize( 16.0 );


    h = pFont->GetFontMetrics()->GetLineSpacing();
    w = pFont->GetFontMetrics()->StringWidth( "Grayscale Colorspace" );

    pPainter->SetFont( pFont );
    pPainter->DrawText( 120000 * CONVERSION_CONSTANT, y - pFont->GetFontMetrics()->GetLineSpacing(), "Grayscale Colorspace" );
    pPainter->DrawRect( 120000 * CONVERSION_CONSTANT , y, w, h );

    // Draw 10 lines in gray scale
    for( int i = 0; i < 10; i++ )
    {
        x += (10000 * CONVERSION_CONSTANT);

        pPainter->SetStrokeWidth( (i*1000) * CONVERSION_CONSTANT );
        pPainter->SetStrokingGray( (double)i/10.0 );
        pPainter->DrawLine( x, y, x, y - dLineLength );
    }

    x = 10000 * CONVERSION_CONSTANT;
    y -= dLineLength;
    y -= (10000 * CONVERSION_CONSTANT);

    pPainter->DrawText( 120000 * CONVERSION_CONSTANT, y - pFont->GetFontMetrics()->GetLineSpacing(), "RGB Colorspace" );

    // Draw 10 lines in rgb
    for( int i = 0; i < 10; i++ )
    {
        x += (10000 * CONVERSION_CONSTANT);

        pPainter->SetStrokeWidth( (i*1000) * CONVERSION_CONSTANT );
        pPainter->SetStrokingColor( (double)i/10.0, 0.0, (double)(10-i)/10.0 );
        pPainter->DrawLine( x, y, x, y - dLineLength );
    }

    x = 10000 * CONVERSION_CONSTANT;
    y -= dLineLength;
    y -= (10000 * CONVERSION_CONSTANT);

    pPainter->DrawText( 120000 * CONVERSION_CONSTANT, y - pFont->GetFontMetrics()->GetLineSpacing(), "CMYK Colorspace" );

    // Draw 10 lines in cmyk
    for( int i = 0; i < 10; i++ )
    {
        x += (10000 * CONVERSION_CONSTANT);

        pPainter->SetStrokeWidth( (i*1000) * CONVERSION_CONSTANT );
        pPainter->SetStrokingColorCMYK( (double)i/10.0, 0.0, (double)(10-i)/10.0, 0.0 );
        pPainter->DrawLine( x, y, x, y - dLineLength );
    }

    x = 20000 * CONVERSION_CONSTANT;
    y -= 60000 * CONVERSION_CONSTANT;

    pPainter->SetStrokeWidth( 1000 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 0.0, 0.0, 0.0 );

    pPainter->SetStrokeStyle( ePdfStrokeStyle_Solid );
    pPainter->DrawLine( x, y, x + (100000 * CONVERSION_CONSTANT), y );
    y -= (10000 * CONVERSION_CONSTANT);

    pPainter->SetStrokeStyle( ePdfStrokeStyle_Dash );
    pPainter->DrawLine( x, y, x + (100000 * CONVERSION_CONSTANT), y );
    y -= (10000 * CONVERSION_CONSTANT);

    pPainter->SetStrokeStyle( ePdfStrokeStyle_Dot );
    pPainter->DrawLine( x, y, x + (100000 * CONVERSION_CONSTANT), y );
    y -= (10000 * CONVERSION_CONSTANT);

    pPainter->SetStrokeStyle( ePdfStrokeStyle_DashDot );
    pPainter->DrawLine( x, y, x + (100000 * CONVERSION_CONSTANT), y );
    y -= (10000 * CONVERSION_CONSTANT);

    pPainter->SetStrokeStyle( ePdfStrokeStyle_DashDotDot );
    pPainter->DrawLine( x, y, x + (100000 * CONVERSION_CONSTANT), y );
    y -= (10000 * CONVERSION_CONSTANT);

    pPainter->SetStrokeStyle( ePdfStrokeStyle_Custom, "[7 9 2] 4" );
    pPainter->DrawLine( x, y, x + (100000 * CONVERSION_CONSTANT), y );
    y -= (10000 * CONVERSION_CONSTANT);
}

void RectTest( PdfPainter* pPainter, PdfPage* pPage, PdfDocument* pDocument )
{
    double     x     = 10000 * CONVERSION_CONSTANT;
    double     y     = pPage->GetPageSize().GetHeight() - 10000 * CONVERSION_CONSTANT;
    PdfFont* pFont;

    const double dWidth  = 50000 * CONVERSION_CONSTANT; // 5cm
    const double dHeight = 30000 * CONVERSION_CONSTANT; // 3cm

    pFont = pDocument->CreateFont( "Arial" );
    if( !pFont )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    pFont->SetFontSize( 16.0 );
    pPainter->SetFont( pFont );

    pPainter->DrawText( 125000 * CONVERSION_CONSTANT, y - pFont->GetFontMetrics()->GetLineSpacing(), "Rectangles" );

    pPainter->SetStrokeWidth( 100 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 0.0, 0.0, 0.0 );
    pPainter->DrawRect( x, y, dWidth, dHeight );

    x += dWidth;
    x += 10000 * CONVERSION_CONSTANT;

    pPainter->SetStrokeWidth( 1000 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 0.0, 0.0, 0.0 );
    pPainter->DrawRect( x, y, dWidth, dHeight );

    y -= dHeight;
    y -= 10000 * CONVERSION_CONSTANT;
    x = 10000 * CONVERSION_CONSTANT;

    pPainter->SetStrokeWidth( 100 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 1.0, 0.0, 0.0 );
    pPainter->DrawRect( x, y, dWidth, dHeight );

    x += dWidth;
    x += 10000 * CONVERSION_CONSTANT;
    pPainter->SetStrokeWidth( 1000 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 0.0, 1.0, 0.0 );
    pPainter->DrawRect( x, y, dWidth, dHeight );

    y -= dHeight;
    y -= 10000 * CONVERSION_CONSTANT;
    x = 10000 * CONVERSION_CONSTANT;

    pPainter->SetStrokeWidth( 100 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 0.0, 0.0, 0.0 );
    pPainter->SetColor( 1.0, 0.0, 0.0 );
    pPainter->FillRect( x, y, dWidth, dHeight );
    pPainter->DrawRect( x, y, dWidth, dHeight );

    x += dWidth;
    x += 10000 * CONVERSION_CONSTANT;
    pPainter->SetStrokeWidth( 100 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 0.0, 1.0, 0.0 );
    pPainter->SetColor( 0.0, 0.0, 1.0 );
    pPainter->FillRect( x, y, dWidth, dHeight );
    pPainter->DrawRect( x, y, dWidth, dHeight );

    y -= dHeight;
    y -= 10000 * CONVERSION_CONSTANT;
    x = (10000 * CONVERSION_CONSTANT) + dWidth;

    pPainter->DrawText( 120000 * CONVERSION_CONSTANT, y - pFont->GetFontMetrics()->GetLineSpacing(), "Triangles" );

    // Draw a triangle at the current position
    pPainter->SetColor( 0.0, 1.0, 1.0 );
    pPainter->MoveTo( x, y );
    pPainter->LineTo( x+dWidth, y-dHeight );
    pPainter->LineTo( x-dWidth, y-dHeight );
    pPainter->ClosePath();
    pPainter->Fill();

    y -= dHeight;
    y -= 10000 * CONVERSION_CONSTANT;
    x = (10000 * CONVERSION_CONSTANT) + dWidth;

    pPainter->SetStrokingColor( 0.0, 0.0, 0.0 );
    pPainter->MoveTo( x, y );
    pPainter->LineTo( x+dWidth, y-dHeight );
    pPainter->LineTo( x-dWidth, y-dHeight );
    pPainter->ClosePath();
    pPainter->Stroke();
}

void TextTest( PdfPainter* pPainter, PdfPage* pPage, PdfDocument* pDocument )
{
    double x = 10000 * CONVERSION_CONSTANT;
    double y = pPage->GetPageSize().GetHeight() - 10000 * CONVERSION_CONSTANT;

    pPainter->SetFont( pDocument->CreateFont( "Times New Roman" ) );
    pPainter->GetFont()->SetFontSize( 24.0 );
    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();

    pPainter->SetColor( 0.0, 0.0, 0.0 );
    pPainter->DrawText( x, y, "Hallo Welt!" );
    
    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();
    pPainter->GetFont()->SetUnderlined( true );
    pPainter->SetStrokingColor( 1.0, 0.0, 0.0 );
    pPainter->DrawText( x, y, "Underlined text in the same font!" );

    pPainter->GetFont()->SetUnderlined( false );
    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();
    pPainter->DrawText( x, y, "Disabled the underline again..." );
    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();
    
    PdfFont* pFont = pDocument->CreateFont( "Arial" );
    pFont->SetFontSize( 12.0 );

    pPainter->SetFont( pFont );
    pPainter->DrawText( x, y, "PoDoFo rocks!" );
}

void ImageTest( PdfPainter* pPainter, PdfPage* pPage, PdfDocument* pDocument )
{
    double        y      = pPage->GetPageSize().GetHeight() - 60000 * CONVERSION_CONSTANT;

    PdfImage image( &(pDocument->GetObjects()) );

    PdfRect        rect( 0, 0, 50000 * CONVERSION_CONSTANT, 50000 * CONVERSION_CONSTANT );
    PdfRect        rect1( 80000 * CONVERSION_CONSTANT, 180000 * CONVERSION_CONSTANT, 20000 * CONVERSION_CONSTANT, 20000 * CONVERSION_CONSTANT );
    PdfRect        rect2( 40000 * CONVERSION_CONSTANT, y, 50000 * CONVERSION_CONSTANT, 50000 * CONVERSION_CONSTANT );
    PdfXObject     xObj( rect, &(pDocument->GetObjects()) );
    PdfPainter     pnt;    // XObject painter

    image.LoadFromFile( "./lena.jpg" );

    pnt.SetPage( &xObj );
    // Draw onto the XObject
    pnt.SetFont( pDocument->CreateFont( "Comic Sans MS" ) );

    pnt.GetFont()->SetFontSize( 8.0 );
    pnt.SetStrokingColor( 1.0, 1.0, 1.0 );
    pnt.SetColor( 1.0, 0.0, 0.0 );
    pnt.FillRect( 0, xObj.GetPageSize().GetHeight(), xObj.GetPageSize().GetWidth(), xObj.GetPageSize().GetHeight()  );
    pnt.SetColor( 0.0, 0.0, 0.0 );
    pnt.DrawRect( 0, 1000 * CONVERSION_CONSTANT, 1000 * CONVERSION_CONSTANT, 1000 * CONVERSION_CONSTANT );
    pnt.DrawText( 0, 1000 * CONVERSION_CONSTANT, "I am a XObject." );

    printf("Drawing on the page!\n");
    // Draw onto the page 

    pPainter->DrawImage( 40000 * CONVERSION_CONSTANT, y, &image, 0.3, 0.3 );
    pPainter->DrawImage( 40000 * CONVERSION_CONSTANT, y - (100000 * CONVERSION_CONSTANT), &image, 0.2, 0.5 );
    pPainter->DrawImage( 40000 * CONVERSION_CONSTANT, y - (200000 * CONVERSION_CONSTANT), &image, 0.3, 0.3 );

    pPainter->DrawXObject( 120000 * CONVERSION_CONSTANT, y - (15000 * CONVERSION_CONSTANT), &xObj, 0.01, 0.01 );

    PdfAnnotation annot1( pPage, ePdfAnnotation_Widget, rect1, &(pDocument->GetObjects()) );
    PdfAnnotation annot2( pPage, ePdfAnnotation_Link, rect2, &(pDocument->GetObjects()) );

    annot1.SetTitle( PdfString("Author: Dominik Seichter") );
    annot1.SetContents( PdfString("Hallo Welt!") );
    annot1.SetAppearanceStream( &xObj );

    PdfAction action( ePdfAction_URI, &(pDocument->GetObjects()) );
    action.SetURI( PdfString("http://podofo.sf.net") );

    //pAnnot2->SetDestination( pPage );
    annot2.SetDestination( &action );
    annot2.SetFlags( ePdfAnnotationFlags_NoZoom );
}

void EllipseTest( PdfPainter* pPainter, PdfPage* pPage, PdfDocument* pDocument )
{
    double        dX     = 10000 * CONVERSION_CONSTANT;
    double        dY     = pPage->GetPageSize().GetHeight() - 40000 * CONVERSION_CONSTANT;

    pPainter->SetStrokingColor( 0.0, 0.0, 0.0 );
    pPainter->DrawEllipse( dX, dY, 20000 * CONVERSION_CONSTANT, 20000 * CONVERSION_CONSTANT );

    dY -= 30000 * CONVERSION_CONSTANT;
    pPainter->SetColor( 1.0, 0.0, 0.0 );
    pPainter->FillEllipse( dX, dY, 20000 * CONVERSION_CONSTANT, 20000 * CONVERSION_CONSTANT );
}

void MMTest( PdfPainterMM* pPainter, PdfPage* pPage, PdfDocument* pDocument )
{
    long        lX     = 10000;
    long        lY     = (long)(pPage->GetPageSize().GetHeight()/CONVERSION_CONSTANT) - 40000;

    pPainter->SetStrokingColor( 0.0, 0.0, 0.0 );
    pPainter->DrawEllipseMM( lX, lY, 20000, 20000 );

    lY -= 30000;

    pPainter->SetColor( 1.0, 0.0, 0.0 );
    pPainter->FillEllipseMM( lX, lY, 20000, 20000 );
}

int main( int argc, char* argv[] ) 
{
    PdfDocument     writer;
    PdfPage*        pPage;
    PdfPainter      painter;
    PdfPainterMM    painterMM;
    PdfOutlines*    outlines;
    PdfOutlineItem* pRoot;
    if( argc != 2 )
    {
        printf("Usage: CreationTest [output_filename]\n");
        return 0;
    }

    printf("This test tests the PdfWriter and PdfDocument classes.\n");
    printf("It creates a new PdfFile from scratch.\n");
    printf("---\n");

    outlines = writer.GetOutlines();
    pRoot = outlines->CreateRoot("PoDoFo Test Document" );

    pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    painter.SetPage( pPage );
    pRoot->CreateChild( "Line Test", PdfDestination( pPage ) );

    printf("Drawing the first page with various lines.\n");
    TEST_SAFE_OP( LineTest( &painter, pPage, &writer ) );

    pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_Letter ) );
    painter.SetPage( pPage );
    pRoot->Last()->CreateNext( "Rectangles Test", PdfDestination( pPage ) );

    printf("Drawing the second page with various rectangle and triangles.\n");
    TEST_SAFE_OP( RectTest( &painter, pPage, &writer ) );

    pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    painter.SetPage( pPage );
    pRoot->Last()->CreateNext( "Text Test", PdfDestination( pPage ) );

    printf("Drawing some text.\n");
    TEST_SAFE_OP( TextTest( &painter, pPage, &writer ) );

    pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    painter.SetPage( pPage );
    pRoot->Last()->CreateNext( "Image Test", PdfDestination( pPage ) );

    printf("Drawing some images.\n");
    TEST_SAFE_OP( ImageTest( &painter, pPage, &writer ) );

    pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    painter.SetPage( pPage );
    pRoot->Last()->CreateNext( "Circle Test", PdfDestination( pPage ) );

    printf("Drawing some circles and ellipsis.\n");
    TEST_SAFE_OP( EllipseTest( &painter, pPage, &writer ) );

    printf("Drawing using PdfPainterMM.\n");
    pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    painterMM.SetPage( pPage );
    pRoot->Last()->CreateNext( "MM Test", PdfDestination( pPage ) );

    TEST_SAFE_OP( MMTest( &painterMM, pPage, &writer ) );

    printf("Setting document informations.\n\n");
    // Setup the document information dictionary
    TEST_SAFE_OP( writer.SetCreator ( PdfString("CreationTest - A simple test application") ) );
    TEST_SAFE_OP( writer.SetAuthor  ( PdfString("Dominik Seichter") ) );
    TEST_SAFE_OP( writer.SetTitle   ( PdfString("Test Document") ) );
    TEST_SAFE_OP( writer.SetSubject ( PdfString("Testing the PDF Library") ) );
    TEST_SAFE_OP( writer.SetKeywords( PdfString("Test;PDF;") ) );

    TEST_SAFE_OP( writer.Write( argv[1] ) );

#ifdef TEST_MEM_BUFFER
    // ---
    const char*   pszMemFile = "/home/dominik/mem_out.pdf";
    char*         pBuffer;
    unsigned long lBufferLen;
    FILE*         hFile;

    printf("Writing document from a memory buffer to: %s\n", pszMemFile );
    TEST_SAFE_OP( writer.WriteToBuffer( &pBuffer, &lBufferLen ) );

    hFile = fopen( pszMemFile, "wb" );
    if( !hFile )
    {
        fprintf( stderr, "Cannot open file %s for writing.\n", pszMemFile );
        return ePdfError_InvalidHandle;
    }

    printf("lBufferLen=%li\n", lBufferLen );
    printf("Wrote=%i\n", (int)fwrite( pBuffer, lBufferLen, sizeof( char ), hFile ) );
    fclose( hFile );
    free( pBuffer );
#endif

    return 0;
}

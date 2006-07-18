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

#include "PdfAnnotation.h"
#include "PdfAction.h"
#include "PdfSimpleWriter.h"
#include "PdfPage.h"
#include "PdfPainter.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfImage.h"
#include "PdfRect.h"
#include "PdfString.h"
#include "PdfXObject.h"

#include "../PdfTest.h"

using namespace PoDoFo;

#define CONVERSION_CONSTANT 0.002834645669291339


void LineTest( PdfPainter* pPainter, PdfPage* pPage, PdfSimpleWriter* pWriter )
{
    long     x     = 10000 * CONVERSION_CONSTANT;
    long     y     = 10000 * CONVERSION_CONSTANT;
    PdfFont* pFont;

    const long lLineLength = 50000 * CONVERSION_CONSTANT; // 5cm
    unsigned long h,w;

    pFont = pWriter->CreateFont( "Arial" );
    if( !pFont )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    pFont->SetFontSize( 16.0 );


    h = pFont->FontMetrics()->LineSpacing();
    w = pFont->FontMetrics()->StringWidth( "Grayscale Colorspace" );

    pPainter->SetFont( pFont );
    pPainter->DrawText( 120000 * CONVERSION_CONSTANT, y + pFont->FontMetrics()->LineSpacing(), "Grayscale Colorspace" );
    pPainter->DrawRect( 120000 * CONVERSION_CONSTANT , y + pFont->FontMetrics()->LineSpacing(), w, -h );

    // Draw 10 lines in gray scale
    for( int i = 0; i < 10; i++ )
    {
        x += (10000 * CONVERSION_CONSTANT);

        pPainter->SetStrokeWidth( (i*1000) * CONVERSION_CONSTANT );
        pPainter->SetStrokingGray( (double)i/10.0 );
        pPainter->DrawLine( x, y, x, y + lLineLength );
    }

    x = 10000 * CONVERSION_CONSTANT;
    y += lLineLength;
    y += (10000 * CONVERSION_CONSTANT);

    pPainter->DrawText( 120000 * CONVERSION_CONSTANT, y + pFont->FontMetrics()->LineSpacing(), "RGB Colorspace" );

    // Draw 10 lines in rgb
    for( int i = 0; i < 10; i++ )
    {
        x += (10000 * CONVERSION_CONSTANT);

        pPainter->SetStrokeWidth( (i*1000) * CONVERSION_CONSTANT );
        pPainter->SetStrokingColor( (double)i/10.0, 0.0, (double)(10-i)/10.0 );
        pPainter->DrawLine( x, y, x, y + lLineLength );
    }

    x = 10000 * CONVERSION_CONSTANT;
    y += lLineLength;
    y += (10000 * CONVERSION_CONSTANT);

    pPainter->DrawText( 120000 * CONVERSION_CONSTANT, y + pFont->FontMetrics()->LineSpacing(), "CMYK Colorspace" );

    // Draw 10 lines in cmyk
    for( int i = 0; i < 10; i++ )
    {
        x += (10000 * CONVERSION_CONSTANT);

        pPainter->SetStrokeWidth( (i*1000) * CONVERSION_CONSTANT );
        pPainter->SetStrokingColorCMYK( (double)i/10.0, 0.0, (double)(10-i)/10.0, 0.0 );
        pPainter->DrawLine( x, y, x, y + lLineLength );
    }

    x = 20000 * CONVERSION_CONSTANT;
    y += 60000 * CONVERSION_CONSTANT;

    pPainter->SetStrokeWidth( 1000 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 0.0, 0.0, 0.0 );

    pPainter->SetStrokeStyle( ePdfStrokeStyle_Solid );
    pPainter->DrawLine( x, y, x + (100000 * CONVERSION_CONSTANT), y );
    y += (10000 * CONVERSION_CONSTANT);

    pPainter->SetStrokeStyle( ePdfStrokeStyle_Dash );
    pPainter->DrawLine( x, y, x + (100000 * CONVERSION_CONSTANT), y );
    y += (10000 * CONVERSION_CONSTANT);

    pPainter->SetStrokeStyle( ePdfStrokeStyle_Dot );
    pPainter->DrawLine( x, y, x + (100000 * CONVERSION_CONSTANT), y );
    y += (10000 * CONVERSION_CONSTANT);

    pPainter->SetStrokeStyle( ePdfStrokeStyle_DashDot );
    pPainter->DrawLine( x, y, x + (100000 * CONVERSION_CONSTANT), y );
    y += (10000 * CONVERSION_CONSTANT);

    pPainter->SetStrokeStyle( ePdfStrokeStyle_DashDotDot );
    pPainter->DrawLine( x, y, x + (100000 * CONVERSION_CONSTANT), y );
    y += (10000 * CONVERSION_CONSTANT);

    pPainter->SetStrokeStyle( ePdfStrokeStyle_Custom, "[7 9 2] 4" );
    pPainter->DrawLine( x, y, x + (100000 * CONVERSION_CONSTANT), y );
    y += (10000 * CONVERSION_CONSTANT);
}

void RectTest( PdfPainter* pPainter, PdfPage* pPage, PdfSimpleWriter* pWriter )
{
    long     x     = 10000 * CONVERSION_CONSTANT;
    long     y     = 10000 * CONVERSION_CONSTANT;
    PdfFont* pFont;

    const long lWidth  = 50000 * CONVERSION_CONSTANT; // 5cm
    const long lHeight = 30000 * CONVERSION_CONSTANT; // 5cm

    pFont = pWriter->CreateFont( "Arial" );
    if( !pFont )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    pFont->SetFontSize( 16.0 );
    pPainter->SetFont( pFont );

    pPainter->DrawText( 125000 * CONVERSION_CONSTANT, y  + pFont->FontMetrics()->LineSpacing(), "Rectangles" );

    pPainter->SetStrokeWidth( 100 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 0.0, 0.0, 0.0 );
    pPainter->DrawRect( x, y, lWidth, lHeight );

    x += lWidth;
    x += 10000 * CONVERSION_CONSTANT;

    pPainter->SetStrokeWidth( 1000 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 0.0, 0.0, 0.0 );
    pPainter->DrawRect( x, y, lWidth, lHeight );

    y += lHeight;
    y += 10000 * CONVERSION_CONSTANT;
    x = 10000 * CONVERSION_CONSTANT;

    pPainter->SetStrokeWidth( 100 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 1.0, 0.0, 0.0 );
    pPainter->DrawRect( x, y, lWidth, lHeight );

    x += lWidth;
    x += 10000 * CONVERSION_CONSTANT;
    pPainter->SetStrokeWidth( 1000 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 0.0, 1.0, 0.0 );
    pPainter->DrawRect( x, y, lWidth, lHeight );

    y += lHeight;
    y += 10000 * CONVERSION_CONSTANT;
    x = 10000 * CONVERSION_CONSTANT;

    pPainter->SetStrokeWidth( 100 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 0.0, 0.0, 0.0 );
    pPainter->SetColor( 1.0, 0.0, 0.0 );
    pPainter->FillRect( x, y, lWidth, lHeight );
    pPainter->DrawRect( x, y, lWidth, lHeight );

    x += lWidth;
    x += 10000 * CONVERSION_CONSTANT;
    pPainter->SetStrokeWidth( 100 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 0.0, 1.0, 0.0 );
    pPainter->SetColor( 0.0, 0.0, 1.0 );
    pPainter->FillRect( x, y, lWidth, lHeight );
    pPainter->DrawRect( x, y, lWidth, lHeight );

    y += lHeight;
    y += 10000 * CONVERSION_CONSTANT;
    x = (10000 * CONVERSION_CONSTANT) + lWidth;

    pPainter->DrawText( 120000 * CONVERSION_CONSTANT, y + pFont->FontMetrics()->LineSpacing(), "Triangles" );

    // Draw a triangle at the current position
    pPainter->SetColor( 0.0, 1.0, 1.0 );
    pPainter->MoveTo( x, y );
    pPainter->LineTo( x+lWidth, y+lHeight );
    pPainter->LineTo( x-lWidth, y+lHeight );
    pPainter->ClosePath();
    pPainter->Fill();

    y += lHeight;
    y += 10000 * CONVERSION_CONSTANT;
    x = (10000 * CONVERSION_CONSTANT) + lWidth;

    pPainter->SetStrokingColor( 0.0, 0.0, 0.0 );
    pPainter->MoveTo( x, y );
    pPainter->LineTo( x+lWidth, y+lHeight );
    pPainter->LineTo( x-lWidth, y+lHeight );
    pPainter->ClosePath();
    pPainter->Stroke();
}

void TextTest( PdfPainter* pPainter, PdfPage* pPage, PdfSimpleWriter* pWriter )
{
    long x = 10000 * CONVERSION_CONSTANT;
    long y = 10000 * CONVERSION_CONSTANT;

    pPainter->SetFont( pWriter->CreateFont( "Times New Roman" ) );
    pPainter->Font()->SetFontSize( 24.0 );
    y += pPainter->Font()->FontMetrics()->LineSpacing();

    pPainter->SetColor( 0.0, 0.0, 0.0 );
    pPainter->DrawText( x, y, "Hallo Welt!" );
    
    y += pPainter->Font()->FontMetrics()->LineSpacing();
    pPainter->Font()->SetUnderlined( true );
    pPainter->SetStrokingColor( 1.0, 0.0, 0.0 );
    pPainter->DrawText( x, y, "Underlined text in the same font!" );

    pPainter->Font()->SetUnderlined( false );
    y += pPainter->Font()->FontMetrics()->LineSpacing();
    pPainter->DrawText( x, y, "Disabled the underline again..." );
    y += pPainter->Font()->FontMetrics()->LineSpacing();
    
    PdfFont* pFont = pWriter->CreateFont( "Arial" );
    pFont->SetFontSize( 12.0 );

    pPainter->SetFont( pFont );
    pPainter->DrawText( x, y, "PoDoFo rocks!" );
}

void ImageTest( PdfPainter* pPainter, PdfPage* pPage, PdfSimpleWriter* pWriter )
{
    long        y      = 60000 * CONVERSION_CONSTANT;

    PdfImageRef    imgRef;
    PdfImageRef    xobjRef;

    PdfImage*      pImg   = pWriter->CreateImage();

	// TODO: These values need to be replaced with PDF unit-based ones where second value is BOTTOM not top!
    PdfRect        rect( 0, 0, 3000 * CONVERSION_CONSTANT, 4000 * CONVERSION_CONSTANT );
    PdfRect        rect1( 80000 * CONVERSION_CONSTANT, 180000 * CONVERSION_CONSTANT, 20000 * CONVERSION_CONSTANT, 20000 * CONVERSION_CONSTANT );
    PdfRect        rect2( 80000 * CONVERSION_CONSTANT, 120000 * CONVERSION_CONSTANT, 10000 * CONVERSION_CONSTANT, 10000 * CONVERSION_CONSTANT );

    PdfAnnotation* pAnnot1 = pWriter->GetObjects().CreateObject<PdfAnnotation>();
    PdfAnnotation* pAnnot2 = pWriter->GetObjects().CreateObject<PdfAnnotation>();

    PdfXObject*    pXObj   = pWriter->GetObjects().CreateObject<PdfXObject>();
    PdfPainter     pnt;    // XObject painter

    pXObj->Init( rect );
    pImg->LoadFromFile( "./lena.jpg" );

    pImg->GetImageReference( imgRef );
    pXObj->GetImageReference( xobjRef );

    pnt.SetPage( pXObj );
    // Draw onto the XObject
#ifdef _WIN32
	pnt.SetFont( pWriter->CreateFont( "Comic Sans MS" ) );
#else
    pnt.SetFont( pWriter->CreateFont( "Comic Sans" ) );
#endif
    pnt.Font()->SetFontSize( 8.0 );
    pnt.SetStrokingColor( 1.0, 1.0, 1.0 );
    pnt.SetColor( 1.0, 0.0, 0.0 );
    pnt.FillRect( 0, 0, pXObj->PageSize().lWidth, pXObj->PageSize().lHeight );
    pnt.SetColor( 0.0, 0.0, 0.0 );
    pnt.DrawRect( 0, 1000 * CONVERSION_CONSTANT, 1000 * CONVERSION_CONSTANT, 1000 * CONVERSION_CONSTANT );
    pnt.DrawText( 0, 1000 * CONVERSION_CONSTANT, "I am a XObject." );

    printf("Drawing on the page!\n");
    // Draw onto the page 
    pPainter->DrawImage( 40000 * CONVERSION_CONSTANT, y, &imgRef, 0.3, 0.3 );
    pPainter->DrawImage( 40000 * CONVERSION_CONSTANT, y + (100000 * CONVERSION_CONSTANT), &imgRef, 0.2, 0.5 );
    pPainter->DrawImage( 40000 * CONVERSION_CONSTANT, y + (200000 * CONVERSION_CONSTANT), &imgRef, 0.3, 0.3 );
    pPainter->DrawXObject( 120000 * CONVERSION_CONSTANT, y + (15000 * CONVERSION_CONSTANT), &xobjRef );

    pAnnot1->Init( pPage, ePdfAnnotation_Widget, rect1 );
    pAnnot2->Init( pPage, ePdfAnnotation_Link, rect2 );

    pAnnot1->SetTitle( PdfString("Author: Dominik Seichter") );
    pAnnot1->SetContents( PdfString("Hallo Welt!") );
    pAnnot1->SetAppearanceStream( pXObj );

    PdfAction* pAction = pWriter->GetObjects().CreateObject<PdfAction>();
    pAction->Init( ePdfAction_URI );
    pAction->SetURI( PdfString("http://www.tec-it.com") );

    //pAnnot2->SetDestination( pPage );
    pAnnot2->SetDestination( pAction );
    pAnnot2->SetFlags( ePdfAnnotationFlags_NoZoom );
}

void EllipseTest( PdfPainter* pPainter, PdfPage* pPage, PdfSimpleWriter* pWriter )
{
    long        lX     = 10000 * CONVERSION_CONSTANT;
    long        lY     = 10000 * CONVERSION_CONSTANT;

    pPainter->SetStrokingColor( 0.0, 0.0, 0.0 );
    pPainter->DrawEllipse( lX, lY, 20000 * CONVERSION_CONSTANT, 20000 * CONVERSION_CONSTANT );

    lY += 30000 * CONVERSION_CONSTANT;
    pPainter->SetColor( 1.0, 0.0, 0.0 );
    pPainter->FillEllipse( lX, lY, 20000 * CONVERSION_CONSTANT, 20000 * CONVERSION_CONSTANT );
}

int main( int argc, char* argv[] ) 
{
    PdfSimpleWriter writer;
    PdfPage*        pPage;
    PdfPainter      painter;

    if( argc != 2 )
    {
        printf("Usage: CreationTest [output_filename]\n");
        return 0;
    }

    printf("This test tests the PdfWriter and PdfSimpleWriter classes.\n");
    printf("It creates a new PdfFile from scratch.\n");
    printf("---\n");

    pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    painter.SetPage( pPage );

    printf("Drawing the first page with various lines.\n");
    TEST_SAFE_OP( LineTest( &painter, pPage, &writer ) );

    pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_Letter ) );
    painter.SetPage( pPage );

    printf("Drawing the second page with various rectangle and triangles.\n");
    TEST_SAFE_OP( RectTest( &painter, pPage, &writer ) );

    pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    painter.SetPage( pPage );

    printf("Drawing some text.\n");
    TEST_SAFE_OP( TextTest( &painter, pPage, &writer ) );

    pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    painter.SetPage( pPage );

    printf("Drawing some images.\n");
    TEST_SAFE_OP( ImageTest( &painter, pPage, &writer ) );

    pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    painter.SetPage( pPage );

    printf("Drawing some circles and ellipsis.\n");
    TEST_SAFE_OP( EllipseTest( &painter, pPage, &writer ) );


    printf("Setting document informations.\n\n");
    // Setup the document information dictionary
    writer.SetDocumentCreator ( PdfString("CreationTest - A simple test application") );
    writer.SetDocumentAuthor  ( PdfString("Dominik Seichter") );
    writer.SetDocumentTitle   ( PdfString("Test Document") );
    writer.SetDocumentSubject ( PdfString("Testing the PDF Library") );
    writer.SetDocumentKeywords( PdfString("Test;PDF;") );

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

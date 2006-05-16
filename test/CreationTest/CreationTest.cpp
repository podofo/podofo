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

PdfError LineTest( PdfPainter* pPainter, PdfPage* pPage, PdfSimpleWriter* pWriter )
{
    PdfError eCode;
    int      i     = 0;
    long     x     = 10000;
    long     y     = 10000;
    PdfFont* pFont;

    const long lLineLength = 50000; // 5cm
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
    TEST_SAFE_OP( pPainter->DrawText( 120000 , y + pFont->FontMetrics()->LineSpacing(), "Grayscale Colorspace" ) );
    TEST_SAFE_OP( pPainter->DrawRect( 120000 , y + pFont->FontMetrics()->LineSpacing(), w, -h ) );

    // Draw 10 lines in gray scale
    for( int i = 0; i < 10; i++ )
    {
        x += 10000;

        TEST_SAFE_OP( pPainter->SetStrokeWidth( i*1000 ) );
        TEST_SAFE_OP( pPainter->SetStrokingGray( (double)i/10.0 ) );
        TEST_SAFE_OP( pPainter->DrawLine( x, y, x, y + lLineLength ) );
    }

    x = 10000;
    y += lLineLength;
    y += 10000;

    TEST_SAFE_OP( pPainter->DrawText( 120000 , y + pFont->FontMetrics()->LineSpacing(), "RGB Colorspace" ) );

    // Draw 10 lines in rgb
    for( int i = 0; i < 10; i++ )
    {
        x += 10000;

        TEST_SAFE_OP( pPainter->SetStrokeWidth( i*1000 ) );
        TEST_SAFE_OP( pPainter->SetStrokingColor( (double)i/10.0, 0.0, (double)(10-i)/10.0 ) );
        TEST_SAFE_OP( pPainter->DrawLine( x, y, x, y + lLineLength ) );
    }

    x = 10000;
    y += lLineLength;
    y += 10000;

    TEST_SAFE_OP( pPainter->DrawText( 120000 , y + pFont->FontMetrics()->LineSpacing(), "CMYK Colorspace" ) );

    // Draw 10 lines in cmyk
    for( int i = 0; i < 10; i++ )
    {
        x += 10000;

        TEST_SAFE_OP( pPainter->SetStrokeWidth( i*1000 ) );
        TEST_SAFE_OP( pPainter->SetStrokingColorCMYK( (double)i/10.0, 0.0, (double)(10-i)/10.0, 0.0 ) );
        TEST_SAFE_OP( pPainter->DrawLine( x, y, x, y + lLineLength ) );
    }

    x = 20000;
    y += 60000;

    TEST_SAFE_OP( pPainter->SetStrokeWidth( 1000 ) );
    TEST_SAFE_OP( pPainter->SetStrokingColor( 0.0, 0.0, 0.0 ) );

    TEST_SAFE_OP( pPainter->SetStrokeStyle( ePdfStrokeStyle_Solid ) );
    TEST_SAFE_OP( pPainter->DrawLine( x, y, x + 100000, y ) );
    y += 10000;

    TEST_SAFE_OP( pPainter->SetStrokeStyle( ePdfStrokeStyle_Dash ) );
    TEST_SAFE_OP( pPainter->DrawLine( x, y, x + 100000, y ) );
    y += 10000;

    TEST_SAFE_OP( pPainter->SetStrokeStyle( ePdfStrokeStyle_Dot ) );
    TEST_SAFE_OP( pPainter->DrawLine( x, y, x + 100000, y ) );
    y += 10000;

    TEST_SAFE_OP( pPainter->SetStrokeStyle( ePdfStrokeStyle_DashDot ) );
    TEST_SAFE_OP( pPainter->DrawLine( x, y, x + 100000, y ) );
    y += 10000;

    TEST_SAFE_OP( pPainter->SetStrokeStyle( ePdfStrokeStyle_DashDotDot ) );
    TEST_SAFE_OP( pPainter->DrawLine( x, y, x + 100000, y ) );
    y += 10000;

    TEST_SAFE_OP( pPainter->SetStrokeStyle( ePdfStrokeStyle_Custom, "[7 9 2] 4" ) );
    TEST_SAFE_OP( pPainter->DrawLine( x, y, x + 100000, y ) );
    y += 10000;

    return eCode;
}

PdfError RectTest( PdfPainter* pPainter, PdfPage* pPage, PdfSimpleWriter* pWriter )
{
    PdfError eCode;
    int      i     = 0;
    long     x     = 10000;
    long     y     = 10000;
    PdfFont* pFont;

    const long lWidth  = 50000; // 5cm
    const long lHeight = 30000; // 5cm

    pFont = pWriter->CreateFont( "Arial" );
    if( !pFont )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    pFont->SetFontSize( 16.0 );
    pPainter->SetFont( pFont );

    TEST_SAFE_OP( pPainter->DrawText( 125000 , y  + pFont->FontMetrics()->LineSpacing(), "Rectangles" ) );

    TEST_SAFE_OP( pPainter->SetStrokeWidth( 100 ) );
    TEST_SAFE_OP( pPainter->SetStrokingColor( 0.0, 0.0, 0.0 ) );
    TEST_SAFE_OP( pPainter->DrawRect( x, y, lWidth, lHeight ) );

    x += lWidth;
    x += 10000;

    TEST_SAFE_OP( pPainter->SetStrokeWidth( 1000 ) );
    TEST_SAFE_OP( pPainter->SetStrokingColor( 0.0, 0.0, 0.0 ) );
    TEST_SAFE_OP( pPainter->DrawRect( x, y, lWidth, lHeight ) );

    y += lHeight;
    y += 10000;
    x = 10000;

    TEST_SAFE_OP( pPainter->SetStrokeWidth( 100 ) );
    TEST_SAFE_OP( pPainter->SetStrokingColor( 1.0, 0.0, 0.0 ) );
    TEST_SAFE_OP( pPainter->DrawRect( x, y, lWidth, lHeight ) );

    x += lWidth;
    x += 10000;
    TEST_SAFE_OP( pPainter->SetStrokeWidth( 1000 ) );
    TEST_SAFE_OP( pPainter->SetStrokingColor( 0.0, 1.0, 0.0 ) );
    TEST_SAFE_OP( pPainter->DrawRect( x, y, lWidth, lHeight ) );

    y += lHeight;
    y += 10000;
    x = 10000;

    TEST_SAFE_OP( pPainter->SetStrokeWidth( 100 ) );
    TEST_SAFE_OP( pPainter->SetStrokingColor( 0.0, 0.0, 0.0 ) );
    TEST_SAFE_OP( pPainter->SetColor( 1.0, 0.0, 0.0 ) );
    TEST_SAFE_OP( pPainter->FillRect( x, y, lWidth, lHeight ) );
    TEST_SAFE_OP( pPainter->DrawRect( x, y, lWidth, lHeight ) );

    x += lWidth;
    x += 10000;
    TEST_SAFE_OP( pPainter->SetStrokeWidth( 100 ) );
    TEST_SAFE_OP( pPainter->SetStrokingColor( 0.0, 1.0, 0.0 ) );
    TEST_SAFE_OP( pPainter->SetColor( 0.0, 0.0, 1.0 ) );
    TEST_SAFE_OP( pPainter->FillRect( x, y, lWidth, lHeight ) );
    TEST_SAFE_OP( pPainter->DrawRect( x, y, lWidth, lHeight ) );

    y += lHeight;
    y += 10000;
    x = 10000 + lWidth;

    TEST_SAFE_OP( pPainter->DrawText( 120000 , y + pFont->FontMetrics()->LineSpacing(), "Triangles" ) );

    // Draw a triangle at the current position
    TEST_SAFE_OP( pPainter->SetColor( 0.0, 1.0, 1.0 ) );
    TEST_SAFE_OP( pPainter->MoveTo( x, y ) );
    TEST_SAFE_OP( pPainter->LineTo( x+lWidth, y+lHeight ) );
    TEST_SAFE_OP( pPainter->LineTo( x-lWidth, y+lHeight ) );
    TEST_SAFE_OP( pPainter->ClosePath() );
    TEST_SAFE_OP( pPainter->Fill() );

    y += lHeight;
    y += 10000;
    x = 10000 + lWidth;

    TEST_SAFE_OP( pPainter->SetStrokingColor( 0.0, 0.0, 0.0 ) );
    TEST_SAFE_OP( pPainter->MoveTo( x, y ) );
    TEST_SAFE_OP( pPainter->LineTo( x+lWidth, y+lHeight ) );
    TEST_SAFE_OP( pPainter->LineTo( x-lWidth, y+lHeight ) );
    TEST_SAFE_OP( pPainter->ClosePath() );
    TEST_SAFE_OP( pPainter->Stroke() );

    return eCode;
}

PdfError TextTest( PdfPainter* pPainter, PdfPage* pPage, PdfSimpleWriter* pWriter )
{
    PdfError eCode;

    long x = 10000;
    long y = 10000;

    SAFE_OP( pPainter->SetFont( pWriter->CreateFont( "Times New Roman" ) ) );
    pPainter->Font()->SetFontSize( 24.0 );
    y += pPainter->Font()->FontMetrics()->LineSpacing();

    TEST_SAFE_OP( pPainter->SetColor( 0.0, 0.0, 0.0 ) );
    TEST_SAFE_OP( pPainter->DrawText( x, y, "Hallo Welt!" ) );
    
    y += pPainter->Font()->FontMetrics()->LineSpacing();
    pPainter->Font()->SetUnderlined( true );
    printf("THE UNDERLINE SHOULD BE BLACK AND NOT RED!!!!\n");
    TEST_SAFE_OP( pPainter->SetStrokingColor( 1.0, 0.0, 0.0 ) );
    TEST_SAFE_OP( pPainter->DrawText( x, y, "Underlined text in the same font!" ) );

    pPainter->Font()->SetUnderlined( false );
    y += pPainter->Font()->FontMetrics()->LineSpacing();
    TEST_SAFE_OP( pPainter->DrawText( x, y, "Disabled the underline again..." ) );
    y += pPainter->Font()->FontMetrics()->LineSpacing();
    
    PdfFont* pFont = pWriter->CreateFont( "Arial" );
    pFont->SetFontSize( 12.0 );

    pPainter->SetFont( pFont );
    TEST_SAFE_OP( pPainter->DrawText( x, y, "PoDoFo rocks!" ) );

    return eCode;
}

PdfError ImageTest( PdfPainter* pPainter, PdfPage* pPage, PdfSimpleWriter* pWriter )
{
    PdfError    eCode;
    long        y      = 60000;

    PdfImageRef    imgRef;
    PdfImageRef    xobjRef;

    PdfImage*      pImg   = pWriter->CreateImage();

    PdfRect        rect( 0, 0, 3000, 4000 );
    PdfRect        rect1( 80000, 180000, 20000, 20000 );
    PdfRect        rect2( 80000, 120000, 10000, 10000 );

    PdfAnnotation* pAnnot1 = pWriter->CreateObject<PdfAnnotation>();
    PdfAnnotation* pAnnot2 = pWriter->CreateObject<PdfAnnotation>();

    PdfXObject*    pXObj   = pWriter->CreateObject<PdfXObject>();
    PdfPainter     pnt;    // XObject painter

    TEST_SAFE_OP( pXObj->Init( rect ) );
    TEST_SAFE_OP( pImg->LoadFromFile( "./lena.jpg" ) );

    pImg->GetImageReference( imgRef );
    pXObj->GetImageReference( xobjRef );

    pnt.SetPage( pXObj );
    // Draw onto the XObject
    TEST_SAFE_OP( pnt.SetFont( pWriter->CreateFont( "Comic Sans" ) ) );
    pnt.Font()->SetFontSize( 8.0 );
    pnt.SetStrokingColor( 1.0, 1.0, 1.0 );
    pnt.SetColor( 1.0, 0.0, 0.0 );
    pnt.FillRect( 0, 0, pXObj->PageSize().lWidth, pXObj->PageSize().lHeight );
    pnt.SetColor( 0.0, 0.0, 0.0 );
    pnt.DrawRect( 0, 1000, 1000, 1000 );
    pnt.DrawText( 0, 1000, "I am a XObject." );

    printf("Drawing on the page!\n");
    // Draw onto the page 
    pPainter->DrawImage( 40000, y, &imgRef, 0.3, 0.3 );
    pPainter->DrawImage( 40000, y + 100000, &imgRef, 0.2, 0.5 );
    pPainter->DrawImage( 40000, y + 200000, &imgRef, 0.3, 0.3 );
    pPainter->DrawXObject( 120000, y + 15000, &xobjRef );

    TEST_SAFE_OP( pAnnot1->Init( pPage, ePdfAnnotation_Widget, rect1 ) );
    TEST_SAFE_OP( pAnnot2->Init( pPage, ePdfAnnotation_Link, rect2 ) );

    pAnnot1->SetTitle( "(Author: Dominik Seichter)" );
    pAnnot1->SetContents( "(Hallo Welt!)" );
    pAnnot1->SetAppearanceStream( pXObj );

    PdfAction* pAction = pWriter->CreateObject<PdfAction>();
    TEST_SAFE_OP( pAction->Init( ePdfAction_URI ) );
    pAction->SetURI( "(http://www.tec-it.com)" );

    //pAnnot2->SetDestination( pPage );
    pAnnot2->SetDestination( pAction );
    pAnnot2->SetFlags( ePdfAnnotationFlags_NoZoom );
    
    return eCode;
}

PdfError EllipseTest( PdfPainter* pPainter, PdfPage* pPage, PdfSimpleWriter* pWriter )
{
    PdfError    eCode;
    long        lX     = 10000;
    long        lY     = 10000;

    TEST_SAFE_OP( pPainter->SetStrokingColor( 0.0, 0.0, 0.0 ) );
    TEST_SAFE_OP( pPainter->DrawEllipse( lX, lY, 20000, 20000 ) );

    lY += 30000;
    TEST_SAFE_OP( pPainter->SetColor( 1.0, 0.0, 0.0 ) );
    TEST_SAFE_OP( pPainter->FillEllipse( lX, lY, 20000, 20000 ) );

    return eCode;
}

int main( int argc, char* argv[] ) 
{
    PdfError        eCode;
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

    TEST_SAFE_OP( writer.Init( argv[1] ) );

    pPage = writer.CreatePage( PdfPage::CreateStadardPageSize( ePdfPageSize_A4 ) );
    painter.SetPage( pPage );

    printf("Drawing the first page with various lines.\n");
    TEST_SAFE_OP( LineTest( &painter, pPage, &writer ) );

    pPage = writer.CreatePage( PdfPage::CreateStadardPageSize( ePdfPageSize_Letter ) );
    painter.SetPage( pPage );

    printf("Drawing the second page with various rectangle and triangles.\n");
    TEST_SAFE_OP( RectTest( &painter, pPage, &writer ) );

    pPage = writer.CreatePage( PdfPage::CreateStadardPageSize( ePdfPageSize_A4 ) );
    painter.SetPage( pPage );

    printf("Drawing some text.\n");
    TEST_SAFE_OP( TextTest( &painter, pPage, &writer ) );

    pPage = writer.CreatePage( PdfPage::CreateStadardPageSize( ePdfPageSize_A4 ) );
    painter.SetPage( pPage );

    printf("Drawing some images.\n");
    TEST_SAFE_OP( ImageTest( &painter, pPage, &writer ) );

    pPage = writer.CreatePage( PdfPage::CreateStadardPageSize( ePdfPageSize_A4 ) );
    painter.SetPage( pPage );

    printf("Drawing some circles and ellipsis.\n");
    TEST_SAFE_OP( EllipseTest( &painter, pPage, &writer ) );


    printf("Setting document informations.\n\n");
    // Setup the document information dictionary
    writer.SetDocumentCreator ( "(CreationTest - A simple test application)" );
    writer.SetDocumentAuthor  ( "(Dominik Seichter)" );
    writer.SetDocumentTitle   ( "(Test Document)" );
    writer.SetDocumentSubject ( "(Testing the PDF Library)" );
    writer.SetDocumentKeywords( "(Test;PDF;)" );

    TEST_SAFE_OP( writer.Write() );
    printf("Error Code: %i\n", eCode.Error() );

    if( eCode.IsError() )
        eCode.PrintErrorMsg();

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

    if( eCode.IsError() )
        eCode.PrintErrorMsg();

    return eCode.Error();
}

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

#include "../PdfTest.h"

#include <cstdlib>
#include <cstdio>
#include <iostream>

using namespace PoDoFo;

#define CONVERSION_CONSTANT 0.002834645669291339

const char* pszLoremIpsum = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Suspendisse lacinia sollicitudin viverra. Praesent augue tellus, feugiat vel tempus ac, semper in tellus. Maecenas et vehicula urna. Suspendisse ullamcorper molestie leo id aliquet. Mauris ultricies porttitor lectus vel facilisis. Integer euismod libero ut lectus mattis sed venenatis metus molestie. Aliquam feugiat, dolor a adipiscing ullamcorper, sapien orci ultrices erat, eget porttitor ipsum purus id magna. Morbi malesuada malesuada sagittis. Curabitur viverra posuere sem, quis condimentum eros viverra et. Pellentesque tristique aliquam orci a aliquam.\n\nIn hac habitasse platea dictumst. Maecenas vitae lorem velit. Donec at ultrices arcu. Phasellus et justo in quam fermentum volutpat. Nam vestibulum tempus lorem nec lacinia. Cras ac dignissim tortor. Morbi pellentesque, nisi sit amet sollicitudin accumsan, ante quam egestas lorem, a dapibus quam orci quis nulla. Donec quis orci ut lacus dictum sollicitudin at eget turpis. Nam condimentum iaculis enim, id volutpat est dapibus id. Quisque sed enim in est condimentum convallis. Cras at posuere ipsum. Cras tempor dui nunc, vel malesuada odio.";

void WriteStringToStream( const PdfString & rsString, std::ostringstream & oss, PdfFont* pFont )
{
    PdfEncoding* pEncoding = new PdfIdentityEncoding( 0, 0xffff, true );
    PdfRefCountedBuffer buffer = pEncoding->ConvertToEncoding( rsString, pFont );
    pdf_long  lLen    = 0;
    char* pBuffer = NULL;

    PODOFO_UNIQUEU_PTR<PdfFilter> pFilter( PdfFilterFactory::Create( ePdfFilter_ASCIIHexDecode ) );
    pFilter->Encode( buffer.GetBuffer(), buffer.GetSize(), &pBuffer, &lLen );

    oss << "<";
    oss << std::string( pBuffer, lLen );
    oss << ">";
    free( pBuffer );
    delete pEncoding;
}

void CreateUnicodeAnnotationText( PdfPage* pPage, PdfDocument* /*pDocument*/ )
{
    PdfString sJap(reinterpret_cast<const pdf_utf8*>("「PoDoFo」は今から日本語も話せます。"));
    PdfAnnotation* pAnnotation =
        pPage->CreateAnnotation( ePdfAnnotation_Text, PdfRect( 400.0, 200.0, 20.0, 20.0 ) );

    PdfString sGerman(reinterpret_cast<const pdf_utf8*>("Unicode Umlauts: ÄÖÜß"));
    pAnnotation->SetTitle( sGerman );
    pAnnotation->SetContents( sJap );
    pAnnotation->SetOpen( true );
}

void CreateUnicodeAnnotationFreeText( PdfPage* pPage, PdfDocument* pDocument )
{
    PdfString sJap(reinterpret_cast<const pdf_utf8*>("「PoDoFo」は今から日本語も話せます。"));
    PdfFont* pFont = pDocument->CreateFont( "Arial Unicode MS", false, new PdfIdentityEncoding( 0, 0xffff, true ) );

    PdfRect rect( 200.0, 200.0, 200.0, 200.0 );
    /*
    PdfXObject xObj( rect, pDocument );

    PdfPainter painter;
    painter.SetPage( &xObj );
    painter.SetFont( pFont );
    painter.SetColor( 1.0, 0.0, 0.0 );
    painter.Rectangle( 10.0, 10.0, 100.0, 100.0 );
    painter.FillAndStroke();
    painter.DrawText( 100.0, 100.0, sJap );
    painter.FinishPage();
    */

    std::ostringstream  oss;
    oss << "BT" << std::endl << "/" <<   pFont->GetIdentifier().GetName()
        << " "  <<   pFont->GetFontSize()
        << " Tf " << std::endl;

    WriteStringToStream( sJap, oss, pFont );
    oss << "Tj ET" << std::endl;

    PdfDictionary fonts;
    fonts.AddKey(pFont->GetIdentifier().GetName(), pFont->GetObject()->Reference());
    PdfDictionary resources;
    resources.AddKey( PdfName("Fonts"), fonts );

    PdfAnnotation* pAnnotation =
        pPage->CreateAnnotation( ePdfAnnotation_FreeText, rect );

    PdfString sGerman(reinterpret_cast<const pdf_utf8*>("Unicode Umlauts: ÄÖÜß"));
    pAnnotation->SetTitle( sGerman );
    pAnnotation->SetContents( sJap );
    //pAnnotation->SetAppearanceStream( &xObj );
    pAnnotation->GetObject()->GetDictionary().AddKey( PdfName("DA"), PdfString(oss.str()) );
    pAnnotation->GetObject()->GetDictionary().AddKey( PdfName("DR"), resources );


}

void LineTest( PdfPainter* pPainter, PdfPage* pPage, PdfDocument* pDocument )
{
    double     x     = 10000 * CONVERSION_CONSTANT;
    double     y     = pPage->GetPageSize().GetHeight() - 10000 * CONVERSION_CONSTANT;
    PdfFont* pFont;

    const double dLineLength = 50000 * CONVERSION_CONSTANT; // 5cm
    double h;
    double w;
    int    i;

	/*
    pFont = pDocument->CreateFont( "Arial Unicode MS", new PdfIdentityEncoding( 0, 0xffff, true ) );
    printf("GOT: %s\n", pFont->GetFontMetrics()->GetFontname() );
    PdfString sJap(reinterpret_cast<const pdf_utf8*>("「Po\tDoFo」は今から日本語も話せます。"));
    const long     lUtf8BufferLen = 256;
    pdf_utf8 pUtf8Buffer[lUtf8BufferLen];


    PdfString::ConvertUTF16toUTF8( sJap.GetUnicode(), sJap.GetUnicodeLength(), pUtf8Buffer, lUtf8BufferLen  );
    printf("UNIC: %s\n", pUtf8Buffer );

    pFont->SetFontSize( 8.0 );
    pPainter->SetFont( pFont );
    pPainter->DrawText( 100.0, 100.0, sJap );
	*/

    std::vector<int> vecCharacters;
    vecCharacters.push_back( static_cast<int>('C') );
    vecCharacters.push_back( static_cast<int>('G') );
    vecCharacters.push_back( static_cast<int>('a') );
    vecCharacters.push_back( static_cast<int>('c') );
    vecCharacters.push_back( static_cast<int>('e') );
    vecCharacters.push_back( static_cast<int>('l') );
    vecCharacters.push_back( static_cast<int>('o') );
    vecCharacters.push_back( static_cast<int>('p') );
    vecCharacters.push_back( static_cast<int>('s') );
    vecCharacters.push_back( static_cast<int>('r') );
    vecCharacters.push_back( static_cast<int>('y') );
    vecCharacters.push_back( static_cast<int>(' ') );
    vecCharacters.push_back( static_cast<int>('-') );

    pFont = pDocument->CreateFont( "Comic Sans MS", false, false ); //, vecCharacters );
    if( !pFont )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    pFont->SetFontSize( 16.0 );


    const wchar_t* msg = L"Grayscale - Colorspace";
    h = pFont->GetFontMetrics()->GetLineSpacing();
    w = pFont->GetFontMetrics()->StringWidth( msg );

    pPainter->SetFont( pFont );
    pPainter->DrawText( 120000 * CONVERSION_CONSTANT, y - pFont->GetFontMetrics()->GetLineSpacing(), msg  );
    pPainter->Rectangle( 120000 * CONVERSION_CONSTANT, y - pFont->GetFontMetrics()->GetLineSpacing(), w, h );
    pPainter->Stroke();

    // Draw 10 lines in gray scale
    for( i = 0; i < 10; i++ )
    {
        x += (10000 * CONVERSION_CONSTANT);

        pPainter->SetStrokeWidth( (i*1000) * CONVERSION_CONSTANT );
        pPainter->SetStrokingGray( static_cast<double>(i)/10.0 );
        pPainter->DrawLine( x, y, x, y - dLineLength );
    }

    x = 10000 * CONVERSION_CONSTANT;
    y -= dLineLength;
    y -= (10000 * CONVERSION_CONSTANT);

    pFont = pDocument->CreateFont( "Arial", true, false ); // arial bold - not italic
    if( !pFont )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    pFont->SetFontSize( 16.0 );
    pPainter->SetFont( pFont );
    pPainter->DrawText( 120000 * CONVERSION_CONSTANT, y - pFont->GetFontMetrics()->GetLineSpacing(), "RGB Colorspace" );

    // Draw 10 lines in rgb
    for( i = 0; i < 10; i++ )
    {
        x += (10000 * CONVERSION_CONSTANT);

        pPainter->SetStrokeWidth( (i*1000) * CONVERSION_CONSTANT );
        pPainter->SetStrokingColor( static_cast<double>(i)/10.0, 0.0, static_cast<double>(10-i)/10.0 );
        pPainter->DrawLine( x, y, x, y - dLineLength );
    }

    x = 10000 * CONVERSION_CONSTANT;
    y -= dLineLength;
    y -= (10000 * CONVERSION_CONSTANT);

    pFont = pDocument->CreateFont( "Arial", false, true ); // arial italic - not bold
    if( !pFont )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    pFont->SetFontSize( 16.0 );
    pPainter->SetFont( pFont );
    pPainter->DrawText( 120000 * CONVERSION_CONSTANT, y - pFont->GetFontMetrics()->GetLineSpacing(), "CMYK Colorspace" );

    // Draw 10 lines in cmyk
    for( i = 0; i < 10; i++ )
    {
        x += (10000 * CONVERSION_CONSTANT);

        pPainter->SetStrokeWidth( (i*1000) * CONVERSION_CONSTANT );
        pPainter->SetStrokingColorCMYK( static_cast<double>(i)/10.0, 0.0, static_cast<double>(10-i)/10.0, 0.0 );
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

    //CreateUnicodeAnnotationText( pPage, pDocument );
    CreateUnicodeAnnotationFreeText( pPage, pDocument );

    return;
    ///////////////////////
    pPage = pDocument->CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    pPainter->SetPage( pPage );

    x     = 10000 * CONVERSION_CONSTANT;
    y     = pPage->GetPageSize().GetHeight() - 10000 * CONVERSION_CONSTANT;
    char buffer[1024];

    double dStroke = 0.01;
    double dLine   = pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();
    for( i=0;i<23; i++ )
    {
        sprintf( buffer, "Linewidth: %.3fpt", dStroke );
        pPainter->DrawText( x, y, PdfString( buffer ) );

        pPainter->Save();
        pPainter->SetStrokeWidth( dStroke );
        pPainter->DrawLine( x + 60000 * CONVERSION_CONSTANT, y + dLine/2.0, x + 140000 * CONVERSION_CONSTANT, y + dLine/2.0 );
        pPainter->DrawLine( x + 60000 * CONVERSION_CONSTANT, y, x + 140000 * CONVERSION_CONSTANT, y  );
        pPainter->DrawLine( x + 60000 * CONVERSION_CONSTANT, y + dLine, x + 140000 * CONVERSION_CONSTANT, y + dLine );

        pPainter->DrawLine( x + 60000 * CONVERSION_CONSTANT, y, x + 60000 * CONVERSION_CONSTANT, y + dLine );
        pPainter->DrawLine( x + 140000 * CONVERSION_CONSTANT, y, x + 140000 * CONVERSION_CONSTANT, y + dLine );

        pPainter->Restore();
        dStroke += 0.05;

        y -= dLine*2.0;
    }
}

void RectTest( PdfPainter* pPainter, PdfPage* pPage, PdfDocument* pDocument )
{
    double     x     = 10000 * CONVERSION_CONSTANT;
    double     y     = pPage->GetPageSize().GetHeight() - 10000 * CONVERSION_CONSTANT;
    PdfFont* pFont;

    const double dWidth  = 50000 * CONVERSION_CONSTANT; // 5cm
	const double dHeight = 30000 * CONVERSION_CONSTANT; // 3cm

	y -= dHeight;

    pFont = pDocument->CreateFont( "Arial" );
    if( !pFont )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    pFont->SetFontSize( 16.0 );
    pPainter->SetFont( pFont );

    pPainter->DrawText( 125000 * CONVERSION_CONSTANT, y - pFont->GetFontMetrics()->GetLineSpacing(), "Rectangles" );

    pPainter->SetStrokeWidth( 100 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 0.0, 0.0, 0.0 );
    pPainter->Rectangle( x, y, dWidth, dHeight );
    pPainter->Stroke();

    PdfString sMultiLine("Hello World! We try to draw text using PdfPainter and DrawMultiLineText into an rectangle - including wordwrapping.");
    pPainter->DrawMultiLineText( x, y, dWidth, dHeight, sMultiLine);

    x += dWidth;
    x += 10000 * CONVERSION_CONSTANT;

    pPainter->SetStrokeWidth( 1000 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 0.0, 0.0, 0.0 );
    pPainter->Rectangle( x, y, dWidth, dHeight );
    pPainter->Stroke();

    y -= dHeight;
    y -= 10000 * CONVERSION_CONSTANT;
    x = 10000 * CONVERSION_CONSTANT;

    pPainter->SetStrokeWidth( 100 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 1.0, 0.0, 0.0 );
    pPainter->Rectangle( x, y, dWidth, dHeight );
    pPainter->Stroke();

    x += dWidth;
    x += 10000 * CONVERSION_CONSTANT;
    pPainter->SetStrokeWidth( 1000 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 0.0, 1.0, 0.0 );
    pPainter->Rectangle( x, y, dWidth, dHeight );
    pPainter->Stroke();

    y -= dHeight;
    y -= 10000 * CONVERSION_CONSTANT;
    x = 10000 * CONVERSION_CONSTANT;

    pPainter->SetStrokeWidth( 100 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 0.0, 0.0, 0.0 );
    pPainter->SetColor( 1.0, 0.0, 0.0 );
    pPainter->Rectangle( x, y, dWidth, dHeight );
    pPainter->FillAndStroke();

    x += dWidth;
    x += 10000 * CONVERSION_CONSTANT;
    pPainter->SetStrokeWidth( 100 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 0.0, 1.0, 0.0 );
    pPainter->SetColor( 0.0, 0.0, 1.0 );
	x = 0.0;
	y = 0.0;
    pPainter->Rectangle( x, y, 50000 * CONVERSION_CONSTANT, 50000 * CONVERSION_CONSTANT);
    pPainter->FillAndStroke();

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

    printf("Embedding Font\n");
    printf("!!!!!!!!!!!!!!!\n");
    pPainter->SetFont( pDocument->CreateFont( "Times New Roman" ) );
    pPainter->GetFont()->SetFontSize( 24.0 );
    printf("!!!!!!!!!!!!!!!\n");
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

    pPainter->DrawText( x, y, "Normal" );
    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();

    pPainter->GetFont()->SetUnderlined( true );
    pPainter->DrawText( x, y, "Normal+underlinded" );
    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();

    pPainter->GetFont()->SetUnderlined( false );
    pFont->SetFontCharSpace( 100.0 );
    pPainter->DrawText( x, y, "Mormal+spaced" );
    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();

    pPainter->GetFont()->SetUnderlined( true );
    pPainter->DrawText( x, y, "Normal+underlined+spaced" );
    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();
    pPainter->GetFont()->SetUnderlined( false );
    pFont->SetFontCharSpace( 0.0 );


    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();


    pFont->SetFontScale( 50.0 );
    pPainter->DrawText( x, y, "Condensed" );
    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();

    pFont->SetFontCharSpace( 0.0 );
    pPainter->GetFont()->SetUnderlined( true );
    pPainter->DrawText( x, y, "Condensed+underlinded" );
    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();

    pPainter->GetFont()->SetUnderlined( false );
    pFont->SetFontCharSpace( 100.0 );
    pPainter->DrawText( x, y, "Condensed+spaced" );
    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();


    pPainter->GetFont()->SetUnderlined( true );
    pPainter->DrawText( x, y, "Condensed+underlined+spaced" );
    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();
    pPainter->GetFont()->SetUnderlined( false );
    pFont->SetFontCharSpace( 0.0 );


    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();

    pFont->SetFontScale( 200.0 );
    pPainter->DrawText( x, y, "Expanded" );
    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();

    pPainter->GetFont()->SetUnderlined( true );
    pPainter->DrawText( x, y, "Expanded+underlinded" );
    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();

    pPainter->GetFont()->SetUnderlined( false );
    pFont->SetFontCharSpace( 100.0 );
    pPainter->DrawText( x, y, "Expanded+spaced" );
    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();

    pPainter->GetFont()->SetUnderlined( true );
    pPainter->DrawText( x, y, "Expanded+underlined+spaced" );
    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();
    pPainter->GetFont()->SetUnderlined( false );
    pFont->SetFontCharSpace( 0.0 );
    pFont->SetFontScale( 100.0 );


    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();
    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();

    pPainter->GetFont()->SetStrikeOut( true );
    pPainter->DrawText( x, y, "Strikeout" );
    y -= pPainter->GetFont()->GetFontMetrics()->GetLineSpacing();
    pPainter->GetFont()->SetUnderlined( false );


    pPainter->DrawText( x, y, "PoDoFo rocks!" );
}

void ImageTest( PdfPainter* pPainter, PdfPage* pPage, PdfDocument* pDocument )
{
    double        y      = pPage->GetPageSize().GetHeight() - 60000 * CONVERSION_CONSTANT;

#ifdef PODOFO_HAVE_JPEG_LIB
    PdfImage image( pDocument );
#endif // PODOFO_HAVE_JPEG_LIB

    PdfRect        rect( 0, 0, 50000 * CONVERSION_CONSTANT, 50000 * CONVERSION_CONSTANT );
    PdfRect        rect1( 80000 * CONVERSION_CONSTANT, 3000 * CONVERSION_CONSTANT, 20000 * CONVERSION_CONSTANT, 20000 * CONVERSION_CONSTANT );
    PdfRect        rect2( 40000 * CONVERSION_CONSTANT, y, 50000 * CONVERSION_CONSTANT, 50000 * CONVERSION_CONSTANT );
    PdfXObject     xObj( rect, pDocument );
    PdfPainter     pnt;    // XObject painter

#ifdef PODOFO_HAVE_JPEG_LIB
    image.LoadFromFile( "resources/watzmann.jpg" );
#endif // PODOFO_HAVE_JPEG_LIB

    pnt.SetPage( &xObj );
    // Draw onto the XObject
    pnt.SetFont( pDocument->CreateFont( "Comic Sans MS" ) );

    pnt.GetFont()->SetFontSize( 8.0 );
    pnt.SetStrokingColor( 1.0, 1.0, 1.0 );
    pnt.SetColor( 1.0, 1.0, 0.0 );
    pnt.Rectangle( 0, 0, xObj.GetPageSize().GetWidth(), xObj.GetPageSize().GetHeight()  );
    pnt.Fill();
    pnt.SetColor( 0.0, 0.0, 0.0 );
    pnt.Rectangle( 0, 1000 * CONVERSION_CONSTANT, 1000 * CONVERSION_CONSTANT, 1000 * CONVERSION_CONSTANT );
    pnt.Stroke();
    pnt.DrawText( 0, 1000 * CONVERSION_CONSTANT, "I am a XObject." );
    pnt.FinishPage();

    printf("Drawing on the page!\n");
    // Draw onto the page

#ifdef PODOFO_HAVE_JPEG_LIB
	/*
    pPainter->DrawImage( 40000 * CONVERSION_CONSTANT, y, &image, 0.3, 0.3 );
    pPainter->DrawImage( 40000 * CONVERSION_CONSTANT, y - (100000 * CONVERSION_CONSTANT), &image, 0.2, 0.5 );
    pPainter->DrawImage( 40000 * CONVERSION_CONSTANT, y - (200000 * CONVERSION_CONSTANT), &image, 0.3, 0.3 );
	*/
	pPainter->DrawImage( 0.0, pPage->GetPageSize().GetHeight() - image.GetHeight(), &image );
#endif // PODOFO_HAVE_JPEG_LIB

    pPainter->DrawXObject( 120000 * CONVERSION_CONSTANT, y - (50000 * CONVERSION_CONSTANT), &xObj );
    pPainter->Rectangle( 120000 * CONVERSION_CONSTANT, y - (50000 * CONVERSION_CONSTANT), 1000 * CONVERSION_CONSTANT, 1000 * CONVERSION_CONSTANT );
    pPainter->Fill();

    PdfAnnotation* pAnnot1 = pPage->CreateAnnotation( ePdfAnnotation_Widget, rect1 );
    PdfAnnotation* pAnnot2 = pPage->CreateAnnotation( ePdfAnnotation_Link, rect2 );
    PdfAnnotation* pAnnot3 = pPage->CreateAnnotation( ePdfAnnotation_Text, PdfRect( 20.0, 20.0, 20.0, 20.0 ) );
    PdfAnnotation* pAnnot4 = pPage->CreateAnnotation( ePdfAnnotation_FreeText, PdfRect( 70.0, 20.0, 250.0, 50.0 ) );
    PdfAnnotation* pAnnot5 = pPage->CreateAnnotation( ePdfAnnotation_Popup, PdfRect( 300.0, 20.0, 250.0, 50.0 ) );

    pAnnot1->SetTitle( PdfString("Author: Dominik Seichter") );
    pAnnot1->SetContents( PdfString("Hallo Welt!") );
    pAnnot1->SetAppearanceStream( &xObj );

    PdfAction action( ePdfAction_URI, pDocument );
    action.SetURI( PdfString("http://podofo.sf.net") );

    //pAnnot2->SetDestination( pPage );
    pAnnot2->SetAction( action );
    pAnnot2->SetFlags( ePdfAnnotationFlags_NoZoom );

    pAnnot3->SetTitle( "A text annotation" );
    pAnnot3->SetContents( "Lorum ipsum dolor..." );

    pAnnot4->SetContents( "An annotation of type ePdfAnnotation_FreeText." );

    pAnnot5->SetContents( "A popup annotation." );
    pAnnot5->SetOpen( true );
}

void EllipseTest( PdfPainter* pPainter, PdfPage* pPage, PdfDocument* pDocument )
{
    PdfAnnotation* pFileAnnotation;

    double        dX     = 10000 * CONVERSION_CONSTANT;
    double        dY     = pPage->GetPageSize().GetHeight() - 40000 * CONVERSION_CONSTANT;

    pPainter->SetStrokingColor( 0.0, 0.0, 0.0 );
    pPainter->Ellipse( dX, dY, 20000 * CONVERSION_CONSTANT, 20000 * CONVERSION_CONSTANT );
    pPainter->Stroke();

    dY -= 30000 * CONVERSION_CONSTANT;
    pPainter->SetColor( 1.0, 0.0, 0.0 );
    pPainter->Ellipse( dX, dY, 20000 * CONVERSION_CONSTANT, 20000 * CONVERSION_CONSTANT );
    pPainter->Fill();

    PdfFileSpec file( "resources/watzmann.jpg", true, pDocument );
    pFileAnnotation =  pPage->CreateAnnotation( ePdfAnnotation_FileAttachement, PdfRect( 300.0, 400.0, 250.0, 50.0 ) );
    pFileAnnotation->SetContents( "A JPEG image of the Watzmann mountain, taken by Dominik 2016-07-05." );
    pFileAnnotation->SetFileAttachement( file );
}

void XObjectTest( PdfPainter* pPainter, PdfPage* pPage, PdfDocument* pDocument )
{
    double     x     = 10000 * CONVERSION_CONSTANT;
    double     y     = pPage->GetPageSize().GetHeight() - 10000 * CONVERSION_CONSTANT;
    const double dWidth  = 180000 * CONVERSION_CONSTANT; // 18cm
    const double dHeight = 270000 * CONVERSION_CONSTANT; // 27cm

    pPainter->SetColor( 1.0, 0.8, 0.8 );
    pPainter->Rectangle( x, y - dHeight, dWidth, dHeight );
    pPainter->Fill();

    // Das funktioniert immer
    PdfXObject xObj1( "resources/Illust.pdf", 0, pDocument );
    pPainter->DrawXObject( x + 90000 * CONVERSION_CONSTANT,
                           y - dHeight,
						   &xObj1 );
    pPainter->SetColor( 1.0, 0.0, 0.0 );
    pPainter->Rectangle( x + 90000 * CONVERSION_CONSTANT,
                        y - dHeight,
                        1000 * CONVERSION_CONSTANT,
                        1000 * CONVERSION_CONSTANT );
    pPainter->Fill();


    // Test XObject in XObject
    PdfRect        rectX( 0, 0, 50000 * CONVERSION_CONSTANT, 50000 * CONVERSION_CONSTANT );
    PdfXObject     xObj3( rectX, pDocument );
    PdfXObject     xObj4( rectX, pDocument );

    // Draw text onto the XObject3
    pPainter->SetPage( &xObj3 );
    pPainter->SetColor( 0.0, 1.0, 0.0 );
    pPainter->Rectangle( 0.0, 0.0, rectX.GetWidth(), rectX.GetHeight() );
    pPainter->Fill();
    pPainter->SetFont( pDocument->CreateFont( "Comic Sans MS" ) );
    pPainter->SetColor( 0.0, 0.0, 0.0 );
    pPainter->DrawText( 0, 1000 * CONVERSION_CONSTANT, "I am XObject 3." );
    pPainter->FinishPage();

    // Draw text and pdf onto the XObject4
    pPainter->SetPage( &xObj4 );
    pPainter->SetColor( 0.0, 1.0, 0.0 );
    pPainter->Rectangle( 0.0, 0.0, rectX.GetWidth(), rectX.GetHeight() );
    pPainter->Fill();
    pPainter->SetFont( pDocument->CreateFont( "Comic Sans MS" ) );
    pPainter->SetColor( 0.0, 0.0, 0.0 );
    pPainter->DrawText( 0, 1000 * CONVERSION_CONSTANT, "I am XObject 4." );
    PdfXObject xObj5( "resources/Illust.pdf", 0, pDocument );
    pPainter->DrawXObject( 5000 * CONVERSION_CONSTANT,
                           5000 * CONVERSION_CONSTANT,
                           &xObj5,
                           0.1,
                           0.1 );
    pPainter->FinishPage();


    // Switch back to page and draw Xobject 3+4
    pPainter->SetPage( pPage );
    pPainter->DrawXObject( 20000 * CONVERSION_CONSTANT,
                           y - 60000 * CONVERSION_CONSTANT,
                           &xObj3 );
    pPainter->DrawXObject( 120000 * CONVERSION_CONSTANT,
						   y - 60000 * CONVERSION_CONSTANT,
                           &xObj4 );
}

void MMTest( PdfPainterMM* pPainter, PdfPage* pPage, PdfDocument* pDocument )
{
    long        lX     = 10000;
    long        lY     = static_cast<long>(pPage->GetPageSize().GetHeight()/CONVERSION_CONSTANT) - 40000;

    pPainter->SetStrokingColor( 0.0, 0.0, 0.0 );
    pPainter->EllipseMM( lX, lY, 20000, 20000 );
    pPainter->Stroke();

    lY -= 30000;

    pPainter->SetColor( 1.0, 0.0, 0.0 );
    pPainter->EllipseMM( lX, lY, 20000, 20000 );
    pPainter->Fill();

    lY -= 60000;

    // let's test out the opacity features
    PdfExtGState	trans( pDocument );
    trans.SetFillOpacity( 0.5 );
    pPainter->SetExtGState( &trans );
    pPainter->SetColor( 1.0, 0.0, 0.0 );
    pPainter->EllipseMM( lX, lY, 20000, 20000 );
    pPainter->Fill();
    pPainter->SetColor( 0.0, 1.0, 0.0 );
    pPainter->EllipseMM( lX+20000, lY, 20000, 20000 );
    pPainter->Fill();
    pPainter->SetColor( 0.0, 0.0, 1.0 );
    pPainter->EllipseMM( lX+10000, lY-10000, 20000, 20000 );
    pPainter->Fill();


}

void TableTest( PdfPainter* pPainter, PdfPage* pPage, PdfDocument* pDocument )
{
    int i,z;
    double        dX     = 10000 * CONVERSION_CONSTANT;
    double        dY     = (pPage->GetPageSize().GetHeight() - 40000 * CONVERSION_CONSTANT);

    PdfFont* pFont = pDocument->CreateFont( "Comic Sans MS" );
    pFont->SetFontSize( 12.0f );
    pPainter->SetFont( pFont );

    const int nCols = 3;
    const int nRows = 10;
    PdfSimpleTableModel model( nCols, nRows );
    for(i=0;i<nCols;i++)
        for(z=0;z<nRows;z++)
        {
            std::ostringstream oss;
            oss << "Cell " << i << " " << z;
            model.SetText( i, z, PdfString( oss.str() ) );
        }

    PdfTable table1( nCols, nRows );
    table1.SetTableWidth ( 80000 * CONVERSION_CONSTANT );
    table1.SetTableHeight( 120000 * CONVERSION_CONSTANT );
    table1.SetModel( &model );
    table1.Draw( dX, dY, pPainter );


    dY = pPage->GetPageSize().GetHeight()/2.0 - 30000 * CONVERSION_CONSTANT;
    dX = 2000.0 * CONVERSION_CONSTANT;

    const int nCols2 = 5;
    const int nRows2 = 4;
    PdfSimpleTableModel model2( nCols2, nRows2 );
    model2.SetAlignment( ePdfAlignment_Center );
    model2.SetBackgroundColor( PdfColor( 0.3 ) );
    model2.SetBackgroundEnabled( true );
    for(i=0;i<nCols2;i++)
        for(z=0;z<nRows2;z++)
        {
            std::ostringstream oss;
            oss << rand();
            model2.SetText( i, z, PdfString( oss.str() ) );
        }

    PdfTable table2( nCols2, nRows2 );
    table2.SetModel( &model2 );
    table2.Draw( dX, dY, pPainter );
}

void LargeMultiLineTextTest( PdfPainter* pPainter, PdfPage* pPage, PdfDocument* pDocument )
{
    double     x     = 10000 * CONVERSION_CONSTANT;
    double     y     = pPage->GetPageSize().GetHeight() - 10000 * CONVERSION_CONSTANT;
    PdfFont* pFont;

    const double dWidth  = 100000 * CONVERSION_CONSTANT; // 10cm
    const double dHeight = 50000 * CONVERSION_CONSTANT; // 5cm

    y -= dHeight;

    pFont = pDocument->CreateFont( "Arial" );
    if( !pFont )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    pFont->SetFontSize( 48.0 );
    pPainter->SetFont( pFont );

    pPainter->SetStrokeWidth( 100 * CONVERSION_CONSTANT );
    pPainter->SetStrokingColor( 0.0, 0.0, 0.0 );
    pPainter->Rectangle( x, y, dWidth, dHeight );
    pPainter->Stroke();

    PdfString sMultiLine("PoDoFo is a library to work with the PDF file format and includes also a few tools. The name comes from the first three letters of PDF (Portable Document Format).");
    pPainter->DrawMultiLineText( x, y, dWidth, dHeight, sMultiLine, ePdfAlignment_Left, ePdfVerticalAlignment_Top );

    y = y - dHeight - dHeight / 2.0;

    pPainter->Rectangle( x, y, dWidth, dHeight );
    pPainter->Stroke();

    pFont->SetFontSize( 12.0 );
    pPainter->DrawMultiLineText( x, y, dWidth, dHeight, sMultiLine, ePdfAlignment_Left, ePdfVerticalAlignment_Top );

    y = y - dHeight - dHeight / 2.0;

    pPainter->Rectangle( x, y, dWidth, dHeight );
    pPainter->Stroke();

    pFont->SetFontSize( 16.0 );
    pPainter->DrawMultiLineText( x, y, dWidth, dHeight, sMultiLine, ePdfAlignment_Left, ePdfVerticalAlignment_Center );

    y = y - dHeight - dHeight / 2.0;

    pPainter->Rectangle( x, y, dWidth, dHeight );
    pPainter->Stroke();

    pFont->SetFontSize( 32.0 );
    pPainter->DrawMultiLineText( x, y, dWidth, dHeight, PdfString("PoDoFo is spelled without a g."), ePdfAlignment_Left, ePdfVerticalAlignment_Bottom );

}

void FontSubsetTest( PdfPainter* pPainter, PdfPage* pPage, PdfDocument* pDocument )
{
    double     x     = 10000 * CONVERSION_CONSTANT;
    double     y     = pPage->GetPageSize().GetHeight() - 10000 * CONVERSION_CONSTANT;
    PdfFont* pFont;

    const double dHeight = 50000 * CONVERSION_CONSTANT; // 5cm

    y -= dHeight;

    PdfEncoding* pEncoding = new PdfIdentityEncoding();
    pFont = pDocument->CreateFontSubset( "Verdana", false, false, false, pEncoding );
    if( !pFont )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    pFont->SetFontSize( 16.0 );
    pPainter->DrawText(x, y, "Hello World!");

    y -= dHeight;

    pFont->SetFontSize( 32.0 );
    pPainter->DrawText(x, y, "Subsetting in action!");
}

int main( int argc, char* argv[] )
{
    try {
        PdfMemDocument  writer;
        //PdfStreamedDocument  writer ( argv[1], ePdfVersion_1_5, &PdfEncrypt( "dominik", "owner" ) );
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

        printf("PoDoFo DataType Size Information:\n");
        printf("---\n");
        printf("sizeof variant=%" PDF_FORMAT_UINT64 "\n", static_cast<pdf_uint64>(sizeof(PdfVariant)) );
        printf("sizeof object=%" PDF_FORMAT_UINT64 "\n", static_cast<pdf_uint64>(sizeof(PdfObject)) );
        printf("sizeof reference=%" PDF_FORMAT_UINT64 "\n", static_cast<pdf_uint64>(sizeof(PdfReference)) );
        printf("---\n\n");

        outlines = writer.GetOutlines();
        pRoot = outlines->CreateRoot("PoDoFo Test Document" );

        pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
        painter.SetPage( pPage );
        pRoot->CreateChild( "Line Test", PdfDestination( pPage ) );

        printf("Drawing the first page with various lines.\n");
        TEST_SAFE_OP( LineTest( &painter, pPage, &writer ) );

        pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_Letter ) );
        painter.SetPage( pPage );

        PdfString sLoremIpsum( pszLoremIpsum );

        painter.DrawMultiLineText( 50.0, 50.0,
                                   pPage->GetMediaBox().GetWidth() - 100.0,
                                   pPage->GetMediaBox().GetHeight() - 100.0, sLoremIpsum );
        painter.FinishPage();

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
        painter.FinishPage();

        printf("Drawing some XObject's.\n");
        pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
        painter.SetPage( pPage );
        TEST_SAFE_OP( XObjectTest( &painter, pPage, &writer ) );
        painter.FinishPage();

        printf("Drawing using PdfTable.\n");
        pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
        painter.SetPage( pPage );
        pRoot->Last()->CreateNext( "PdfTable Test", PdfDestination( pPage ) );
        TEST_SAFE_OP( TableTest( &painter, pPage, &writer ) );
        painter.FinishPage();

        printf("Drawing using PdfPainterMM.\n");
        pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
        painterMM.SetPage( pPage );
        pRoot->Last()->CreateNext( "MM Test", PdfDestination( pPage ) );
        TEST_SAFE_OP( MMTest( &painterMM, pPage, &writer ) );
        painterMM.FinishPage();

        printf("Drawing using PdfPainter MultilineText.\n");
        pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
        painter.SetPage( pPage );
        pRoot->Last()->CreateNext( "Large MultilineText Test", PdfDestination( pPage ) );
        TEST_SAFE_OP( LargeMultiLineTextTest( &painter, pPage, &writer ) );
        painter.FinishPage();

        printf("Drawing using Font Subset.\n");
        pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
        painter.SetPage( pPage );
        pRoot->Last()->CreateNext( "Font Subset Test", PdfDestination( pPage ) );
        TEST_SAFE_OP( FontSubsetTest( &painter, pPage, &writer ) );
        painter.FinishPage();

#if 0
        /** Create a really large name tree to test the name tree implementation
         */
        for( int zz=1;zz<500;zz++ )
        {
            std::ostringstream oss;
            oss << "A" << zz;

            writer.GetNamesTree()->AddValue( "TestDict", PdfString( oss.str() ), PdfVariant( static_cast<long>(zz) )  );
        }

        writer.GetNamesTree()->AddValue( "TestDict", PdfString( "Berta" ), PdfVariant( 42L )  );
#endif

        printf("Setting document informations.\n\n");
        // Setup the document information dictionary
        TEST_SAFE_OP( writer.GetInfo()->SetCreator ( PdfString("CreationTest - A simple test application") ) );
        TEST_SAFE_OP( writer.GetInfo()->SetAuthor  ( PdfString("Dominik Seichter") ) );
        TEST_SAFE_OP( writer.GetInfo()->SetTitle   ( PdfString("Test Document") ) );
        //TEST_SAFE_OP( writer.GetInfo()->SetSubject ( PdfString("Testing the PDF Library") ) );
        TEST_SAFE_OP( writer.GetInfo()->SetSubject (
                          PdfString(reinterpret_cast<const pdf_utf8*>("「PoDoFo」は今から日本語も話せます。") ) ) );
        TEST_SAFE_OP( writer.GetInfo()->SetKeywords( PdfString("Test;PDF;") ) );

        //xTEST_SAFE_OP( writer.AttachFile( PdfFileSpec("../../../podofo/test/CreationTest/CreationTest.cpp", true, &writer ) ) );

        TEST_SAFE_OP( writer.Write( argv[1] ) );
        //TEST_SAFE_OP( writer.Close() );

#ifdef TEST_MEM_BUFFER
        // ---
        const char*   pszMemFile = "./mem_out.pdf";
        FILE*         hFile;

        PdfRefCountedBuffer buffer;
        PdfOutputDevice device( &buffer );
        printf("Writing document from a memory buffer to: %s\n", pszMemFile );
        TEST_SAFE_OP( writer.Write( &device ) );

        hFile = fopen( pszMemFile, "wb" );
        if( !hFile )
        {
            fprintf( stderr, "Cannot open file %s for writing.\n", pszMemFile );
            return ePdfError_InvalidHandle;
        }

        long lBufferLen = device.GetLength();
        printf("lBufferLen=%li\n", lBufferLen );
        printf("Wrote=%i\n", static_cast<int>(fwrite( buffer.GetBuffer(), lBufferLen, sizeof( char ), hFile )) );
        fclose( hFile );
#endif
    } catch( PdfError & e ) {
        std::cerr << "Error: An error " << e.GetError() << " ocurred." << std::endl;
        e.PrintErrorMsg();
        return e.GetError();
    }

    return 0;
}

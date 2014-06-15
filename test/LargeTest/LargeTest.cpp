/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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

#include <podofo.h>
#include <fontconfig/fontconfig.h>
#include <cstdio>

using namespace PoDoFo;

#define MIN_PAGES 100

bool writeImmediately = true;

void AddPage( PdfDocument* pDoc, const char* pszFontName, const char* pszImagePath )
{
    PdfPainter painter;
    PdfPage*   pPage;
    PdfFont*   pFont;
    PdfFont*   pArial;
    PdfImage*  pImage;
    PdfRect    rect;
 
    try {
        pFont  = pDoc->CreateFont( pszFontName );
    } catch ( const PdfError & e ) {
        e.PrintErrorMsg();
        return;
    }
    pPage  = pDoc->CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    pArial = pDoc->CreateFont( "Arial" );
    pImage = new PdfImage( pDoc );

    rect   = pPage->GetMediaBox();

    const char* pszText = "The red brown fox jumps over the lazy dog!";
    double     dX       = rect.GetLeft() + 20.0;
    double     dY       = rect.GetBottom() + rect.GetHeight() - 20.0;
    double     dW, dH;

    pFont->SetFontSize( 16.0 );
    pArial->SetFontSize( 24.0 );

    painter.SetPage( pPage );
    painter.SetFont( pFont );

    dW = pFont->GetFontMetrics()->StringWidth( pszText );
    dH = -pFont->GetFontMetrics()->GetDescent(); // GetDescent is usually negative!

    pFont->SetFontSize( 24.0 );
    dH += pFont->GetFontMetrics()->GetLineSpacing() * 2.0;

    painter.Rectangle( dX, dY, dW, dH );
    painter.Stroke();

    dY -= pFont->GetFontMetrics()->GetLineSpacing();
    painter.DrawText( dX, dY, "Hello World!" );
    dY -= pFont->GetFontMetrics()->GetLineSpacing();
    pFont->SetFontSize( 16.0 );
    painter.DrawText( dX, dY, pszText );

    painter.SetFont( pArial );
    dY -= pArial->GetFontMetrics()->GetLineSpacing();
    painter.DrawText( dX, dY, "The font used in this example is:" );
    dY -= pArial->GetFontMetrics()->GetLineSpacing();
    painter.DrawText( dX, dY, pszFontName );
    dY -= pArial->GetFontMetrics()->GetLineSpacing();

#if defined PODOFO_HAVE_JPEG_LIB || defined PODOFO_HAVE_TIFF_LIB
    try {
        pImage->LoadFromFile( pszImagePath );
    }
    catch( const PdfError & e ) 
    {
        e.PrintErrorMsg();
    }

    dY -= (pImage->GetHeight() * 0.5);
    dX = ((rect.GetWidth() - (pImage->GetWidth()*0.5))/2.0);
    painter.DrawImage( dX, dY, pImage, 0.5, 0.5 );
    delete pImage; // delete image right after drawing
#endif

    painter.FinishPage();
}

void CreateLargePdf( const char* pszFilename, const char* pszImagePath )
{
    PdfDocument*         pDoc = NULL;
    FcObjectSet*         pObjectSet;
    FcFontSet*           pFontSet;
    FcPattern*           pPattern;

    if( !FcInit() ) 
    {
        fprintf( stderr, "Cannot load fontconfig!\n");
        return;
    }

    pPattern   = FcPatternCreate();
    pObjectSet = FcObjectSetBuild( FC_FAMILY, FC_STYLE, NULL );
    pFontSet   = FcFontList( 0, pPattern, pObjectSet );

    FcObjectSetDestroy( pObjectSet );
    FcPatternDestroy( pPattern );

    if( writeImmediately ) 
        pDoc = new PdfStreamedDocument( pszFilename );
    else 
        pDoc = new PdfMemDocument();

    if( pFontSet )
    {
        for( int i=0; i< (pFontSet->nfont > MIN_PAGES ? MIN_PAGES : pFontSet->nfont );i++ )
        {
            FcValue v;

            //FcPatternPrint( pFontSet->fonts[i] );
            FcPatternGet( pFontSet->fonts[i], FC_FAMILY, 0, &v );
            //font = FcNameUnparse( pFontSet->fonts[i] );
            printf(" -> Drawing with font: %s\n", reinterpret_cast<const char*>(v.u.s) );
            AddPage( pDoc, reinterpret_cast<const char*>(v.u.s), pszImagePath );
        }

        FcFontSetDestroy( pFontSet );
    }
    

    if( writeImmediately )
        static_cast<PdfStreamedDocument*>(pDoc)->Close();
    else
        static_cast<PdfMemDocument*>(pDoc)->Write( pszFilename );
    delete pDoc;
}

void usage()
{
    printf("Usage: LargetTest [-m] output_filename image_file\n"
           "       output_filename: filename to write produced pdf to\n"
           "       image_file:      An image to embed in the PDF file\n"
           "Options:\n"
           "       -m               Build entire document in memory before writing\n"
           "\n"
           "Note that output should be the same with and without the -m option.\n");
}

int main( int argc, char* argv[] ) 
{
    if( argc < 3 || argc > 4 )
    {
        usage();
        return 1;
    }
    else if ( argc == 4)
    {
        // Handle options
        // Is this argument an option?
        if (argv[1][0] != '-')
        {
            usage();
            return 1;
        }
        // Is it a recognised option?
        if (argv[1][1] == 'm')
        {
            // User wants us to build the whole doc in RAM before writing it out.
            writeImmediately = false;
            ++argv;
        }
        else
        {
            printf("Unrecognised argument: %s", argv[1]);
            usage();
            return 1;
        }
    }

    try {
        CreateLargePdf( argv[1], argv[2] );

        printf("\nWrote the PDF file %s successfully\n", argv[1] ); 

    } catch( PdfError & e ) {
        e.PrintErrorMsg();
        return e.GetError();
    }

    return 0;
}

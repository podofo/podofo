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

using namespace PoDoFo;

#define MIN_PAGES 100

bool writeImmediately = true;

void AddPage( PdfDocument* pDoc, PdfStreamedDocument* pStreamed, const char* pszFontName, const char* pszImagePath )
{
    PdfPainter painter;
    PdfPage*   pPage;
    PdfFont*   pFont;
    PdfFont*   pArial;
    PdfRect    rect;
 
    if( pDoc ) 
    {
        pPage  = pDoc->CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
        pFont  = pDoc->CreateFont( pszFontName );
        pArial = pDoc->CreateFont( "Arial" );
    }
    else
    {
        pPage  = pStreamed->CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
        pFont  = pStreamed->CreateFont( pszFontName );
        pArial = pStreamed->CreateFont( "Arial" );
    }

    //PdfImage   img( pDoc );
    //img.LoadFromFile( pszImagePath );

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

    painter.DrawRect( dX, dY, dW, dH );

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

    //dY -= (img.GetHeight() * 0.5);
    //dX = ((rect.GetWidth() - (img.GetWidth()*0.5))/2.0);
    //painter.DrawImage( dX, dY, &img, 0.5, 0.5 );
}

void CreateLargePdf( const char* pszFilename, const char* pszImagePath )
{
    PdfStreamedDocument* pStreamed = NULL;
    PdfDocument        * pDoc = NULL;
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
        pStreamed = new PdfStreamedDocument( pszFilename );
    else 
        pDoc = new PdfDocument();

    if( pFontSet )
    {
        for( int i=0; i< (pFontSet->nfont > MIN_PAGES ? MIN_PAGES : pFontSet->nfont );i++ )
        {
            FcValue v;

            //FcPatternPrint( pFontSet->fonts[i] );
            FcPatternGet( pFontSet->fonts[i], FC_FAMILY, 0, &v );
            //font = FcNameUnparse( pFontSet->fonts[i] );
            printf(" -> Drawing with font: %s\n", reinterpret_cast<const char*>(v.u.s) );
            AddPage( pDoc, pStreamed, reinterpret_cast<const char*>(v.u.s), pszImagePath );
        }

        FcFontSetDestroy( pFontSet );
    }
    

    if( writeImmediately )
    {
        pStreamed->Close();
        delete pStreamed;
    }
    else
    {
        pDoc->Write( pszFilename );
        delete pDoc;
    }
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

    } catch( PdfError & e ) {
        e.PrintErrorMsg();
        return e.GetError();
    }

    return 0;
}

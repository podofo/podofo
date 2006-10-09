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

void AddPage( PdfDocument* pDoc, const char* pszFontName, const char* pszImagePath )
{
    PdfPainter painter;
    PdfPage*   pPage  = pDoc->CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    PdfFont*   pFont  = pDoc->CreateFont( pszFontName );
    PdfFont*   pArial = pDoc->CreateFont( "Arial" );
    PdfRect    rect   = pPage->GetMediaBox();
    PdfImage   img( pDoc );

    const char* pszText = "The red brown fox jumps over the lazy dog!";
    double     dX       = rect.GetLeft() + 20.0;
    double     dY       = rect.GetBottom() + rect.GetHeight() - 20.0;

    pFont->SetFontSize( 24.0 );
    pArial->SetFontSize( 24.0 );

    painter.SetPage( pPage );
    painter.SetFont( pFont );

    painter.DrawRect( dX, dY, pFont->GetFontMetrics()->StringWidth( pszText ), 2 * pFont->GetFontMetrics()->GetLineSpacing() );

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

    img.LoadFromFile( pszImagePath );
    dY -= (img.GetHeight() * 0.5);
    dX = ((rect.GetWidth() - (img.GetWidth()*0.5))/2.0);
    painter.DrawImage( dX, dY, &img, 0.5, 0.5 );
}

void CreateLargePdf( const char* pszFilename )
{
    PdfDocument  doc;
    FcObjectSet* pObjectSet;
    FcFontSet*   pFontSet;
    FcPattern*   pPattern;

    if( !FcInit() ) 
    {
        fprintf( stderr, "Cannot load fontconfig!\n");
        return;
    }

    pPattern   = FcPatternCreate();
    pObjectSet = FcObjectSetBuild( FC_FAMILY, FC_STYLE, (char *)0 );
    pFontSet    = FcFontList( 0, pPattern, pObjectSet );

    FcObjectSetDestroy( pObjectSet );
    FcPatternDestroy( pPattern );

    if( pFontSet )
    {
	for( int i=0; i<pFontSet->nfont;i++ )
	{
            FcValue v;
            
            //FcPatternPrint( pFontSet->fonts[i] );
            FcPatternGet( pFontSet->fonts[i], FC_FAMILY, 0, &v );
	    //font = FcNameUnparse( pFontSet->fonts[i] );
            printf(" -> Drawing with font: %s\n", (const char*)(v.u.s) );
            AddPage( &doc, (const char*)(v.u.s), "../CreationTest/lena.jpg" );
	}

	FcFontSetDestroy( pFontSet );
    }

    doc.Write( pszFilename );
}

int main( int argc, char* argv[] ) 
{
    if( argc != 2 )
    {
        printf("Usage: LargetTest [output_filename]\n");
        return 0;
    }

    try {
        CreateLargePdf( argv[1] );
    } catch( PdfError & e ) {
        e.PrintErrorMsg();
        return e.GetError();
    }

    return 0;
}

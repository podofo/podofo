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

#include <podofo.h>

#include <stdlib.h>
#include <string.h>
#include <cstdio>

using namespace PoDoFo;

#ifdef _HAVE_CONFIG
#include <config.h>
#endif // _HAVE_CONFIG

#define BORDER_TOP   10000 * CONVERSION_CONSTANT
#define BORDER_LEFT  10000 * CONVERSION_CONSTANT
#define FONT_SIZE    12.0
#define DEFAULT_FONT "Arial"

void print_help()
{
  printf("Usage: podofotxt2pdf [inputfile] [outputfile]\n\n");
  printf("Optional parameters:\n");
  printf("\t-fontname [name]\t Use the font [name]\n");
  printf("\t-utf8\t that specifies that the input text\n");
  printf("\t\tis UTF8 encoded instead of Windows ANSI encoding.\n");       
  printf("\nPoDoFo Version: %s\n\n", PODOFO_VERSION_STRING);
}

void draw( char* pszBuffer, PdfDocument* pDocument, bool bUtf8, const char* pszFontName )
{
    PdfPage*           pPage;
    PdfPainter         painter;
    PdfFont*           pFont;
    PdfRect            size;
    const PdfEncoding* pEncoding;

    double dX       = BORDER_LEFT;
    double dY       = BORDER_TOP;
    char*  pszStart = pszBuffer;

    if( bUtf8 ) 
    {
        pEncoding = new PdfIdentityEncoding();
    }
    else
    {
        pEncoding = PdfEncodingFactory::GlobalWinAnsiEncodingInstance();
    }

    size            = PdfPage::CreateStandardPageSize( ePdfPageSize_A4 );
    pFont = pDocument->CreateFont( pszFontName, false, pEncoding );
    pPage = pDocument->CreatePage( size );
    dY    = size.GetHeight() - dY;

    if( !pFont )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
    pFont->SetFontSize( FONT_SIZE );
    
    painter.SetPage( pPage );
    painter.SetFont( pFont );

    while( *pszBuffer )
    {
        if( *pszBuffer == '\n' )
        {
            if( bUtf8 ) 
            {
                painter.DrawText( dX, dY, PdfString( reinterpret_cast<const pdf_utf8*>(pszStart), 
                                                     pszBuffer-pszStart ) );
            }
            else
            {
                painter.DrawText( dX, dY, pszStart, pszBuffer-pszStart );
            }
    
            pszStart = (++pszBuffer);            

            dY -= pFont->GetFontMetrics()->GetLineSpacing();
            if( dY < BORDER_TOP )
            {
                pPage = pDocument->CreatePage( size );
                painter.SetPage( pPage );
                dY       = size.GetHeight() - dY;
            }
        }
        else
            ++pszBuffer;
    }

    painter.FinishPage();
}

void init( const char* pszInput, const char* pszOutput, bool bUtf8, const char* pszFontName )
{
    FILE*   hFile;

    PdfStreamedDocument doc( pszOutput );

    char*  pszBuf;
    size_t lSize;

    hFile = fopen( pszInput, "rb" );	// read it as binary if we are going to compare sizes!
    if( !hFile )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    fseek( hFile, 0x00, SEEK_END );
    lSize  = ftell( hFile );

    pszBuf = static_cast<char*>(malloc( sizeof( char ) * (lSize+1) ));
    fseek( hFile, 0x00, SEEK_SET );
    if( !pszBuf )
    {
        PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
    }

    // read the whole file into memory at once.
    // this not very efficient, but as this is 
    // a library demonstration I do not care.
    // If anyone wants to improve this: Go for it!
    if( fread( pszBuf, sizeof(char), lSize, hFile ) != lSize )
    {
        free( pszBuf );
        PODOFO_RAISE_ERROR( ePdfError_UnexpectedEOF );
    }

    fclose( hFile );

    pszBuf[lSize] = '\0';

    draw( pszBuf, &doc, bUtf8, pszFontName );

    doc.GetInfo()->SetCreator( PdfString("podofotxt2pdf") );
    doc.GetInfo()->SetTitle( PdfString("Converted to PDF from a text file") );
    doc.Close();

    free( pszBuf );
}

int main( int argc, char* argv[] )
{
  const char*   pszInput = NULL;
  const char*   pszOutput = NULL;
  const char*   pszFontName = DEFAULT_FONT;
  bool          bUtf8 = false;

  if( argc < 3 ) 
  {
    print_help();
    exit( -1 );
  }

  for(int i=1;i<argc;i++) 
  {
      if( strcmp("-utf8", argv[i]) == 0 ) 
      {
          bUtf8 = true;
      }
      else if( strcmp("-fontname", argv[i]) == 0 ) 
      {
          ++i;
          pszFontName = argv[i];
      }
      else 
      {
          if( pszInput == NULL )
          {
              pszInput = argv[i];
          }
          else
          {
              pszOutput = argv[i];
          }
      }
  }

  try {
      init( pszInput, pszOutput, bUtf8, pszFontName );
  } catch( PdfError & e ) {
      fprintf( stderr, "Error %i occurred!\n", e.GetError() );
      e.PrintErrorMsg();
      return e.GetError();
  }

  return 0;
}


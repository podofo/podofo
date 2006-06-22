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

#include <PdfDefines.h>

#include <PdfSimpleWriter.h>
#include <PdfPage.h>
#include <PdfPainter.h>
#include <PdfFont.h>
#include <PdfFontMetrics.h>

using namespace PoDoFo;

#ifdef _HAVE_CONFIG
#include <config.h>
#endif // _HAVE_CONFIG

#define BORDER_TOP   10000
#define BORDER_LEFT  10000
#define FONT_SIZE    12.0

void print_help()
{
  printf("Usage: podofotxt2pdf [inputfile] [outputfile]\n\n");
}

PdfError draw( char* pszBuffer, PdfSimpleWriter* pWriter )
{
    PdfError eCode;

    PdfPage*        pPage;
    PdfPainter      painter;
    PdfFont*        pFont;

    long   lX       = BORDER_LEFT;
    long   lY       = BORDER_TOP;
    long   w        = 0;
    char*  pszStart = pszBuffer;

    pFont = pWriter->CreateFont( "Arial" );
    pPage = pWriter->CreatePage( PdfPage::CreateStadardPageSize( ePdfPageSize_A4 ) );

    if( !pFont || !pPage )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }
    pFont->SetFontSize( FONT_SIZE );
    
    painter.SetPage( pPage );
    painter.SetFont( pFont );

    while( *pszBuffer )
    {
        if( *pszBuffer == '\n' )
        {
            painter.DrawText( lX, lY, pszStart, pszBuffer-pszStart );
            pszStart = (++pszBuffer);            

            lY += pFont->FontMetrics()->LineSpacing();
            if( lY > (pPage->PageSize().lHeight - BORDER_TOP) )
            {
                pPage = pWriter->CreatePage( PdfPage::CreateStadardPageSize( ePdfPageSize_A4 ) );
                if( !pPage )
                {
                    RAISE_ERROR( ePdfError_InvalidHandle );
                }

                painter.SetPage( pPage );
                lY       = BORDER_TOP;
            }
        }
        else
            ++pszBuffer;
    }

    return eCode;
}

PdfError init( const char* pszInput, const char* pszOutput )
{
    PdfError eCode;
    FILE*   hFile;

    PdfSimpleWriter writer;

    char*  pszBuf;
    long   lSize;

    hFile = fopen( pszInput, "rb" );	// read it as binary if we are going to compare sizes!
    if( !hFile )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    SAFE_OP( writer.Init() );

    fseek( hFile, 0x00, SEEK_END );
    lSize  = ftell( hFile );

    pszBuf = (char*)malloc( sizeof( char ) * lSize );
    fseek( hFile, 0x00, SEEK_SET );
    if( !pszBuf )
    {
        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    // read the whole file into memory at once.
    // this not very efficient, but as this is 
    // a library demonstration I do not care.
    // If anyone wants to improve this: Go for it!
    if( fread( pszBuf, sizeof(char), lSize, hFile ) != lSize )
    {
        free( pszBuf );
        RAISE_ERROR( ePdfError_UnexpectedEOF );
    }

    fclose( hFile );

    SAFE_OP( draw( pszBuf, &writer ) );
    SAFE_OP( writer.Write( pszOutput ) );

    free( pszBuf );
    return eCode;
}

int main( int argc, char* argv[] )
{
  PdfError eCode;
  char*   pszInput;
  char*   pszOutput;

  if( argc != 3 )
  {
    print_help();
    exit( -1 );
  }

  pszInput  = argv[1];
  pszOutput = argv[2];

  eCode = init( pszInput, pszOutput );
  if( eCode.IsError() )
      fprintf( stderr, "Error %i occurred!\n", eCode.Error() );

  return eCode.Error();
}


/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
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

#include <cstdlib>
#include <cstdio>

#include <vector>

using namespace PoDoFo;

#ifdef _HAVE_CONFIG
#include <config.h>
#endif // _HAVE_CONFIG

void print_help()
{
    printf("Usage: podofocrop input.pdf output.pdf\n");
    printf("       This tool will crop all pages.\n");
    printf("       It requires ghostscript to be in your PATH\n");
    printf("\nPoDoFo Version: %s\n\n", PODOFO_VERSION_STRING);
}

const char* find_ghostscript()
{
    return "gs";
}

void crop_page( PdfPage* pPage, const PdfRect & rCropBox ) 
{
    PdfVariant var;
    printf("%f %f %f %f\n",
           rCropBox.GetLeft(),
           rCropBox.GetBottom(),
           rCropBox.GetWidth(),
           rCropBox.GetHeight());

    rCropBox.ToVariant( var );
    pPage->GetObject()->GetDictionary().AddKey( PdfName("MediaBox"), var );
}

std::vector<PdfRect> get_crop_boxes( const char* pszInput )
{
    const int lBufferLen = 256;
    const char* pszFormat = 
        "gs -dSAFER -sDEVICE=bbox -sNOPAUSE -q %s -c quit 2>&1";
    char buffer[lBufferLen];
    char line[lBufferLen];

    snprintf( buffer, lBufferLen, pszFormat, pszInput );
    printf("cmd=%s\n", buffer);
    FILE* hPipe = popen(buffer, "r");
    
    std::vector<PdfRect> rects;
    while( !feof( hPipe ) ) 
    {
        //fread( line, sizeof(char), lBufferLen, hPipe );
        fgets( line, lBufferLen, hPipe );
        fgets( line, lBufferLen, hPipe );
        
        if( strncmp( "%%BoundingBox: ", line, 15 ) == 0)
        {
            printf("Read: (%s)\n", line );
            int x, y, w, h;
            sscanf(line+15, "%i %i %i %i\n", &x, &y, &w, &h);
            printf("x=%i y=%i w=%i h=%i\n", x,y,w,h);
            PdfRect rect( static_cast<double>(x), 
                          static_cast<double>(y),
                          static_cast<double>(w-x),
                          static_cast<double>(h-y) );
            
            rects.push_back( rect );
            //break;
        } 
    }

    pclose(hPipe); 
    return rects;
}

int main( int argc, char* argv[] )
{
    PdfError::EnableDebug( false );

    if( argc != 3 )
    {
        print_help();
        exit( -1 );
    }
    
    const char* pszInput = argv[1];
    const char* pszOutput = argv[2];

    try {
        printf("Cropping file:\t%s\n", pszInput);
        printf("Writing to   :\t%s\n", pszOutput);
 
        std::vector<PdfRect> cropBoxes = get_crop_boxes( pszInput );

        PdfMemDocument doc;
        doc.Load( pszInput );

        if( cropBoxes.size() != doc.GetPageCount() ) 
        {
            printf("Number of cropboxes obtained form ghostscript does not match with page count (%i, %i)\n",
                   cropBoxes.size(), doc.GetPageCount() );
        }

        for( int i=0;i<doc.GetPageCount(); i++ ) 
        {
            PdfPage* pPage = doc.GetPage( i ); 
            crop_page( pPage, cropBoxes[i] );
        }

        doc.Write( pszOutput );
        
    } catch( PdfError & e ) {
        fprintf( stderr, "Error: An error %i ocurred during croppping pages in the pdf file.\n", e.GetError() );
        e.PrintErrorMsg();
        return e.GetError();
    }
    
    return 0;
}


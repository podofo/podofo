/***************************************************************************
 *   Copyright (C) 2009 by Dominik Seichter                                *
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

using namespace PoDoFo;

#ifdef _HAVE_CONFIG
#include <config.h>
#endif // _HAVE_CONFIG

void print_help()
{
    printf("Usage: podofocountpages [-s] [-t] file1.pdf ... \n\n");
    printf("       This tool counts the pages in a PDF file.\n");
    printf("       -s will enable the short format, which ommites\n");
    printf("          printing of the filename in the output.\n");
    printf("       -t print the total sum of all pages.\n");
    printf("\nPoDoFo Version: %s\n\n", PODOFO_VERSION_STRING);
}

int count_pages( const char* pszFilename, const bool & bShortFormat ) 
{
    PdfMemDocument document;
    document.Load( pszFilename );
    int nPages = document.GetPageCount(); 

    if( bShortFormat ) 
        printf("%i\n", nPages );
    else
        printf("%s:\t%i\n", pszFilename, nPages );

    return nPages;
}

int main( int argc, char* argv[] )
{
    PdfError::EnableDebug( false );

    if( argc <= 1 )
    {
        print_help();
        exit( -1 );
    }
    
    
    try {
        bool bTotal = false;
        bool bShortFormat = false;
        int sum = 0;
        
        for(int i=1;i<argc;i++) 
        {
            const char* pszArg = argv[i];

            if( strcmp(pszArg, "-s") == 0 ) 
            {
                bShortFormat = true;
            }
            else if( strcmp(pszArg, "-t") == 0 ) 
            {
                bTotal = true;
            }
            else
            {
                sum += count_pages( pszArg, bShortFormat );
            }
        }

        if( bTotal ) 
        {
            printf("Total:\t%i\n", sum );
        }
    } catch( PdfError & e ) {
        fprintf( stderr, "Error: An error %i ocurred during counting pages in the pdf file.\n", e.GetError() );
        e.PrintErrorMsg();
        return e.GetError();
    }
    
    return 0;
}


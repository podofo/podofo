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
    printf("Usage: podofoincrementalupdates [-e N out.pdf] file.pdf\n\n");
    printf("       This tool prints information of incremental updates to file.pdf.\n");
    printf("       By default the number of incremental updates will be printed.\n");
    printf("       -e N out.pdf\n");
    printf("       Extract the Nth update from file.pdf and write it to out.pdf.\n");
    printf("\nPoDoFo Version: %s\n\n", PODOFO_VERSION_STRING);
}

int get_info( const char* pszFilename )
{
    int nUpdates = 0;

    PdfVecObjects vecObjects;
    PdfParser parser( &vecObjects, pszFilename, true );

    nUpdates = parser.GetNumberOfIncrementalUpdates();

    printf( "%s\t=\t%i\t(Number of incremental updates)\n", pszFilename, nUpdates );

    return nUpdates;
}

void extract(const char* PODOFO_UNUSED_PARAM(pszFilename), int PODOFO_UNUSED_PARAM(nExtract), const char* PODOFO_UNUSED_PARAM(pszOutputFilename))
{
    //int nUpdates = 0;

    PdfVecObjects vecObjects;
    PdfParser parser( &vecObjects );
    //parser.ParseFile( pszOutputFilename, true, nExtract );

    // TODO
    fprintf( stderr, "extraction is not implemented\n" );
    exit( -2 );
}

int main( int argc, char* argv[] )
{
    PdfError::EnableDebug( false );

    if( argc != 2 && argc != 5 )
    {
        print_help();
        exit( -1 );
    }
    
    
    try {
        const char* pszFilename;
        const char* pszOutputFilename;
        int nExtract = -1;

        if(argc == 2) 
        {
            pszFilename = argv[1];
            get_info(pszFilename);
        }
        else if(argc == 5) 
        {
            nExtract = strtol(argv[2], NULL, 10);
            pszOutputFilename = argv[3];
            pszFilename = argv[4];
            extract(pszFilename, nExtract, pszOutputFilename);
        }


    } catch( PdfError & e ) {
        fprintf( stderr, "Error: An error %i ocurred during counting pages in the pdf file.\n", e.GetError() );
        e.PrintErrorMsg();
        return e.GetError();
    }
    
    return 0;
}


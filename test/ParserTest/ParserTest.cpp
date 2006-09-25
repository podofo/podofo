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

#include "PdfError.h"
#include "PdfParser.h"
#include "PdfVecObjects.h"
#include "PdfWriter.h"

using namespace PoDoFo;

void write_back( PdfParser* pParser, const char* pszFilename )
{
    PdfWriter writer( pParser );

    //writer.SetUseXRefStream( true );
    //writer.SetLinearized( true );
    writer.SetPdfCompression( true );
    writer.Write( pszFilename );
}

int main( int argc, char*  argv[] )
{
    PdfVecObjects objects;
    PdfParser     parser( &objects );
    
    objects.SetAutoDelete( true );

    if( argc != 3 )
    {
        printf("Usage: ParserTest [input_filename] [output_filename]\n");
        return 0;
    }

    printf("This test reads a PDF file from disc and writes it to a new pdf file.\n");
    printf("The PDF file should look unmodified in any viewer\n");
    printf("---\n");

    try {
        parser.ParseFile( argv[1], false );

        printf("PdfVersion=%i\n", (int)parser.GetPdfVersion() );
        printf("PdfVersionString=%s\n", parser.GetPdfVersionString() );

        /*
        printf("=============\n");
        PdfObject* pCheat = objects.CreateObject( "Cheat" );
        std::reverse( objects.begin(), objects.end() );
        objects.RenumberObjects( const_cast<PdfObject*>(parser.GetTrailer()) );
        pCheat = objects.CreateObject("LastObject");
        printf("=============\n");
        */

        write_back( &parser, argv[2] );
    } catch( PdfError & e ) {
        e.PrintErrorMsg();
        return e.Error();
    }

    printf("Parsed and wrote successfully\n");

    return 0;
}

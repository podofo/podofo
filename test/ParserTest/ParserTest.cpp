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

#include <iostream>
#include <string>
using std::cerr;
using std::flush;
using std::endl;
using std::string;

using namespace PoDoFo;

void enc_test() 
{
    PdfString documentId;
    documentId.SetHexData( "BF37541A9083A51619AD5924ECF156DF", 32 );

    PdfEncrypt enc( "user", "podofo", 0 );
    enc.GenerateEncryptionKey( documentId );

    printf("\n\nTrying authentication!\n");

    PdfOutputDevice debug( &(std::cout) );
    printf("Debug: ");
    documentId.Write( &debug );
    printf("\n\n");
    std::string documentIdStr( documentId.GetString(), documentId.GetLength() );
    std::string password = "user";
    std::string uValue( reinterpret_cast<const char*>(enc.GetUValue()), 32 );
    std::string oValue( reinterpret_cast<const char*>(enc.GetOValue()), 32 );

    if( enc.Authenticate(documentIdStr, password,
                         uValue, oValue,
                         enc.GetPValue(), 40, 2) )
    {

        printf("Successfull\n");
    }
    else
        printf("FAILED\n");
}

void write_back( PdfParser* pParser, const char* pszFilename )
{
    enc_test();

    PdfWriter writer( pParser );
    PdfEncrypt encrypt( "user", "podofo", 0,
                        PdfEncrypt::ePdfEncryptAlgorithm_RC4V2, PdfEncrypt::ePdfKeyLength_128 );

    //writer.SetUseXRefStream( true );
    //writer.SetLinearized( true );
    writer.SetPdfVersion( ePdfVersion_1_6 );
    writer.SetEncrypted( encrypt );
    writer.Write( pszFilename );
}

int main( int argc, char*  argv[] )
{
    PdfVecObjects objects;
    PdfParser     parser( &objects );
    
    objects.SetAutoDelete( true );

    bool useDemandLoading = false;
    if ( argc >= 2 )
    {
        if (argv[1][0] == '-')
        {
            if (string("-d") == argv[1])
            {
                useDemandLoading = true;
                ++argv;
                --argc;
            }
        }
    }

    if( argc < 2 || argc > 3 )
    {
        cerr << "Usage: ParserTest [-d] <input_filename> [<output_filename>]\n"
             << "    -d       Enable demand loading of objects\n"
             << flush;
        return 0;
    }

    cerr << "This test reads a PDF file from disk and writes it to a new pdf file." << endl;
    cerr << "The PDF file should look unmodified in any viewer" << endl;
    cerr << "---" << endl;

    try {
        cerr << "Parsing  " << argv[1] << " with demand loading "
             << (useDemandLoading ? "on" : "off")
             << " ..." << flush;
        parser.ParseFile( argv[1], useDemandLoading );
        cerr << " done" << endl;

        cerr << "PdfVersion=" << parser.GetPdfVersion() << endl;
        cerr << "PdfVersionString=" << parser.GetPdfVersionString() << endl;

        /*
        cerr << "=============\n");
        PdfObject* pCheat = objects.CreateObject( "Cheat" );
        std::reverse( objects.begin(), objects.end() );
        objects.RenumberObjects( const_cast<PdfObject*>(parser.GetTrailer()) );
        pCheat = objects.CreateObject("LastObject");
        cerr << "=============\n");
        */

        if (argc == 3)
        {
            cerr << "Writing..." << flush;
            write_back( &parser, argv[2] );
            cerr << " done" << endl;
        }
    } catch( PdfError & e ) {
        e.PrintErrorMsg();
        return e.GetError();
    }

    if (argc == 3)
        cerr << "Parsed and wrote successfully" << endl;
    else
        cerr << "Parsed successfully" << endl;

    return 0;
}

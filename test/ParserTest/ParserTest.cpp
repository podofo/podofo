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

#include <podofo-base.h>

#include <iostream>
#include <string>
using std::cerr;
using std::cout;
using std::cin;
using std::flush;
using std::endl;
using std::string;

using namespace PoDoFo;

void print_help()
{
        cerr << "Usage: ParserTest [-d] [-clean] [-compact] <input_filename> [<output_filename>]\n"
             << "    -d       Enable demand loading of objects\n"
             << "    -clean   Write a clean PDF that is readable in a text editor\n"
             << "    -compact Write the PDF as compact as possible\n"
             << flush;
}

void enc_test() 
{
    /*
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

    enc.SetCurrentReference( PdfReference( 7, 0 ) );
    const char* pBuffer1 = "Somekind of drawing \001 buffer that possibly \003 could contain PDF drawing commands";
    const char* pBuffer2 = " possibly could contain PDF drawing\003  commands";
    long        lLen    = strlen( pBuffer1 ) + 2 * strlen( pBuffer2 );


    char* pEncBuffer = static_cast<char*>(malloc( sizeof(char) * lLen ));
    memcpy( pEncBuffer, pBuffer1, strlen( pBuffer1 ) * sizeof(char) );
    memcpy( pEncBuffer + strlen(pBuffer1), pBuffer2, strlen( pBuffer2 ) );
    memcpy( pEncBuffer + strlen(pBuffer1) + strlen( pBuffer2 ), pBuffer2, strlen( pBuffer2 ) );

    enc.Encrypt( reinterpret_cast<unsigned char*>(pEncBuffer), lLen );

    PdfMemoryOutputStream mem( lLen );
    PdfOutputStream* pStream = enc.CreateEncryptionOutputStream( &mem ); 
    pStream->Write( pBuffer1, strlen( pBuffer1 ) );
    pStream->Write( pBuffer2, strlen( pBuffer2 ) );
    pStream->Write( pBuffer2, strlen( pBuffer2 ) );
    pStream->Close();

    printf("Result: %i \n", memcmp( pEncBuffer, mem.TakeBuffer(), lLen ) );


    enc.Encrypt( reinterpret_cast<unsigned char*>(pEncBuffer), lLen );
    printf("Decrypted buffer: %s\n", pEncBuffer );
    */
}

void write_back( PdfParser* pParser, const char* pszFilename, EPdfWriteMode eWriteMode )
{
    //enc_test();

    PdfWriter writer( pParser );
    writer.SetWriteMode( eWriteMode );
    /*
    PdfEncrypt encrypt( "user", "podofo", 0,
                        PdfEncrypt::ePdfEncryptAlgorithm_RC4V2, PdfEncrypt::ePdfKeyLength_128 );
    */
    //writer.SetUseXRefStream( true );
    //writer.SetLinearized( true );
    writer.SetPdfVersion( ePdfVersion_1_6 );
    //writer.SetEncrypted( encrypt );
    writer.Write( pszFilename );
}

int main( int argc, char*  argv[] )
{
    PdfError::EnableLogging(true);
    PdfError::EnableDebug(true);

    PdfVecObjects objects;
    PdfParser     parser( &objects );
    EPdfWriteMode eWriteMode = ePdfWriteMode_Default;
    objects.SetAutoDelete( true );
    
    bool useDemandLoading = false;
    bool useStrictMode = false;
    const char* pszInput = NULL;
    const char* pszFilename = NULL;

    for( int i=1; i<argc; i++ ) 
    {
        if( argv[i][0] == '-' ) 
        {

            if (string("-d") == argv[i]) 
            {
                useDemandLoading = true;
            }
            else if (string("-s") == argv[1]) 
            {
                useStrictMode = true;
            } 
            else if (string("-clean") == argv[i])
            {
                eWriteMode = ePdfWriteMode_Clean;
            }
            else if (string("-compact") == argv[i])
            {
                eWriteMode = ePdfWriteMode_Compact;
            }
        }
        else
        {
            if( pszInput == NULL ) 
            {
                pszInput = argv[i];
            }
            else if( pszFilename == NULL )
            {
                pszFilename = argv[i];
            }
            else
            {
                print_help();
                return 0;
            }
        }
    }

    if( pszInput == NULL )
    {
        print_help();
        cerr << "Usage: ParserTest [-d] [-s] [-clean] [-compact] <input_filename> [<output_filename>]\n"
             << "    -d       Enable demand loading of objects\n"
             << "    -s       Enable strict parsing mode\n"
             << "    -clean   Enable clean writing mode\n"
             << "    -compact Enable compact writing mode\n"
             << flush;
        return 0;
    }

    cerr << "This test reads a PDF file from disk and writes it to a new pdf file." << endl;
    cerr << "The PDF file should look unmodified in any viewer" << endl;
    cerr << "---" << endl;
    cerr << "Number of incremental updates: " << parser.GetNumberOfIncrementalUpdates() << endl;

    try {
        cerr << "Parsing  " << pszInput << " with demand loading "
             << (useDemandLoading ? "on" : "off")
             << " with strict parsing "
             << (useStrictMode ? "on" : "off")
             << " ..." << flush;

        bool bIncorrectPw = false;
        std::string pw;
        parser.SetStrictParsing( useStrictMode );
        do {
            try {
                if( !bIncorrectPw ) 
                    parser.ParseFile( pszInput, useDemandLoading );
                else 
                    parser.SetPassword( pw );
                
                bIncorrectPw = false;
            } catch( PdfError & e ) {
                if( e.GetError() == ePdfError_InvalidPassword ) 
                {
                    cout << endl << "Password :";
                    std::getline( cin, pw );
                    cout << endl;
                    
                    // try to continue with the new password
                    bIncorrectPw = true;
                }
                else
                    throw e;
            }
        } while( bIncorrectPw );

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

        if (pszFilename)
        {
            cerr << "Writing..." << flush;
            write_back( &parser, pszFilename, eWriteMode );
            cerr << " done" << endl;
        }
    } catch( PdfError & e ) {
        e.PrintErrorMsg();
        return e.GetError();
    }

    if (pszFilename)
        cerr << "Parsed and wrote successfully" << endl;
    else
        cerr << "Parsed successfully" << endl;

    /*
    cerr << "Now with PdfMemDocument" << endl;
    PdfMemDocument document;
    document.Load( pszInput );
    document.Write( pszFilename );
    cerr << "DONE: Now with PdfMemDocument" << endl;
    */
    
    return 0;
}

/***************************************************************************
 *   Copyright (C) 2010 by Ian Ashley                                      *
 *   Ian Ashley <Ian.Ashley@opentext.com>                                  *
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

#include <iostream>
#include <cstdlib>
#include <cstdio>

#include <podofo.h>

using namespace std;
using namespace PoDoFo;

int main (int argc, char *argv[])
{
    PdfError::EnableLogging(true);
    PdfError::EnableDebug(true);

    PdfVecObjects objects;
    PdfParser     parser( &objects );
    objects.SetAutoDelete( true );

    if( argc != 3 )
    {
        cerr << "Usage: podofogc <input_filename> <output_filename>\n"
             << "    Performs garbage collection on a PDF file.\n"
             << "    All objects that are not reachable from within\n"
             << "    the trailer are deleted.\n"
             << flush;
        return 0;
    }
    

    try {
        cerr << "Parsing  " << argv[1] << " ... (this might take a while)"
             << flush;

        bool bIncorrectPw = false;
        std::string pw;
        do {
            try {
                if( !bIncorrectPw ) 
                    parser.ParseFile( argv[1], false );
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

        cerr << "Writing..." << flush;
        PdfWriter writer( &parser );
        writer.SetPdfVersion( parser.GetPdfVersion() );
        if( parser.GetEncrypted() )
        {
            writer.SetEncrypted( *(parser.GetEncrypt()) );
        }
        writer.Write( argv[2] );
        cerr << " done" << endl;
    } catch( PdfError & e ) {
        e.PrintErrorMsg();
        return e.GetError();
    }

    cerr << "Parsed and wrote successfully" << endl;
	return EXIT_SUCCESS;
}

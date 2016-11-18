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

#include "../PdfTest.h"

#include <stdio.h>

#define BUFFER_SIZE 4096

using namespace PoDoFo;

int main( int argc, char* argv[] )
{
    printf("Tokenizer Test\n");
    printf("==============\n");

    if( argc != 2 )
    {
        printf("Usage: TokenizerTest [input_filename]\n");
        return 0;
    }

    try {
        PdfRefCountedInputDevice device( argv[1], "rb" );
        PdfRefCountedBuffer      buffer( BUFFER_SIZE );
        PdfTokenizer             tokenizer( device, buffer );
        const char*              pszToken;

        while ( tokenizer.GetNextToken(pszToken) )
        {
            printf("Got token: %s\n", pszToken );
        }

    } catch( PdfError & e ) {
        e.PrintErrorMsg();
        return e.GetError();
    }

    return 0;
}

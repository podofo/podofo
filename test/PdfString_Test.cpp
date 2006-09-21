/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "PdfTest.h"

#include "PdfString.h"

using namespace PoDoFo;

int main( int argc, char* argv[] ) 
{
    try {
        char        binary[] = { 0x0a, 0xef, 0xb0, 0x69, 0x65,0xf7, 0x31, 0x45 };

        PdfString string( "Hello World!");
        PdfString hex( binary, 8, true );
        //PdfString hexPad( binary, 8, true, 30 );
        
        if( strcmp( string.String(), "Hello World!") != 0 )
        {
            RAISE_ERROR( ePdfError_TestFailed );
        }
        
        printf("string.String()=%s\n", string.String() );
        printf("string.Size()=%li\n", string.Size() );
        if( string.Size() != 13 )
        {
            RAISE_ERROR( ePdfError_TestFailed );
        }
         
        
        printf("hex.String()=%s\n", hex.String() );
        printf("hex.Size()=%li\n", hex.Size() );
        if( strcmp( hex.String(), "0AEFB06965F73145" ) != 0 )
        {
            RAISE_ERROR( ePdfError_TestFailed );
        }
        
        if( hex.Size() != 16 )
        {
            RAISE_ERROR( ePdfError_TestFailed );
        }
        
        /*
          printf("hexPad.String()=%s\n", hexPad.String() );
          printf("hexPad.Size()=%i\n", hexPad.Size() );
          if( strcmp( hexPad.String(), "0AEFB06965F7314500000000000000" ) != 0 )
          eCode.SetError( ePdfError_TestFailed, __FILE__, __LINE__  );
          
          if( hexPad.Size() != 31 )
          eCode.SetError( ePdfError_TestFailed, __FILE__, __LINE__ );
        */
        
        printf("Comparing hex and normal string\n");
        PdfString normal( " " );
        PdfString hexa(" ", 2, true );
        if( !(normal == hexa) ) 
        {
            printf("String normal: %s\n", normal.String() );
            printf("String hexa  : %s\n", hexa.String() );
            printf("Comparison failed!\n");
            RAISE_ERROR( ePdfError_TestFailed );
        }
    } catch( const PdfError & eCode ) {
        eCode.PrintErrorMsg();
        return eCode.Error();
    }
    
    printf("\nTest successfull!\n");

    return 0;
}

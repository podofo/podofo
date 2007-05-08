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

void testUnicode() 
{
    long            lBufferLen;
    const pdf_utf8* pszString  = reinterpret_cast<const pdf_utf8*>("Hallo Welt äääöööö mit Ümlaut!");
    
    long        lUtf16BufferLen = 128;
    pdf_utf16be pUtf16Buffer[lUtf16BufferLen];

    long     lUtf8BufferLen = 128;
    pdf_utf8 pUtf8Buffer[lUtf8BufferLen];

    lBufferLen = strlen( reinterpret_cast<const char*>(pszString) );

    printf("Converting UTF8 -> UTF16\n");
    lUtf16BufferLen = PdfString::ConvertUTF8toUTF16( pszString, lBufferLen, pUtf16Buffer, lUtf16BufferLen );

    printf("Converting UTF16 -> UTF8\n");
    lUtf8BufferLen = PdfString::ConvertUTF16toUTF8( pUtf16Buffer, lUtf16BufferLen, pUtf8Buffer, lUtf8BufferLen  );

    printf("Original Length: %li\n", lBufferLen );
    printf("UTF16 Length   : %li\n", lUtf16BufferLen );
    printf("UTF8  Length   : %li\n", lUtf8BufferLen );

    if( strcmp( reinterpret_cast<const char*>(pszString), reinterpret_cast<const char*>(pUtf8Buffer) ) != 0 ) 
    {
        printf("Error during comparing\n");
        printf("Original : %s\n", pszString );
        printf("Converted: %s\n", reinterpret_cast<const char*>(pUtf8Buffer) );

        PODOFO_RAISE_ERROR( ePdfError_TestFailed );

    }
}

int main()
{
    try {
        char        binary[] = { 0x0a, 0xef, 0xb0, 0x69, 0x65,0xf7, 0x31, 0x45 };

        PdfString string( "Hello World!");
        PdfString hex( binary, 8, true );
        //PdfString hexPad( binary, 8, true, 30 );
        
        if( strcmp( string.GetString(), "Hello World!") != 0 )
        {
            PODOFO_RAISE_ERROR( ePdfError_TestFailed );
        }
        
        printf("string.String()=%s\n", string.GetString() );
        printf("string.Size()=%li\n", string.GetSize() );
        if( string.GetSize() != 13 )
        {
            PODOFO_RAISE_ERROR( ePdfError_TestFailed );
        }
         
        
        printf("hex.String()=%s\n", hex.GetString() );
        printf("hex.Size()=%li\n", hex.GetSize() );
        if( strcmp( hex.GetString(), "0AEFB06965F73145" ) != 0 )
        {
            PODOFO_RAISE_ERROR( ePdfError_TestFailed );
        }
        
        if( hex.GetSize() != 16 )
        {
            PODOFO_RAISE_ERROR( ePdfError_TestFailed );
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
            printf("String normal: %s\n", normal.GetString() );
            printf("String hexa  : %s\n", hexa.GetString() );
            printf("Comparison failed!\n");
            PODOFO_RAISE_ERROR( ePdfError_TestFailed );
        }

        PdfString a("aaaaa");
        PdfString b("b");

        if( !(a < b) )
        {
            printf("Comparison failed a < b !\n");
        }

        if( !(b > a) )
        {
            printf("Comparison failed b > a !\n");
        }

        testUnicode();

    } catch( const PdfError & eCode ) {
        eCode.PrintErrorMsg();
        return eCode.GetError();
    }
    
    printf("\nTest successfull!\n");

    return 0;
}

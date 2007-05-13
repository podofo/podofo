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

void testUnicodeString( const pdf_utf8* pszString, long lBufferLen )
{
    
    long        lUtf16BufferLen = 256;
    pdf_utf16be pUtf16Buffer[lUtf16BufferLen];

    long     lUtf8BufferLen = 256;
    pdf_utf8 pUtf8Buffer[lUtf8BufferLen];

    lBufferLen = strlen( reinterpret_cast<const char*>(pszString) );

    printf("Converting UTF8 -> UTF16: lBufferLen=%i\n", lBufferLen);
    lUtf16BufferLen = PdfString::ConvertUTF8toUTF16( pszString, lBufferLen, pUtf16Buffer, lUtf16BufferLen );

    printf("Converting UTF16 -> UTF8: lBufferLen=%i\n", lBufferLen);
    lUtf8BufferLen = PdfString::ConvertUTF16toUTF8( pUtf16Buffer, lUtf16BufferLen, pUtf8Buffer, lUtf8BufferLen  );

    printf("Original Length: %li\n", lBufferLen );
    printf("UTF16 Length   : %li\n", lUtf16BufferLen );
    printf("UTF8  Length   : %li\n", lUtf8BufferLen );
    printf("Original String: %s\n", reinterpret_cast<const char*>(pszString) );
    //wprintf(L"UTF16 String   : %s\n", reinterpret_cast<const wchar_t*>(pUtf16Buffer) );
    printf("UTF8  String   : %s\n", reinterpret_cast<const char*>(pUtf8Buffer) );

    if( strcmp( reinterpret_cast<const char*>(pszString), reinterpret_cast<const char*>(pUtf8Buffer) ) != 0 ) 
    {
        printf("Error during comparing\n");
        printf("Original : %s\n", pszString );
        printf("Converted: %s\n", reinterpret_cast<const char*>(pUtf8Buffer) );

        PODOFO_RAISE_ERROR( ePdfError_TestFailed );

    }
}

void testUnicode() 
{
    printf("\nUnicode conversion tests:\n\n");

    const char* pszString = "String with German Umlauts: Hallo schöne Welt: äöüÄÖÜß€\n";
    
    testUnicodeString( reinterpret_cast<const pdf_utf8*>(pszString), strlen( pszString ) );

    const char* pszStringJap = "「PoDoFo」は今から日本語も話せます。";
    testUnicodeString( reinterpret_cast<const pdf_utf8*>(pszStringJap), strlen( pszStringJap ) );


    PdfString simple("Hallo World");
    PdfString unicode = simple.ToUnicode();

    long     lUtf8BufferLen = 256;
    pdf_utf8 pUtf8Buffer[lUtf8BufferLen];
    lUtf8BufferLen = PdfString::ConvertUTF16toUTF8( unicode.GetUnicode(), unicode.GetUnicodeLength(), 
                                                    pUtf8Buffer, lUtf8BufferLen  );
    printf("Utf8: %s\n", pUtf8Buffer );

}


void testString( const char* pszString, const PdfString & str, const PdfString & hex ) 
{
    printf("\t->Got string: %s\n", pszString );
    printf("\t->    length: %i\n", strlen( pszString ) );

    if( strcmp( str.GetString(), pszString ) != 0 )
    {
        printf("Strings are not equal!\n");
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }
    

    if( str.GetLength() != strlen( pszString ) + 1 ) 
    {
        printf("Strings length is not equal!\n");
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }        

    PdfString hex2 = str.HexEncode();
    PdfString str2 = hex.HexDecode();

    if( !(hex2 == str) )
    {
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    if( !(hex == str2) )
    {
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    if( hex2 < str || hex2 > str ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    if( hex < str2 || hex > str2 ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }
}

void testHexEncodeDecode() 
{
    printf("\nHex conversion tests:\n\n");

    char      helloBar[] = { 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x21 };
    PdfString helloStr("Hello World!");
    PdfString helloBin( helloBar, 12, true );

    testString( "Hello World!", helloStr, helloBin );
}

int main()
{
    try {
        testUnicode();
        testHexEncodeDecode();

    } catch( const PdfError & eCode ) {
        eCode.PrintErrorMsg();
        return eCode.GetError();
    }
    
    printf("\nTest successfull!\n");

    return 0;
}

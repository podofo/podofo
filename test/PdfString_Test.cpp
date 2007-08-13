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
#include "PdfOutputDevice.h"

#include <ostream>
#include <sstream>

using namespace PoDoFo;

void testUnicodeString( const pdf_utf8* pszString, long lBufferLen )
{
    
    const long        lUtf16BufferLen = 256;
    pdf_utf16be pUtf16Buffer[lUtf16BufferLen];

    const long     lUtf8BufferLen = 256;
    pdf_utf8 pUtf8Buffer[lUtf8BufferLen];

    lBufferLen = strlen( reinterpret_cast<const char*>(pszString) );

    printf("Converting UTF8 -> UTF16: lBufferLen=%li\n", lBufferLen);
    const long lUtf16BufferLenUsed = 
		PdfString::ConvertUTF8toUTF16( pszString, lBufferLen, pUtf16Buffer, lUtf16BufferLen );

    printf("Converting UTF16 -> UTF8: lBufferLen=%li\n", lBufferLen);
    const long lUtf8BufferLenUsed =
		PdfString::ConvertUTF16toUTF8( pUtf16Buffer, lUtf16BufferLenUsed, pUtf8Buffer, lUtf8BufferLen  );

    printf("Original Length: %li\n", lBufferLen );
    printf("UTF16 Length   : %li\n", lUtf16BufferLenUsed );
    printf("UTF8  Length   : %li\n", lUtf8BufferLenUsed );
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

    // UTF-8 encoded data; make sure your editor is set up correctly!
    const char* pszString = "String with German Umlauts: Hallo schöne Welt: äöüÄÖÜß€\n";
    
    testUnicodeString( reinterpret_cast<const pdf_utf8*>(pszString), strlen( pszString ) );

    // UTF-8 encoded Japanese; make sure your editor is set up correctly and you have suitable fonts!
    const char* pszStringJap = "「PoDoFo」は今から日本語も話せます。";
    testUnicodeString( reinterpret_cast<const pdf_utf8*>(pszStringJap), strlen( pszStringJap ) );


    PdfString simple("Hallo World");
    PdfString unicode = simple.ToUnicode();

    const long     lUtf8BufferLen = 256;
    pdf_utf8 pUtf8Buffer[lUtf8BufferLen];
    const long lUtf8BufferLenUsed =
        PdfString::ConvertUTF16toUTF8( unicode.GetUnicode(), unicode.GetUnicodeLength(), 
                                       pUtf8Buffer, lUtf8BufferLen  );
    printf("Utf8: %s\n", pUtf8Buffer );


    char* pBuffer = static_cast<char*>(malloc( unicode.GetLength() + 2 ));
    pBuffer[0] = 0xFE;
    pBuffer[1] = 0xFF;
    memcpy( pBuffer+2, unicode.GetString(), unicode.GetLength() );

    PdfString unicodeHex( pBuffer, unicode.GetLength() + 2, true );
    printf("Hexdata: %s\n", unicodeHex.GetString() );
    printf("IsUnicode: %i\n", unicodeHex.IsUnicode() );
    free( pBuffer );
}


void testString( const char* pszString, const PdfString & str, const PdfString & hex ) 
{
    printf("\t->    Got string: %s\n", pszString );
    printf("\t-> ... of length: %li\n", strlen( pszString ) );
    printf("\t-> Got PdfString: %s\n", str.GetString() );
    printf("\t-> ... of length: %li\n", str.GetLength() );
    printf("\t-> Got hexstring: %s\n", hex.GetString() );
    printf("\t-> ... of length: %li\n", hex.GetLength() );

    if( strcmp( str.GetString(), pszString ) != 0 )
    {
        printf("Strings are not equal!\n");
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    if( static_cast<size_t>(str.GetLength()) != strlen( pszString ) + 1 ) 
    {
        printf("Strings length is not equal!\n");
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    if ( str.GetLength() != hex.GetLength() )
    {
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    if( strcmp( str.GetString(), hex.GetString() ) != 0 ) 
    {
        printf("Str: %s\n", str.GetString() );
        printf("Hex: %s\n", hex.GetString() );

        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    if( hex < str || hex > str ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    if( hex < str || hex > str ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    if ( ! (hex == str) )
    {
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    if ( hex != str )
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


    PdfString helloHex;
    helloHex.SetHexData( "48656c6c6f 20576f726c6421" );

    testString( "Hello World!", helloStr, helloHex );

}

void testEscape() 
{
    printf("\nEscaping tests:\n\n");

    std::ostringstream oss;
    PdfOutputDevice    out( &oss);

    PdfString str( "Hello (cruel) World" );
    const char* pszExpected = "(Hello \\(cruel\\) World)";
    out.Seek( 0 );
    str.Write( &out, NULL );
    if( strcmp( oss.str().c_str(), pszExpected ) != 0 ) 
    {
        printf( "Expected: %s\n", pszExpected );
        printf( "Got     : %s\n", oss.str().c_str() );

        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    str         = PdfString( "Path: C:\\Temp\\out.pdf" );
    pszExpected = "(Path: C:\\\\Temp\\\\out.pdf)";
    out.Seek( 0 );
    str.Write( &out, NULL );
    
    if( strcmp( oss.str().c_str(), pszExpected ) != 0 ) 
    {
        printf( "Expected: %s\n", pszExpected );
        printf( "Got     : %s\n", oss.str().c_str() );

        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }
}

int main()
{
    // The following text is encoded as UTF-8 literals. Your editor must be configured
    // to treat this file as UTF-8, and must have a suitable Japanese font, to correctly
    // display the following.
    const char* pszStringJap = "「PoDoFo」は今から日本語も話せます。";
    printf("Jap: %s\n", pszStringJap );

    try {
        testUnicode();
        testHexEncodeDecode();
        testEscape();

    } catch( const PdfError & eCode ) {
        eCode.PrintErrorMsg();
        return eCode.GetError();
    }
    
    printf("\nTest successfull!\n");

    return 0;
}

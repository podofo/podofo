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

#include <stdlib.h>
#include <string.h>

using namespace PoDoFo;

#if _MSC_VER
// Visual Studo warns about truncation of the following values. We know, and don't care.
#pragma warning( push )
#pragma warning( disable: 4309 )
#endif

// The same string as a NULL-terminated UTF-8 string. This is a UTF-8 literal, so your editor
// must be configured to handle this file as UTF-8 to see something sensible below.
const char* pszStringJapUtf8 = "「PoDoFo」は今から日本語も話せます。";
// and a UTF-16 BE encoded char array w/o NULL terminator
const char psStringJapUtf16BE[44] = { 0xfe, 0xff, 0x30, 0x0c, 0x00, 0x50, 0x00,
    0x6f, 0x00, 0x44, 0x00, 0x6f, 0x00, 0x46, 0x00, 0x6f, 0x30, 0xd0, 0x30,
    0x6f, 0x4e, 0xca, 0x30, 0x4b, 0x30, 0x89, 0x65, 0xe5, 0x67, 0x2c, 0x8a,
    0x9e, 0x30, 0x82, 0x8a, 0x71, 0x30, 0x5b, 0x30, 0x7e, 0x30, 0x59, 0x30,
    0x02 };

// Some accented chars within the latin-1-with-euro range (UTF-8 encoded in the source)
const char* pszStringUmlUtf8 = "String with German Umlauts: Hallo schöne Welt: äöüÄÖÜß€\n";
// The same string in PdfDoc encoding - see PDF Reference Section D.1 Latin Character Set and Encodings
const char* pszStringUmlPdfDoc = "String with German Umlauts: Hallo sch\366ne Welt: \344\366\374\304\326\334\337\240\n";
// and the same string in UTF-16 BE
const char psStringUmlUtf16BE[114] = { 0xfe, 0xff, 0x00, 0x53, 0x00, 0x74,
    0x00, 0x72, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x67, 0x00, 0x20, 0x00, 0x77,
    0x00, 0x69, 0x00, 0x74, 0x00, 0x68, 0x00, 0x20, 0x00, 0x47, 0x00, 0x65,
    0x00, 0x72, 0x00, 0x6d, 0x00, 0x61, 0x00, 0x6e, 0x00, 0x20, 0x00, 0x55,
    0x00, 0x6d, 0x00, 0x6c, 0x00, 0x61, 0x00, 0x75, 0x00, 0x74, 0x00, 0x73,
    0x00, 0x3a, 0x00, 0x20, 0x00, 0x48, 0x00, 0x61, 0x00, 0x6c, 0x00, 0x6c,
    0x00, 0x6f, 0x00, 0x20, 0x00, 0x73, 0x00, 0x63, 0x00, 0x68, 0x00, 0xf6,
    0x00, 0x6e, 0x00, 0x65, 0x00, 0x20, 0x00, 0x57, 0x00, 0x65, 0x00, 0x6c,
    0x00, 0x74, 0x00, 0x3a, 0x00, 0x20, 0x00, 0xe4, 0x00, 0xf6, 0x00, 0xfc,
    0x00, 0xc4, 0x00, 0xd6, 0x00, 0xdc, 0x00, 0xdf, 0x20, 0xac, 0x00, 0x0a};

#if _MSC_VER
#pragma warning( pop )
#endif

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

    testUnicodeString( reinterpret_cast<const pdf_utf8*>(pszStringUmlUtf8), strlen( pszStringUmlUtf8 ) );
    testUnicodeString( reinterpret_cast<const pdf_utf8*>(pszStringJapUtf8), strlen( pszStringJapUtf8 ) );

    PdfString simple("Hallo World");
    PdfString unicode = simple.ToUnicode();

    const long     lUtf8BufferLen = 256;
    pdf_utf8 pUtf8Buffer[lUtf8BufferLen];
    const long lUtf8BufferLenUsed =
        PdfString::ConvertUTF16toUTF8( unicode.GetUnicode(), unicode.GetUnicodeLength(), 
                                       pUtf8Buffer, lUtf8BufferLen  );
    printf("Utf8: %s\n", pUtf8Buffer );


    char* pBuffer = static_cast<char*>(malloc( unicode.GetLength() + 2 ));
    pBuffer[0] = '\xFE';
    pBuffer[1] = '\xFF';
    memcpy( pBuffer+2, unicode.GetString(), unicode.GetLength() );

    PdfString unicodeHex( pBuffer, unicode.GetLength() + 2, true );
    printf("Hexdata: %s\n", unicodeHex.GetString() );
    printf("IsUnicode: %i\n", unicodeHex.IsUnicode() );
    free( pBuffer );

    // Test automatic UTF16BE encoding detection
    PdfString fromUml16BE( psStringUmlUtf16BE, sizeof(psStringUmlUtf16BE), false );
    if (!fromUml16BE.IsUnicode())
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    // Make sure PdfDoc strings are not interpreted as UTF16BE
    PdfString fromUmlPdfDoc( pszStringUmlPdfDoc, strlen(pszStringUmlPdfDoc), false);
    if (fromUmlPdfDoc.IsUnicode())
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    // and ensure that the two representations of the same thing compare equal
    if (fromUml16BE != fromUmlPdfDoc)
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    // TODO: ensure exact char count match

    // Also make sure that another UTF16BE string is detected correctly.
    // We can't compare against PdfDoc strings for this since there is no
    // equivalent PdfDoc string.
    PdfString fromJap16BE( psStringJapUtf16BE, sizeof(psStringJapUtf16BE), false);
    if (!fromJap16BE.IsUnicode())
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
}


void testString( const char* pszString, const PdfString & str, const PdfString & hex ) 
{
    printf("\t->    Got string: %s\n", pszString );
    printf("\t-> ... of length: %li\n", static_cast<long>(strlen( pszString )) );
    printf("\t-> Got PdfString: %s\n", str.GetString() );
    printf("\t-> ... of length: %li\n", str.GetLength() );
    printf("\t-> Got hexstring: %s\n", hex.GetString() );
    printf("\t-> ... of length: %li\n", hex.GetLength() );

    if( strcmp( str.GetString(), pszString ) != 0 )
    {
        printf("Strings are not equal!\n");
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    if( static_cast<size_t>(str.GetLength()) != strlen( pszString ) ) 
    {
        printf("Strings length is not equal: %li vs %li!\n", str.GetLength(), static_cast<long>(strlen( pszString )) );
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
    printf("\nHex conversion tests:\n");
    printf("ASCII input:\n");

    char      helloBar[12] = { 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x21 }; // "Hello World!" sans null terminator
    PdfString helloStr("Hello World!");
    PdfString helloBin( helloBar, 12, true );

    testString( "Hello World!", helloStr, helloBin );

    printf("Hex input:\n");
    PdfString helloHex;
    helloHex.SetHexData( "48656c6c6f 20576f726c6421" );

    testString( "Hello World!", helloStr, helloHex );

    if ( PdfString("fred", 4, false) != PdfString("fred", 4, true) )
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
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

    printf("Escaping tests done\n");
}

void testLengthCompare()
{
    char ps1[8] = {'f','r','e','d','f','r','e','d'};
    PdfString s1(ps1,8,false);
    PdfString s2(ps1,4,false);
    if (s1 == s2)
        // whoops - prefix equality test!
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );

    PdfString us1( s1.ToUnicode() );
    PdfString us2( s2.ToUnicode() );
    if (us1 == us2)
        // whoops - prefix equality test!
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
}

int main()
{
    // The following will only print correctly if your output device
    // is expecting UTF-8 encoded data.
    printf("UTF-8 Jap: %s\n", pszStringJapUtf8 );

    try {
        testUnicode();
        testHexEncodeDecode();
        testEscape();
        testLengthCompare();

    } catch( const PdfError & eCode ) {
        eCode.PrintErrorMsg();
        return eCode.GetError();
    }
    
    printf("\nTests successful!\n");

    return 0;
}

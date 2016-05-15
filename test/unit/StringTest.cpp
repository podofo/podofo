/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
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

#include "StringTest.h"

#include <podofo.h>

#ifndef __clang__

using namespace PoDoFo;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( StringTest );

inline std::ostream& operator<<(std::ostream& o, const PdfString& s)
{
    return o << s.GetStringUtf8();
}

void StringTest::setUp()
{
}

void StringTest::tearDown()
{
}

void StringTest::testGetStringUtf8()
{
    const std::string src1 = "Hello World!";
    const std::string src2 = src1;
    const std::string src3 = "「Po\tDoFo」は今から日本語も話せます。";

    // Normal ascii string should be converted to UTF8
    PdfString str1( src1.c_str() );
    std::string res1 = str1.GetStringUtf8();

    CPPUNIT_ASSERT_EQUAL_MESSAGE( "testing const char* ASCII -> UTF8", src1, res1 );

    // Normal std::string string should be converted to UTF8
    PdfString str2( src2 );
    std::string res2 = str2.GetStringUtf8();

    CPPUNIT_ASSERT_EQUAL_MESSAGE( "testing std::string ASCII -> UTF8", src2, res2 );

    // UTF8 data in std::string cannot be converted as we do not know it is UTF8
    PdfString str3( src3 );
    std::string res3 = str3.GetStringUtf8();

    CPPUNIT_ASSERT_EQUAL_MESSAGE( "testing std::string UTF8 -> UTF8", (res3 != src3) , true );

    // UTF8 data as pdf_utf8* must be convertible
    PdfString str4( reinterpret_cast<const pdf_utf8*>(src3.c_str()) );
    std::string res4 = str4.GetStringUtf8();

    CPPUNIT_ASSERT_EQUAL_MESSAGE( "testing pdf_utf8* UTF8 -> UTF8", res4, src3 );    
}

void StringTest::testUtf16beContructor()
{
    const char* pszStringJapUtf8 = "「PoDoFo」は今から日本語も話せます。";
    // The same string as a NULL-terminated UTF-8 string. This is a UTF-8 literal, so your editor
    // must be configured to handle this file as UTF-8 to see something sensible below.
    // The same string in UTF16BE encoding
    const char psStringJapUtf16BE[44] = { 0x30, 0x0c, 0x00, 0x50, 0x00, 0x6f, 
                                          0x00, 0x44, 0x00, 0x6f, 0x00, 0x46, 
                                          0x00, 0x6f, 0x30, 0x0d, 0x30, 0x6f, 
                                          0x4e, static_cast<char>(0xca), 0x30, 0x4b, 0x30, static_cast<char>(0x89), 
                                          0x65, static_cast<char>(0xe5), 0x67, 0x2c, static_cast<char>(0x8a), static_cast<char>(0x9e), 
                                          0x30, static_cast<char>(0x82), static_cast<char>(0x8a), static_cast<char>(0x71), 0x30, 0x5b, 
                                          0x30, static_cast<char>(0x7e), 0x30, 0x59, 0x30, 0x02, 
                                          0x00, 0x00 };

    PdfString strUtf8( reinterpret_cast<const pdf_utf8*>(pszStringJapUtf8) );
    PdfString strUtf16( reinterpret_cast<const pdf_utf16be*>(psStringJapUtf16BE), 21 );
    PdfString strUtf16b( reinterpret_cast<const pdf_utf16be*>(psStringJapUtf16BE), 21 );

    /*
    std::cout << std::endl;
    std::cout << "utf8 :" << strUtf8 << "  " << strUtf8.GetCharacterLength() << std::endl;
    std::cout << "utf16:" << strUtf16 << "  " << strUtf16.GetCharacterLength() <<  std::endl;
    std::cout << "wide : ";
    for( int i=0;i<=strUtf16.GetCharacterLength();i++ )
        printf("%04x ", strUtf16.GetUnicode()[i]);
    std::cout << std::endl;


    std::cout << "wide : ";
    for( int i=0;i<=strUtf16.GetCharacterLength();i++ )
        printf("%4i ", i );
    std::cout << std::endl;
    */
    
    // Compare UTF16 to UTF8 string
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "Comparing string length", 
                                  strUtf8.GetCharacterLength(), strUtf16.GetCharacterLength() );

    CPPUNIT_ASSERT_EQUAL_MESSAGE( "Comparing UTF8 and UTF16 string converted to UTF8", 
                                  strUtf8.GetStringUtf8(), strUtf16.GetStringUtf8() );

    CPPUNIT_ASSERT_EQUAL_MESSAGE( "Comparing UTF8 and UTF16 string", strUtf8, strUtf16 );

    // Compare two UTF16 strings
    CPPUNIT_ASSERT_EQUAL( strUtf16.GetCharacterLength(), strUtf16b.GetCharacterLength() );
    CPPUNIT_ASSERT_EQUAL( strUtf16.GetStringUtf8(), strUtf16b.GetStringUtf8() );
    CPPUNIT_ASSERT_EQUAL( strUtf16, strUtf16b );

}

void StringTest::testWCharConstructor()
{
    CPPUNIT_ASSERT_EQUAL( PdfString("Hallo World"), PdfString(L"Hallo World") );
    CPPUNIT_ASSERT_EQUAL( PdfString(L"Hallo World"), PdfString(L"Hallo World") );
}

void StringTest::testEscapeBrackets()
{
    // Test balanced brackets ansi
    const char* pszAscii       = "Hello (balanced) World";
    const char* pszAsciiExpect = "(Hello \\(balanced\\) World)";

    PdfString   sAscii( pszAscii );
    PdfVariant  varAscii( sAscii );
    std::string strAscii;
    varAscii.ToString( strAscii );

    CPPUNIT_ASSERT_EQUAL( strAscii == pszAsciiExpect, true );

    // Test un-balanced brackets ansi
    const char* pszAscii2       = "Hello ((unbalanced World";
    const char* pszAsciiExpect2 = "(Hello \\(\\(unbalanced World)";

    PdfString   sAscii2( pszAscii2 );
    PdfVariant  varAscii2( sAscii2 );
    std::string strAscii2;
    varAscii2.ToString( strAscii2 );

    CPPUNIT_ASSERT_EQUAL( strAscii2 == pszAsciiExpect2, true );

    // Test balanced brackets unicode
    const char* pszUnic       = "Hello (balanced) World";
    const char pszUnicExpect[]= { 0x28, static_cast<char>(0xFE), static_cast<char>(0xFF), 0x00, 0x48, 0x00, 0x65, 0x00, 
                                  0x6C, 0x00, 0x6C, 0x00, 0x6F, 0x00, 0x20, 0x00, 
                                  0x5C, 0x28, 0x00, 0x62, 0x00, 0x61, 0x00, 0x6C, 
                                  0x00, 0x61, 0x00, 0x6E, 0x00, 0x63, 0x00, 0x65, 
                                  0x00, 0x64, 0x00, 0x5C, 0x29, 0x00, 0x20, 0x00, 
                                  0x57, 0x00, 0x6F, 0x00, static_cast<char>(0x72), 0x00, 0x6C, 0x00, 
                                  0x64, 0x29, 0x00, 0x00 };
    
    // Force unicode string
    PdfString   sUnic( reinterpret_cast<const pdf_utf8*>(pszUnic) );
    PdfVariant  varUnic( sUnic );
    std::string strUnic;
    varUnic.ToString( strUnic );

    CPPUNIT_ASSERT_EQUAL( memcmp( strUnic.c_str(), pszUnicExpect, strUnic.length() ) == 0, true );

    // Test un-balanced brackets unicode
    const char* pszUnic2       = "Hello ((unbalanced World";
    const char pszUnicExpect2[]= { 0x28, static_cast<char>(0xFE), static_cast<char>(0xFF), 0x00, 0x48, 0x00, 0x65, 0x00, 
                                   0x6C, 0x00, 0x6C, 0x00, 0x6F, 0x00, 0x20, 0x00, 
                                   0x5C, 0x28, 0x00, 0x5C, 0x28, 0x00, 0x75, 0x00, 
                                   0x6E, 0x00, 0x62, 0x00, 0x61, 0x00, 0x6C, 0x00, 
                                   0x61, 0x00, 0x6E, 0x00, 0x63, 0x00, 0x65, 0x00, 
                                   0x64, 0x00, 0x20, 0x00, 0x57, 0x00, 0x6F, 0x00, 
                                   0x72, 0x00, 0x6C, 0x00, 0x64, 0x29 };
    
    // Force unicode string
    PdfString   sUnic2( reinterpret_cast<const pdf_utf8*>(pszUnic2) );
    PdfVariant  varUnic2( sUnic2 );
    std::string strUnic2;
    varUnic2.ToString( strUnic2 );

    CPPUNIT_ASSERT_EQUAL( memcmp( strUnic2.c_str(), pszUnicExpect2, strUnic2.length() ) == 0, true );

    // Test reading the unicode string back in
    PdfVariant varRead;
    PdfTokenizer tokenizer( strUnic2.c_str(), strUnic2.length() );
    tokenizer.GetNextVariant( varRead, NULL );
    CPPUNIT_ASSERT_EQUAL( varRead.GetString() == sUnic2, true );
}

void StringTest::testWriteEscapeSequences()
{
    TestWriteEscapeSequences("(Hello\\nWorld)", "(Hello\\nWorld)");
    TestWriteEscapeSequences("(Hello\nWorld)", "(Hello\\nWorld)");
    TestWriteEscapeSequences("(Hello\012World)", "(Hello\\nWorld)");
    TestWriteEscapeSequences("(Hello\\012World)", "(Hello\\nWorld)");

    TestWriteEscapeSequences("(Hello\\rWorld)", "(Hello\\rWorld)");
    TestWriteEscapeSequences("(Hello\rWorld)", "(Hello\\rWorld)");
    TestWriteEscapeSequences("(Hello\015World)", "(Hello\\rWorld)");
    TestWriteEscapeSequences("(Hello\\015World)", "(Hello\\rWorld)");

    TestWriteEscapeSequences("(Hello\\tWorld)", "(Hello\\tWorld)");
    TestWriteEscapeSequences("(Hello\tWorld)", "(Hello\\tWorld)");
    TestWriteEscapeSequences("(Hello\011World)", "(Hello\\tWorld)");
    TestWriteEscapeSequences("(Hello\\011World)", "(Hello\\tWorld)");

    TestWriteEscapeSequences("(Hello\\fWorld)", "(Hello\\fWorld)");
    TestWriteEscapeSequences("(Hello\fWorld)", "(Hello\\fWorld)");
    TestWriteEscapeSequences("(Hello\014World)", "(Hello\\fWorld)");
    TestWriteEscapeSequences("(Hello\\014World)", "(Hello\\fWorld)");

    TestWriteEscapeSequences("(Hello\\(World)", "(Hello\\(World)");
    TestWriteEscapeSequences("(Hello\\050World)", "(Hello\\(World)");

    TestWriteEscapeSequences("(Hello\\)World)", "(Hello\\)World)");
    TestWriteEscapeSequences("(Hello\\051World)", "(Hello\\)World)");

    TestWriteEscapeSequences("(Hello\\\\World)", "(Hello\\\\World)");
    TestWriteEscapeSequences("(Hello\\\134World)", "(Hello\\\\World)");

    // Special case, \ at end of line
    TestWriteEscapeSequences("(Hello\\\nWorld)", "(HelloWorld)");


    TestWriteEscapeSequences("(Hello\003World)", "(Hello\003World)");
}

void StringTest::TestWriteEscapeSequences(const char* pszSource, const char* pszExpected)
{
    PdfVariant  variant;
    std::string ret;
    std::string expected = pszExpected;

    printf("Testing with value: %s\n", pszSource );
    PdfTokenizer tokenizer( pszSource, strlen( pszSource ) );

    tokenizer.GetNextVariant( variant, NULL );
    CPPUNIT_ASSERT_EQUAL( variant.GetDataType(), ePdfDataType_String );

    variant.ToString( ret );
    printf("   -> Convert To String: %s\n", ret.c_str() );

    CPPUNIT_ASSERT_EQUAL( expected, ret );

}

void StringTest::testEmptyString()
{
    const char* pszEmpty = "";
    std::string sEmpty;
    std::string sEmpty2( pszEmpty );

    PdfString str1;
    PdfString str2( sEmpty );
    PdfString str3( sEmpty2 );
    PdfString str4( pszEmpty );
    PdfString str5( pszEmpty, 0, false );
    PdfString str6( reinterpret_cast<const pdf_utf8*>(pszEmpty) );
    PdfString str7( reinterpret_cast<const pdf_utf8*>(pszEmpty), 0 );
    PdfString str8( reinterpret_cast<const pdf_utf16be*>(L""), 0 );

    CPPUNIT_ASSERT( !str1.IsValid() );

    CPPUNIT_ASSERT( str2.IsValid() );
    CPPUNIT_ASSERT_EQUAL( static_cast<pdf_long>(0), str2.GetLength() );
    CPPUNIT_ASSERT_EQUAL( static_cast<pdf_long>(0), str2.GetCharacterLength() );
    CPPUNIT_ASSERT_EQUAL( std::string(), str2.GetStringUtf8() );
    CPPUNIT_ASSERT_EQUAL( std::string(""), str2.GetStringUtf8() );

    CPPUNIT_ASSERT( str3.IsValid() );
    CPPUNIT_ASSERT_EQUAL( static_cast<pdf_long>(0), str3.GetLength() );
    CPPUNIT_ASSERT_EQUAL( static_cast<pdf_long>(0), str3.GetCharacterLength() );
    CPPUNIT_ASSERT_EQUAL( std::string(), str3.GetStringUtf8() );
    CPPUNIT_ASSERT_EQUAL( std::string(""), str3.GetStringUtf8() );

    CPPUNIT_ASSERT( str4.IsValid() );
    CPPUNIT_ASSERT_EQUAL( static_cast<pdf_long>(0), str4.GetLength() );
    CPPUNIT_ASSERT_EQUAL( static_cast<pdf_long>(0), str4.GetCharacterLength() );
    CPPUNIT_ASSERT_EQUAL( std::string(), str4.GetStringUtf8() );
    CPPUNIT_ASSERT_EQUAL( std::string(""), str4.GetStringUtf8() );

    CPPUNIT_ASSERT( str5.IsValid() );
    CPPUNIT_ASSERT_EQUAL( static_cast<pdf_long>(0), str5.GetLength() );
    CPPUNIT_ASSERT_EQUAL( static_cast<pdf_long>(0), str5.GetCharacterLength() );
    CPPUNIT_ASSERT_EQUAL( std::string(), str5.GetStringUtf8() );
    CPPUNIT_ASSERT_EQUAL( std::string(""), str5.GetStringUtf8() );
    
    CPPUNIT_ASSERT( str6.IsValid() );
    CPPUNIT_ASSERT_EQUAL( static_cast<pdf_long>(0), str6.GetLength() );
    CPPUNIT_ASSERT_EQUAL( static_cast<pdf_long>(0), str6.GetCharacterLength() );
    CPPUNIT_ASSERT_EQUAL( std::string(), str6.GetStringUtf8() );
    CPPUNIT_ASSERT_EQUAL( std::string(""), str6.GetStringUtf8() );

    CPPUNIT_ASSERT( str7.IsValid() );
    CPPUNIT_ASSERT_EQUAL( static_cast<pdf_long>(0), str7.GetLength() );
    CPPUNIT_ASSERT_EQUAL( static_cast<pdf_long>(0), str7.GetCharacterLength() );
    CPPUNIT_ASSERT_EQUAL( std::string(), str7.GetStringUtf8() );
    CPPUNIT_ASSERT_EQUAL( std::string(""), str7.GetStringUtf8() );

    CPPUNIT_ASSERT( str8.IsValid() );
    CPPUNIT_ASSERT_EQUAL( static_cast<pdf_long>(0), str8.GetLength() );
    CPPUNIT_ASSERT_EQUAL( static_cast<pdf_long>(0), str8.GetCharacterLength() );
    CPPUNIT_ASSERT_EQUAL( std::string(), str8.GetStringUtf8() );
    CPPUNIT_ASSERT_EQUAL( std::string(""), str8.GetStringUtf8() );

}

void StringTest::testInitFromUtf8()
{
    const char* pszUtf8 = "This string contains UTF-8 Characters: ÄÖÜ.";
    const PdfString str( reinterpret_cast<const pdf_utf8*>(pszUtf8) );
    
    CPPUNIT_ASSERT_EQUAL( true, str.IsUnicode() );
    CPPUNIT_ASSERT_EQUAL( static_cast<pdf_long>(43*2), str.GetLength() );
    CPPUNIT_ASSERT_EQUAL( static_cast<pdf_long>(43), str.GetCharacterLength() );
    CPPUNIT_ASSERT_EQUAL( std::string(pszUtf8), str.GetStringUtf8() );
    
}

#endif // __clang__

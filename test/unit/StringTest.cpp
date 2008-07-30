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
                                          0x4e, 0xca, 0x30, 0x4b, 0x30, 0x89, 
                                          0x65, 0xe5, 0x67, 0x2c, 0x8a, 0x9e, 
                                          0x30, 0x82, 0x8a, 0x71, 0x30, 0x5b, 
                                          0x30, 0x7e, 0x30, 0x59, 0x30, 0x02, 
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
}

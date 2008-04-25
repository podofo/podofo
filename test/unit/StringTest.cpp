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

    CPPUNIT_ASSERT_EQUAL_MESSAGE( "testing const char* ASCII -> UTF8", res1, src1 );

    // Normal std::string string should be converted to UTF8
    PdfString str2( src2 );
    std::string res2 = str2.GetStringUtf8();

    CPPUNIT_ASSERT_EQUAL_MESSAGE( "testing std::string ASCII -> UTF8", res2, src2 );

    // UTF8 data in std::string cannot be converted as we do not know it is UTF8
    PdfString str3( src3 );
    std::string res3 = str3.GetStringUtf8();

    CPPUNIT_ASSERT_EQUAL_MESSAGE( "testing std::string UTF8 -> UTF8", (res3 != src3) , true );

    // UTF8 data as pdf_utf8* must be convertible
    PdfString str4( reinterpret_cast<const pdf_utf8*>(src3.c_str()) );
    std::string res4 = str4.GetStringUtf8();

    CPPUNIT_ASSERT_EQUAL_MESSAGE( "testing pdf_utf8* UTF8 -> UTF8", res4, src3 );

    
}

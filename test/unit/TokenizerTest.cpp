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

#include "TokenizerTest.h"

#include <cppunit/Asserter.h>

using namespace PoDoFo;

CPPUNIT_TEST_SUITE_REGISTRATION( TokenizerTest );

void TokenizerTest::Test( const char* pszString, EPdfDataType eDataType, const char* pszExpected )
{
    PdfVariant  variant;
    std::string ret;
    std::string expected;

    expected = pszExpected ? pszExpected : pszString;

    printf("Testing with value: %s\n", pszString );
    PdfTokenizer tokenizer( pszString, strlen( pszString ) );

    tokenizer.GetNextVariant( variant, NULL );

    printf("   -> Expected Datatype: %i\n", eDataType );
    printf("   -> Got      Datatype: %i\n", variant.GetDataType() );
    CPPUNIT_ASSERT_EQUAL( variant.GetDataType(), eDataType );

    variant.ToString( ret );
    printf("   -> Convert To String: %s\n", ret.c_str() );

    CPPUNIT_ASSERT_EQUAL( expected, ret );
}

void TokenizerTest::setUp()
{

}

void TokenizerTest::tearDown()
{

}

void TokenizerTest::testString()
{
    // testing strings
    Test( "(Hallo Welt!)", ePdfDataType_String );
    Test( "(Hallo \\(schöne\\) Welt!)", ePdfDataType_String );
    Test( "(Balanced () brackets are (ok ()) in PDF Strings)", ePdfDataType_String,
                        "(Balanced \\(\\) brackets are \\(ok \\(\\)\\) in PDF Strings)" );
    Test( "()", ePdfDataType_String );
    Test( "(Test: \\064)", ePdfDataType_String, "(Test: \064)" );
    Test( "(Test: \\0645)", ePdfDataType_String, "(Test: 45)" );
    Test( "(Test: \\478)", ePdfDataType_String, "(Test: '8)" );
}

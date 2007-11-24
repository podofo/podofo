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

#include "NameTest.h"

#include <podofo.h>

using namespace PoDoFo;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( NameTest );

void NameTest::setUp()
{
}

void NameTest::tearDown()
{
}

void NameTest::testNameEncoding()
{
    // Test some names. The first argument is the unencoded representation, the second
    // is the expected encoded result. The result must not only be /a/ correct encoded
    // name for the unencoded form, but must be the exact one PoDoFo should produce.
    TestName( "Length With Spaces", "Length#20With#20Spaces" );
    TestName( "Length\001\002\003Spaces\177",  "Length#01#02#03Spaces#7F" );
    TestName( "Length#01#02#03Spaces#7F", "Length#2301#2302#2303Spaces#237F" );
    TestName( "Tab\tTest", "Tab#09Test" );
}

void NameTest::testEncodedNames()
{
    // Test some pre-encoded names. The first argument is the encoded name that'll be
    // read from the PDF; the second is the expected representation.
    TestEncodedName( "PANTONE#205757#20CV", "PANTONE 5757 CV");
    TestEncodedName( "paired#28#29parentheses", "paired()parentheses");
    TestEncodedName( "The_Key_of_F#23_Minor", "The_Key_of_F#_Minor");
    TestEncodedName( "A#42", "AB");
}

void NameTest::testEquality()
{
    // Make sure differently encoded names compare equal if their decoded values
    // are equal.
    TestNameEquality( "With Spaces", "With#20Spaces" );
    TestNameEquality( "#57#69#74#68#20#53#70#61#63#65#73", "With#20Spaces" );
}

//
// Test encoding of names.
// pszString : internal representation, ie unencoded name
// pszExpectedEncoded: the encoded string PoDoFo should produce
//
void NameTest::TestName( const char* pszString, const char* pszExpectedEncoded ) 
{
    printf("Testing name: %s\n", pszString );

    PdfName name( pszString );
    printf("   -> Expected   Value: %s\n", pszExpectedEncoded );
    printf("   -> Got        Value: %s\n", name.GetEscapedName().c_str() );
    printf("   -> Unescaped  Value: %s\n", name.GetName().c_str() );

    CPPUNIT_ASSERT_EQUAL( strcmp( pszExpectedEncoded, name.GetEscapedName().c_str() ), 0 );

    // Ensure the encoded string compares equal to its unencoded
    // variant
    CPPUNIT_ASSERT_EQUAL( name == PdfName::FromEscaped(pszExpectedEncoded), true );
}

void NameTest::TestEncodedName( const char* pszString, const char* pszExpected ) 
{
    PdfName name( PdfName::FromEscaped(pszString) );
    printf("Testing encoded name: %s\n", pszString );
    printf("   -> Expected   Value: %s\n", pszExpected );
    printf("   -> Got        Value: %s\n", name.GetName().c_str() );
    printf("   -> Escaped    Value: %s\n", name.GetEscapedName().c_str() );

    if ( strcmp( pszExpected, name.GetName().c_str() ) != 0 )
    {
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    // Ensure the name compares equal with one constructed from the
    // expected unescaped form
    CPPUNIT_ASSERT_EQUAL( name == PdfName(pszExpected), true );
}

void NameTest::TestNameEquality( const char * pszName1, const char* pszName2 )
{
    PdfName name1( PdfName::FromEscaped(pszName1) );
    PdfName name2( PdfName::FromEscaped(pszName2) );

    printf("Testing equality of encoded names '%s' and '%s'\n", pszName1, pszName2);
    printf("   -> Name1    Decoded Value: %s\n", name1.GetName().c_str());
    printf("   -> Name2    Decoded Value: %s\n", name2.GetName().c_str());

    CPPUNIT_ASSERT_EQUAL( name1 == name2, true  ); // use operator==
    CPPUNIT_ASSERT_EQUAL( name1 != name2, false ); // use operator!=
}


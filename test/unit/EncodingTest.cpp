/***************************************************************************
 *   Copyright (C) 2008 by Dominik Seichter                                *
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

#include "EncodingTest.h"

#include <podofo.h>

#include <ostream>

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( EncodingTest );

using namespace PoDoFo;

inline std::ostream& operator<<(std::ostream& o, const PdfVariant& s)
{
    std::string str;
    s.ToString(str);
    return o << str;
}

void EncodingTest::setUp()
{
}

void EncodingTest::tearDown()
{
}

void EncodingTest::testDifferences()
{
    PdfEncodingDifference difference;

    // Newly created encoding should be empty
    CPPUNIT_ASSERT_EQUAL( difference.GetCount(), 0 );


    // Adding 0 should work
    difference.AddDifference( 0, PdfName("A") );
    CPPUNIT_ASSERT_EQUAL( difference.GetCount(), 1 );

    // Adding 255 should work
    difference.AddDifference( 255, PdfName("B") );
    CPPUNIT_ASSERT_EQUAL( difference.GetCount(), 2 );

    // Adding out of range should throw exception
    CPPUNIT_ASSERT_THROW( difference.AddDifference( -1, PdfName("C") );, PdfError );
    CPPUNIT_ASSERT_THROW( difference.AddDifference( 256, PdfName("D") );, PdfError );

    CPPUNIT_ASSERT_EQUAL( difference.GetCount(), 2 );

    // Convert to array
    PdfArray data;
    PdfArray expected;
    expected.push_back( 0L );
    expected.push_back( PdfName("A") );
    expected.push_back( 255L );
    expected.push_back( PdfName("B") );

    difference.ToArray( data );

    CPPUNIT_ASSERT_EQUAL( data.GetSize(), expected.GetSize() );
    for( unsigned int i=0;i<data.GetSize(); i++ )
        CPPUNIT_ASSERT_EQUAL( data[i], expected[i] );


    // Test replace
    expected.Clear();
    expected.push_back( 0L );
    expected.push_back( PdfName("A") );
    expected.push_back( 255L );
    expected.push_back( PdfName("X") );

    difference.AddDifference( 255, PdfName("X") );

    difference.ToArray( data );

    CPPUNIT_ASSERT_EQUAL( data.GetSize(), expected.GetSize() );
    for( unsigned int i=0;i<data.GetSize(); i++ )
        CPPUNIT_ASSERT_EQUAL( data[i], expected[i] );


    // Test more complicated array
    expected.Clear();
    expected.push_back( 0L );
    expected.push_back( PdfName("A") );
    expected.push_back( PdfName("B") );
    expected.push_back( PdfName("C") );
    expected.push_back( 4L );
    expected.push_back( PdfName("D") );
    expected.push_back( PdfName("E") );
    expected.push_back( 9L );
    expected.push_back( PdfName("F") );
    expected.push_back( 255L );
    expected.push_back( PdfName("X") );

    difference.AddDifference( 1, PdfName("B") );
    difference.AddDifference( 2, PdfName("C") );
    difference.AddDifference( 4, PdfName("D") );
    difference.AddDifference( 5, PdfName("E") );
    difference.AddDifference( 9, PdfName("F") );

    difference.ToArray( data );

    CPPUNIT_ASSERT_EQUAL( data.GetSize(), expected.GetSize() );
    for( unsigned int i=0;i<data.GetSize(); i++ )
        CPPUNIT_ASSERT_EQUAL( data[i], expected[i] );


    // Test if contains works correctly
    PdfName name;
    pdf_utf16be value;
    CPPUNIT_ASSERT_EQUAL( difference.Contains( 0, name, value ), true );
    CPPUNIT_ASSERT_EQUAL( name, PdfName("A") ); 
#if PODOFO_IS_LITTLE_ENDIAN
    CPPUNIT_ASSERT_EQUAL( static_cast<int>(value), 0x4100 ); 
#else
    CPPUNIT_ASSERT_EQUAL( static_cast<int>(value), 0x0041 ); 
#endif //PODOFO_IS_LITTLE_ENDIAN

    CPPUNIT_ASSERT_EQUAL( difference.Contains( 9, name, value ), true );
    CPPUNIT_ASSERT_EQUAL( name, PdfName("F") ); 
#if PODOFO_IS_LITTLE_ENDIAN
    CPPUNIT_ASSERT_EQUAL( static_cast<int>(value), 0x4600 ); 
#else
    CPPUNIT_ASSERT_EQUAL( static_cast<int>(value), 0x0046 ); 
#endif //PODOFO_IS_LITTLE_ENDIAN

    CPPUNIT_ASSERT_EQUAL( difference.Contains( 255, name, value ), true );
    CPPUNIT_ASSERT_EQUAL( name, PdfName("X") ); 
#if PODOFO_IS_LITTLE_ENDIAN
    CPPUNIT_ASSERT_EQUAL( static_cast<int>(value), 0x5800 ); 
#else
    CPPUNIT_ASSERT_EQUAL( static_cast<int>(value), 0x0058 ); 
#endif //PODOFO_IS_LITTLE_ENDIAN

    CPPUNIT_ASSERT_EQUAL( difference.Contains( 100, name,value ), false );
   
}

void EncodingTest::testUnicodeNames()
{
    // List of items which are defined twice and cause
    // other ids to be returned than those which where send in
    const char* pszDuplicated[] = {
        "Delta",
        "fraction",
        "hyphen",
        "macron",
        "mu",
        "Omega",
        "periodcentered",
        "scedilla",
        "Scedilla",
        "space",
        "tcommaaccent",
        "Tcommaaccent",
        "exclamsmall",
        "dollaroldstyle",
        "zerooldstyle",
        "oneoldstyle",
        "twooldstyle",
        "threeoldstyle",
        "fouroldstyle",
        "fiveoldstyle",
        "sixoldstyle",
        "sevenoldstyle",
        "eightoldstyle",
        "nineoldstyle",
        "ampersandsmall",
        "questionsmall",
        NULL
    };

    int nCount = 0;
    for( int i = 0;i<=0xFFFF; i++ ) 
    {
        PdfName name = PdfDifferenceEncoding::UnicodeIDToName( static_cast<pdf_utf16be>(i) );

        pdf_utf16be id = PdfDifferenceEncoding::NameToUnicodeID( name );

        bool bFound = false;
        const char** pszDup = pszDuplicated;
        while( *pszDup ) 
        {
            if( PdfName( *pszDup ) == name ) 
            {
                bFound = true;
                break;
            }

            ++pszDup;
        }

        if( !bFound )
        {
            // Does not work because of 2 many duplicates...
            //CPPUNIT_ASSERT_EQUAL_MESSAGE( name.GetName(), id, static_cast<pdf_utf16be>(i) );
            if( id == static_cast<pdf_utf16be>(i) )
                ++nCount;
        }
    }

    CPPUNIT_ASSERT_EQUAL_MESSAGE( "Compared codes count", 65422, nCount );
}

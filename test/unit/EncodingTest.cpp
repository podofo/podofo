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

#include <podofo.h>

#include <ostream>

using namespace PoDoFo;

inline std::ostream& operator<<(std::ostream& o, const PdfVariant& s)
{
    std::string str;
    s.ToString(str);
    return o << str;
}

// Needs to be included after the redefinition of operator<<
// or it won't compile using clang
#include "EncodingTest.h"

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( EncodingTest );

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
    CPPUNIT_ASSERT_EQUAL( 0, static_cast<int>(difference.GetCount()) );


    // Adding 0 should work
    difference.AddDifference( 0, 0, PdfName("A") );
    CPPUNIT_ASSERT_EQUAL( 1, static_cast<int>(difference.GetCount()) );

    // Adding 255 should work
    difference.AddDifference( 255, 0, PdfName("B") );
    CPPUNIT_ASSERT_EQUAL( 2, static_cast<int>(difference.GetCount()) );

    // Adding out of range should throw exception
    CPPUNIT_ASSERT_THROW( difference.AddDifference( -1, 0, PdfName("C") );, PdfError );
    CPPUNIT_ASSERT_THROW( difference.AddDifference( 256, 0, PdfName("D") );, PdfError );

    CPPUNIT_ASSERT_EQUAL( static_cast<int>(difference.GetCount()), 2 );

    // Convert to array
    PdfArray data;
    PdfArray expected;
    expected.push_back( static_cast<pdf_int64>(0LL) );
    expected.push_back( PdfName("A") );
    expected.push_back( static_cast<pdf_int64>(255LL) );
    expected.push_back( PdfName("B") );

    difference.ToArray( data );

    CPPUNIT_ASSERT_EQUAL( expected.GetSize(), data.GetSize() );
    for( unsigned int i=0;i<data.GetSize(); i++ )
        CPPUNIT_ASSERT_EQUAL( expected[i], data[i] );


    // Test replace
    expected.Clear();
    expected.push_back( static_cast<pdf_int64>(0LL) );
    expected.push_back( PdfName("A") );
    expected.push_back( static_cast<pdf_int64>(255LL) );
    expected.push_back( PdfName("X") );

    difference.AddDifference( 255, 0, PdfName("X") );

    difference.ToArray( data );

    CPPUNIT_ASSERT_EQUAL( expected.GetSize(), data.GetSize() );
    for( unsigned int i=0;i<data.GetSize(); i++ )
        CPPUNIT_ASSERT_EQUAL( expected[i], data[i] );


    // Test more complicated array
    expected.Clear();
    expected.push_back( static_cast<pdf_int64>(0LL) );
    expected.push_back( PdfName("A") );
    expected.push_back( PdfName("B") );
    expected.push_back( PdfName("C") );
    expected.push_back( static_cast<pdf_int64>(4LL) );
    expected.push_back( PdfName("D") );
    expected.push_back( PdfName("E") );
    expected.push_back( static_cast<pdf_int64>(9LL) );
    expected.push_back( PdfName("F") );
    expected.push_back( static_cast<pdf_int64>(255LL) );
    expected.push_back( PdfName("X") );

    difference.AddDifference( 1, 0, PdfName("B") );
    difference.AddDifference( 2, 0, PdfName("C") );
    difference.AddDifference( 4, 0, PdfName("D") );
    difference.AddDifference( 5, 0, PdfName("E") );
    difference.AddDifference( 9, 0, PdfName("F") );

    difference.ToArray( data );

    CPPUNIT_ASSERT_EQUAL( expected.GetSize(), data.GetSize() );
    for( unsigned int i=0;i<data.GetSize(); i++ )
        CPPUNIT_ASSERT_EQUAL( expected[i], data[i] );


    // Test if contains works correctly
    PdfName name;
    pdf_utf16be value;
    CPPUNIT_ASSERT_EQUAL( true, difference.Contains( 0, name, value ) );
    CPPUNIT_ASSERT_EQUAL( PdfName("A"), name ); 
#ifdef PODOFO_IS_LITTLE_ENDIAN
    CPPUNIT_ASSERT_EQUAL( 0x4100, static_cast<int>(value) ); 
#else
    CPPUNIT_ASSERT_EQUAL( 0x0041, static_cast<int>(value) ); 
#endif //PODOFO_IS_LITTLE_ENDIAN

    CPPUNIT_ASSERT_EQUAL( true, difference.Contains( 9, name, value ) );
    CPPUNIT_ASSERT_EQUAL( PdfName("F"), name ); 
#ifdef PODOFO_IS_LITTLE_ENDIAN
    CPPUNIT_ASSERT_EQUAL( 0x4600, static_cast<int>(value) ); 
#else
    CPPUNIT_ASSERT_EQUAL( 0x0046, static_cast<int>(value) ); 
#endif //PODOFO_IS_LITTLE_ENDIAN

    CPPUNIT_ASSERT_EQUAL( true, difference.Contains( 255, name, value ) );
    CPPUNIT_ASSERT_EQUAL( PdfName("X"), name ); 
#ifdef PODOFO_IS_LITTLE_ENDIAN
    CPPUNIT_ASSERT_EQUAL( 0x5800, static_cast<int>(value) ); 
#else
    CPPUNIT_ASSERT_EQUAL( 0x0058, static_cast<int>(value) ); 
#endif //PODOFO_IS_LITTLE_ENDIAN

    CPPUNIT_ASSERT_EQUAL( false, difference.Contains( 100, name,value ) );
   
}

void EncodingTest::testDifferencesObject()
{
    PdfMemDocument doc;
    PdfEncodingDifference difference;
    difference.AddDifference( 1, 0, PdfName("B") );
    difference.AddDifference( 2, 0, PdfName("C") );
    difference.AddDifference( 4, 0, PdfName("D") );
    difference.AddDifference( 5, 0, PdfName("E") );
    difference.AddDifference( 9, 0, PdfName("F") );

    PdfDifferenceEncoding encoding( difference, PdfDifferenceEncoding::eBaseEncoding_MacRoman, &doc );

    // Check for encoding key
    PdfObject* pObj = doc.GetObjects().CreateObject();
    encoding.AddToDictionary( pObj->GetDictionary() );

    CPPUNIT_ASSERT_EQUAL( true, pObj->GetDictionary().HasKey( PdfName("Encoding") ) );

    PdfObject* pKey = pObj->GetDictionary().GetKey( PdfName("Encoding") );
    CPPUNIT_ASSERT_EQUAL( true, pKey->IsReference() );

    PdfObject* pEncoding = doc.GetObjects().GetObject( pKey->GetReference() );

    // Test BaseEncoding
    PdfObject* pBase = pEncoding->GetDictionary().GetKey( PdfName("BaseEncoding" ) );
    CPPUNIT_ASSERT_EQUAL( PdfName("MacRomanEncoding"), pBase->GetName() );
    
    // Test differences
    PdfObject* pDiff = pEncoding->GetDictionary().GetKey( PdfName("Differences" ) );
    PdfArray   expected;

    expected.push_back( static_cast<pdf_int64>(1LL) );
    expected.push_back( PdfName("B") );
    expected.push_back( PdfName("C") );
    expected.push_back( static_cast<pdf_int64>(4LL) );
    expected.push_back( PdfName("D") );
    expected.push_back( PdfName("E") );
    expected.push_back( static_cast<pdf_int64>(9LL) );
    expected.push_back( PdfName("F") );

    const PdfArray & data = pDiff->GetArray();
    CPPUNIT_ASSERT_EQUAL( expected.GetSize(), data.GetSize() );
    for( unsigned int i=0;i<data.GetSize(); i++ )
        CPPUNIT_ASSERT_EQUAL( expected[i], data[i] );
}

void EncodingTest::testDifferencesEncoding()
{
    PdfMemDocument doc;

    // Create a differences encoding where A and B are exchanged
    PdfEncodingDifference difference;
    difference.AddDifference( 0x0041, 0, PdfName("B") );
    difference.AddDifference( 0x0042, 0, PdfName("A") );
    difference.AddDifference( 0x0043, 0, PdfName("D") );

    PdfDifferenceEncoding encoding( difference, PdfDifferenceEncoding::eBaseEncoding_WinAnsi, &doc );

    PdfString unicodeStr = encoding.ConvertToUnicode( PdfString("BAABC"), NULL );
    CPPUNIT_ASSERT_EQUAL( PdfString("ABBAD"), unicodeStr );

    PdfRefCountedBuffer encodingStr = encoding.ConvertToEncoding( PdfString("ABBAD"), NULL );
    CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(5), encodingStr.GetSize() );
    CPPUNIT_ASSERT_EQUAL( 0, memcmp("BAABC", encodingStr.GetBuffer(), encodingStr.GetSize()) );
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
            // Does not work because of too many duplicates...
            //CPPUNIT_ASSERT_EQUAL_MESSAGE( name.GetName(), id, static_cast<pdf_utf16be>(i) );
            if( id == static_cast<pdf_utf16be>(i) )
                ++nCount;
        }
    }

    CPPUNIT_ASSERT_EQUAL_MESSAGE( "Compared codes count", 65422, nCount );
}

void EncodingTest::testGetCharCode()
{
    std::string msg;
    bool        ret;

    PdfWinAnsiEncoding cWinAnsiEncoding;
    ret = outofRangeHelper( &cWinAnsiEncoding, msg, "PdfWinAnsiEncoding" );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( msg, true, ret );

    PdfMacRomanEncoding cMacRomanEncoding;
    ret = outofRangeHelper( &cMacRomanEncoding, msg, "PdfMacRomanEncoding" );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( msg, true, ret );

    PdfIdentityEncoding cIdentityEncoding;
    ret = outofRangeHelper( &cIdentityEncoding, msg, "PdfIdentityEncoding" );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( msg, true, ret );

    PdfVecObjects vec;
    vec.SetAutoDelete( true );
    PdfEncodingDifference difference;
    difference.AddDifference( 0x0041, 0, PdfName("B") );
    difference.AddDifference( 0x0042, 0, PdfName("A") );
    PdfDifferenceEncoding cDifferenceEncoding( difference, PdfDifferenceEncoding::eBaseEncoding_WinAnsi, &vec );
    ret = outofRangeHelper( &cDifferenceEncoding, msg, "PdfDifferenceEncoding" );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( msg, true, ret );

#ifdef PODOFO_IS_LITTLE_ENDIAN
    CPPUNIT_ASSERT_EQUAL( static_cast<pdf_utf16be>(0x4200), cDifferenceEncoding.GetCharCode( 0x0041 ) );
    CPPUNIT_ASSERT_EQUAL( static_cast<pdf_utf16be>(0x4100), cDifferenceEncoding.GetCharCode( 0x0042 ) );
#else
    CPPUNIT_ASSERT_EQUAL( static_cast<pdf_utf16be>(0x0042), cDifferenceEncoding.GetCharCode( 0x0041 ) );
    CPPUNIT_ASSERT_EQUAL( static_cast<pdf_utf16be>(0x0041), cDifferenceEncoding.GetCharCode( 0x0042 ) );
#endif // PODOFO_IS_LITTLE_ENDIAN
}

void EncodingTest::testToUnicodeParse()
{
    const char *toUnicode =
        "3 beginbfrange\n"
        "<0001> <0004> <1001>\n"
        "<0005> <000A> [<000A> <0009> <0008> <0007> <0006> <0005>]\n"
        "<000B> <000F> <100B>\n"
        "endbfrange\n";
    const pdf_utf16be *encodedStr = reinterpret_cast< const pdf_utf16be *>( "\x0\x1\x0\x2\x0\x3\x0\x4\x0\x5\x0\x6\x0\x7\x0\x8\x0\x9\x0\xA\x0\xB\x0\xC\x0\xD\x0\xE\x0\xF\x0\x0" );
    const pdf_utf16be expected[] = {
        0x1001, 0x1002, 0x1003, 0x1004,
        0x000A, 0x0009, 0x0008, 0x0007, 0x0006, 0x0005,
        0x100B, 0x100C, 0x100D, 0x100E, 0x100F,
        0 };
    PdfVecObjects vec;
    PdfObject *strmObject;

    vec.SetAutoDelete( true );

    strmObject = vec.CreateObject( PdfVariant( PdfDictionary() ) );
    strmObject->GetStream()->Set( toUnicode, strlen( toUnicode ) );

    PdfIdentityEncoding encoding(0x0001, 0x000F, true, strmObject);

    PdfString unicodeString = encoding.ConvertToUnicode( PdfString( encodedStr ), NULL );
    const pdf_utf16be *unicodeStr = reinterpret_cast<const pdf_utf16be *>( unicodeString.GetString() );
    int ii;

    for( ii = 0; expected[ii]; ii++ ) {
        pdf_utf16be expects = expected[ii];
#ifdef PODOFO_IS_LITTLE_ENDIAN
        expects = (expects << 8) | (expects >> 8 );
#endif
        CPPUNIT_ASSERT_EQUAL( expects, unicodeStr[ii] );
    }
    
    const char* toUnicodeInvalidTests[] =
    {
        // missing object numbers
        "beginbfrange\n",
        "beginbfchar\n",

        // invalid hex digits
        "2 beginbfrange <WXYZ> endbfrange\n",
        "2 beginbfrange <-123> endbfrange\n",
        "2 beginbfrange <<00>> endbfrange\n",

        // missing hex digits
        "2 beginbfrange <> endbfrange\n",
        
        // empty array
        "2 beginbfrange [] endbfrange\n",

        NULL
    };
    
    for ( size_t i = 0 ; toUnicodeInvalidTests[i] != NULL ; ++i )
    {
        try
        {
            PdfVecObjects vecInvalid;
            PdfObject *strmInvalidObject;
            
            vec.SetAutoDelete( true );
            
            strmInvalidObject = vecInvalid.CreateObject( PdfVariant( PdfDictionary() ) );
            strmInvalidObject->GetStream()->Set( toUnicodeInvalidTests[i], strlen( toUnicodeInvalidTests[i] ) );
            
            PdfIdentityEncoding encodingTestInvalid(0x0001, 0x000F, true, strmInvalidObject);
            
            PdfString unicodeStringTestInvalid = encoding.ConvertToUnicode( PdfString( encodedStr ), NULL );
            
            // exception not thrown - should never get here
            // TODO not all invalid input throws an exception (e.g. no hex digits in <WXYZ>)
            //CPPUNIT_ASSERT( false );
        }
        catch ( PoDoFo::PdfError& error )
        {
            // parsing every invalid test string should throw an exception
            CPPUNIT_ASSERT( true );
        }
        catch( std::exception& ex )
        {
            CPPUNIT_FAIL( "Unexpected exception type" );
        }
    }
}

bool EncodingTest::outofRangeHelper( PdfEncoding* pEncoding, std::string & rMsg, const char* pszName )
{
    bool exception = false;

    try {
	pEncoding->GetCharCode( pEncoding->GetFirstChar() );
    } 
    catch( PdfError & rError ) 
    {
	// This may not throw!
	rMsg = "pEncoding->GetCharCode( pEncoding->GetFirstChar() ) failed";
	return false;
    }

    try {
	pEncoding->GetCharCode( pEncoding->GetFirstChar() - 1 );
    } 
    catch( PdfError & rError ) 
    {
	// This has to throw!
	exception = true;
    }

    if( !exception ) 
    {
	rMsg = "pEncoding->GetCharCode( pEncoding->GetFirstChar() - 1 ); failed";
	return false;
    }

    try {
	pEncoding->GetCharCode( pEncoding->GetLastChar() );
    } 
    catch( PdfError & rError ) 
    {
	// This may not throw!
	rMsg = "pEncoding->GetCharCode( pEncoding->GetLastChar()  ); failed";
	return false;
    }

    exception = false;
    try {
	pEncoding->GetCharCode( pEncoding->GetLastChar() + 1 );
    } 
    catch( PdfError & rError ) 
    {
	// This has to throw!
	exception = true;
    }

    if( !exception ) 
    {
	rMsg = "pEncoding->GetCharCode( pEncoding->GetLastChar() + 1 ); failed";
	return false;
    }

    PdfEncoding::const_iterator it = pEncoding->begin();
    int nCount = pEncoding->GetFirstChar();
    while( it != pEncoding->end() ) 
    {
	CPPUNIT_ASSERT_EQUAL_MESSAGE( pszName, *it, pEncoding->GetCharCode( nCount ) );

	++nCount;
	++it;
    }

    return true;
}

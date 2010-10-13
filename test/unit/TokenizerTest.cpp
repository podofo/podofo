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
    // Nothing todo here
}

void TokenizerTest::tearDown()
{
    // Nothing todo here
}

void TokenizerTest::testArrays()
{
    Test( "[]", ePdfDataType_Array, "[ ]" );
    Test( "[ ]", ePdfDataType_Array );
    Test( "[ / ]", ePdfDataType_Array, "[ / ]" ); // empty names are legal, too!
    Test( "[ / [ ] ]", ePdfDataType_Array, "[ / [ ] ]" ); // empty names are legal, too!
    Test( "[/[]]", ePdfDataType_Array, "[ / [ ] ]" ); // empty names are legal, too!
    Test( "[ 1 2 3 4 ]", ePdfDataType_Array );
    Test( "[1 2 3 4]", ePdfDataType_Array, "[ 1 2 3 4 ]" );
    Test( "[ 2 (Hallo Welt!) 3.500000 /FMC ]", ePdfDataType_Array );
    Test( "[ [ 1 2 ] (Hallo Welt!) 3.500000 /FMC ]", ePdfDataType_Array );
    Test( "[/ImageA/ImageB/ImageC]", ePdfDataType_Array, "[ /ImageA /ImageB /ImageC ]" );
    Test( "[<530464995927cef8aaf46eb953b93373><530464995927cef8aaf46eb953b93373>]", ePdfDataType_Array, "[ <530464995927CEF8AAF46EB953B93373> <530464995927CEF8AAF46EB953B93373> ]" );
    Test( "[ 2 0 R (Test Data) 4 << /Key /Data >> 5 0 R ]", ePdfDataType_Array, "[ 2 0 R (Test Data) 4 <<\n/Key /Data\n>> 5 0 R ]" );
    Test( "[<</key/name>>2 0 R]", ePdfDataType_Array, "[ <<\n/key /name\n>> 2 0 R ]" );
    Test( "[<<//name>>2 0 R]", ePdfDataType_Array,"[ <<\n/ /name\n>> 2 0 R ]" );
    Test( "[ 27.673200 27.673200 566.256000 651.295000 ]", ePdfDataType_Array );
}

void TokenizerTest::testBool()
{
    Test( "false", ePdfDataType_Bool);
    Test( "true", ePdfDataType_Bool);
}

void TokenizerTest::testHexString()
{
    Test( "<FFEB0400A0CC>", ePdfDataType_HexString );
    Test( "<FFEB0400A0C>", ePdfDataType_HexString, "<FFEB0400A0C0>" );
    Test( "<>", ePdfDataType_HexString );
}

void TokenizerTest::testName()
{
    Test( "/Type", ePdfDataType_Name );
    Test( "/Length", ePdfDataType_Name );
    Test( "/Adobe#20Green", ePdfDataType_Name );
    Test( "/$$", ePdfDataType_Name );
    Test( "/1.2", ePdfDataType_Name );
    Test( "/.notdef", ePdfDataType_Name );
    Test( "/@pattern", ePdfDataType_Name );
    Test( "/A;Name_With-Various***Characters?", ePdfDataType_Name );
    Test( "/", ePdfDataType_Name ); // empty names are legal, too!

    // Some additional tests, which cause errors for Sebastian Loch
    
    const char* pszString = "/CheckBox#C3#9Cbersetzungshinweis";
    PdfVariant variant;
    PdfTokenizer tokenizer( pszString, strlen( pszString ) );
    tokenizer.GetNextVariant( variant, NULL );

    PdfName name2( variant.GetName() );


    std::ostringstream oss;
    PdfOutputDevice output(&oss);
    name2.Write(&output, ePdfWriteMode_Default);

    CPPUNIT_ASSERT_EQUAL( variant.GetName().GetName(), name2.GetName() );
    CPPUNIT_ASSERT_EQUAL( oss.str(), std::string(pszString) );

    printf("!!! Name=[%s]\n", variant.GetName().GetName().c_str() );
    printf("!!! Name2=[%s]\n", name2.GetName().c_str() );
    printf("!!! oss=[%s]\n", oss.str().c_str() );
}

void TokenizerTest::testNull()
{
    Test( "null", ePdfDataType_Null );
}

void TokenizerTest::testNumbers()
{
    Test( "145", ePdfDataType_Number );
    Test( "-12", ePdfDataType_Number );    
    Test( "3.141230", ePdfDataType_Real );
    Test( "-2.970000", ePdfDataType_Real );
    Test( "0", ePdfDataType_Number );
    Test( "4.", ePdfDataType_Real, "4.000000" );

}

void TokenizerTest::testReference()
{
    Test( "2 0 R", ePdfDataType_Reference );
    Test( "3 0 R", ePdfDataType_Reference );
    Test( "4 1 R", ePdfDataType_Reference );
}

void TokenizerTest::testString()
{
    // testing strings
    Test( "(Hallo Welt!)", ePdfDataType_String );
    Test( "(Hallo \\(schöne\\) Welt!)", ePdfDataType_String );
    Test( "(Balanced () brackets are (ok ()) in PDF Strings)", ePdfDataType_String,
                        "(Balanced \\(\\) brackets are \\(ok \\(\\)\\) in PDF Strings)" );
    Test( "()", ePdfDataType_String );
    
    // Test octal strings
    Test( "(Test: \\064)", ePdfDataType_String, "(Test: \064)" );
    Test( "(Test: \\064\\064)", ePdfDataType_String, "(Test: \064\064)" );
    Test( "(Test: \\0645)", ePdfDataType_String, "(Test: 45)" );
    Test( "(Test: \\478)", ePdfDataType_String, "(Test: '8)" );

    // Test line breaks 
    Test( "(Hallo\nWelt!)", ePdfDataType_String, "(Hallo\\nWelt!)" );
    Test( "(These \\\ntwo strings \\\nare the same.)", ePdfDataType_String, 
	  "(These two strings are the same.)" );

    // Test escape sequences
    Test( "(Hallo\\nWelt!)", ePdfDataType_String, "(Hallo\\nWelt!)" );
    Test( "(Hallo\\rWelt!)", ePdfDataType_String, "(Hallo\\rWelt!)" );
    Test( "(Hallo\\tWelt!)", ePdfDataType_String, "(Hallo\\tWelt!)" );
    Test( "(Hallo\\bWelt!)", ePdfDataType_String, "(Hallo\\bWelt!)" );
    Test( "(Hallo\\fWelt!)", ePdfDataType_String, "(Hallo\\fWelt!)" );
}

void TokenizerTest::testDictionary() 
{
    const char* pszDictIn = 
        "<< /CheckBox#C3#9Cbersetzungshinweis(False)/Checkbox#C3#9Cbersetzungstabelle(False) >>";
    const char* pszDictOut = 
        "<<\n/CheckBox#C3#9Cbersetzungshinweis (False)\n/Checkbox#C3#9Cbersetzungstabelle (False)\n>>";

    Test( pszDictIn, ePdfDataType_Dictionary, pszDictOut );
}

void TokenizerTest::TestStream( const char* pszBuffer, const char* pszTokens[] )
{

    long          lLen = strlen( pszBuffer );
    PdfTokenizer  tokenizer( pszBuffer, lLen );
    EPdfTokenType eType;
    const char*   pszCur;
    int           i = 0;
    while( pszTokens[i] )
    {
        CPPUNIT_ASSERT_EQUAL( tokenizer.GetNextToken( pszCur, &eType ), true );
        
        std::string sCur( pszCur );
        std::string sToken( pszTokens[i] );

        CPPUNIT_ASSERT_EQUAL( sCur, sToken );

        ++i;
    }

    // We are at the end, so GetNextToken has to return false!
    CPPUNIT_ASSERT_EQUAL( tokenizer.GetNextToken( pszCur, &eType ), false );
}

void TokenizerTest::TestStreamIsNextToken( const char* pszBuffer, const char* pszTokens[] )
{

    long          lLen = strlen( pszBuffer );
    PdfTokenizer  tokenizer( pszBuffer, lLen );

    int           i = 0;
    while( pszTokens[i] )
        CPPUNIT_ASSERT_EQUAL( tokenizer.IsNextToken( pszTokens[i++] ), true );
}

void TokenizerTest::testTokens()
{
    const char* pszBuffer = "613 0 obj"
        "<< /Length 141 /Filter [ /ASCII85Decode /FlateDecode ] >>"
        "endobj";

    const char* pszTokens[] = {
        "613", "0", "obj", "<<", "/", "Length", "141", "/", "Filter", "[", "/",
        "ASCII85Decode", "/", "FlateDecode", "]", ">>", "endobj", NULL
    };

    TestStream( pszBuffer, pszTokens );
    TestStreamIsNextToken( pszBuffer, pszTokens );
}

void TokenizerTest::testComments()
{
    const char* pszBuffer = "613 0 obj\n"
        "% A comment that should be ignored\n"
        "<< /Length 141 /Filter\n% A comment in a dictionary\n[ /ASCII85Decode /FlateDecode ] >>"
        "endobj";

    const char* pszTokens[] = {
        "613", "0", "obj", "<<", "/", "Length", "141", "/", "Filter", "[", "/",
        "ASCII85Decode", "/", "FlateDecode", "]", ">>", "endobj", NULL
    };

    TestStream( pszBuffer, pszTokens );
    TestStreamIsNextToken( pszBuffer, pszTokens );
}

void TokenizerTest::testLocale()
{
    // Test with a locale thate uses "," instead of "." for doubles 
    char *old = setlocale( LC_ALL, "de_DE" ); 

    const char* pszNumber = "3.140000";
    Test( pszNumber, ePdfDataType_Real, pszNumber );

    setlocale( LC_ALL, old );
}

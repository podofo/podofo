/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "../PdfTest.h"
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <cstdio>

using namespace PoDoFo;

//
// Test encoding of names.
// pszString : internal representation, ie unencoded name
// pszExpectedEncoded: the encoded string PoDoFo should produce
//
void TestName( const char* pszString, const char* pszExpectedEncoded ) 
{
    printf("Testing name: %s\n", pszString );

    PdfName name( pszString );
    printf("   -> Expected   Value: %s\n", pszExpectedEncoded );
    printf("   -> Got        Value: %s\n", name.GetEscapedName().c_str() );
    printf("   -> Unescaped  Value: %s\n", name.GetName().c_str() );

    if( strcmp( pszExpectedEncoded, name.GetEscapedName().c_str() ) != 0 ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    // Ensure the encoded string compares equal to its unencoded
    // variant
    if( name != PdfName::FromEscaped(pszExpectedEncoded) )
    {
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }
}

void TestEncodedName( const char* pszString, const char* pszExpected ) 
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
    if ( ! (name == PdfName(pszExpected)) )
    {
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }
}

void TestNameEquality( const char * pszName1, const char* pszName2 )
{
    PdfName name1( PdfName::FromEscaped(pszName1) );
    PdfName name2( PdfName::FromEscaped(pszName2) );

    printf("Testing equality of encoded names '%s' and '%s'\n", pszName1, pszName2);
    printf("   -> Name1    Decoded Value: %s\n", name1.GetName().c_str());
    printf("   -> Name2    Decoded Value: %s\n", name2.GetName().c_str());

    if (name1 != name2)
    {
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }
}

void Test( const char* pszString, EPdfDataType eDataType, const char* pszExpected = NULL )
{
    PdfVariant  variant;
    std::string ret;

    if( !pszExpected )
        pszExpected = pszString;

    printf("Testing with value: %s\n", pszString );
    PdfTokenizer tokenizer( pszString, strlen( pszString ) );

    tokenizer.GetNextVariant( variant, NULL );

    printf("   -> Expected Datatype: %i\n", eDataType );
    printf("   -> Got      Datatype: %i\n", variant.GetDataType() );
    if( variant.GetDataType() != eDataType )
    {
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    variant.ToString( ret );
    printf("   -> Convert To String: %s\n", ret.c_str() );
    if( strcmp( pszExpected, ret.c_str() ) != 0 )
    {
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }
}

int main() 
{
    PdfError   eCode;

    printf("This test tests the PdfVariant class.\n");
    printf("---\n");

    std::auto_ptr<PdfFilter> pFilter = PdfFilterFactory::Create( ePdfFilter_ASCIIHexDecode );

    // testing strings
    TEST_SAFE_OP( Test( "(Hallo Welt!)", ePdfDataType_String ) );
    TEST_SAFE_OP( Test( "(Hallo \\(schöne\\) Welt!)", ePdfDataType_String ) );
    TEST_SAFE_OP( Test( "(Balanced () brackets are (ok ()) in PDF Strings)", ePdfDataType_String,
                        "(Balanced \\(\\) brackets are \\(ok \\(\\)\\) in PDF Strings)" ) );
    TEST_SAFE_OP( Test( "()", ePdfDataType_String ) );
    TEST_SAFE_OP( Test( "(Test: \\064)", ePdfDataType_String, "(Test: \064)" ) );
    TEST_SAFE_OP( Test( "(Test: \\0645)", ePdfDataType_String, "(Test: 45)" ) );
    TEST_SAFE_OP( Test( "(Test: \\478)", ePdfDataType_String, "(Test: '8)" ) );
    printf("---\n");


    // testing HEX Strings
    TEST_SAFE_OP( Test( "<FFEB0400A0CC>", ePdfDataType_HexString ) );
    TEST_SAFE_OP( Test( "<FFEB0400A0C>", ePdfDataType_HexString, "<FFEB0400A0C0>" ) );
    TEST_SAFE_OP( Test( "<>", ePdfDataType_HexString ) );
    printf("---\n");

    // testing bool
    TEST_SAFE_OP( Test( "false", ePdfDataType_Bool ) );
    TEST_SAFE_OP( Test( "true", ePdfDataType_Bool ) );
    printf("---\n");

    // testing null
    TEST_SAFE_OP( Test( "null", ePdfDataType_Null ) );
    printf("---\n");

    // testing numbers
    TEST_SAFE_OP( Test( "145", ePdfDataType_Number ) );
    TEST_SAFE_OP( Test( "-12", ePdfDataType_Number ) );    
    TEST_SAFE_OP( Test( "3.140000", ePdfDataType_Real ) );
    TEST_SAFE_OP( Test( "-2.970000", ePdfDataType_Real ) );
    TEST_SAFE_OP( Test( "0", ePdfDataType_Number ) );
    TEST_SAFE_OP_IGNORE( Test( "4.", ePdfDataType_Real ) );
    printf("---\n");

    // testing references
    TEST_SAFE_OP( Test( "2 0 R", ePdfDataType_Reference ) );
    TEST_SAFE_OP( Test( "3 0 R", ePdfDataType_Reference ) );
    TEST_SAFE_OP( Test( "4 1 R", ePdfDataType_Reference ) );
    printf("---\n");

    // testing names
    TEST_SAFE_OP( Test( "/Type", ePdfDataType_Name ) );
    TEST_SAFE_OP( Test( "/Length", ePdfDataType_Name ) );
    TEST_SAFE_OP( Test( "/Adobe#20Green", ePdfDataType_Name ) );
    TEST_SAFE_OP( Test( "/$$", ePdfDataType_Name ) );
    TEST_SAFE_OP( Test( "/1.2", ePdfDataType_Name ) );
    TEST_SAFE_OP( Test( "/.notdef", ePdfDataType_Name ) );
    TEST_SAFE_OP( Test( "/@pattern", ePdfDataType_Name ) );
    TEST_SAFE_OP( Test( "/A;Name_With-Various***Characters?", ePdfDataType_Name ) );
    TEST_SAFE_OP( Test( "/", ePdfDataType_Name ) ); // empty names are legal, too!
    printf("---\n");

    // testing arrays
    TEST_SAFE_OP_IGNORE( Test( "[]", ePdfDataType_Array ) );  // this test may fail as the formating is different
    TEST_SAFE_OP( Test( "[ ]", ePdfDataType_Array ) );
    TEST_SAFE_OP( Test( "[ 1 2 3 4 ]", ePdfDataType_Array ) );
    TEST_SAFE_OP_IGNORE( Test( "[1 2 3 4]", ePdfDataType_Array ) ); // this test may fail as the formating is different
    TEST_SAFE_OP( Test( "[ 2 (Hallo Welt!) 3.500000 /FMC ]", ePdfDataType_Array ) );
    TEST_SAFE_OP( Test( "[ [ 1 2 ] (Hallo Welt!) 3.500000 /FMC ]", ePdfDataType_Array ) );
    TEST_SAFE_OP_IGNORE( Test( "[/ImageA/ImageB/ImageC]", ePdfDataType_Array ) ); // this test may fail as the formating is different
    TEST_SAFE_OP_IGNORE( Test( "[<530464995927cef8aaf46eb953b93373><530464995927cef8aaf46eb953b93373>]", ePdfDataType_Array ) );
    TEST_SAFE_OP_IGNORE( Test( "[ 2 0 R (Test Data) 4 << /Key /Data >> 5 0 R ]", ePdfDataType_Array ) );
    TEST_SAFE_OP_IGNORE( Test( "[<</key/name>>2 0 R]", ePdfDataType_Array ) );
    printf("---\n");

    // Test some names. The first argument is the unencoded representation, the second
    // is the expected encoded result. The result must not only be /a/ correct encoded
    // name for the unencoded form, but must be the exact one PoDoFo should produce.
    TEST_SAFE_OP( TestName( "Length With Spaces", "Length#20With#20Spaces" ) );
    TEST_SAFE_OP( TestName( "Length\001\002\003Spaces\177",  "Length#01#02#03Spaces#7F" ) );
    TEST_SAFE_OP( TestName( "Length#01#02#03Spaces#7F", "Length#2301#2302#2303Spaces#237F" ) );
    TEST_SAFE_OP( TestName( "Tab\tTest", "Tab#09Test" ) );
    printf("---\n");

    // Test some pre-encoded names. The first argument is the encoded name that'll be
    // read from the PDF; the second is the expected representation.
    TEST_SAFE_OP( TestEncodedName( "PANTONE#205757#20CV", "PANTONE 5757 CV") );
    TEST_SAFE_OP( TestEncodedName( "paired#28#29parentheses", "paired()parentheses") );
    TEST_SAFE_OP( TestEncodedName( "The_Key_of_F#23_Minor", "The_Key_of_F#_Minor") );
    TEST_SAFE_OP( TestEncodedName( "A#42", "AB") );
    printf("---\n");

    // Make sure differently encoded names compare equal if their decoded values
    // are equal.
    TEST_SAFE_OP( TestNameEquality( "With Spaces", "With#20Spaces" ) );
    TEST_SAFE_OP( TestNameEquality( "#57#69#74#68#20#53#70#61#63#65#73", "With#20Spaces" ) );
    printf("---\n");

    // TODO: Move to AlgorithmTest
    char* pszHex = static_cast<char*>(malloc( sizeof(char) * 256 ));
    memset(pszHex, 0, 256);
    strcpy( pszHex, "Hallo Du schoene Welt!" );
    pdf_long lLen = strlen( pszHex );
    char* pszResult = NULL;
    pdf_long lRes = -1;
    std::string out;

    TEST_SAFE_OP( pFilter->Encode( pszHex, lLen, &pszResult, &lRes ) );
    free( pszHex );
    pszHex = pszResult;
    lLen = lRes;
    out.assign(pszHex, lLen);
    std::cerr << "Encoded buffer: (" << out << ')' << std::endl;

    TEST_SAFE_OP( pFilter->Decode( pszHex, lLen, &pszResult, &lRes ) );
    free( pszHex );
    pszHex = pszResult;
    lLen = lRes;
    out.assign(pszHex, lLen);
    std::cerr << "Encoded buffer: (" << out << ')' << std::endl;

    TEST_SAFE_OP( pFilter->Encode( pszHex, lLen, &pszResult, &lRes ) );
    free( pszHex );
    pszHex = pszResult;
    lLen = lRes;
    out.assign(pszHex, lLen);
    std::cerr << "Encoded buffer: (" << out << ')' << std::endl;

    TEST_SAFE_OP( pFilter->Decode( pszHex, lLen, &pszResult, &lRes  ) );
    free( pszHex );
    pszHex = pszResult;
    lLen = lRes;
    out.assign(pszHex, lLen);
    std::cerr << "Encoded buffer: (" << out << ')' << std::endl;
    free( pszHex );


    // test a hex string containing a whitespace character
    pszHex = static_cast<char*>(malloc( sizeof(char) * 256 ));
    strcpy( pszHex, "48616C6C6F2044\n75207363686F656E652057656C7421");
    lLen = strlen( pszHex );

    TEST_SAFE_OP( pFilter->Decode( pszHex, lLen, &pszResult, &lRes  ) );
    free( pszHex );
    pszHex = pszResult;
    lLen = lRes;
    out.assign(pszHex, lLen);
    std::cerr << "Encoded buffer: (" << out << ')' << std::endl;
    free( pszResult );

    // TODO: test variant equality comparisons

    printf("Test completed with error code: %i\n", eCode.GetError() );
    return eCode.GetError();
}

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

#include "PdfVariant.h"
#include "../PdfTest.h"

#include "PdfFilter.h"
#include "PdfName.h"

using namespace PoDoFo;

void TestName( const char* pszString, const char* pszExpected ) 
{
    printf("Testing name: %s\n", pszString );

    PdfName name( pszString );
    printf("   -> Expected Value: %s\n", pszExpected );
    printf("   -> Got      Value: %s\n", name.GetName().c_str() );
    printf("   -> Escaped  Value: %s\n", name.GetUnescapedName().c_str() );

    if( strcmp( pszExpected, name.GetName().c_str() ) != 0 ) 
    {
        RAISE_ERROR( ePdfError_TestFailed );
    }
}

void Test( const char* pszString, EPdfDataType eDataType, const char* pszExpected = NULL )
{
    PdfVariant  variant;
    std::string ret;
    long lLen           = 0;

    if( !pszExpected )
        pszExpected = pszString;

    printf("Testing with value: %s\n", pszString );
    variant.Parse( pszString, strlen( pszString ), &lLen );

    printf("   -> Expected Datatype: %i\n", eDataType );
    printf("   -> Got      Datatype: %i\n", variant.GetDataType() );
    if( variant.GetDataType() != eDataType )
    {
        RAISE_ERROR( ePdfError_TestFailed );
    }

    variant.ToString( ret );
    printf("   -> Convert To String: %s\n", ret.c_str() );
    if( strcmp( pszExpected, ret.c_str() ) != 0 )
    {
        RAISE_ERROR( ePdfError_TestFailed );
    }

    printf("   -> Parsed Length    : %li (%i)\n", lLen, strlen(pszExpected) );
    if( lLen != strlen( pszExpected ) )
    {
        RAISE_ERROR( ePdfError_TestFailed );
    }
}

int main() 
{
    PdfError   eCode;
    const PdfFilter* pFilter;

    printf("This test tests the PdfVariant class.\n");
    printf("---\n");

    pFilter = PdfFilterFactory::Create( ePdfFilter_ASCIIHexDecode );

    // testing strings
    TEST_SAFE_OP( Test( "(Hallo Welt!)", ePdfDataType_String ) );
    TEST_SAFE_OP( Test( "(Hallo \\(schöne\\) Welt!)", ePdfDataType_String ) );
    TEST_SAFE_OP( Test( "()", ePdfDataType_String ) );
    printf("---\n");

    // testing HEX Strings
    TEST_SAFE_OP( Test( "<FFEB0400A0CC>", ePdfDataType_HexString ) );
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
    TEST_SAFE_OP( Test( "3.14", ePdfDataType_Real ) );
    TEST_SAFE_OP( Test( "-2.97", ePdfDataType_Real ) );
    TEST_SAFE_OP( Test( "0", ePdfDataType_Number ) );
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
    printf("---\n");

    // testing arrays
    TEST_SAFE_OP_IGNORE( Test( "[]", ePdfDataType_Array ) );  // this test may fail as the formating is different
    TEST_SAFE_OP( Test( "[ ]", ePdfDataType_Array ) );
    TEST_SAFE_OP( Test( "[ 1 2 3 4 ]", ePdfDataType_Array ) );
    TEST_SAFE_OP_IGNORE( Test( "[1 2 3 4]", ePdfDataType_Array ) ); // this test may fail as the formating is different
    TEST_SAFE_OP( Test( "[ 2 (Hallo Welt!) 3.5 /FMC ]", ePdfDataType_Array ) );
    TEST_SAFE_OP( Test( "[ [ 1 2 ] (Hallo Welt!) 3.5 /FMC ]", ePdfDataType_Array ) );
    TEST_SAFE_OP_IGNORE( Test( "[/ImageA/ImageB/ImageC]", ePdfDataType_Array ) ); // this test may fail as the formating is different
    TEST_SAFE_OP_IGNORE( Test( "[<530464995927cef8aaf46eb953b93373><530464995927cef8aaf46eb953b93373>]", ePdfDataType_Array ) );
    TEST_SAFE_OP_IGNORE( Test( "[ 2 0 R (Test Data) 4 << /Key /Data >> 5 0 R ]", ePdfDataType_Array ) );
    printf("---\n");

    // testing some PDF names
    TEST_SAFE_OP( TestName( "Length With Spaces", "Length#20With#20Spaces" ) );
    TEST_SAFE_OP( TestName( "Length\001\002\003Spaces\177", "Length#01#02#03Spaces#7F" ) );
    TEST_SAFE_OP( TestName( "Length#01#02#03Spaces#7F", "Length#01#02#03Spaces#7F" ) );
    TEST_SAFE_OP( TestName( "Tab\tTest", "Tab#09Test" ) );
    printf("---\n");


    // TODO: Move to AlgorithmTest
    char* pszHex = (char*)malloc( sizeof(char) * 256 );
    char* pszResult;
    strcpy( pszHex, "Hallo Du schoene Welt!" );
    long lLen = strlen( pszHex );
    long lRes;

    TEST_SAFE_OP( pFilter->Encode( pszHex, lLen, &pszResult, &lRes ) );
    free( pszHex );
    pszHex = pszResult;
    lLen = lRes;
    //pszHex[lLen] = '\0';
    pszHex[lLen-1] = '\0';
    printf("Encoded Buffer: (%s)\n", pszHex );

    TEST_SAFE_OP( pFilter->Decode( pszHex, lLen, &pszResult, &lRes ) );
    free( pszHex );
    pszHex = pszResult;
    lLen = lRes;
    //pszHex[lLen] = '\0';
    pszHex[lLen-1] = '\0';
    printf("Decoded Buffer: (%s)\n", pszHex );

    TEST_SAFE_OP( pFilter->Encode( pszHex, lLen, &pszResult, &lRes ) );
    free( pszHex );
    pszHex = pszResult;
    lLen = lRes;
    //pszHex[lLen] = '\0';
    pszHex[lLen-1] = '\0';
    printf("Encoded Buffer: (%s)\n", pszHex );

    TEST_SAFE_OP( pFilter->Decode( pszHex, lLen, &pszResult, &lRes  ) );
    free( pszHex );
    pszHex = pszResult;
    lLen = lRes;
    //pszHex[lLen] = '\0';
    pszHex[lLen-1] = '\0';
    printf("Decoded Buffer: (%s)\n", pszHex );
    free( pszHex );


    // test a hex string containing a whitespace character
    pszHex = (char*)malloc( sizeof(char) * 256 );
    strcpy( pszHex, "48616C6C6F2044\n75207363686F656E652057656C7421");
    lLen = strlen( pszHex );

    TEST_SAFE_OP( pFilter->Decode( pszHex, lLen, &pszResult, &lRes  ) );
    free( pszHex );
    pszHex = pszResult;
    lLen = lRes;
    pszHex[lLen] = '\0';
    printf("Decoded Buffer: (%s)\n", pszHex );
    free( pszResult );

    printf("Test completed with error code: %i\n", eCode.GetError() );
    return eCode.GetError();
}

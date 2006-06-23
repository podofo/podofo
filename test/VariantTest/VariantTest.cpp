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

using namespace PoDoFo;

PdfError Test( const char* pszString, EPdfDataType eDataType )
{
    PdfError    eCode;
    PdfVariant  variant;
    std::string ret;
    long lLen           = 0;

    printf("Testing with value: %s\n", pszString );
    SAFE_OP( variant.Init( pszString, strlen( pszString ), &lLen ) );

    printf("   -> Expected Datatype: %i\n", eDataType );
    printf("   -> Got      Datatype: %i\n", variant.GetDataType() );
    if( variant.GetDataType() != eDataType )
    {
        RAISE_ERROR( ePdfError_TestFailed );
    }

    SAFE_OP( variant.ToString( ret ) );
    printf("   -> Convert To String: %s\n", ret.c_str() );
    if( strcmp( pszString, ret.c_str() ) != 0 )
    {
        RAISE_ERROR( ePdfError_TestFailed );
    }

    printf("   -> Parsed Length    : %li (%i)\n", lLen, strlen(pszString) );
    if( lLen != strlen( pszString ) )
    {
        RAISE_ERROR( ePdfError_TestFailed );
    }

    return eCode;
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
    printf("---\n");

    // testing arrays
    Test( "[]", ePdfDataType_Array );  // this test may fail as the formating is different
    TEST_SAFE_OP( Test( "[ 1 2 3 4 ]", ePdfDataType_Array ) );
    Test( "[1 2 3 4]", ePdfDataType_Array ); // this test may fail as the formating is different
    TEST_SAFE_OP( Test( "[ 2 (Hallo Welt!) 3.5 /FMC ]", ePdfDataType_Array ) );
    TEST_SAFE_OP( Test( "[ [ 1 2 ] (Hallo Welt!) 3.5 /FMC ]", ePdfDataType_Array ) );
    Test( "[/ImageA/ImageB/ImageC]", ePdfDataType_Array ); // this test may fail as the formating is different
    Test( "[<530464995927cef8aaf46eb953b93373><530464995927cef8aaf46eb953b93373>]", ePdfDataType_Array );
    Test( "[ 2 0 R (Test Data) 4 << /Key /Data >> 5 0 R ]", ePdfDataType_Array );
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

    printf("Test completed with error code: %i\n", eCode.Error() );
    return eCode.Error();
}

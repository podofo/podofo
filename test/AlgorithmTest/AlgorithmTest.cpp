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

#include "PdfDefines.h"
#include "PdfError.h"
#include "PdfAlgorithm.h"
#include "../PdfTest.h"

#include <stdio.h>

#define BUFFER_SIZE 512

using namespace PoDoFo;

const char* pszData = "Hallo schoene Welt!";

void InitForTest( char** ppBuf, long* plLen )
{
    *plLen  = BUFFER_SIZE;
    *ppBuf  = (char*) malloc( sizeof(char) * BUFFER_SIZE );

    strcpy( *ppBuf, pszData );
    *plLen  = strlen( pszData );
}

PoDoFo::PdfError TestHexDecode( char** pBuf, long lLen )
{
    PdfError eCode;
    char*    pBuffer;
    long     lLength;

    TEST_SAFE_OP( PdfAlgorithm::HexEncodeBuffer( *pBuf, lLen, &pBuffer, &lLength ) );
    free( *pBuf );
    *pBuf = pBuffer;
    lLen = lLength;

    TEST_SAFE_OP( PdfAlgorithm::HexDecodeBuffer( *pBuf, lLen, &pBuffer, &lLength ) );

    free( *pBuf );
    *pBuf = pBuffer;
    lLen = lLength;

    (*pBuf)[lLen] = '\0';
    printf("Test Result (%s):", *pBuf );

    if( strncmp( *pBuf, pszData, lLen ) != 0 )
    {
        RAISE_ERROR( ePdfError_TestFailed );
    }
     
    printf(".... [OK]\n");

    return eCode;
}

PdfError TestFlateDecode( char** pBuf, long lLen )
{
    PdfError eCode;
    char*    pBuffer;
    long     lLength;

    TEST_SAFE_OP( PdfAlgorithm::FlateEncodeBuffer( *pBuf, lLen, &pBuffer, &lLength ) );

    free( *pBuf );
    *pBuf = pBuffer;
    lLen = lLength;

    printf("Lenght=%li\n", lLen );
    printf("Data=(%s)\n", *pBuf );

    TEST_SAFE_OP( PdfAlgorithm::FlateDecodeBuffer( *pBuf, lLen, &pBuffer, &lLength ) );

    free( *pBuf );
    *pBuf = pBuffer;
    lLen = lLength;

    (*pBuf)[lLen] = '\0';
    printf("Test Result (%s)\n", *pBuf );

    if( strncmp( *pBuf, pszData, lLen ) != 0 )
    {
        RAISE_ERROR( ePdfError_TestFailed );
    }

    printf(".... [OK]\n");

    return eCode;
}

PdfError TestRLE()
{
    PdfError eCode;

/*
  1 100 101 254 107 128
   decodes to
   100 101 107 107 107 (end) 
*/
    char pIn[]  = { 0x01, 0x64, 0x65, 0xFE, 0x6B, 0x80, 0x00 };
    char pOut[] = { 0x64, 0x65, 0x6B, 0x6B, 0x6B, 0x00 };

    long  lLen;
    char* pBuf;

    TEST_SAFE_OP( PdfAlgorithm::RunLengthDecodeBuffer( (char*)pIn, strlen( pIn ), &pBuf, &lLen ) );
    printf("Test Data Length: %i\n", lLen );
    if( lLen != 5 )
    {
        printf("Error: Wrong Test Data Length\n");
        RAISE_ERROR( ePdfError_TestFailed );
    }

    if( !memcmp( pOut, pBuf, lLen ) == 0 )
    {
        printf("Error: Wrong Test Data\n");
        RAISE_ERROR( ePdfError_TestFailed );
    }
    
    free( pBuf );

    return eCode;
}

int main()
{
    PdfError eCode;
    long    lLen;
    char*   pBuf;

    InitForTest( &pBuf, &lLen );
    printf("Test Data: (%s)\n", pBuf );
    free( pBuf );
    printf("----\n");

    printf("Testing the hexadecimal functions:\n");
    InitForTest( &pBuf, &lLen );
    TEST_SAFE_OP( TestHexDecode( &pBuf, lLen ) );
    free( pBuf );
    printf("----\n");

    printf("Testing the flate (zip) functions:\n");
    InitForTest( &pBuf, &lLen );
    TEST_SAFE_OP( TestFlateDecode( &pBuf, lLen ) );
    free( pBuf );
    printf("----\n");

    printf("Testing the rle functions:\n");
    TEST_SAFE_OP( TestRLE() );
    printf("----\n");

    printf("All Tests sucessfull!\n");

    return eCode.Error();
}



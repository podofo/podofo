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

#include "PdfParserObject.h"
#include "PdfOutputDevice.h"
#include "PdfWriter.h"
#include "../PdfTest.h"

#include <stdio.h>
#include <sstream>

#define HEADER_LEN 15

using namespace PoDoFo;

PdfError TestSingleObject( const char* pszFilename, const char* pszData, long lObjNo, long lGenNo, const char* pszExpectedValue )
{
    PdfError      eCode;
    FILE*         hFile;
    unsigned long lObjLen;
    std::string   sLen;

    hFile = fopen( pszFilename, "w" );
    if( !hFile )
    {
        fprintf( stderr, "Cannot open %s for writing.\n", pszFilename );
        RAISE_ERROR( ePdfError_TestFailed );
    }

    fprintf( hFile, pszData );
    fclose( hFile );

    hFile = fopen( pszFilename, "r" );
    if( !hFile )
    {
        fprintf( stderr, "Cannot open %s for reading.\n", pszFilename );
        RAISE_ERROR( ePdfError_TestFailed );
    }

    printf("Parsing Object: %li %li\n", lObjNo, lGenNo );

    PdfParserObject obj( NULL, hFile, NULL, 0 );
    eCode = obj.ParseFile( false );
    if( eCode.IsError() ) 
    {
        fprintf( stderr, "Error during test: %i\n", eCode.Error() );
        eCode.PrintErrorMsg();
        fclose( hFile );
        unlink( pszFilename ); // do not care for unlink errors in this case

        return eCode;
    }

    fclose( hFile );
    unlink( pszFilename );

    printf("  -> Object Number: %u Generation Number: %u\n", obj.ObjectNumber(), obj.GenerationNumber() );
    if( lObjNo != obj.ObjectNumber() || lGenNo != obj.GenerationNumber() )
    {
        RAISE_ERROR( ePdfError_TestFailed );
    }
        
    if( obj.HasSingleValue() )
    {
        PdfVariant var;
        std::string str;

        var = obj.GetSingleValueVariant();
        TEST_SAFE_OP( var.ToString( str ) );
        printf("  -> Expected value of this object: (%s)\n", pszExpectedValue );
        printf("  -> Single Value in this object: (%s)\n", str.c_str() );
        if( strcmp( str.c_str(), pszExpectedValue ) != 0 )
        {
            RAISE_ERROR( ePdfError_TestFailed );
        }
    }

    TEST_SAFE_OP( obj.GetObjectLength( &lObjLen ) );
    printf("  -> Object Length: %li\n", lObjLen );

    PdfOutputDevice deviceTest;
    std::ostringstream os;
    TEST_SAFE_OP( deviceTest.Init( &os ) );
    TEST_SAFE_OP( obj.Write( &deviceTest ) );

    sLen = os.str();
    printf("  -> Object String: %s\n", sLen.c_str() );
    printf("  -> Object String Length: %li\n", sLen.length() );

    if( lObjLen != sLen.length() )
    {
        fprintf( stderr, "Object length does not macht! Object Length: %li String Length: %i\n", lObjLen, sLen.length() );
        RAISE_ERROR( ePdfError_TestFailed );
    }

    return eCode;
}

PdfError TestObject( const char* pszFilename, const char* pszData, long lObjNo, long lGenNo )
{
    PdfError eCode;
    FILE*    hFile;

    hFile = fopen( pszFilename, "w" );
    if( !hFile )
    {
        fprintf( stderr, "Cannot open %s for writing.\n", pszFilename );
        RAISE_ERROR( ePdfError_TestFailed );
    }

    fprintf( hFile, pszData );
    fclose( hFile );

    hFile = fopen( pszFilename, "r" );
    if( !hFile )
    {
        fprintf( stderr, "Cannot open %s for reading.\n", pszFilename );
        RAISE_ERROR( ePdfError_TestFailed );
    }

    printf("Parsing Object: %li %li\n", lObjNo, lGenNo );

    PdfParserObject obj( NULL, hFile, NULL, 0 );
    eCode = obj.ParseFile( false );
    if( eCode.IsError() ) 
    {
        fprintf( stderr, "Error during test: %i\n", eCode.Error() );
        eCode.PrintErrorMsg();
        unlink( pszFilename ); // do not care for unlink errors in this case

        return eCode;
    }

    fclose( hFile );
    unlink( pszFilename );

    printf("  -> Object Number: %u Generation Number: %u\n", obj.ObjectNumber(), obj.GenerationNumber() );
    if( lObjNo != obj.ObjectNumber() || lGenNo != obj.GenerationNumber() )
    {
        RAISE_ERROR( ePdfError_TestFailed );
    }

    return eCode;
}

const char* pszSimpleObjectBoolean = "1 0 obj\ntrue\nendobj\n";
const char* pszSimpleObjectNumber  = "2 1 obj\n23\nendobj\n";
const char* pszSimpleObjectReal    = "3 0 obj\n3.14\nendobj\n";
const char* pszSimpleObjectString  = "4 0 obj\n(Hallo Welt!)\nendobj\n";
const char* pszSimpleObjectString2 = "5 0 obj\n(Hallo \\(schöne\\) Welt!)\nendobj\n";
const char* pszSimpleObjectHex     = "6 0 obj\n<48656C6C6F20576F726C64>\nendobj\n"; // Hello World
const char* pszSimpleObjectRef     = "7 0 obj\n6 0 R\nendobj\n";
const char* pszSimpleObjectArray   = "8 0 obj\n[100 200 300 400 500]\nendobj\n"; 
const char* pszSimpleObjectArray2  = "9 0 obj\n[100 (Hallo Welt) 3.14 400 500]\nendobj\n"; 
const char* pszSimpleObjectArray3  = "9 1 obj\n[100/Name(Hallo Welt)[1 2]3.14 400 500]\nendobj\n"; 
const char* pszSimpleObjectArray4  = "9 1 obj\n[100/Name(Hallo Welt)[1 2]3.14 400 500 /Dict << /A (Hallo) /B [21 22] >> /Wert /Farbe]\nendobj\n"; 

const char* pszObject = "10 0 obj\n"
                        "<<\n" 
                        "/Type/Test\n"
                        "/Key /Value\n"
                        "/Hard<ff00ffaa>>>\n"
                        "endobj\n";

const char* pszObject2 = "11 0 obj\n"
                        "<<\n" 
                        "/Type/Test2\n"
                        "/Key /Value\n"
                        "/Key2[100/Name(Hallo Welt)[1 2] 3.14 400 500]/Key2<AAFF>/Key4(Hallo \(Welt!)\n"
                        "/ID[<530464995927cef8aaf46eb953b93373><530464995927cef8aaf46eb953b93373>]\n"
                        ">>\n"
                        "endobj\n";

const char* pszObject3 = "12 0 obj\n"
                        "<<\n" 
                        "/Type/Test3\n"
                        "/Font<</F1 13 0 R>>\n"
                        ">>\n"
                        "endobj\n";

const char* pszObject4 = "271 0 obj\n"
                         "<< /Type /Pattern /PatternType 1 /PaintType 1 /TilingType 1 /BBox [ 0 0 45 45 ] \n"
                         "/Resources << /ProcSet [ /ImageI ] /XObject << /BGIm 7 0 R >> >> \n"
                         "/XStep 45 /YStep 45 /Matrix [ 1 0 0 1 0 27 ] /Length 272 0 R >>\nendobj\n";
                        
int main()
{
    PdfError      eCode;
    const char*   pszTmp  = "/tmp/pdfobjectparsertest";
    PdfWriter     writer;

    printf("This test tests the PdfParserObject class.\n");
    printf("---\n");

    TEST_SAFE_OP( TestSingleObject( pszTmp, pszSimpleObjectBoolean, 1, 0, "true" ) );
    TEST_SAFE_OP( TestSingleObject( pszTmp, pszSimpleObjectNumber , 2, 1, "23" ) );
    TEST_SAFE_OP( TestSingleObject( pszTmp, pszSimpleObjectReal   , 3, 0, "3.14" ) );
    TEST_SAFE_OP( TestSingleObject( pszTmp, pszSimpleObjectString , 4, 0, "(Hallo Welt!)" ) );
    TEST_SAFE_OP( TestSingleObject( pszTmp, pszSimpleObjectString2, 5, 0, "(Hallo \\(schöne\\) Welt!)" ) );
    TEST_SAFE_OP( TestSingleObject( pszTmp, pszSimpleObjectHex    , 6, 0, "<48656C6C6F20576F726C64>" ) );
    TEST_SAFE_OP( TestSingleObject( pszTmp, pszSimpleObjectRef    , 7, 0, "6 0 R" ) );
    TEST_SAFE_OP( TestSingleObject( pszTmp, pszSimpleObjectArray  , 8, 0, "[ 100 200 300 400 500 ]" ) );
    TEST_SAFE_OP( TestSingleObject( pszTmp, pszSimpleObjectArray2 , 9, 0, "[ 100 (Hallo Welt) 3.14 400 500 ]" ) );
    TEST_SAFE_OP( TestSingleObject( pszTmp, pszSimpleObjectArray3 , 9, 1, "[ 100 /Name (Hallo Welt) [ 1 2 ] 3.14 400 500 ]" ) );
    TEST_SAFE_OP( TestSingleObject( pszTmp, pszSimpleObjectArray4 , 9, 1, "[ 100 /Name (Hallo Welt) [ 1 2 ] 3.14 400 500 /Dict <<\n/A (Hallo)\n/B [ 21 22 ]\n>>\n /Wert /Farbe ]" ) );
    printf("---\n");

    TEST_SAFE_OP( TestObject( pszTmp, pszObject, 10, 0 ) );
    TEST_SAFE_OP( TestObject( pszTmp, pszObject2, 11, 0 ) );
    TEST_SAFE_OP( TestObject( pszTmp, pszObject3, 12, 0 ) );
    TEST_SAFE_OP( TestObject( pszTmp, pszObject4, 271, 0 ) );

    printf("---\n");

    printf("All tests sucessful!\n");

    return eCode.Error();
}

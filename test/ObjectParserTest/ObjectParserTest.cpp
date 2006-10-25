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
#include <string>

#define HEADER_LEN    15
#define BUFFER_SIZE 4096

static const bool KeepTempFiles = false;

using namespace PoDoFo;
using namespace std;

void TestSingleObject( string sFilename, const char* pszData, long lObjNo, long lGenNo, const char* pszExpectedValue )
{
    unsigned long lObjLen;
    std::string   sLen;
    std::string   str;
    PdfVecObjects parser;

    std::ostringstream ss;
    ss << sFilename << '_' << lObjNo << '_' << lGenNo;
    sFilename = ss.str();

    PdfRefCountedBuffer      buffer( BUFFER_SIZE );
    FILE*                    hFile = fopen( sFilename.c_str(), "w" );
    if( !hFile )
    {
        fprintf( stderr, "Cannot open %s for writing.\n", sFilename.c_str() );
        RAISE_ERROR( ePdfError_TestFailed );
    }

    fprintf( hFile, pszData );
    fclose( hFile );

    PdfRefCountedInputDevice device( sFilename.c_str(), "r" );
    if( !device.Device() )
    {
        fprintf( stderr, "Cannot open %s for reading.\n", sFilename.c_str() );
        RAISE_ERROR( ePdfError_TestFailed );
    }

    printf("Parsing Object: %li %li\n", lObjNo, lGenNo );

    PdfParserObject obj( &parser, device, buffer );
    try {
        obj.ParseFile( false );
    } catch( PdfError & e ) {
        fprintf( stderr, "Error during test: %i\n", e.GetError() );
        e.PrintErrorMsg();
        device = PdfRefCountedInputDevice();
        if (!KeepTempFiles) unlink( sFilename.c_str() ); // do not care for unlink errors in this case

        e.AddToCallstack( __FILE__, __LINE__ );
        throw e;
    }

    device = PdfRefCountedInputDevice();
    if (!KeepTempFiles) unlink( sFilename.c_str() );

    printf("  -> Object Number: %u Generation Number: %u\n", obj.Reference().ObjectNumber(), obj.Reference().GenerationNumber() );
    if( lObjNo != obj.Reference().ObjectNumber() || lGenNo != obj.Reference().GenerationNumber() )
    {
        RAISE_ERROR( ePdfError_TestFailed );
    }

    obj.ToString( str );
    printf("  -> Expected value of this object: (%s)\n", pszExpectedValue );
    printf("  -> Value in this object         : (%s)\n", str.c_str() );
    if( strcmp( str.c_str(), pszExpectedValue ) != 0 )
    {
        RAISE_ERROR( ePdfError_TestFailed );
    }

    lObjLen = obj.GetObjectLength();
    printf("  -> Object Length: %li\n", lObjLen );

    std::ostringstream os;
    PdfOutputDevice deviceTest( &os );
    obj.Write( &deviceTest );

    sLen = os.str();
    printf("  -> Object String: %s\n", sLen.c_str() );
    printf("  -> Object String Length: %li\n", sLen.length() );

    if( lObjLen != sLen.length() )
    {
        fprintf( stderr, "Object length does not macht! Object Length: %li String Length: %i\n", lObjLen, sLen.length() );
        RAISE_ERROR( ePdfError_TestFailed );
    }
}

void TestObject( string sFilename, const char* pszData, long lObjNo, long lGenNo )
{
    PdfVecObjects              parser;
    FILE*                      hFile;
    PdfRefCountedBuffer buffer( BUFFER_SIZE );

    std::ostringstream ss;
    ss << sFilename << '_' << lObjNo << '_' << lGenNo;
    sFilename = ss.str();

    hFile = fopen( sFilename.c_str(), "w" );
    if( !hFile )
    {
        fprintf( stderr, "Cannot open %s for writing.\n", sFilename.c_str() );
        RAISE_ERROR( ePdfError_TestFailed );
    }

    fprintf( hFile, pszData );
    fclose( hFile );

    PdfRefCountedInputDevice device( sFilename.c_str(), "r" );
    if( !device.Device() )
    {
        fprintf( stderr, "Cannot open %s for reading.\n", sFilename.c_str() );
        RAISE_ERROR( ePdfError_TestFailed );
    }

    printf("Parsing Object: %li %li\n", lObjNo, lGenNo );

    PdfParserObject obj( &parser, device, buffer );
    try {
        obj.ParseFile( false );
    } catch( PdfError & e ) {
        fprintf( stderr, "Error during test: %i\n", e.GetError() );
        e.PrintErrorMsg();
        device = PdfRefCountedInputDevice();
        if (!KeepTempFiles) unlink( sFilename.c_str() ); // do not care for unlink errors in this case

        e.AddToCallstack( __FILE__, __LINE__  );
        throw e;
    }

    device = PdfRefCountedInputDevice();
    if (!KeepTempFiles) unlink( sFilename.c_str() );

    printf("  -> Object Number: %u Generation Number: %u\n", obj.Reference().ObjectNumber(), obj.Reference().GenerationNumber() );
    if( lObjNo != obj.Reference().ObjectNumber() || lGenNo != obj.Reference().GenerationNumber() )
    {
        RAISE_ERROR( ePdfError_TestFailed );
    }
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
const char* pszSimpleObjectArray5  = "1 2 obj\n[123 0 R]\nendobj\n";

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

// PDF reference, Example 3.2 (LZW and ASCII85 encoded stream)
const char* pszObject5 ="32 0 obj\n"
        "  << /Length 534\n"
        "    /Filter [/ASCII85Decode /LZWDecode]\n"
        "  >>\n"
        "stream\n"
        "J..)6T`?p&<!J9%_[umg\"B7/Z7KNXbN'S+,*Q/&\"OLT'F\n"
        "LIDK#!n`$\"<Atdi`\\Vn\%b%)&'cA*VnK\\CJY(sF>c!Jnl@\n"
        "RM]WM;jjH6Gnc75idkL5]+cPZKEBPWdR>FF(kj1_R%W_d\n"
        "&/jS!;iuad7h?[L.F$+]]0A3Ck*$I0KZ?;<)CJtqi65Xb\n"
        "Vc3\\n5ua:Q/=0$W<#N3U;H,MQKqfg1?:lUpR;6oN[C2E4\n"
        "ZNr8Udn.'p+?#X+1>0Kuk$bCDF/(3fL5]Oq)^kJZ!C2H1\n"
        "'TO]Rl?Q:&'<5&iP!$Rq;BXRecDN[IJB`,)o8XJOSJ9sD\n"
        "S]hQ;Rj@!ND)bD_q&C\\g:inYC%)&u#:u,M6Bm%IY!Kb1+\n"
        "\":aAa'S`ViJglLb8<W9k6Yl\\0McJQkDeLWdPN?9A'jX*\n"
        "al>iG1p&i;eVoK&juJHs9%;Xomop\"5KatWRT\"JQ#qYuL,\n"
        "JD?M$0QP)lKn06l1apKDC@\\qJ4B!!(5m+j.7F790m(Vj8\n"
        "8l8Q:_CZ(Gm1\%X\\N1&u!FKHMB~>\n"
        "endstream\n"
        "endobj\n";

// PDF reference, Example 3.4
const char * pszObject6 = "33 0 obj\n"
        "<< /Length 568 >>\n"
        "stream\n"
        "2 J\n"
        "BT\n"
        "/F1 12 Tf\n"
        "0 Tc\n"
        "0 Tw\n"
        "72.5 712 TD\n"
        "[(Unencoded streams can be read easily) 65 (, )] TJ\n"
        "0 .14 TD\n"
        "[(b) 20 (ut generally tak ) 10 (e more space than \\311)] TJ\n"
        "T* (encoded streams.) Tj\n"
        "0 .28 TD\n"
        "[(Se) 25 (v) 15 (eral encoding methods are a) 20 (v) 25 (ailable in PDF ) 80 (.)] TJ\n"
        "0 .14 TD\n"
        "(Some are used for compression and others simply ) Tj\n"
        "T* [(to represent binary data in an ) 55 (ASCII format.)] TJ\n"
        "T* (Some of the compression encoding methods are \\\n"
        "suitable ) Tj\n"
        "T* (for both data and images, while others are \\\n"
        "suitable only ) Tj\n"
        "T* (for continuous.tone images.) Tj\n"
        "ET\n"
        "endstream\n"
        "endobj\n";

// Use a FULL statement in this macro, it will not add any trailing
// semicolons etc.
#define TRY_TEST(x) \
    try {\
        ++tests;\
        x\
        ++tests_ok;\
    } \
    catch (PdfError & e) \
    {\
        e.PrintErrorMsg();\
        ++tests_error;\
    }

                        
int main()
{
    int tests = 0, tests_error = 0, tests_ok=0;

    PdfError      eCode;
    std::string   pszTmp("/tmp/pdfobjectparsertest");

    printf("This test tests the PdfParserObject class.\n");
    printf("---\n");

    TRY_TEST(TestSingleObject( pszTmp, pszSimpleObjectBoolean, 1, 0, "true" );)
    TRY_TEST(TestSingleObject( pszTmp, pszSimpleObjectNumber , 2, 1, "23" );)
    TRY_TEST(TestSingleObject( pszTmp, pszSimpleObjectReal   , 3, 0, "3.14" );)
    TRY_TEST(TestSingleObject( pszTmp, pszSimpleObjectString , 4, 0, "(Hallo Welt!)" );)
    TRY_TEST(TestSingleObject( pszTmp, pszSimpleObjectString2, 5, 0, "(Hallo \\(schöne\\) Welt!)" );)
    TRY_TEST(TestSingleObject( pszTmp, pszSimpleObjectHex    , 6, 0, "<48656C6C6F20576F726C64>" );)
    TRY_TEST(TestSingleObject( pszTmp, pszSimpleObjectRef    , 7, 0, "6 0 R" );)
    TRY_TEST(TestSingleObject( pszTmp, pszSimpleObjectArray  , 8, 0, "[ 100 200 300 400 500 ]" );)
    TRY_TEST(TestSingleObject( pszTmp, pszSimpleObjectArray2 , 9, 0, "[ 100 (Hallo Welt) 3.14 400 500 ]" );)
    TRY_TEST(TestSingleObject( pszTmp, pszSimpleObjectArray3 , 9, 1, "[ 100 /Name (Hallo Welt) [ 1 2 ] 3.14 400 500 ]" );)
    TRY_TEST(TestSingleObject( pszTmp, pszSimpleObjectArray4 , 9, 1, "[ 100 /Name (Hallo Welt) [ 1 2 ] 3.14 400 500 /Dict <<\n/A (Hallo)\n/B [ 21 22 ]\n>>\n /Wert /Farbe ]" );)
    TRY_TEST(TestSingleObject( pszTmp, pszSimpleObjectArray5 , 1, 2, "[ 123 0 R ]" );)
    TRY_TEST(printf("---\n");)

    TRY_TEST(TestObject( pszTmp, pszObject5, 32, 0 );)
    TRY_TEST(TestObject( pszTmp, pszObject6, 33, 0 );)
    TRY_TEST(TestObject( pszTmp, pszObject, 10, 0 );)
    TRY_TEST(TestObject( pszTmp, pszObject2, 11, 0 );)
    TRY_TEST(TestObject( pszTmp, pszObject3, 12, 0 );)
    TRY_TEST(TestObject( pszTmp, pszObject4, 271, 0 );)

    printf("---\n");

    if (!tests_error)
        printf("All %i tests sucesseeded!\n", tests);
    else
        printf("%i of %i tests failed, %i succeeded\n", tests_error, tests, tests_ok);

    return tests_error;
}

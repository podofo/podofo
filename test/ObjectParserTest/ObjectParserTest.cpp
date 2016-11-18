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

#ifdef _WIN32
// Get access to POSIX unlink()
#include <io.h>
#define unlink _unlink
#else
#include <unistd.h>
#endif

#include <cstdio>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cassert>

#define HEADER_LEN    15
#define BUFFER_SIZE 4096


using namespace PoDoFo;
using namespace std;

#ifdef _WIN32
static const string sTmp("%TEMP%");
#else
static const string sTmp("/tmp/pdfobjectparsertest_");
#endif
static const bool KeepTempFiles = true;

// Undefine if you want to let the tests crash when an exception
// is thrown, so you can see the full call path.
#ifndef CATCH_EXCEPTIONS
#define CATCH_EXCEPTIONS
#endif

#ifdef CATCH_EXCEPTIONS
#define TRY try
#else
#define TRY
#endif

// Print out an object in a human readable form.
void PrintObject( const std::string & objectData, ostream & s, bool escapeNewlines, bool wrap )
{
    int wrapCtr = 0, numNonprintables = 0;
    for (string::const_iterator it = objectData.begin();
         it != objectData.end();
         ++it)
    {
        ++wrapCtr;
        char ch = *it;
        if (
                ch != '\\'
                && (
                    ( !escapeNewlines && (ch == '\r' || ch == '\n') )
                    || ch == ' '
                    || PdfTokenizer::IsPrintable(ch)
                )
           )
            s << ch;
        else if (ch == '\r')
            s << "\\r";
        else if (ch == '\n')
            s << "\\n";
        else
        {
            static const char hx[17] = "0123456789ABCDEF";
            s << '\\' << 'x' << hx[ch / 16] << hx[ch % 16];
            wrapCtr += 3;
            // If we're processing binary data, just print part of it.
            ++numNonprintables;
            if (numNonprintables > 72)
            {
                cerr << "... [snip]";
                break;
            }
        }
        if (wrap && wrapCtr > 72)
        {
            wrapCtr = 0;
            s << endl;
        }
    }
}


string WriteTempFile( const string & sFilename, const string & sData, long lObjNo, long lGenNo )
{
    std::ostringstream ss;
    ss << sFilename << lObjNo << '_' << lGenNo << "_obj";
    string sNewFileName = ss.str();

    ofstream f;
    f.open( sNewFileName.c_str(), ios_base::out|ios_base::trunc|ios_base::binary );
    f.write( sData.data(), sData.length() );

    return sNewFileName;
}

string ReadFile( string sFilename )
{
    FILE * f = fopen(sFilename.c_str(), "r");
    fseek(f, 0L, SEEK_END);
    size_t bytes = ftell(f);
    fseek(f, 0L, SEEK_SET);
    char* buf = new char[bytes];
    size_t bytesRead = fread(buf, 1, bytes, f);
    if (bytesRead != bytes)
    {
        cerr << "Failed to read file " << sFilename <<endl;
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }
    fclose(f);
    return string(buf, bytesRead);
}

// Parameters:
//
//    sFileName: Path to a file containing `sData'
//    lObjNo: object number in data
//    lGenNo: Generation number in data
//    pTestExpected: Should we test against sExpectedData?
//    sExpectedData: Parsed & written object's expected data
//
//    You'll probably want to call this with a wrapper that creates
//    the input temp file when working with small amounts of data, and
//    one that reads the expected results file for larger amounts of data.
//
void TestObject( const string & sFilename,
                       unsigned long lObjNo, unsigned long lGenNo,
                       bool bTestExpected,
                       const string & sExpectedData,
                       bool bHasStream
                       )
{
    // We end up re-reading a temp file we just wrote sometimes,
    // but this isn't exactly perf critical code.
    const string sOrigData = ReadFile(sFilename);

    PdfVecObjects parser;

    PdfRefCountedInputDevice device( sFilename.c_str(), "r" );
    if( !device.Device() )
    {
        cerr << "Cannot open " << sFilename << " for reading." << endl;
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    cerr << "Parsing Object: " << lObjNo << ' ' << lGenNo << endl;

    PdfRefCountedBuffer      buffer( BUFFER_SIZE );
    PdfParserObject obj( &parser, device, buffer );
    TRY {
        obj.ParseFile( NULL, false );
#ifdef CATCH_EXCEPTIONS
    } catch( PdfError & e ) {
        cerr << "Error during test: " << e.GetError() << endl;
        device = PdfRefCountedInputDevice();

        e.AddToCallstack( __FILE__, __LINE__ );
        throw e;
#endif
    }

    device = PdfRefCountedInputDevice();

    cerr << "  -> Object Number: " << obj.Reference().ObjectNumber()
         << " Generation Number: " << obj.Reference().GenerationNumber() << endl;
    if( lObjNo != obj.Reference().ObjectNumber() || lGenNo != obj.Reference().GenerationNumber() )
    {
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    if (bHasStream != obj.HasStream())
    {
        cerr << "ERROR: This object should've had an associated stream, but none was loaded" << endl;
        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }

    if (bHasStream)
    {
        char*    pBuffer;
        pdf_long lLen;

        cerr << "  -> Has Stream, loading ... " << flush;
        PdfMemStream * const ps = dynamic_cast<PdfMemStream*>(obj.GetStream());
        PODOFO_ASSERT(ps);
        cerr << " ok, length: " << ps->GetLength() << endl;

        ps->GetFilteredCopy( &pBuffer, &lLen );
        cerr << " got a filtered copy of length: " << lLen << endl;
        cerr << "Data:\n" << endl;
        cerr.write( pBuffer, lLen );
        cerr << "\n====\n" << endl;

        podofo_free( pBuffer );
    }

    string str;
    obj.ToString( str );

    {
        size_t objOff = sOrigData.find(" obj")+5;
        if (sOrigData[objOff] == '\r' || sOrigData[objOff] == '\n')
            objOff++;
        const size_t endobjOff = sOrigData.rfind(string("endobj"));
        const string sOrigData_Base(sOrigData, objOff, endobjOff-objOff-1);
        cerr << "  -> Input object data              (";
        PrintObject(sOrigData_Base, cerr, true, false);
        cerr << ")\n";
    }

    cerr << "  -> Parsed Value in this object  : (";
    PrintObject(str, cerr, true, false);
    cerr << ')' << endl;

    // TODO: ensure comparison correct after nulls
    if (bTestExpected)
    {
        cerr << "  -> Expected value of this object: (";
        PrintObject(sExpectedData, cerr, true, false);
        cerr << ')' << endl;

        if( str != sExpectedData )
        {
            PODOFO_RAISE_ERROR( ePdfError_TestFailed );
        }
    }

    const unsigned long lObjLen = obj.GetObjectLength( ePdfWriteMode_Default );
    cerr << "  -> Object Length: " << lObjLen << endl;

    std::ostringstream os;
    PdfOutputDevice deviceTest( &os );
    obj.WriteObject( &deviceTest, ePdfWriteMode_Default, NULL );

    string sLen = os.str();
    cerr << "  -> Object String Length: " << sLen.length() << endl;

    if( lObjLen != sLen.length() )
    {
        cerr << "Object length does not match! Object Length: " << lObjLen
             << " String Length: " << sLen.length() << endl;

        cerr << "  -> Object String begins\n"
             << "----------- begin " << lObjNo << ' ' << lGenNo << " --------------" << endl;
        PrintObject(sLen, cerr, false, false);
        cerr << "------------ end " << lObjNo << ' ' << lGenNo << " ---------------\n\n" << flush;

        PODOFO_RAISE_ERROR( ePdfError_TestFailed );
    }


    cerr << "\n\n";
}

// Test an object passed as a string against the expected value, also a string.
void TestObject_String( const string & sData,
                        long lObjNo, long lGenNo,
                        bool bTestExpected = false, const string & sExpectedData = string(),
                        bool bHasStream = false)
{
    string sTempFile = WriteTempFile(sTmp, sData.c_str(), lObjNo, lGenNo);
    TRY {
        TestObject(sTempFile, lObjNo, lGenNo, bTestExpected, sExpectedData, bHasStream);
        if (!KeepTempFiles) unlink(sTempFile.c_str());
#ifdef CATCH_EXCEPTIONS
    } catch (PdfError & e) {
        if (!KeepTempFiles) unlink(sTempFile.c_str());
        e.AddToCallstack( __FILE__, __LINE__  );
        throw e;
#endif
    }
}

// overload of TestObject_String that takes a `const char*' expected data argument that may be null.
// This may only be used for objects without associated streams.
void TestObject_String( const string & sData, long lObjNo, long lGenNo, const char * expectedData )
{
    TestObject_String(sData, lObjNo, lGenNo, expectedData != NULL, string( expectedData ? expectedData : "" ), false);
}

// Test an object stored in a file against the expected value, also a file (if provided).
// Also ensures an object has a stream if set.
void TestObject_File( const string & sFilename,
                      long lObjNo, long lGenNo,
                      bool bTestExpected = false, const string & sExpectedFile = string(),
                      bool bHasStream = false )
{
    string sExpectedData;
    if (bTestExpected)
        sExpectedData = ReadFile(sExpectedFile);
    TestObject(sFilename, lObjNo, lGenNo, bTestExpected, sExpectedData, bHasStream);
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
                        "/Key2[100/Name(Hallo Welt)[1 2] 3.14 400 500]/Key2<AAFF>/Key4(Hallo \\(Welt!)\n"
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
        "LIDK#!n`$\"<Atdi`\\Vn%b%)&'cA*VnK\\CJY(sF>c!Jnl@\n"
        "RM]WM;jjH6Gnc75idkL5]+cPZKEBPWdR>FF(kj1_R%W_d\n"
        "&/jS!;iuad7h?[L.F$+]]0A3Ck*$I0KZ?;<)CJtqi65Xb\n"
        "Vc3\\n5ua:Q/=0$W<#N3U;H,MQKqfg1?:lUpR;6oN[C2E4\n"
        "ZNr8Udn.'p+?#X+1>0Kuk$bCDF/(3fL5]Oq)^kJZ!C2H1\n"
        "'TO]Rl?Q:&'<5&iP!$Rq;BXRecDN[IJB`,)o8XJOSJ9sD\n"
        "S]hQ;Rj@!ND)bD_q&C\\g:inYC%)&u#:u,M6Bm%IY!Kb1+\n"
        "\":aAa'S`ViJglLb8<W9k6Yl\\0McJQkDeLWdPN?9A'jX*\n"
        "al>iG1p&i;eVoK&juJHs9%;Xomop\"5KatWRT\"JQ#qYuL,\n"
        "JD?M$0QP)lKn06l1apKDC@\\qJ4B!!(5m+j.7F790m(Vj8\n"
        "8l8Q:_CZ(Gm1%X\\N1&u!FKHMB~>\n"
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

// Comment tokenizer test adapted from PDF Reference, section 3.1.2 . Should parse as [ /abc 123 ] .
const char* pszCommentObject       = "91 0 obj\n[/abc% comment {/%) blah blah blah\n123]\nendobj\n";

// Use a FULL statement in this macro, it will not add any trailing
// semicolons etc.
#ifdef CATCH_EXCEPTIONS
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
#else
#define TRY_TEST(x) \
    {\
        ++tests;\
        x\
        ++tests_ok;\
    }
#endif

                        
int main()
{
    int tests = 0, tests_error = 0, tests_ok=0;

    PdfError      eCode;

    fprintf(stderr,"This test tests the PdfParserObject class.\n");
    fprintf(stderr,"---\n");

    TRY_TEST(TestObject_String( pszSimpleObjectBoolean, 1, 0, "true" );)
    TRY_TEST(TestObject_String( pszSimpleObjectNumber , 2, 1, "23" );)
    TRY_TEST(TestObject_String( pszSimpleObjectReal   , 3, 0, "3.140000" );)
    TRY_TEST(TestObject_String( pszSimpleObjectString , 4, 0, "(Hallo Welt!)" );)
    TRY_TEST(TestObject_String( pszSimpleObjectString2, 5, 0, "(Hallo \\(schöne\\) Welt!)" );)
    TRY_TEST(TestObject_String( pszSimpleObjectHex    , 6, 0, "<48656C6C6F20576F726C64>" );)
    TRY_TEST(TestObject_String( pszSimpleObjectRef    , 7, 0, "6 0 R" );)
    TRY_TEST(TestObject_String( pszSimpleObjectArray  , 8, 0, "[ 100 200 300 400 500 ]" );)
    TRY_TEST(TestObject_String( pszSimpleObjectArray2 , 9, 0, "[ 100 (Hallo Welt) 3.140000 400 500 ]" );)
    TRY_TEST(TestObject_String( pszSimpleObjectArray3 , 9, 1, "[ 100 /Name (Hallo Welt) [ 1 2 ] 3.140000 400 500 ]" );)
    TRY_TEST(TestObject_String( pszSimpleObjectArray4 , 9, 1, "[ 100 /Name (Hallo Welt) [ 1 2 ] 3.140000 400 500 /Dict <<\n/A (Hallo)\n/B [ 21 22 ]\n>> /Wert\n/Farbe ]" );)
    TRY_TEST(TestObject_String( pszSimpleObjectArray5 , 1, 2, "[ 123 0 R ]" );)
    TRY_TEST(TestObject_String( pszCommentObject, 91, 0, "[ /abc 123 ]" );)

    fprintf(stderr,"---\n");

    TRY_TEST(TestObject_String( pszObject, 10, 0 );)
    TRY_TEST(TestObject_String( pszObject2, 11, 0 );)
    TRY_TEST(TestObject_String( pszObject3, 12, 0 );)
    TRY_TEST(TestObject_String( pszObject4, 271, 0 );)
    // These ones have attached streams
    TRY_TEST(TestObject_String( pszObject5, 32, 0, false, string(), true);)
    TRY_TEST(TestObject_String( pszObject6, 33, 0, false, string(), true);)
    TRY_TEST(TestObject_File( "objects/27_0_R.obj", 27, 0, false, string(), true );)
    TRY_TEST(TestObject_File( "objects/613_0_R.obj", 613, 0, false, string(), true );)

    cerr << "---\n" << flush;

    if (!tests_error)
        cerr << "All " << tests << " tests succeeded!" << endl;
    else
        cerr << tests_error << " of " << tests << " tests failed, " << tests_ok << " succeeded" << endl;

    return tests_error;
}

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

/* 
    Notes: 

    1) out of memory tests don't run if Address Santizer (ASAN) is enabled because
       ASAN terminates the unit test process the first time it attempts to allocate
       too much memory (so running the tests with and without ASAN is recommended)

    2) PoDoFo log warnings about inconsistencies or values out of range are expected
       because the tests are supplying invalid values to check PoDoFo behaves correctly 
       in those situations
*/

#include "ParserTest.h"

#include <cppunit/Asserter.h>

#if defined( __APPLE__ )
#include <sys/resource.h>
#endif

#include <limits>

CPPUNIT_TEST_SUITE_REGISTRATION( ParserTest );

// this value is from Table C.1 in Appendix C.2 Architectural Limits in PDF 32000-1:2008
// on 32-bit systems sizeof(PoDoFo::PdfParser::TXRefEntry)=16 => max size of m_offsets=16*8,388,607 = 134 MB
// on 64-bit systems sizeof(PoDoFo::PdfParser::TXRefEntry)=24 => max size of m_offsets=16*8,388,607 = 201 MB
const long maxNumberOfIndirectObjects = 8388607;    

class PdfParserTestWrapper : public PoDoFo::PdfParser
{
public:
    PdfParserTestWrapper( PoDoFo::PdfVecObjects* pVecObjects, const char* pBuffer, long lLen )
    : PoDoFo::PdfParser( pVecObjects )
    {
        // sets up parser ready to read pBuffer
        m_device = PoDoFo::PdfRefCountedInputDevice( pBuffer, lLen );
    }

    void SetupTrailer()
    {
        // this creates m_pTrailer
        PoDoFo::PdfParser::ReadTrailer();
    }
    
    PoDoFo::PdfRefCountedInputDevice& GetDevice() { return m_device; }
    PoDoFo::PdfRefCountedBuffer&      GetBuffer() { return m_buffer; }

    void ReadXRefContents( PoDoFo::pdf_long lOffset, bool bPositionAtEnd )
    {
        // call protected method
        PoDoFo::PdfParser::ReadXRefContents( lOffset, bPositionAtEnd );
    }

    void ReadXRefSubsection( PoDoFo::pdf_int64 nFirstObject, PoDoFo::pdf_int64 nNumObjects )
    {
        // call protected method
        PoDoFo::PdfParser::ReadXRefSubsection( nFirstObject, nNumObjects );
    }

    void ReadXRefStreamContents( PoDoFo::pdf_long lOffset, bool bReadOnlyTrailer )
    {
        // call protected method
        PoDoFo::PdfParser::ReadXRefStreamContents( lOffset, bReadOnlyTrailer );        
    }

    void ReadObjects()
    {
        // call protected method
        PoDoFo::PdfParser::ReadObjects();
    }

    void ReadTrailer()
    {
        // call protected method
        PoDoFo::PdfParser::ReadTrailer();
    }
    
    bool IsPdfFile()
    {
        // call protected method
        return PoDoFo::PdfParser::IsPdfFile();
    }
};

void ParserTest::setUp()
{
    // Nothing todo here
}

void ParserTest::tearDown()
{
    // Nothing todo here
}

void ParserTest::testMaxObjectCount()
{
    const long defaultObjectCount = PoDoFo::PdfParser::GetMaxObjectCount();

    CPPUNIT_ASSERT( defaultObjectCount == maxNumberOfIndirectObjects );
        
    // test methods that use PdfParser::s_nMaxObjects or GetMaxObjectCount
    // with a range of different maximums
    PoDoFo::PdfParser::SetMaxObjectCount( std::numeric_limits<long>::max() );
    testReadXRefSubsection();
    testReadDocumentStructure();

    PoDoFo::PdfParser::SetMaxObjectCount( maxNumberOfIndirectObjects );
    testReadXRefSubsection();
    testReadDocumentStructure();

    PoDoFo::PdfParser::SetMaxObjectCount( std::numeric_limits<short>::max() );
    testReadXRefSubsection();        
    testReadDocumentStructure();

    PoDoFo::PdfParser::SetMaxObjectCount( std::numeric_limits<int>::max() );
    testReadXRefSubsection();        
    testReadDocumentStructure();
    
    PoDoFo::PdfParser::SetMaxObjectCount( std::numeric_limits<long>::max() );
    testReadXRefSubsection();
    testReadDocumentStructure();

    PoDoFo::PdfParser::SetMaxObjectCount( defaultObjectCount );
}

void ParserTest::testReadDocumentStructure()
{
    // TODO no tests yet - stub method needed by testMaxObjectCount
}

void ParserTest::testReadXRefContents()
{
    try
    {
        // generate an xref section
        // xref
        // 0 3
        // 0000000000 65535 f 
        // 0000000018 00000 n 
        // 0000000077 00000 n
        // trailer << /Root 1 0 R /Size 3 >>
        // startxref
        // 0
        // %%EOF
        std::ostringstream oss;        
        oss << "xref\r\n0 3\r\n";
        oss << generateXRefEntries(3);
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref 0\r\n";
        oss << "%EOF";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        parser.SetupTrailer();
        parser.ReadXRefContents( 0, false );
        // expected to succeed
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_FAIL( "should not throw PdfError" );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }

    try
    {
        // generate an xref section with missing xref entries
        // xref
        // 0 3
        // 0000000000 65535 f 
        // 0000000018 00000 n 
        // 
        // trailer << /Root 1 0 R /Size 3 >>
        // startxref
        // 0
        // %%EOF        
        std::ostringstream oss;        
        oss << "xref\r\n0 3\r\n";
        oss << generateXRefEntries(2); // 2 entries supplied, but expecting 3 entries
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref 0\r\n";
        oss << "%EOF";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        parser.SetupTrailer();
        parser.ReadXRefContents( 0, false );
        // expected to succeed
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_FAIL( "should not throw PdfError" );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }
    
    try
    {
        // TODO malformed entries are not detected
        // generate an xref section with badly formed xref entries
        // xref
        // 0 3        
        // 000000000 65535
        // 00000000065535 x
        // 0000000
        // 0000000018 00000 n
        // 0000000077 00000 n        
        // trailer << /Root 1 0 R /Size 3 >>
        // startxref
        // 0
        // %%EOF
        std::ostringstream oss;        
        oss << "xref\r\n0 5\r\n";
        oss << "000000000 65535\r\n";
        oss << "00000000065535 x\r\n";
        oss << "0000000\r\n";
        oss << generateXRefEntries(2); 
        oss << "trailer << /Root 1 0 R /Size 5 >>\r\n";
        oss << "startxref 0\r\n";
        oss << "%EOF";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        parser.SetupTrailer();
        parser.ReadXRefContents( 0, false );
        // succeeds reading badly formed xref entries  - should it?
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }

    // CVE-2017-8053 ReadXRefContents and ReadXRefStreamContents are mutually recursive   
    // and can cause stack overflow

    try
    {
        // generate an xref section and one XRef stream that references itself
        // via the /Prev entry (but use a slightly lower offset by linking to
        // to whitespace discarded by the tokenizer just before the xref section)
        // xref
        // 0 1
        // 000000000 65535
        // 2 0 obj << /Type XRef /Prev offsetXrefStmObj2 >> stream ... endstream
        // trailer << /Root 1 0 R /Size 3 >>
        // startxref
        // offsetXrefStmObj2
        // %%EOF
        std::ostringstream oss;

        // object stream contents - length excludes trailing whitespace
        std::string streamContents = 
            "01 0E8A 0\r\n"
            "02 0002 00\r\n";
        size_t streamContentsLength = streamContents.size() - strlen("\r\n");
        
        // xref section at offset 0
        //size_t offsetXref = 0;
        oss << "xref\r\n0 1\r\n";
        oss << generateXRefEntries(1);
        
        // XRef stream at offsetXrefStm1, but any /Prev entries pointing to any offet between
        // offsetXrefStm1Whitespace and offsetXrefStm1 point to the same /Prev section
        // because the PDF processing model says tokenizer must discard whitespace and comments
        size_t offsetXrefStm1Whitespace = oss.str().length();
        oss << "    \r\n";
        oss << "% comments and leading white space are ignored - see PdfTokenizer::GetNextToken\r\n";
        size_t offsetXrefStm1 = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << streamContentsLength << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 3 ";
        oss << "/Prev " << offsetXrefStm1Whitespace << " ";     // xref /Prev offset points back to start of this stream object
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        oss << streamContents;
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXrefStm1 << "\r\n";
        oss << "%EOF";
        
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        parser.SetupTrailer();
        parser.ReadXRefContents( offsetXrefStm1, false );
        // succeeds in current code - should it?
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }

    try
    {
        // generate an xref section and two XRef streams that reference each other
        // via the /Prev entry
        // xref
        // 0 1
        // 000000000 65535
        // 2 0 obj << /Type XRef /Prev offsetXrefStmObj3 >> stream ...  endstream
        // 3 0 obj << /Type XRef /Prev offsetXrefStmObj2 >> stream ...  endstream
        // trailer << /Root 1 0 R /Size 3 >>
        // startxref
        // offsetXrefStmObj2
        // %%EOF
        std::ostringstream oss;

        // object stream contents - length excludes trailing whitespace
        std::string streamContents = 
            "01 0E8A 0\r\n"
            "02 0002 00\r\n";
        size_t streamContentsLength = streamContents.size() - strlen("\r\n");
        
        // xref section at offset 0
        //size_t offsetXref = 0;
        oss << "xref\r\n0 1\r\n";
        oss << generateXRefEntries(1);
        
        // xrefstm at offsetXrefStm1
        size_t offsetXrefStm1 = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << streamContentsLength << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 3 ";
        oss << "/Prev 185 ";     // xref stream 1 sets xref stream 2 as previous in chain
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        oss << streamContents;
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        
        // xrefstm at offsetXrefStm2
        size_t offsetXrefStm2 = oss.str().length();
        CPPUNIT_ASSERT( offsetXrefStm2 == 185 ); // hard-coded in /Prev entry in XrefStm1 above
        oss << "3 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << streamContentsLength << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 3 ";
        oss << "/Prev " << offsetXrefStm1 << " ";     // xref stream 2 sets xref stream 1 as previous in chain
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        oss << streamContents;
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXrefStm2 << "\r\n";
        oss << "%EOF";
        
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        parser.SetupTrailer();
        parser.ReadXRefContents( offsetXrefStm2, false );
        // succeeds in current code - should it?
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }

    try
    {
        // generate an xref section and lots of XRef streams without loops but reference 
        // the previous stream via the /Prev entry
        // xref
        // 0 1
        // 000000000 65535
        // 2 0 obj << /Type XRef >> stream ...  endstream
        // 3 0 obj << /Type XRef /Prev offsetStreamObj(2) >> stream ...  endstream
        // 4 0 obj << /Type XRef /Prev offsetStreamObj(3) >> stream ...  endstream
        // ...
        // N 0 obj << /Type XRef /Prev offsetStreamObj(N-1) >> stream ...  endstream
        // trailer << /Root 1 0 R /Size 3 >>
        // startxref
        // offsetStreamObj(N)
        // %%EOF
        std::ostringstream oss;
        size_t prevOffset = 0;
        size_t currentOffset = 0;
        
        // object stream contents - length excludes trailing whitespace
        std::string streamContents = 
            "01 0E8A 0\r\n"
            "02 0002 00\r\n";
        size_t streamContentsLength = streamContents.size() - strlen("\r\n");
        
        // xref section at offset 0
        //size_t offsetXref = 0;
        oss << "xref\r\n0 1\r\n";
        oss << generateXRefEntries(1);

        // this caused stack overflow on macOS 64-bit with around 3000 streams
        // and on Windows 32-bit with around 1000 streams
        
        const int maxXrefStreams = 10000;
        for ( int i = 0 ; i < maxXrefStreams ; ++i )
        {
            int objNo = i + 2;

            // xrefstm at currentOffset linked back to stream at prevOffset
            prevOffset = currentOffset;
            currentOffset = oss.str().length();
            oss << objNo << " 0 obj ";
            oss << "<< /Type /XRef ";
            oss << "/Length " << streamContentsLength << " ";
            oss << "/Index [2 2] ";
            oss << "/Size 3 ";
            if ( prevOffset > 0 )
                oss << "/Prev " << prevOffset << " ";
            oss << "/W [1 2 1] ";
            oss << "/Filter /ASCIIHexDecode ";
            oss << ">>\r\n";
            oss << "stream\r\n";
            oss << streamContents;
            oss << "endstream\r\n";
            oss << "endobj\r\n";
        }
        
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << currentOffset << "\r\n";
        oss << "%EOF";
        
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        parser.SetupTrailer();
        parser.ReadXRefContents( currentOffset, false );
        // succeeds in current code - should it?
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }    
}

void ParserTest::testReadXRefSubsection()
{
    PoDoFo::pdf_int64 nFirstObject = 0;
    PoDoFo::pdf_int64 nNumObjects = 0;
    
    // TODO does ReadXRefSubsection with nNumObjects = 0 make sense ???

    // CVE-2017-5855 m_offsets.resize() NULL ptr read
    // CVE-2017-6844 m_offsets.resize() buffer overwrite 
    // false positives due to AFL setting allocator_may_return_null=1 which causes
    // ASAN to return NULL instead of throwing std::bad_alloc for out-of-memory conditions
    // https://github.com/mirrorer/afl/blob/master/docs/env_variables.txt#L248
    // https://github.com/google/sanitizers/issues/295#issuecomment-234273218 
    // the test for CVE-2018-5296 below checks that PoDoFo restricts allocations

    // CVE-2018-5296 m_offsets.resize() malloc failure when large size specified
    // check PoDoFo throws PdfError and not anything derived from std::exception
    // check PoDoFo can't allocate unrestricted amounts of memory

    if ( PoDoFo::PdfParser::GetMaxObjectCount() <= maxNumberOfIndirectObjects )
    {
        try
        {
            std::string strInputStream = generateXRefEntries( PoDoFo::PdfParser::GetMaxObjectCount() );
            PoDoFo::PdfVecObjects objects;
            PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
            nFirstObject = 0;
            nNumObjects = PoDoFo::PdfParser::GetMaxObjectCount();
            parser.ReadXRefSubsection( nFirstObject, nNumObjects );
            // expected to succeed
        }
        catch ( PoDoFo::PdfError& error )
        {
            CPPUNIT_FAIL( "should not throw PdfError" );
        }
        catch( std::exception& ex )
        {
            CPPUNIT_FAIL( "Unexpected exception type" );
        }
    }
    else
    {
        // test has been called from testMaxObjectCount with PoDoFo::PdfParser::SetMaxObjectCount()
        // set to a large value (large allocs are tested in address space tests below)
    }

    // don't run the following test if PoDoFo::PdfParser::GetMaxObjectCount()+1 will overflow
	// in the numXRefEntries calculation below (otherwise we get an ASAN error)
    if ( PoDoFo::PdfParser::GetMaxObjectCount() < std::numeric_limits<long>::max() )
    {
		// don't generate xrefs for high values of GetMaxObjectCount() e.g. don't try to generate 2**63 xrefs
		size_t numXRefEntries = std::min(maxNumberOfIndirectObjects + 1, PoDoFo::PdfParser::GetMaxObjectCount() + 1);

		try
        {
            std::string strInputStream = generateXRefEntries( numXRefEntries );
            PoDoFo::PdfVecObjects objects;
            PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
            nFirstObject = 0;
            nNumObjects = PoDoFo::PdfParser::GetMaxObjectCount()+1;
            parser.ReadXRefSubsection( nFirstObject, nNumObjects );
            CPPUNIT_FAIL( "PdfError not thrown" );
        }
        catch ( PoDoFo::PdfError& error )
        {
            // too many indirect objects in Trailer /Size key throws ePdfError_ValueOutOfRange
            // but too many indirect objects in xref table throws ePdfError_InvalidXRef
            CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
        }
        catch( std::exception& ex )
        {
            CPPUNIT_FAIL( "Wrong exception type" );
        }
    }

    // CVE-2018-5296 try to allocate more than address space size 
    // should throw a std::bad_length exception in STL which is rethrown as a PdfError
    try
    {
        // this attempts to allocate std::numeric_limits<size_t>::max()/2 * sizeof(TXRefEntry)
        // on 32-bit systems this allocates 2**31 * sizeof(TXRefEntry) = 2**31 * 16 (larger than 32-bit address space)
        // on LP64 (macOS,*nix) systems this allocates 2**63 * sizeof(TXRefEntry) = 2**63 * 24 (larger than 64-bit address space)
        // on LLP64 (Win64) systems this allocates 2**31 * sizeof(TXRefEntry) = 2**31 * 16 (smaller than 64-bit address space)
        std::string strInputStream = " ";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        nFirstObject = 1;
        nNumObjects = std::numeric_limits<size_t>::max() / 2 - 1;
        parser.ReadXRefSubsection( nFirstObject, nNumObjects );
        CPPUNIT_FAIL( "PdfError not thrown" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        // if nNumObjects > PdfParser::GetMaxObjectCount() then we'll see ePdfError_InvalidXRef
        // otherwise we'll see ePdfError_ValueOutOfRange or ePdfError_OutOfMemory (see testMaxObjectCount)
       CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef || error.GetError() == PoDoFo::ePdfError_ValueOutOfRange || error.GetError() == PoDoFo::ePdfError_OutOfMemory );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }            

    // CVE-2018-5296 try to allocate 95% of VM address space size (which should always fail)
    if ( !canOutOfMemoryKillUnitTests() )
    {
        size_t maxObjects = std::numeric_limits<size_t>::max() / sizeof(PoDoFo::PdfParser::TXRefEntry) / 100 * 95;

        try
        {
            std::string strInputStream = " ";
            PoDoFo::PdfVecObjects objects;
            PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
            nFirstObject = 1;
            nNumObjects = maxObjects;
            parser.ReadXRefSubsection( nFirstObject, nNumObjects );
            CPPUNIT_FAIL( "PdfError not thrown" );
        }
        catch ( PoDoFo::PdfError& error )
        {
            if ( maxObjects >= (size_t)PoDoFo::PdfParser::GetMaxObjectCount() )
                CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
            else
                CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_OutOfMemory );
        }
        catch( std::exception& ex )
        {
            CPPUNIT_FAIL( "Wrong exception type" );
        }
    } 

    // CVE-2015-8981 happens because this->GetNextNumber() can return negative numbers 
    // in range (LONG_MIN to LONG_MAX) so the xref section below causes a buffer underflow
    // because m_offsets[-5].bParsed is set to true when first entry is read
    // NOTE: std::vector operator[] is not bounds checked

    // xref
    // -5 5
    // 0000000000 65535 f 
    // 0000000018 00000 n 
    // 0000000077 00000 n 
    // 0000000178 00000 n 
    // 0000000457 00000 n 
    // trailer
    // <<  /Root 1 0 R
    //    /Size 5
    //>>
    // startxref
    // 565
    // %%EOF
    
    try
    {
        std::string strInputStream = "0000000000 65535 f\r\n";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        nFirstObject = -5LL;
        nNumObjects = 5;
        parser.ReadXRefSubsection( nFirstObject, nNumObjects );
        CPPUNIT_FAIL( "PdfError not thrown" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_ValueOutOfRange || error.GetError() == PoDoFo::ePdfError_NoXRef );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }
    
    // CVE-2015-8981 can also happen due to integer overflow in nFirstObject+nNumObjects
    // in the example below 2147483647=0x7FFF, so 0x7FFF + 0x7FFF = 0XFFFE = -2 on a 32-bit system
    // which means m_offsets.size()=5 because m_offsets.resize() is never called and 
    // m_offsets[2147483647].bParsed is set to true when first entry is read
    // NOTE: std::vector operator[] is not bounds checked

    // 2147483647 2147483647 
    // 0000000000 65535 f 
    // 0000000018 00000 n 
    // 0000000077 00000 n 
    // 0000000178 00000 n 
    // 0000000457 00000 n 
    // trailer
    // <<  /Root 1 0 R
    //    /Size 5
    //>>
    // startxref
    // 565
    // %%EOF

    try
    {
        std::string strInputStream = "0000000000 65535 f\r\n";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        nFirstObject = std::numeric_limits<long>::max();
        nNumObjects = std::numeric_limits<long>::max();
        parser.ReadXRefSubsection( nFirstObject, nNumObjects );
        CPPUNIT_FAIL( "PdfError not thrown" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }
    
    try
    {
        std::string strInputStream = "0000000000 65535 f\r\n";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        nFirstObject = std::numeric_limits<PoDoFo::pdf_int64>::max();
        nNumObjects = std::numeric_limits<PoDoFo::pdf_int64>::max();
        parser.ReadXRefSubsection( nFirstObject, nNumObjects );
        CPPUNIT_FAIL( "PdfError not thrown" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }            

    // test for integer overflows in ReadXRefSubsection (CVE-2017-5853) which caused
    // wrong buffer size to be calculated and then triggered buffer overflow (CVE-2017-6844)   
    // the overflow checks in ReadXRefSubsection depend on the value returned by GetMaxObjectCount
    // if the value changes these checks need looked at again
    CPPUNIT_ASSERT( PoDoFo::PdfParser::GetMaxObjectCount() <= std::numeric_limits<long>::max() );

    // test CVE-2017-5853 signed integer overflow in nFirstObject + nNumObjects
    // CVE-2017-5853 1.1 - nFirstObject < 0
    try
    {
        std::string strInputStream = " ";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        nFirstObject = -1LL;
        nNumObjects = 1;
        parser.ReadXRefSubsection( nFirstObject, nNumObjects );
        CPPUNIT_FAIL( "PdfError not thrown" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_ValueOutOfRange );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }
    
    // CVE-2017-5853 1.2 - nFirstObject = min value of long
    try
    {
        std::string strInputStream = " ";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        nFirstObject = std::numeric_limits<long>::min();
        nNumObjects = 1;
        parser.ReadXRefSubsection( nFirstObject, nNumObjects );
        CPPUNIT_FAIL( "PdfError not thrown" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_ValueOutOfRange );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }
    
    // CVE-2017-5853 1.3 - nFirstObject = min value of pdf_int64
    try
    {
        std::string strInputStream = " ";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        nFirstObject = std::numeric_limits<PoDoFo::pdf_int64>::min();
        nNumObjects = 1;
        parser.ReadXRefSubsection( nFirstObject, nNumObjects );
        CPPUNIT_FAIL( "PdfError not thrown" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_ValueOutOfRange );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }
    
    // CVE-2017-5853 1.4 - nFirstObject = min value of size_t is zero (size_t is unsigned)
    // and zero is a valid value for nFirstObject
    
    // CVE-2017-5853 1.5 - nFirstObject = max value of long
    try
    {
        std::string strInputStream = " ";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        nFirstObject = std::numeric_limits<long>::max();
        nNumObjects = 1;
        parser.ReadXRefSubsection( nFirstObject, nNumObjects );
        CPPUNIT_FAIL( "PdfError not thrown" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }
    
    // CVE-2017-5853 1.6 - nFirstObject = max value of pdf_int64
    try
    {
        std::string strInputStream = " ";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        nFirstObject = std::numeric_limits<PoDoFo::pdf_int64>::max();
        nNumObjects = 1;
        parser.ReadXRefSubsection( nFirstObject, nNumObjects );
        CPPUNIT_FAIL( "PdfError not thrown" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }
    
    // CVE-2017-5853 1.7 - nFirstObject = max value of size_t
    try
    {
        std::string strInputStream = " ";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        nFirstObject = std::numeric_limits<size_t>::max();
        nNumObjects = 1;
        parser.ReadXRefSubsection( nFirstObject, nNumObjects );
        CPPUNIT_FAIL( "PdfError not thrown" );
    }
    catch ( PoDoFo::PdfError& error )
    {
		// weird: different errors returned depending on architecture 
		CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_ValueOutOfRange || sizeof(size_t) == 4 );
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef || sizeof(size_t) == 8 );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }
    
    // CVE-2017-5853 1.8 - nFirstObject = PdfParser::GetMaxObjectCount()
    try
    {
        std::string strInputStream = " ";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        CPPUNIT_ASSERT( PoDoFo::PdfParser::GetMaxObjectCount() > 0 );
        nFirstObject = PoDoFo::PdfParser::GetMaxObjectCount();
        nNumObjects = 1;
        parser.ReadXRefSubsection( nFirstObject, nNumObjects );
        CPPUNIT_FAIL( "PdfError not thrown" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }
    
    // CVE-2017-5853 2.1 - nNumObjects < 0
    try
    {
        std::string strInputStream = " ";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        nFirstObject = 1;
        nNumObjects = -1LL;
        parser.ReadXRefSubsection( nFirstObject, nNumObjects );
        CPPUNIT_FAIL( "PdfError not thrown" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_ValueOutOfRange );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }
    
    // CVE-2017-5853 2.2 - nNumObjects = min value of long
    try
    {
        std::string strInputStream = " ";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        nFirstObject = 1;
        nNumObjects = std::numeric_limits<long>::min();
        parser.ReadXRefSubsection( nFirstObject, nNumObjects );
        CPPUNIT_FAIL( "PdfError not thrown" );
    }
    catch ( PoDoFo::PdfError& error )
    {
       CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_ValueOutOfRange );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }
    
    // CVE-2017-5853 2.3 - nNumObjects = min value of pdf_int64
    try
    {
        std::string strInputStream = " ";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        nFirstObject = 1;
        nNumObjects = std::numeric_limits<PoDoFo::pdf_int64>::min();
        parser.ReadXRefSubsection( nFirstObject, nNumObjects );
        CPPUNIT_FAIL( "PdfError not thrown" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_ValueOutOfRange );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }
    
    // CVE-2017-5853 2.4 - nNumObjects = min value of size_t is zero (size_t is unsigned)
    // and zero is a valid value for nFirstObject
    // TODO
    
    // CVE-2017-5853 2.5 - nNumObjects = max value of long
    try
    {
        std::string strInputStream = " ";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        nFirstObject = 1;
        nNumObjects = std::numeric_limits<long>::max();
        parser.ReadXRefSubsection( nFirstObject, nNumObjects );
        CPPUNIT_FAIL( "PdfError not thrown" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }
    
    // CVE-2017-5853 2.6 - nNumObjects = max value of pdf_int64
    try
    {
        std::string strInputStream = " ";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        nFirstObject = 1;
        nNumObjects = std::numeric_limits<PoDoFo::pdf_int64>::max();
        parser.ReadXRefSubsection( nFirstObject, nNumObjects );
        CPPUNIT_FAIL( "PdfError not thrown" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }
    
    // CVE-2017-5853 2.7 - nNumObjects = max value of size_t
    try
    {
        std::string strInputStream = " ";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        nFirstObject = 1;
        nNumObjects = std::numeric_limits<size_t>::max();
        parser.ReadXRefSubsection( nFirstObject, nNumObjects );
        CPPUNIT_FAIL( "PdfError not thrown" );
    }
    catch ( PoDoFo::PdfError& error )
    {
		// weird: different errors returned depending on architecture 
		CPPUNIT_ASSERT(error.GetError() == PoDoFo::ePdfError_ValueOutOfRange || sizeof(size_t) == 4);
		CPPUNIT_ASSERT(error.GetError() == PoDoFo::ePdfError_InvalidXRef || sizeof(size_t) == 8);
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }
    
    // CVE-2017-5853 2.8 - nNumObjects = PdfParser::GetMaxObjectCount()
    try
    {
        std::string strInputStream = " ";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        nFirstObject = 1;
        nNumObjects = PoDoFo::PdfParser::GetMaxObjectCount();
        parser.ReadXRefSubsection( nFirstObject, nNumObjects );
        CPPUNIT_FAIL( "PdfError not thrown" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }

    // CVE-2017-5853 2.9 - finally - loop through a set of interesting bit patterns
    static PoDoFo::pdf_uint64 s_values[] =
    {
        //(1ull << 64) - 1,
        //(1ull << 64),
        //(1ull << 64) + 1,
        (1ull << 63) - 1,
        (1ull << 63),
        (1ull << 63) + 1,
        (1ull << 62) - 1,
        (1ull << 62),
        (1ull << 62) + 1,

        (1ull << 49) - 1,
        (1ull << 49),
        (1ull << 49) + 1,
        (1ull << 48) - 1,
        (1ull << 48),
        (1ull << 48) + 1,
        (1ull << 47) - 1,
        (1ull << 47),
        (1ull << 47) + 1,        

        (1ull << 33) - 1,
        (1ull << 33),
        (1ull << 33) + 1,
        (1ull << 32) - 1,
        (1ull << 32),
        (1ull << 32) + 1,
        (1ull << 31) - 1,
        (1ull << 31),
        (1ull << 31) + 1,

        (1ull << 25) - 1,
        (1ull << 33),
        (1ull << 33) + 1,
        (1ull << 24) - 1,
        (1ull << 24),
        (1ull << 24) + 1,
        (1ull << 31) - 1,
        (1ull << 31),
        (1ull << 31) + 1,        

        (1ull << 17) - 1,
        (1ull << 17),
        (1ull << 17) + 1,
        (1ull << 16) - 1,
        (1ull << 16),
        (1ull << 16) + 1,
        (1ull << 15) - 1,
        (1ull << 15),
        (1ull << 15) + 1,

        (PoDoFo::pdf_uint64)-1,
        0,
        1
    };
    const size_t numValues = sizeof(s_values)/sizeof(s_values[0]);

    for ( int i = 0 ; i < static_cast<int>(numValues) ; ++i )
    {
        for ( size_t j = 0 ; j < numValues ; ++j )
        {
            try
            {
                std::string strInputStream = " ";
                PoDoFo::PdfVecObjects objects;
                PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
                nFirstObject = s_values[i];
                nNumObjects = s_values[j];
                
                if ( canOutOfMemoryKillUnitTests() && (nFirstObject > maxNumberOfIndirectObjects || nNumObjects > maxNumberOfIndirectObjects) )
                {
                    // can't call this in test environments where an out-of-memory condition terminates
                    // unit test process before all tests have run (e.g. AddressSanitizer)
                }
                else
                {
                    parser.ReadXRefSubsection( nFirstObject, nNumObjects );
                    // some combinations of nFirstObject/nNumObjects from s_values are legal - so we expect to reach here sometimes
                }
            }
            catch ( PoDoFo::PdfError& error )
            {
                // other combinations of nFirstObject/nNumObjects from s_values are illegal 
                // if we reach here it should be an invalid xref value of some type
                CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef || error.GetError() == PoDoFo::ePdfError_ValueOutOfRange || error.GetError() == PoDoFo::ePdfError_NoXRef || error.GetError() == PoDoFo::ePdfError_OutOfMemory );
            }
            catch( std::exception& ex )
            {
                // and should never reach here
                CPPUNIT_FAIL( "Wrong exception type" );
            }
        }
    }
}

void ParserTest::testReadXRefStreamContents()
{
    // test valid stream
    try
    {
        // generate an XRef stream with valid /W values
        std::ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        // XRef stream with 5 entries
        size_t lengthXRefObject = 57; 
        size_t offsetXRefObject = oss.str().length();        
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 5 ";
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        CPPUNIT_ASSERT( offsetEndstream-offsetStream-strlen("\r\n") == lengthXRefObject ); // hard-coded in /Length entry in XRef stream above

        // trailer        
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";
        
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        
        parser.SetupTrailer();
        parser.ReadXRefStreamContents( offsetXRefObject, false );
        // should succeed
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_FAIL( "Unexpected PdfError" );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }

    // CVE-2018-5295: integer overflow caused by checking sum of /W entry values /W [ 1 2 9223372036854775807 ]
    // see https://bugzilla.redhat.com/show_bug.cgi?id=1531897 (/W values used were extracted from PoC file)
    try
    {
        std::ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;
        
        // XRef stream
        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = 0;
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 5 ";
        oss << "/W [ 1 2 9223372036854775807 ] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        CPPUNIT_ASSERT( offsetEndstream-offsetStream-strlen("\r\n") == lengthXRefObject ); // check /Length entry in XRef stream above
        
        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";
        
        PoDoFo::PdfVecObjects objects;
        PoDoFo::PdfParser::TVecOffsets offsets;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        
        PoDoFo::PdfXRefStreamParserObject xrefStreamParser(&objects, parser.GetDevice(),
                                                             parser.GetBuffer(), &offsets );
        
        // parse the dictionary then try reading the XRef stream using the invalid /W entries
        offsets.resize(5);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        CPPUNIT_FAIL( "Should throw exception" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_NoXRef || error.GetError() == PoDoFo::ePdfError_InvalidXRefStream );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }    

    // CVE-2017-8787: heap based overflow caused by unchecked /W entry values /W [ 1 -4 2 ]
    // see https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=861738 for value of /W array
    try
    {
        std::ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;
        
        // XRef stream
        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = 0;
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 5 ";
        oss << "/W [ 1 -4 2 ] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        CPPUNIT_ASSERT( offsetEndstream-offsetStream-strlen("\r\n") == lengthXRefObject ); // check /Length entry in XRef stream above
        
        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";
        
        PoDoFo::PdfVecObjects objects;
        PoDoFo::PdfParser::TVecOffsets offsets;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        
        PoDoFo::PdfXRefStreamParserObject xrefStreamParser(&objects, parser.GetDevice(),
                                                             parser.GetBuffer(), &offsets );
        
        // parse the dictionary then try reading the XRef stream using the invalid /W entries
        offsets.resize(5);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        CPPUNIT_FAIL( "Should throw exception" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_NoXRef );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }
    
    // /W entry values /W [ 4095 1 1 ] for data in form 02 0002 00 (doesn't match size of entry)
    try
    {
        std::ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;
        
        // XRef stream
        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = 0;
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 5 ";
        oss << "/W [ 4095 1 1 ] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        CPPUNIT_ASSERT( offsetEndstream-offsetStream-strlen("\r\n") == lengthXRefObject ); // check /Length entry in XRef stream above
        
        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";
        
        PoDoFo::PdfVecObjects objects;
        PoDoFo::PdfParser::TVecOffsets offsets;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        
        PoDoFo::PdfXRefStreamParserObject xrefStreamParser(&objects, parser.GetDevice(),
                                                           parser.GetBuffer(), &offsets );
        
        // parse the dictionary then try reading the XRef stream using the invalid /W entries
        offsets.resize(5);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        CPPUNIT_FAIL( "Should throw exception" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRefStream );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }
    
    // /W entry values /W [ 4 4 4 ] for data in form 02 0002 00 (doesn't match size of entry)
    try
    {
        std::ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;
        
        // XRef stream
        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = 0;
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 5 ";
        oss << "/W [ 4 4 4 ] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        CPPUNIT_ASSERT( offsetEndstream-offsetStream-strlen("\r\n") == lengthXRefObject ); // check /Length entry in XRef stream above
        
        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";
        
        PoDoFo::PdfVecObjects objects;
        PoDoFo::PdfParser::TVecOffsets offsets;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        
        PoDoFo::PdfXRefStreamParserObject xrefStreamParser(&objects, parser.GetDevice(),
                                                           parser.GetBuffer(), &offsets );
        
        // parse the dictionary then try reading the XRef stream using the invalid /W entries
        offsets.resize(5);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        CPPUNIT_FAIL( "Should throw exception" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRefType );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }
    
    // /W entry values /W [ 1 4 4 ] (size=9) for data 01 0E8A 0\r\n02 0002 00\r\n (size=8 bytes)
    try
    {
        std::ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;
        
        // XRef stream
        size_t lengthXRefObject = 21;
        size_t offsetXRefObject = 0;
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 2 ";
        oss << "/W [ 1 4 4 ] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        CPPUNIT_ASSERT( offsetEndstream-offsetStream-strlen("\r\n") == lengthXRefObject ); // check /Length entry in XRef stream above
        
        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";
        
        PoDoFo::PdfVecObjects objects;
        PoDoFo::PdfParser::TVecOffsets offsets;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        
        PoDoFo::PdfXRefStreamParserObject xrefStreamParser(&objects, parser.GetDevice(),
                                                           parser.GetBuffer(), &offsets );
        
        // parse the dictionary then try reading the XRef stream using the invalid /W entries
        offsets.resize(5);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        CPPUNIT_FAIL( "Should throw exception" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_NoXRef );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }
    
    // XRef stream with 5 entries but /Size 2 specified
    try
    {
        std::ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;
        
        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 2 ";
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        CPPUNIT_ASSERT( offsetEndstream-offsetStream-strlen("\r\n") == lengthXRefObject ); // hard-coded in /Length entry in XRef stream above
        
        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";
        
        PoDoFo::PdfVecObjects objects;
        PoDoFo::PdfParser::TVecOffsets offsets;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        
        PoDoFo::PdfXRefStreamParserObject xrefStreamParser(&objects, parser.GetDevice(),
                                                           parser.GetBuffer(), &offsets );
        
        offsets.resize(2);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        // should this succeed ???
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_FAIL( "Unexpected PdfError" );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }
    
    // XRef stream with 5 entries but /Size 10 specified
    try
    {
        std::ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;
        
        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 10 ";
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        CPPUNIT_ASSERT( offsetEndstream-offsetStream-strlen("\r\n") == lengthXRefObject ); // hard-coded in /Length entry in XRef stream above
        
        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";
        
        PoDoFo::PdfVecObjects objects;
        PoDoFo::PdfParser::TVecOffsets offsets;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        
        PoDoFo::PdfXRefStreamParserObject xrefStreamParser(&objects, parser.GetDevice(),
                                                           parser.GetBuffer(), &offsets );
        
        offsets.resize(2);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        // should this succeed ???
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_FAIL( "Unexpected PdfError" );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }
    
    // XRef stream with /Index [0 0] array
    try
    {
        std::ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;
        
        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [0 0] ";
        oss << "/Size 5 ";
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        CPPUNIT_ASSERT( offsetEndstream-offsetStream-strlen("\r\n") == lengthXRefObject ); // hard-coded in /Length entry in XRef stream above
        
        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";
        
        PoDoFo::PdfVecObjects objects;
        PoDoFo::PdfParser::TVecOffsets offsets;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        
        PoDoFo::PdfXRefStreamParserObject xrefStreamParser(&objects, parser.GetDevice(),
                                                           parser.GetBuffer(), &offsets );
        
        offsets.resize(5);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        // should this succeed ???
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_FAIL( "Unexpected PdfError" );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }
    
    // XRef stream with /Index [-1 -1] array
    try
    {
        std::ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;
        
        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [-1 -1] ";
        oss << "/Size 5 ";
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        CPPUNIT_ASSERT( offsetEndstream-offsetStream-strlen("\r\n") == lengthXRefObject ); // hard-coded in /Length entry in XRef stream above
        
        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";
        
        PoDoFo::PdfVecObjects objects;
        PoDoFo::PdfParser::TVecOffsets offsets;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        
        PoDoFo::PdfXRefStreamParserObject xrefStreamParser(&objects, parser.GetDevice(),
                                                           parser.GetBuffer(), &offsets );
        
        offsets.resize(5);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        // should this succeed ???
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_FAIL( "Unexpected PdfError" );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }
    
    // XRef stream with /Index array with no entries
    try
    {
        std::ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;
        
        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [ ] ";
        oss << "/Size 5 ";
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        CPPUNIT_ASSERT( offsetEndstream-offsetStream-strlen("\r\n") == lengthXRefObject ); // hard-coded in /Length entry in XRef stream above
        
        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";
        
        PoDoFo::PdfVecObjects objects;
        PoDoFo::PdfParser::TVecOffsets offsets;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        
        PoDoFo::PdfXRefStreamParserObject xrefStreamParser(&objects, parser.GetDevice(),
                                                           parser.GetBuffer(), &offsets );
        
        offsets.resize(5);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        // should this succeed ???
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_FAIL( "Unexpected PdfError" );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }
    
    // XRef stream with /Index array with 3 entries
    try
    {
        std::ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;
        
        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2 2] ";
        oss << "/Size 5 ";
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        CPPUNIT_ASSERT( offsetEndstream-offsetStream-strlen("\r\n") == lengthXRefObject ); // hard-coded in /Length entry in XRef stream above
        
        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";
        
        PoDoFo::PdfVecObjects objects;
        PoDoFo::PdfParser::TVecOffsets offsets;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        
        PoDoFo::PdfXRefStreamParserObject xrefStreamParser(&objects, parser.GetDevice(),
                                                           parser.GetBuffer(), &offsets );
        
        offsets.resize(5);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        CPPUNIT_FAIL( "Should throw exception" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_NoXRef );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }
    
    // XRef stream with /Index array with 22 entries
    try
    {
        std::ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;
        
        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22] ";
        oss << "/Size 5 ";
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "00 0000 0\r\n";
        oss << "00 0000 00\r\n";
        oss << "00 0000 00\r\n";
        oss << "00 0000 00\r\n";
        oss << "00 0000 00\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        CPPUNIT_ASSERT( offsetEndstream-offsetStream-strlen("\r\n") == lengthXRefObject ); // hard-coded in /Length entry in XRef stream above
        
        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";
        
        PoDoFo::PdfVecObjects objects;
        PoDoFo::PdfParser::TVecOffsets offsets;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        
        PoDoFo::PdfXRefStreamParserObject xrefStreamParser(&objects, parser.GetDevice(),
                                                           parser.GetBuffer(), &offsets );
        
        offsets.resize(5);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        CPPUNIT_FAIL( "Should throw exception" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_NoXRef );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }
}

void ParserTest::testReadObjects()
{
    // CVE-2017-8378 - m_offsets out-of-bounds access when referenced encryption dictionary object doesn't exist
    try
    {
        // generate an xref section
        // xref
        // 0 3
        // 0000000000 65535 f 
        // 0000000018 00000 n 
        // 0000000077 00000 n
        // trailer << /Root 1 0 R /Size 3 >>
        // startxref
        // 0
        // %%EOF
        std::ostringstream oss;     
        oss << "%PDF–1.0\r\n"; 
        oss << "xref\r\n0 3\r\n";
        oss << generateXRefEntries(3);
        oss << "trailer << /Root 1 0 R /Size 3 /Encrypt 2 0 R >>\r\n";
        oss << "startxref 0\r\n";
        oss << "%EOF";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        parser.ReadTrailer();
        parser.ReadObjects();
        CPPUNIT_FAIL( "Should throw exception" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidEncryptionDict );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }    
}

void ParserTest::testIsPdfFile()
{
    try
    {
        std::string strInputStream = "%PDF-1.0";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        CPPUNIT_ASSERT( parser.IsPdfFile() );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_FAIL( "Unexpected PdfError" );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }

    try
    {
        std::string strInputStream = "%PDF-1.1";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        CPPUNIT_ASSERT( parser.IsPdfFile() );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_FAIL( "Unexpected PdfError" );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }

    try
    {
        std::string strInputStream = "%PDF-1.7";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        CPPUNIT_ASSERT( parser.IsPdfFile() );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_FAIL( "Unexpected PdfError" );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }

    try
    {
        std::string strInputStream = "%PDF-1.9";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        CPPUNIT_ASSERT( parser.IsPdfFile() );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_FAIL( "Unexpected PdfError" );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }

    try
    {
        std::string strInputStream = "%PDF-1.99";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        CPPUNIT_ASSERT( parser.IsPdfFile() );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_FAIL( "Unexpected PdfError" );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }    

    try
    {
        std::string strInputStream = "%!PS-Adobe-2.0";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        CPPUNIT_ASSERT( !parser.IsPdfFile() );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_FAIL( "Unexpected PdfError" );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }     

    try
    {
        std::string strInputStream = "GIF89a";
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, strInputStream.c_str(), strInputStream.length() );
        CPPUNIT_ASSERT( !parser.IsPdfFile() );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_FAIL( "Unexpected PdfError" );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Wrong exception type" );
    }     
}

void ParserTest::testNestedArrays()
{
    // test valid stream
    try
    {
        // generate an XRef stream with no deeply nested arrays
        std::ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        // XRef stream with 5 entries
        size_t lengthXRefObject = 57; 
        size_t offsetXRefObject = oss.str().length();        
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 5 ";
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        CPPUNIT_ASSERT( offsetEndstream-offsetStream-strlen("\r\n") == lengthXRefObject ); // hard-coded in /Length entry in XRef stream above

        // trailer        
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";
        
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        
        parser.SetupTrailer();
        parser.ReadXRefStreamContents( offsetXRefObject, false );
        // should succeed
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_FAIL( "Unexpected PdfError" );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }

    // CVE-2021-30470 - lots of [[[[[]]]]] brackets represent nested arrays which caused stack overflow
    try
    {
        // generate an XRef stream with deeply nested arrays
        std::ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;
        size_t maxNesting = getStackOverflowDepth(); // big enough to cause stack overflow
        // XRef stream with 5 entries
        size_t lengthXRefObject = 57; 
        size_t offsetXRefObject = oss.str().length();        
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 5 ";
        oss << "/W [1 2 1] ";

        // output [[[[[[[[[[[0]]]]]]]]]]]
        for ( size_t i = 0 ; i < maxNesting ; ++i )
        {
            oss << "[";
        }
        oss << "0";
        for ( size_t i = 0 ; i < maxNesting ; ++i )
        {
            oss << "]";
        }        
        oss << " ";

        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        CPPUNIT_ASSERT( offsetEndstream-offsetStream-strlen("\r\n") == lengthXRefObject ); // hard-coded in /Length entry in XRef stream above

        // trailer        
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";
        
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        
        parser.SetupTrailer();
        parser.ReadXRefStreamContents( offsetXRefObject, false );
        CPPUNIT_FAIL( "Should throw exception" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        // this must match the error value thrown by PdfRecursionGuard
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    } 
}

void ParserTest::testNestedDictionaries()
{
    // test valid stream
    try
    {
        // generate an XRef stream with no deeply nested dictionaries
        std::ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        // XRef stream with 5 entries
        size_t lengthXRefObject = 57; 
        size_t offsetXRefObject = oss.str().length();        
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 5 ";
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        CPPUNIT_ASSERT( offsetEndstream-offsetStream-strlen("\r\n") == lengthXRefObject ); // hard-coded in /Length entry in XRef stream above

        // trailer        
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";
        
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        
        parser.SetupTrailer();
        parser.ReadXRefStreamContents( offsetXRefObject, false );
        // should succeed
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_FAIL( "Unexpected PdfError" );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }

    // CVE-2021-30470 - lots of <<<>>> brackets represent nested dictionaries which caused stack overflow
    try
    {
        // generate an XRef stream with deeply nested dictionaries
        std::ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;
        size_t maxNesting = getStackOverflowDepth(); // big enough to cause stack overflow 
        // XRef stream with 5 entries
        size_t lengthXRefObject = 57; 
        size_t offsetXRefObject = oss.str().length();        
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 5 ";
        oss << "/W [1 2 1] ";

        // output << << << /Test 0 >> >> >>
        for ( size_t i = 0 ; i < maxNesting ; ++i )
        {
            oss << "<< ";
        }
        oss << " /Test 0";
        for ( size_t i = 0 ; i < maxNesting ; ++i )
        {
            oss << " >>";
        }        
        oss << " ";

        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        CPPUNIT_ASSERT( offsetEndstream-offsetStream-strlen("\r\n") == lengthXRefObject ); // hard-coded in /Length entry in XRef stream above

        // trailer        
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";
        
        PoDoFo::PdfVecObjects objects;
        PdfParserTestWrapper parser( &objects, oss.str().c_str(), oss.str().length() );
        
        parser.SetupTrailer();
        parser.ReadXRefStreamContents( offsetXRefObject, false );
        CPPUNIT_FAIL( "Should throw exception" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        // this must match the error value thrown by PdfRecursionGuard
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
    }
    catch( std::exception& ex )
    {
        CPPUNIT_FAIL( "Unexpected exception type" );
    }
}

void ParserTest::testNestedNameTree()
{
    // test for valid but deeply nested name tree
    // maxDepth must be less than GetMaxObjectCount otherwise PdfParser::ResizeOffsets
    // throws an error when reading the xref offsets table, and no outlines are read
    std::ostringstream oss;
    const long maxDepth = getStackOverflowDepth() - 6 - 1;
    //const long maxDepth = 8;
    const long numObjects = maxDepth + 6;
    std::vector<size_t> offsets( numObjects );
    size_t xrefOffset = 0;

    offsets[0] = 0;
    oss << "%PDF-1.0\r\n";

    offsets[1] = oss.tellp();
    oss << "1 0 obj<</Type/Catalog /Pages 2 0 R /Names 4 0 R>>endobj ";

    offsets[2] = oss.tellp();
    oss << "2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj ";
    
    offsets[3] = oss.tellp();
    oss << "3 0 obj<</Type/Page/MediaBox[0 0 3 3]>>endobj ";

    // the name dictionary
    offsets[4] = oss.tellp();
    oss << "4 0 obj<</Dests 5 0 R>>endobj ";

    // root of /Dests name tree
    offsets[5] = oss.tellp();
    oss << "5 0 obj<</Kids [6 0 R]>>endobj ";

    // create name tree nested to maxDepth where each intermediate node has one child
    // except single leaf node at maxDepth
    for ( int obj = 6 ; obj < numObjects ; ++obj )
    {
        offsets[obj] = oss.tellp();

        if ( obj < numObjects - 1 )
            oss << obj << " 0 obj<</Kids [" << obj+1 << " 0 R] /Limits [(A) (Z)]>>endobj ";
        else
            oss << obj << " 0 obj<</Limits [(A) (Z)] /Names [ (A) (Avalue) (Z) (Zvalue) ] >>endobj ";
    }

    // output xref table
    oss <<  "\r\n";
    xrefOffset = oss.tellp();
    oss << "xref\r\n";
    oss << "0 " << numObjects << "\r\n";

    oss << "0000000000 65535 f\r\n";

    for ( size_t obj = 1 ; obj < offsets.size() ; ++obj )
    {
        // write xref entries like
        // "0000000010 00000 n\r\n"
        char szXrefEntry[21];
        snprintf( szXrefEntry, 21, "%010zu 00000 n\r\n", offsets[obj] );

        oss << szXrefEntry;
    }

    oss << "trailer<</Size " << numObjects << "/Root 1 0 R>>\r\n";
    oss << "startxref\r\n";
    oss << xrefOffset << "\r\n";
    oss << "%%EOF";

    try
    {
        PoDoFo::PdfMemDocument doc;
        doc.LoadFromBuffer( oss.str().c_str(), oss.str().size(), true );

        PoDoFo::PdfNamesTree* pNamesObj = doc.GetNamesTree( PoDoFo::ePdfDontCreateObject );
        if ( pNamesObj != nullptr )
        {
            PoDoFo::PdfDictionary dict;
            pNamesObj->ToDictionary( PoDoFo::PdfName( "Dests" ), dict );
        }
        
        CPPUNIT_FAIL( "Should throw exception" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        // this must match the error value thrown by PdfRecursionGuard
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
    }
}

void ParserTest::testLoopingNameTree()
{
    std::string strNoLoop =
    "%PDF-1.0\r\n"
    "1 0 obj<</Type/Catalog/Pages 2 0 R /Names 4 0 R>>endobj 2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj 3 0 obj<</Type/Page/MediaBox[0 0 3 3]>>endobj 4 0 obj<</Dests 2 0 R>>endobj\r\n"
    "xref\r\n"
    "0 5\r\n"
    "0000000000 65535 f\r\n"
    "0000000010 00000 n\r\n"
    "0000000066 00000 n\r\n"
    "0000000115 00000 n\r\n"
    "0000000161 00000 n\r\n"
    "trailer<</Size 4/Root 1 0 R>>\r\n"
    "startxref\r\n"
    "192\r\n"
    "%%EOF";

    try
    {
        PoDoFo::PdfMemDocument doc;
        doc.LoadFromBuffer( strNoLoop.c_str(), strNoLoop.size(), true );

		PoDoFo::PdfNamesTree* pNamesObj = doc.GetNamesTree( PoDoFo::ePdfDontCreateObject );
		if ( pNamesObj != nullptr )
		{
			PoDoFo::PdfDictionary dict;
			pNamesObj->ToDictionary( PoDoFo::PdfName( "Dests" ), dict );
		}
        
        // should not throw
        CPPUNIT_ASSERT( true );
    } 
    catch ( PoDoFo::PdfError& error ) 
    {
        CPPUNIT_FAIL( "Unexpected PdfError" );
    }

    // CVE-2021-30471 /Dests points at pages tree root which has a /Kids entry loooping back to pages tree root
    std::string strSelfLoop =
    "%PDF-1.0\r\n"
    "1 0 obj<</Type/Catalog/Pages 2 0 R /Names 4 0 R>>endobj 2 0 obj<</Type/Pages/Kids[2 0 R]/Count 1>>endobj 3 0 obj<</Type/Page/MediaBox[0 0 3 3]>>endobj 4 0 obj<</Dests 2 0 R>>endobj\r\n"
    "xref\r\n"
    "0 5\r\n"
    "0000000000 65535 f\r\n"
    "0000000010 00000 n\r\n"
    "0000000066 00000 n\r\n"
    "0000000115 00000 n\r\n"
    "0000000161 00000 n\r\n"
    "trailer<</Size 4/Root 1 0 R>>\r\n"
    "startxref\r\n"
    "192\r\n"
    "%%EOF";

    try
    {
        PoDoFo::PdfMemDocument doc;
        doc.LoadFromBuffer( strSelfLoop.c_str(), strSelfLoop.size(), true );

        PoDoFo::PdfNamesTree* pNamesObj = doc.GetNamesTree( PoDoFo::ePdfDontCreateObject );
        if ( pNamesObj != nullptr )
        {
            PoDoFo::PdfDictionary dict;
            pNamesObj->ToDictionary( PoDoFo::PdfName( "Dests" ), dict );
        }
        
        CPPUNIT_FAIL( "Should throw exception" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        // this must match the error value thrown by PdfRecursionGuard
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
    }    
    
    // CVE-2021-30471 /Dests points at pages tree which has a /Kids entry loooping back to ancestor (document root)
    std::string strAncestorLoop =
    "%PDF-1.0\r\n"
    "1 0 obj<</Type/Catalog/Pages 2 0 R /Names 4 0 R>>endobj 2 0 obj<</Type/Pages/Kids[1 0 R]/Count 1>>endobj 3 0 obj<</Type/Page/MediaBox[0 0 3 3]>>endobj 4 0 obj<</Dests 2 0 R>>endobj\r\n"
    "xref\r\n"
    "0 5\r\n"
    "0000000000 65535 f\r\n"
    "0000000010 00000 n\r\n"
    "0000000066 00000 n\r\n"
    "0000000115 00000 n\r\n"
    "0000000161 00000 n\r\n"
    "trailer<</Size 4/Root 1 0 R>>\r\n"
    "startxref\r\n"
    "192\r\n"
    "%%EOF";

    try
    {
        PoDoFo::PdfMemDocument doc;
        doc.LoadFromBuffer( strAncestorLoop.c_str(), strAncestorLoop.size(), true );

        PoDoFo::PdfNamesTree* pNamesObj = doc.GetNamesTree( PoDoFo::ePdfDontCreateObject );
        if ( pNamesObj != nullptr )
        {
            PoDoFo::PdfDictionary dict;
            pNamesObj->ToDictionary( PoDoFo::PdfName( "Dests" ), dict );
        }
        
        CPPUNIT_FAIL( "Should throw exception" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidDataType );
    }
}

void ParserTest::testNestedPageTree()
{
    // test for valid but deeply nested page tree
    // maxDepth must be less than GetMaxObjectCount otherwise PdfParser::ResizeOffsets
    // throws an error when reading the xref offsets table, and no outlines are read
    std::ostringstream oss;
    const long maxDepth = getStackOverflowDepth() - 4 - 1;
    const long numObjects = maxDepth + 4 ;
    std::vector<size_t> offsets( numObjects );
    size_t xrefOffset = 0;

    offsets[0] = 0;
    oss << "%PDF-1.0\r\n";

    offsets[1] = oss.tellp();
    oss << "1 0 obj<</Type/Catalog /AcroForm 2 0 R /Pages 3 0 R>>endobj ";

    offsets[2] = oss.tellp();
    oss << "2 0 obj<</Type/AcroForm >>endobj ";
    
    offsets[3] = oss.tellp();
    oss << "3 0 obj<</Type/Pages /Kids [4 0 R] /Count 1 >>endobj ";

    // create pages tree nested to maxDepth where each node has one child 
    // except single leaf node at maxDepth
    for ( int obj = 4 ; obj < numObjects ; ++obj )
    {
        offsets[obj] = oss.tellp();

        if ( obj < numObjects - 1 )
            oss << obj << " 0 obj<</Type/Pages /Kids [" << obj+1 << " 0 R] /Parent " << obj-1 << " 0 R /Count 1 >>endobj ";
        else
            oss << obj << " 0 obj<</Type/Page  /Parent " << obj-1 << " 0 R >>endobj ";
    }

    // output xref table
    oss <<  "\r\n";
    xrefOffset = oss.tellp();
    oss << "xref\r\n";
    oss << "0 " << numObjects << "\r\n";

    oss << "0000000000 65535 f\r\n";

    for ( size_t obj = 1 ; obj < offsets.size() ; ++obj )
    {
        // write xref entries like
        // "0000000010 00000 n\r\n"
        char szXrefEntry[21];
        snprintf( szXrefEntry, 21, "%010zu 00000 n\r\n", offsets[obj] );

        oss << szXrefEntry;
    }

    oss << "trailer<</Size " << numObjects << "/Root 1 0 R>>\r\n";
    oss << "startxref\r\n";
    oss << xrefOffset << "\r\n";
    oss << "%%EOF";

    try
    {
        PoDoFo::PdfMemDocument doc;
        doc.LoadFromBuffer( oss.str().c_str(), oss.str().size(), true );

        for (int pageNo = 0; pageNo < doc.GetPageCount(); pageNo++)
        {
            PoDoFo::PdfPage* pPage = doc.GetPage( pageNo );
            CPPUNIT_ASSERT( pPage != NULL );
        }

        CPPUNIT_FAIL( "Should throw exception" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
    }
}

void ParserTest::testLoopingPageTree()
{
    // test PDF without nested kids
    std::string strNoLoop =
    "%PDF-1.0\r\n"
    "1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj 2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj 3 0 obj<</Type/Page/MediaBox[0 0 3 3]>>endobj\r\n"
    "xref\r\n"
    "0 4\r\n"
    "0000000000 65535 f\r\n"
    "0000000010 00000 n\r\n"
    "0000000053 00000 n\r\n"
    "0000000102 00000 n\r\n"
    "trailer<</Size 4/Root 1 0 R>>\r\n"
    "startxref\r\n"
    "149\r\n"
    "%%EOF";

    try
    {
        PoDoFo::PdfMemDocument doc;
        doc.LoadFromBuffer( strNoLoop.c_str(), strNoLoop.size(), true );

        for (int pageNo = 0; pageNo < doc.GetPageCount(); pageNo++)
        {
            PoDoFo::PdfPage* pPage = doc.GetPage( pageNo );
            CPPUNIT_ASSERT( pPage != NULL );
        }
                
        // should not throw
        CPPUNIT_ASSERT( true );
    } 
    catch ( PoDoFo::PdfError& error ) 
    {
        CPPUNIT_FAIL( "Unexpected PdfError" );
    }

    // CVE-2021-30471 test for pages tree /Kids array that refer back to pages tree root
    std::string strSelfLoop =
    "%PDF-1.0\r\n"
    "1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj 2 0 obj<</Type/Pages/Kids[2 0 R]/Count 1>>endobj 3 0 obj<</Type/Page/MediaBox[0 0 3 3]>>endobj\r\n"
    "xref\r\n"
    "0 4\r\n"
    "0000000000 65535 f\r\n"
    "0000000010 00000 n\r\n"
    "0000000053 00000 n\r\n"
    "0000000102 00000 n\r\n"
    "trailer<</Size 4/Root 1 0 R>>\r\n"
    "startxref\r\n"
    "149\r\n"
    "%%EOF";
    
    try
    {
        PoDoFo::PdfMemDocument doc;
        doc.LoadFromBuffer( strSelfLoop.c_str(), strSelfLoop.size(), true );

        for (int pageNo = 0; pageNo < doc.GetPageCount(); pageNo++)
        {
            PoDoFo::PdfPage* pPage = doc.GetPage( pageNo );
            CPPUNIT_ASSERT( pPage == NULL );
        }
        
        CPPUNIT_FAIL( "Should throw exception" );

    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_PageNotFound );
    }

    // CVE-2021-30471 test for pages tree /Kids array that refer back to an ancestor (document root object)
    std::string strAncestorLoop =
    "%PDF-1.0\r\n"
    "1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj 2 0 obj<</Type/Pages/Kids[1 0 R]/Count 1>>endobj 3 0 obj<</Type/Page/MediaBox[0 0 3 3]>>endobj\r\n"
    "xref\r\n"
    "0 4\r\n"
    "0000000000 65535 f\r\n"
    "0000000010 00000 n\r\n"
    "0000000053 00000 n\r\n"
    "0000000102 00000 n\r\n"
    "trailer<</Size 4/Root 1 0 R>>\r\n"
    "startxref\r\n"
    "149\r\n"
    "%%EOF";
    
    try
    {
        PoDoFo::PdfMemDocument doc;
        doc.LoadFromBuffer( strAncestorLoop.c_str(), strAncestorLoop.size(), true );

        for (int pageNo = 0; pageNo < doc.GetPageCount(); pageNo++)
        {
            PoDoFo::PdfPage* pPage = doc.GetPage( pageNo );
            CPPUNIT_ASSERT( pPage == NULL );
        }
        
        // should return null for doc.GetPage and not throw
        CPPUNIT_ASSERT( true );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_FAIL( "Unexpected PdfError" );
    }
}

void ParserTest::testNestedOutlines()
{
    // test for valid but deeply nested outlines
    // maxDepth must be less than GetMaxObjectCount otherwise PdfParser::ResizeOffsets
    // throws an error when reading the xref offsets table, and no outlines are read
    std::ostringstream oss;
    const long maxDepth = getStackOverflowDepth() - 4 - 1;
    const long numObjects = maxDepth + 4 ;
    std::vector<size_t> offsets( numObjects );
    size_t xrefOffset = 0;

    offsets[0] = 0;
    oss << "%PDF-1.0\r\n";

    offsets[1] = oss.tellp();
    oss << "1 0 obj<</Type/Catalog /AcroForm 2 0 R /Outlines 3 0 R>>endobj ";

    offsets[2] = oss.tellp();
    oss << "2 0 obj<</Type/AcroForm >>endobj ";
    
    offsets[3] = oss.tellp();
    oss << "3 0 obj<</Type/Outlines /First 4 0 R /Count " << maxDepth << " /Last 5 0 R >>endobj ";

    // create outlines tree nested to maxDepth where each node has one child 
    // except single leaf node at maxDepth
    for ( int obj = 4 ; obj < numObjects ; ++obj )
    {
        offsets[obj] = oss.tellp();

        if ( obj < numObjects - 1 )
            oss << obj << " 0 obj<</Title (Outline Item) /First " << obj+1 << " 0 R /Last " << obj+1 << " 0 R>>endobj ";
        else
            oss << obj << " 0 obj<</Title (Outline Item)>>endobj ";
    }

    // output xref table
    oss <<  "\r\n";
    xrefOffset = oss.tellp();
    oss << "xref\r\n";
    oss << "0 " << numObjects << "\r\n";

    oss << "0000000000 65535 f\r\n";

    for ( size_t obj = 1 ; obj < offsets.size() ; ++obj )
    {
        // write xref entries like
        // "0000000010 00000 n\r\n"
        char szXrefEntry[21];
        snprintf( szXrefEntry, 21, "%010zu 00000 n\r\n", offsets[obj] );

        oss << szXrefEntry;
    }

    oss << "trailer<</Size " << numObjects << "/Root 1 0 R>>\r\n";
    oss << "startxref\r\n";
    oss << xrefOffset << "\r\n";
    oss << "%%EOF";

    try
    {
        PoDoFo::PdfMemDocument doc;
        doc.LoadFromBuffer( oss.str().c_str(), oss.str().size(), true );

        // load should succeed, then GetOutlines goes recursive due to /Outlines deep nesting
        PoDoFo::PdfOutlines* pOutlines = doc.GetOutlines();
        CPPUNIT_ASSERT_MESSAGE( "Should throw exception", pOutlines != nullptr );
        CPPUNIT_FAIL( "Should throw exception" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
    }
}

void ParserTest::testLoopingOutlines()
{
    // CVE-2020-18971 - PdfOutlineItem /Next refers a preceding sibling
    std::string strNextLoop =
    "%PDF-1.0\r\n"
    "1 0 obj<</Type/Catalog /AcroForm 2 0 R /Outlines 3 0 R>>endobj "
    "2 0 obj<</Type/AcroForm >>endobj "
    "3 0 obj<</Type/Outlines /First 4 0 R /Count 2 /Last 5 0 R >>endobj "
    "4 0 obj<</Title (Outline Item 1) /Next 5 0 R>>endobj "
    "5 0 obj<</Title (Outline Item 2) /Next 4 0 R>>endobj " // /Next loops back to previous outline item
    "\r\n"
    "xref\r\n"
    "0 6\r\n"
    "0000000000 65535 f\r\n"
    "0000000010 00000 n\r\n"
    "0000000073 00000 n\r\n"
    "0000000106 00000 n\r\n"
    "0000000173 00000 n\r\n"
    "0000000226 00000 n\r\n"
    "trailer<</Size 6/Root 1 0 R>>\r\n"
    "startxref\r\n"
    "281\r\n"
    "%%EOF";

    try
    {
        PoDoFo::PdfMemDocument doc;
        doc.LoadFromBuffer( strNextLoop.c_str(), strNextLoop.size(), true );

        // load should succeed, then GetOutlines goes recursive due to /Outlines loop
        PoDoFo::PdfOutlines* pOutlines = doc.GetOutlines();
        CPPUNIT_ASSERT_MESSAGE( "Should throw exception", pOutlines != nullptr );
        CPPUNIT_FAIL( "Should throw exception" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
    }

    // https://sourceforge.net/p/podofo/tickets/25/
    std::string strSelfLoop =
    "%PDF-1.0\r\n"
    "1 0 obj<</Type/Catalog/Outlines 2 0 R>>endobj "
    "2 0 obj<</Type/Outlines /First 2 0 R /Last 2 0 R /Count 1>>endobj" // /First and /Last loop to self
    "\r\n"
    "xref\r\n"
    "0 3\r\n"
    "0000000000 65535 f\r\n"
    "0000000010 00000 n\r\n"
    "0000000056 00000 n\r\n"
    "trailer<</Size 3/Root 1 0 R>>\r\n"
    "startxref\r\n"
    "123\r\n"
    "%%EOF";

    try
    {
        PoDoFo::PdfMemDocument doc;
        doc.LoadFromBuffer( strSelfLoop.c_str(), strSelfLoop.size(), true );

        // load should succeed, then GetOutlines goes recursive due to /Outlines loop
        PoDoFo::PdfOutlines* pOutlines = doc.GetOutlines();
        CPPUNIT_ASSERT_MESSAGE( "Should throw exception", pOutlines != nullptr );
        CPPUNIT_FAIL( "Should throw exception" );
    }
    catch ( PoDoFo::PdfError& error )
    {
        CPPUNIT_ASSERT( error.GetError() == PoDoFo::ePdfError_InvalidXRef );
    }
}


void ParserTest::testRoundTripIndirectTrailerID()
{
    std::ostringstream oss;
    oss << "%PDF-1.1\n";
    int nCurObj = 0;
    int objPos[20];

    // Pages

    int nPagesObj = nCurObj;
    objPos[nCurObj] = oss.tellp();
    oss << nCurObj++ << " 0 obj\n";
    oss << "<</Type /Pages /Count 0 /Kids []>>\n";
    oss << "endobj";

    // Root catalog

    int rootObj = nCurObj;
    objPos[nCurObj] = oss.tellp();
    oss << nCurObj++ << " 0 obj\n";
    oss << "<</Type /Catalog /Pages " << nPagesObj << " 0 R>>\n";
    oss << "endobj\n";

    // ID
    int nIdObj = nCurObj;
    objPos[nCurObj] = oss.tellp();
    oss << nCurObj++ << " 0 obj\n";
    oss << "[<F1E375363A6314E3766EDF396D614748> <F1E375363A6314E3766EDF396D614748>]\n";
    oss << "endobj\n";

    int nXrefPos = oss.tellp();
    oss << "xref\n";
    oss << "0 " << nCurObj << "\n";
    char objRec[21];
    for ( int i = 0; i < nCurObj; i++ ) {
        snprintf( objRec, 21, "%010d 00000 n \n", objPos[i] );
        oss << objRec;
    }
    oss << "trailer <<\n"
        << "  /Size " << nCurObj << "\n"
        << "  /Root " << rootObj << " 0 R\n"
        << "  /ID " << nIdObj << " 0 R\n" // indirect ID
        << ">>\n"
        << "startxref\n"
        << nXrefPos << "\n"
        << "%%EOF\n";

    std::string sInBuf = oss.str();
    try {
        PoDoFo::PdfMemDocument doc;
        // load for update
        doc.LoadFromBuffer( sInBuf.c_str(), sInBuf.size(), true );

        PoDoFo::PdfRefCountedBuffer outBuf;
        PoDoFo::PdfOutputDevice outDev( &outBuf );

        doc.WriteUpdate( &outDev );
        // should not throw
        CPPUNIT_ASSERT( true );
    } catch ( PoDoFo::PdfError& error ) {
        CPPUNIT_FAIL( "Unexpected PdfError" );
    }
}

std::string ParserTest::generateXRefEntries( size_t count )
{
    std::string strXRefEntries;

    // generates a block of 20-byte xref entries
    // 0000000000 65535 f\r\n
    // 0000000120 00000 n\r\n    
    // 0000000120 00000 n\r\n
    // 0000000120 00000 n\r\n
	try
	{
		strXRefEntries.reserve(count * 20);
		for (size_t i = 0; i < count; ++i)
		{
			if (i == 0)
				strXRefEntries.append("0000000000 65535 f\r\n");
			else
				strXRefEntries.append("0000000120 00000 n\r\n");
		}
	}
	catch (std::exception& ex)
	{
		// if this fails it's a bug in the unit tests and not PoDoFo
		CPPUNIT_FAIL("generateXRefEntries memory allocation failure");
	}

    return strXRefEntries;
}

bool ParserTest::canOutOfMemoryKillUnitTests()
{
    // test if out of memory conditions will kill the unit test process
    // which prevents tests completing

#if defined(_WIN32) || defined(_WIN64)
    // on Windows 32/64 allocations close to size of VM address space always fail gracefully
    bool bCanTerminateProcess = false;
#elif defined( __APPLE__ )
    // on macOS/iOS allocations close to size of VM address space fail gracefully
    // unless Address Sanitizer (ASAN) is enabled
    #if __has_feature(address_sanitizer)
        // ASAN terminates the process if alloc fails - and using allocator_may_return_null=1
        // to continue after an allocation doesn't work in C++ because new returns null which is
        // forbidden by the C++ spec and terminates process when 'this' is dereferenced in constructor
        // see https://github.com/google/sanitizers/issues/295
        bool bCanTerminateProcess = true;   
    #else
        // if alloc fails following message is logged
        // *** mach_vm_map failed (error code=3)
        // *** error: can't allocate region
        // *** set a breakpoint in malloc_error_break to debug
        bool bCanTerminateProcess = false;
    #endif
#elif defined( __linux__ )
    // TODO do big allocs succeed then trigger OOM-killer fiasco??
    bool bCanTerminateProcess = false;
#else
    // other systems - assume big allocs faily gracefully and throw bad_alloc
    bool bCanTerminateProcess = false;
#endif

    return bCanTerminateProcess;
}

size_t ParserTest::getStackOverflowDepth()
{
    // calculate stack overflow depth - need to do this because a value that consistently overflows a 64-bit stack
    // doesn't work on 32-bit systems because they run out of heap in ReadObjects before they get a chance to overflow stack
    // this is because sizeof(PdfParserObject) = 472 bytes (and there's one of these for every object read)
    const size_t parserObjectSize = sizeof( PoDoFo::PdfParserObject );

#if defined(_WIN64)
    // 1 MB default stack size, 64-bit address space, Windows x64 ABI
    // each stack frame has at least 4 64-bit stack params, 4 64-bit register params, plus 64-bit return address
    // stack frame size increases if function contains local variables or more than 4 parameters  
    // see https://docs.microsoft.com/en-us/cpp/build/stack-usage?view=msvc-170
    const size_t stackSize = 1 * 1024 * 1024;
    const size_t frameSize = sizeof( void* ) * (4 + 4 + 1); // 4 stack params + 4 register params + return address
    const size_t maxFrames = stackSize / frameSize; // overflows at 14,563 recursive calls (or sooner if functions contain local variables)
#elif defined(_WIN32)
    // 1 MB default stack size, 32-bit address space (can't allocate more than 2GB), Windows x86 thiscall calling convention
    // each stack frame has at least 32-bit EBP and return address
    // stack frame size increases if function contains local variables or any parameters  
    const size_t stackSize = 1 * 1024 * 1024;
    const size_t frameSize = sizeof( void* ) * (1 + 1); // EBP and return address
    const size_t maxFrames = stackSize / frameSize; // overflows at 131,072 recursive calls (or sooner if functions contain local variables or has parameters)
#else
    // assume 8MB macOS / Linux default stack size, 64-bit address space, System V AMD64 ABI
    // each stack frame has at least 64-bit EBP and return address
    // stack frame size increases if function contains local variables or any parameters  
    const size_t stackSize = 8 * 1024 * 1024;
    const size_t frameSize = sizeof( void* ) * (1 + 1); // EBP and return address
    const size_t maxFrames = stackSize / frameSize; // overflows at 524,288 recursive calls (or sooner if functions contain local variables or has parameters)
#endif

    // add a few frames to sure we go beyond end of stack
    const size_t overflowDepth = maxFrames + 1000;

    // overflowDepth must be less than GetMaxObjectCount otherwise PdfParser::ResizeOffsets
    // throws an error when reading the xref offsets table, and no recursive calls are made
    // must also be allocate less than half of address space to prevent out-of-memory exceptions
    CPPUNIT_ASSERT( overflowDepth < static_cast <size_t> (PoDoFo::PdfParser::GetMaxObjectCount() ) );
    CPPUNIT_ASSERT( overflowDepth * parserObjectSize < std::numeric_limits<size_t>::max() / 2 );

    return overflowDepth;
}

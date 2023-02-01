/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

/*
    Notes:

    1) out of memory tests don't run if Address Santizer (ASAN) is enabled because
       ASAN terminates the unit test process the first time it attempts to allocate
       too much memory (so running the tests with and without ASAN is recommended)

    2) PoDoFo log warnings about inconsistencies or values out of range are expected
       because the tests are supplying invalid values to check PoDoFo behaves correctly
       in those situations
*/

#include <limits>

#include <sstream>

#include <PdfTest.h>

using namespace std;
using namespace PoDoFo;

static string generateXRefEntries(size_t count);
static bool canOutOfMemoryKillUnitTests();
static void testReadXRefSubsection();
static size_t getStackOverflowDepth();

// this value is from Table C.1 in Appendix C.2 Architectural Limits in PDF 32000-1:2008
// on 32-bit systems sizeof(PdfParser::TXRefEntry)=16 => max size of m_offsets=16*8,388,607 = 134 MB
// on 64-bit systems sizeof(PdfParser::TXRefEntry)=24 => max size of m_offsets=16*8,388,607 = 201 MB
constexpr unsigned maxNumberOfIndirectObjects = 8388607;

namespace PoDoFo
{
    class PdfParserTest : public PdfParser
    {
    public:
        PdfParserTest(PdfIndirectObjectList& objectList, string buff)
            : PdfParser(objectList), m_buffer(std::move(buff)), m_device(new SpanStreamDevice(m_buffer))
        {
        }

        void ReadXRefContents(size_t offset, bool positionAtEnd)
        {
            // call protected method
            PdfParser::ReadXRefContents(*m_device, offset, positionAtEnd);
        }

        void ReadXRefSubsection(int64_t firstObject, int64_t objectCount)
        {
            // call protected method
            PdfParser::ReadXRefSubsection(*m_device, firstObject, objectCount);
        }

        void ReadXRefStreamContents(size_t offset, bool readOnlyTrailer)
        {
            // call protected method
            PdfParser::ReadXRefStreamContents(*m_device, offset, readOnlyTrailer);
        }

        void ReadDocumentStructure()
        {
            // call protected method
            PdfParser::ReadDocumentStructure(*m_device);
        }

        void ReadObjects()
        {
            // call protected method
            PdfParser::ReadObjects(*m_device);
        }

        bool IsPdfFile()
        {
            // call protected method
            return PdfParser::IsPdfFile(*m_device);
        }

        const shared_ptr<InputStreamDevice>& GetDevice() { return m_device; }

    private:
        string m_buffer;
        shared_ptr<InputStreamDevice> m_device;
    };
}

TEST_CASE("TestMaxObjectCount")
{
    PdfParser::SetMaxObjectCount(numeric_limits<unsigned short>::max());
    testReadXRefSubsection();

    PdfParser::SetMaxObjectCount(maxNumberOfIndirectObjects);
    testReadXRefSubsection();
}

// NOTE: This test is too long to be normally done on every run
TEST_CASE("TestMaxObjectCount2", "[.]")
{
    PdfParser::SetMaxObjectCount(numeric_limits<unsigned>::max());
    testReadXRefSubsection();
}


TEST_CASE("TestReadXRefContents")
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
        ostringstream oss;
        oss << "xref\r\n0 3\r\n";
        oss << generateXRefEntries(3);
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref 0\r\n";
        oss << "%EOF";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, oss.str());
        parser.ReadXRefContents(0, false);
        // expected to succeed
    }
    catch (PdfError&)
    {
        FAIL("Should not throw PdfError");
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
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
        ostringstream oss;
        oss << "xref\r\n0 3\r\n";
        oss << generateXRefEntries(2); // 2 entries supplied, but expecting 3 entries
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref 0\r\n";
        oss << "%EOF";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, oss.str());
        parser.ReadXRefContents(0, false);
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRef);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
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
        ostringstream oss;
        oss << "xref\r\n0 5\r\n";
        oss << "000000000 65535\r\n";
        oss << "00000000065535 x\r\n";
        oss << "0000000\r\n";
        oss << generateXRefEntries(2);
        oss << "trailer << /Root 1 0 R /Size 5 >>\r\n";
        oss << "startxref 0\r\n";
        oss << "%EOF";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, oss.str());
        parser.ReadXRefContents(0, false);
        // succeeds reading badly formed xref entries  - should it?
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRef);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
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
        ostringstream oss;

        // object stream contents - length excludes trailing whitespace
        string streamContents =
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

        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, oss.str());
        parser.ReadXRefContents(offsetXrefStm1, false);
        // succeeds in current code - should it?
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRefType);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
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
        ostringstream oss;

        // object stream contents - length excludes trailing whitespace
        string streamContents =
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
        REQUIRE(offsetXrefStm2 == 185); // hard-coded in /Prev entry in XrefStm1 above
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

        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, oss.str());
        parser.ReadXRefContents(offsetXrefStm2, false);
        // succeeds in current code - should it?
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRefType);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
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
        ostringstream oss;
        size_t prevOffset = 0;
        size_t currentOffset = 0;

        // object stream contents - length excludes trailing whitespace
        string streamContents =
            "01 0E8A 0\r\n"
            "02 0002 00\r\n";
        size_t streamContentsLength = streamContents.size() - strlen("\r\n");

        // xref section at offset 0
        //size_t offsetXref = 0;
        oss << "xref\r\n0 1\r\n";
        oss << generateXRefEntries(1);

        // this caused stack overflow on macOS 64-bit with around 3000 streams
        // and on Windows 32-bit with around 1000 streams

        constexpr size_t maxXrefStreams = 10000;
        for (size_t i = 0; i < maxXrefStreams; i++)
        {
            size_t objNo = i + 2;

            // xrefstm at currentOffset linked back to stream at prevOffset
            prevOffset = currentOffset;
            currentOffset = oss.str().length();
            oss << objNo << " 0 obj ";
            oss << "<< /Type /XRef ";
            oss << "/Length " << streamContentsLength << " ";
            oss << "/Index [2 2] ";
            oss << "/Size 3 ";
            if (prevOffset > 0)
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

        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, oss.str());
        parser.ReadXRefContents(currentOffset, false);
        // succeeds in current code - should it?
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRefType);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }
}

void testReadXRefSubsection()
{
    int64_t firstObject = 0;
    int64_t objectCount = 0;

    // TODO does ReadXRefSubsection with objectCount = 0 make sense ???

    // CVE-2017-5855 m_offsets.resize() NULL ptr read
    // CVE-2017-6844 m_offsets.resize() buffer overwrite 
    // false positives due to AFL setting allocator_may_return_null=1 which causes
    // ASAN to return NULL instead of throwing bad_alloc for out-of-memory conditions
    // https://github.com/mirrorer/afl/blob/master/docs/env_variables.txt#L248
    // https://github.com/google/sanitizers/issues/295#issuecomment-234273218 
    // the test for CVE-2018-5296 below checks that PoDoFo restricts allocations

    // CVE-2018-5296 m_offsets.resize() malloc failure when large size specified
    // check PoDoFo throws PdfError and not anything derived from exception
    // check PoDoFo can't allocate unrestricted amounts of memory

    if (PdfParser::GetMaxObjectCount() <= maxNumberOfIndirectObjects)
    {
        try
        {
            string strInput = generateXRefEntries(PdfParser::GetMaxObjectCount());
            PdfIndirectObjectList objects;
            PdfParserTest parser(objects, strInput);
            firstObject = 0;
            objectCount = PdfParser::GetMaxObjectCount();
            parser.ReadXRefSubsection(firstObject, objectCount);
            // expected to succeed
        }
        catch (PdfError&)
        {
            FAIL("should not throw PdfError");
        }
        catch (exception&)
        {
            FAIL("Unexpected exception type");
        }
    }
    else
    {
        // test has been called from testMaxObjectCount with PdfParser::SetMaxObjectCount()
        // set to a large value (large allocs are tested in address space tests below)
    }

    // don't run the following test if PdfParser::GetMaxObjectCount()+1 will overflow
    // in the numXRefEntries calculation below (otherwise we get an ASAN error)
    if (PdfParser::GetMaxObjectCount() < numeric_limits<unsigned>::max())
    {
        // don't generate xrefs for high values of GetMaxObjectCount() e.g. don't try to generate 2**63 xrefs
        unsigned numXRefEntries = std::min(maxNumberOfIndirectObjects + 1, PdfParser::GetMaxObjectCount() + 1);

        try
        {
            string strInput = generateXRefEntries(numXRefEntries);
            PdfIndirectObjectList objects;
            PdfParserTest parser(objects, strInput);
            firstObject = 0;
            objectCount = (int64_t)PdfParser::GetMaxObjectCount() + 1;
            parser.ReadXRefSubsection(firstObject, objectCount);
            FAIL("PdfError not thrown");
        }
        catch (PdfError& error)
        {
            // too many indirect objects in Trailer /Size key throws PdfErrorCode::ValueOutOfRange
            // but too many indirect objects in xref table throws PdfErrorCode::InvalidXRef
            REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRef);
        }
        catch (exception&)
        {
            FAIL("Wrong exception type");
        }
    }

    // CVE-2018-5296 try to allocate more than address space size 
    // should throw a bad_length exception in STL which is rethrown as a PdfError
    try
    {
        // this attempts to allocate numeric_limits<size_t>::max()/2 * sizeof(TXRefEntry)
        // on 32-bit systems this allocates 2**31 * sizeof(TXRefEntry) = 2**31 * 16 (larger than 32-bit address space)
        // on LP64 (macOS,*nix) systems this allocates 2**63 * sizeof(TXRefEntry) = 2**63 * 24 (larger than 64-bit address space)
        // on LLP64 (Win64) systems this allocates 2**31 * sizeof(TXRefEntry) = 2**31 * 16 (smaller than 64-bit address space)
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        firstObject = 1;
        objectCount = numeric_limits<size_t>::max() / 2 - 1;
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        // if objectCount > PdfParser::GetMaxObjectCount() then we'll see PdfErrorCode::InvalidXRef
        // otherwise we'll see PdfErrorCode::ValueOutOfRange or PdfErrorCode::OutOfMemory (see testMaxObjectCount)
        REQUIRE((error.GetCode() == PdfErrorCode::InvalidXRef
            || error.GetCode() == PdfErrorCode::ValueOutOfRange
            || error.GetCode() == PdfErrorCode::OutOfMemory));
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2018-5296 try to allocate 95% of VM address space size (which should always fail)
    if (!canOutOfMemoryKillUnitTests())
    {
        constexpr size_t maxObjects = numeric_limits<size_t>::max() / sizeof(PdfXRefEntry) / 100 * 95;

        try
        {
            string strInput = " ";
            PdfIndirectObjectList objects;
            PdfParserTest parser(objects, strInput);
            firstObject = 1;
            objectCount = maxObjects;
            parser.ReadXRefSubsection(firstObject, objectCount);
            FAIL("PdfError not thrown");
        }
        catch (PdfError& error)
        {
            if (maxObjects >= (size_t)PdfParser::GetMaxObjectCount())
                REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRef);
            else
                REQUIRE(error.GetCode() == PdfErrorCode::OutOfMemory);
        }
        catch (exception&)
        {
            FAIL("Wrong exception type");
        }
    }

    // CVE-2015-8981 happens because this->GetNextNumber() can return negative numbers 
    // in range (LONG_MIN to LONG_MAX) so the xref section below causes a buffer underflow
    // because m_offsets[-5].bParsed is set to true when first entry is read
    // NOTE: vector operator[] is not bounds checked

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
        string strInput = "0000000000 65535 f\r\n";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        firstObject = -5LL;
        objectCount = 5;
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE((error.GetCode() == PdfErrorCode::ValueOutOfRange || error.GetCode() == PdfErrorCode::NoXRef));
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2015-8981 can also happen due to integer overflow in firstObject+objectCount
    // in the example below 2147483647=0x7FFF, so 0x7FFF + 0x7FFF = 0XFFFE = -2 on a 32-bit system
    // which means m_offsets.size()=5 because m_offsets.resize() is never called and 
    // m_offsets[2147483647].bParsed is set to true when first entry is read
    // NOTE: vector operator[] is not bounds checked

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
        string strInput = "0000000000 65535 f\r\n";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        firstObject = numeric_limits<unsigned>::max();
        objectCount = numeric_limits<unsigned>::max();
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRef);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    try
    {
        string strInput = "0000000000 65535 f\r\n";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        firstObject = numeric_limits<int64_t>::max();
        objectCount = numeric_limits<int64_t>::max();
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::ValueOutOfRange);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // test for integer overflows in ReadXRefSubsection (CVE-2017-5853) which caused
    // wrong buffer size to be calculated and then triggered buffer overflow (CVE-2017-6844)   
    // the overflow checks in ReadXRefSubsection depend on the value returned by GetMaxObjectCount
    // if the value changes these checks need looked at again
    REQUIRE(PdfParser::GetMaxObjectCount() <= numeric_limits<unsigned>::max());

    // test CVE-2017-5853 signed integer overflow in firstObject + objectCount
    // CVE-2017-5853 1.1 - firstObject < 0
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        firstObject = -1LL;
        objectCount = 1;
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::ValueOutOfRange);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 1.2 - firstObject = min value of unsigned
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        firstObject = numeric_limits<unsigned>::min();
        objectCount = 1;
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::UnexpectedEOF);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 1.3 - firstObject = min value of int64_t
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        firstObject = numeric_limits<int64_t>::min();
        objectCount = 1;
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::ValueOutOfRange);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 1.4 - firstObject = min value of size_t is zero (size_t is unsigned)
    // and zero is a valid value for firstObject

    // CVE-2017-5853 1.5 - firstObject = max value of unsigned
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        firstObject = numeric_limits<unsigned>::max();
        objectCount = 1;
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRef);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 1.6 - firstObject = max value of int64_t
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        firstObject = numeric_limits<int64_t>::max();
        objectCount = 1;
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::ValueOutOfRange);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 1.7 - firstObject = max value of size_t
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        firstObject = numeric_limits<size_t>::max();
        objectCount = 1;
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        // weird: different errors returned depending on architecture 
        REQUIRE((error.GetCode() == PdfErrorCode::ValueOutOfRange || sizeof(size_t) == 4));
        REQUIRE((error.GetCode() == PdfErrorCode::InvalidXRef || sizeof(size_t) == 8));
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 1.8 - firstObject = PdfParser::GetMaxObjectCount()
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        REQUIRE(PdfParser::GetMaxObjectCount() > 0);
        firstObject = PdfParser::GetMaxObjectCount();
        objectCount = 1;
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRef);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 2.1 - objectCount < 0
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        firstObject = 1;
        objectCount = -1LL;
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::ValueOutOfRange);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 2.2 - objectCount = min value of int
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        firstObject = 1;
        objectCount = numeric_limits<int>::min();
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::ValueOutOfRange);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 2.3 - objectCount = min value of int64_t
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        firstObject = 1;
        objectCount = numeric_limits<int64_t>::min();
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::ValueOutOfRange);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 2.4 - objectCount = min value of size_t is zero (size_t is unsigned)
    // and zero is a valid value for firstObject
    // TODO

    // CVE-2017-5853 2.5 - objectCount = max value of unsigned
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        firstObject = 1;
        objectCount = numeric_limits<unsigned>::max();
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRef);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 2.6 - objectCount = max value of int64_t
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        firstObject = 1;
        objectCount = numeric_limits<int64_t>::max();
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::ValueOutOfRange);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 2.7 - objectCount = max value of size_t
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        firstObject = 1;
        objectCount = numeric_limits<size_t>::max();
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        // weird: different errors returned depending on architecture 
        REQUIRE((error.GetCode() == PdfErrorCode::ValueOutOfRange || sizeof(size_t) == 4));
        REQUIRE((error.GetCode() == PdfErrorCode::InvalidXRef || sizeof(size_t) == 8));
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 2.8 - objectCount = PdfParser::GetMaxObjectCount()
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        firstObject = 1;
        objectCount = PdfParser::GetMaxObjectCount();
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRef);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 2.9 - finally - loop through a set of interesting bit patterns
    static uint64_t s_values[] =
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

        (uint64_t)-1,
        0,
        1
    };
    constexpr size_t numValues = sizeof(s_values) / sizeof(s_values[0]);

    for (size_t i = 0; i < numValues; i++)
    {
        for (size_t j = 0; j < numValues; j++)
        {
            try
            {
                string strInput = " ";
                PdfIndirectObjectList objects;
                PdfParserTest parser(objects, strInput);
                firstObject = s_values[i];
                objectCount = s_values[j];

                if (canOutOfMemoryKillUnitTests() && (firstObject > maxNumberOfIndirectObjects || objectCount > maxNumberOfIndirectObjects))
                {
                    // can't call this in test environments where an out-of-memory condition terminates
                    // unit test process before all tests have run (e.g. AddressSanitizer)
                }
                else
                {
                    parser.ReadXRefSubsection(firstObject, objectCount);
                    // some combinations of firstObject/objectCount from s_values are legal - so we expect to reach here sometimes
                }
            }
            catch (PdfError& error)
            {
                // other combinations of firstObject/objectCount from s_values are illegal 
                // if we reach here it should be an invalid xref value of some type
                REQUIRE((error.GetCode() == PdfErrorCode::InvalidXRef || error.GetCode() == PdfErrorCode::ValueOutOfRange
                    || error.GetCode() == PdfErrorCode::UnexpectedEOF
                    || error.GetCode() == PdfErrorCode::OutOfMemory));
            }
            catch (exception&)
            {
                // and should never reach here
                FAIL("Wrong exception type");
            }
        }
    }
}

TEST_CASE("testReadXRefStreamContents")
{
    // test valid stream
    try
    {
        // generate an XRef stream with valid /W values
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        // XRef stream with 5 entries
        size_t lengthXRefObject = 58;
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
        oss << "01 0E8A 00\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

        // trailer
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%%EOF";

        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, oss.str());
        parser.ReadXRefStreamContents(offsetXRefObject, false);
        // should succeed
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // CVE-2018-5295: integer overflow caused by checking sum of /W entry values /W [ 1 2 9223372036854775807 ]
    // see https://bugzilla.redhat.com/show_bug.cgi?id=1531897 (/W values used were extracted from PoC file)
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        // XRef stream
        size_t lengthXRefObject = 58;
        size_t offsetXRefObject = 10;
        oss << "%PDF-1.4\r\n";
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
        oss << "01 0E8A 00\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // check /Length entry in XRef stream above

        // trailer
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<SpanStreamDevice>(inputStr);
        PdfMemDocument doc;
        // Parse a doc using XRef stream with invalid /W entries
        doc.LoadFromDevice(device);
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::NoXRef);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // CVE-2017-8787: heap based overflow caused by unchecked /W entry values /W [ 1 -4 2 ]
    // see https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=861738 for value of /W array
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        // XRef stream
        size_t lengthXRefObject = 58;
        size_t offsetXRefObject = 10;
        oss << "%PDF-1.4\r\n";
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
        oss << "01 0E8A 00\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // check /Length entry in XRef stream above

        // trailer
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<SpanStreamDevice>(inputStr);
        PdfMemDocument doc;
        // Parse a doc using XRef stream with invalid /W entries
        doc.LoadFromDevice(device);
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::NoXRef);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // /W entry values /W [ 4095 1 1 ] for data in form 02 0002 00 (doesn't match size of entry)
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        // XRef stream
        size_t lengthXRefObject = 58;
        size_t offsetXRefObject = 10;
        oss << "%PDF-1.4\r\n";
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
        oss << "01 0E8A 00\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE((offsetEndstream - offsetStream - strlen("\r\n")) == lengthXRefObject); // check /Length entry in XRef stream above

        // trailer
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<SpanStreamDevice>(inputStr);
        PdfMemDocument doc;
        // Parse a doc using XRef stream with invalid /W entries
        doc.LoadFromDevice(device);
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRefStream);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // /W entry values /W [ 4 4 4 ] for data in form 02 0002 00 (doesn't match size of entry)
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        // XRef stream
        size_t lengthXRefObject = 58;
        size_t offsetXRefObject = 10;
        oss << "%PDF-1.4\r\n";
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
        oss << "01 0E8A 00\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // check /Length entry in XRef stream above

        // trailer
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<SpanStreamDevice>(inputStr);
        PdfMemDocument doc;
        // Parse a doc using XRef stream with invalid /W entries
        doc.LoadFromDevice(device);
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRefType);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // /W entry values /W [ 1 4 4 ] (size=9) for data 01 0E8A 0\r\n02 0002 00\r\n (size=8 bytes)
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        // XRef stream
        size_t lengthXRefObject = 22;
        size_t offsetXRefObject = 10;
        oss << "%PDF-1.4\r\n";
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
        oss << "01 0E8A 00\r\n";
        oss << "02 0002 00\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // check /Length entry in XRef stream above

        // trailer
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<SpanStreamDevice>(inputStr);
        PdfMemDocument doc;
        // Parse a doc using XRef stream with invalid /W entries
        doc.LoadFromDevice(device);
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::NoXRef);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        size_t lengthXRefObject = 22;
        size_t offsetXRefObject = 34;
        oss << "%PDF-1.4\r\n";
        oss << "1 0 obj\r\n";
        oss << "<< >>\r\n";
        oss << "endobj\r\n";
        oss << "2 0 obj\r\n";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [1 2] ";
        oss << "/Root 1 0 R ";
        oss << "/Size 3 ";
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 000A 00\r\n";
        oss << "01 001A 00\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

        // trailer
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<SpanStreamDevice>(inputStr);
        PdfMemDocument doc;
        doc.LoadFromDevice(device);
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // XRef stream with 5 entries but /Size 2 specified
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        size_t lengthXRefObject = 58;
        size_t offsetXRefObject = 10;
        oss << "%PDF-1.4\r\n";
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
        oss << "01 0E8A 00\r\n";
        oss << "01 0002 00\r\n";
        oss << "01 0002 01\r\n";
        oss << "01 0002 02\r\n";
        oss << "01 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

        // trailer
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<SpanStreamDevice>(inputStr);
        PdfMemDocument doc;
        doc.LoadFromDevice(device);
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        (void)error;
        // FIXME We should throw on exact error
        //REQUIRE(error.GetError() == PdfErrorCode::NoXRef);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // XRef stream with 5 entries but /Size 10 specified
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        size_t lengthXRefObject = 58;
        size_t offsetXRefObject = 10;
        oss << "%PDF-1.4\r\n";
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
        oss << "01 0E8A 00\r\n";
        oss << "01 0002 00\r\n";
        oss << "01 0002 01\r\n";
        oss << "01 0002 02\r\n";
        oss << "01 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

        // trailer
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<SpanStreamDevice>(inputStr);
        PdfMemDocument doc;
        doc.LoadFromDevice(device);
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        (void)error;
        // FIXME We should throw on exact error
        //REQUIRE(error.GetError() == PdfErrorCode::NoXRef);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // XRef stream with /Index [0 0] array
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        size_t lengthXRefObject = 58;
        size_t offsetXRefObject = 10;
        oss << "%PDF-1.4\r\n";
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
        oss << "01 0E8A 00\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

        // trailer
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<SpanStreamDevice>(inputStr);
        PdfMemDocument doc;
        doc.LoadFromDevice(device);
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        (void)error;
        // FIXME We should throw on exact error
        //REQUIRE(error.GetError() == PdfErrorCode::NoXRef);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // XRef stream with /Index [-1 -1] array
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        size_t lengthXRefObject = 58;
        size_t offsetXRefObject = 10;
        oss << "%PDF-1.4\r\n";
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
        oss << "01 0E8A 00\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

        // trailer
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<SpanStreamDevice>(inputStr);
        PdfMemDocument doc;
        doc.LoadFromDevice(device);
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        (void)error;
        // FIXME We should throw on exact error
        //REQUIRE(error.GetError() == PdfErrorCode::NoXRef);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // XRef stream with /Index array with no entries
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        size_t lengthXRefObject = 58;
        size_t offsetXRefObject = 10;
        oss << "%PDF-1.4\r\n";
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
        oss << "01 0E8A 00\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

        // trailer
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<SpanStreamDevice>(inputStr);
        PdfMemDocument doc;
        doc.LoadFromDevice(device);
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        (void)error;
        // FIXME We should throw on exact error
        //REQUIRE(error.GetError() == PdfErrorCode::NoXRef);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // XRef stream with /Index array with 3 entries
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        size_t lengthXRefObject = 58;
        size_t offsetXRefObject = 10;
        oss << "%PDF-1.4\r\n";
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
        oss << "01 0E8A 00\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

        // trailer
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<SpanStreamDevice>(inputStr);
        PdfMemDocument doc;
        doc.LoadFromDevice(device);
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::NoXRef);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // XRef stream with /Index array with 22 entries
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        size_t lengthXRefObject = 58;
        size_t offsetXRefObject = 10;
        oss << "%PDF-1.4\r\n";
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
        oss << "00 0000 00\r\n";
        oss << "00 0000 00\r\n";
        oss << "00 0000 00\r\n";
        oss << "00 0000 00\r\n";
        oss << "00 0000 00\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

        // trailer
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<SpanStreamDevice>(inputStr);
        PdfMemDocument doc;
        doc.LoadFromDevice(device);
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::NoXRef);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }
}

TEST_CASE("testReadObjects")
{
    // CVE-2017-8378 - m_offsets out-of-bounds access when referenced encryption dictionary object doesn't exist
    try
    {
        ostringstream oss;
        oss << "%PDF-1.0\r\n";
        oss << "xref\r\n0 3\r\n";
        oss << generateXRefEntries(3);
        oss << "trailer << /Root 1 0 R /Size 3 /Encrypt 3 0 R >>\r\n";
        oss << "startxref 0\r\n";
        oss << "%%EOF";
        PdfIndirectObjectList objects;
        auto docbuff = oss.str();
        PdfParserTest parser(objects, docbuff);
        parser.ReadDocumentStructure();
        parser.ReadObjects();
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidEncryptionDict);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }
}

TEST_CASE("testIsPdfFile")
{
    try
    {
        string strInput = "%PDF-1.0";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        REQUIRE(parser.IsPdfFile());
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    try
    {
        string strInput = "%PDF-1.1";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        REQUIRE(parser.IsPdfFile());
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    try
    {
        string strInput = "%PDF-1.7";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        REQUIRE(parser.IsPdfFile());
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    try
    {
        string strInput = "%PDF-1.9";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        REQUIRE(!parser.IsPdfFile());
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    try
    {
        string strInput = "%PDF-2.0";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        REQUIRE(parser.IsPdfFile());
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    try
    {
        string strInput = "%!PS-Adobe-2.0";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        REQUIRE(!parser.IsPdfFile());
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    try
    {
        string strInput = "GIF89a";
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, strInput);
        REQUIRE(!parser.IsPdfFile());
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }
}

TEST_CASE("testSaveIncrementalRoundTrip")
{
    ostringstream oss;
    oss << "%PDF-1.1\n";
    unsigned currObj = 1;
    streamoff objPos[20];

    // Pages

    unsigned pagesObj = currObj;
    objPos[currObj] = oss.tellp();
    oss << currObj++ << " 0 obj\n";
    oss << "<</Type /Pages /Count 0 /Kids []>>\n";
    oss << "endobj\n";

    // Root catalog

    unsigned rootObj = currObj;
    objPos[currObj] = oss.tellp();
    oss << currObj++ << " 0 obj\n";
    oss << "<</Type /Catalog /Pages " << pagesObj << " 0 R>>\n";
    oss << "endobj\n";

    // ID
    unsigned idObj = currObj;
    objPos[currObj] = oss.tellp();
    oss << currObj++ << " 0 obj\n";
    oss << "[<F1E375363A6314E3766EDF396D614748> <F1E375363A6314E3766EDF396D614748>]\n";
    oss << "endobj\n";

    streamoff xrefPos = oss.tellp();
    oss << "xref\n";
    oss << "0 " << currObj << "\n";
    oss << "0000000000 65535 f \n";
    for (unsigned i = 1; i < currObj; i++)
        oss << utls::Format("{:010d} 00000 n \n", objPos[i]);

    oss << "trailer <<\n"
        << "  /Size " << currObj << "\n"
        << "  /Root " << rootObj << " 0 R\n"
        << "  /ID " << idObj << " 0 R\n" // indirect ID
        << ">>\n"
        << "startxref\n"
        << xrefPos << "\n"
        << "%%EOF\n";

    string docBuff = oss.str();
    try
    {
        PdfMemDocument doc;
        // load for update
        doc.LoadFromBuffer(docBuff);

        StringStreamDevice outDev(docBuff);

        doc.SaveUpdate(outDev);
        doc.LoadFromBuffer(docBuff);
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
}

// CVE-2018-8002, CVE-2021-30470
TEST_CASE("testNestedArrays")
{
    // test valid stream
    // generate an XRef stream with no deeply nested arrays
    ostringstream oss;
    size_t offsetStream;
    size_t offsetEndstream;
    size_t offsetXRefObject;
    string buffer;

    // XRef stream with 5 entries
    constexpr size_t lengthXRefObject = 58;

    offsetXRefObject = oss.str().length();
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
    oss << "01 0E8A 00\r\n";
    oss << "02 0002 00\r\n";
    oss << "02 0002 01\r\n";
    oss << "02 0002 02\r\n";
    oss << "02 0002 03\r\n";
    offsetEndstream = oss.str().length();
    oss << "endstream\r\n";
    oss << "endobj\r\n";
    REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

    // trailer
    oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
    oss << "startxref " << offsetXRefObject << "\r\n";
    oss << "%EOF";

    buffer = oss.str();

    {
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, buffer);
        parser.ReadXRefStreamContents(offsetXRefObject, false);
        REQUIRE(true);
    }

    // CVE-2021-30470 - lots of [[[[[]]]]] brackets represent nested arrays which caused stack overflow
    try
    {
        // generate an XRef stream with deeply nested arrays
        oss.str("");
        const size_t maxNesting = getStackOverflowDepth(); // big enough to cause stack overflow
        // XRef stream with 5 entries
        offsetXRefObject = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 5 ";
        oss << "/W [1 2 1] ";
        
        // output [[[[[[[[[[[0]]]]]]]]]]]
        for (size_t i = 0; i < maxNesting; i++)
        {
            oss << "[";
        }
        oss << "0";
        for (size_t i = 0; i < maxNesting; i++)
        {
            oss << "]";
        }
        oss << " ";

        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 00\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";

        buffer = oss.str();

        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, buffer);
        parser.ReadXRefStreamContents(offsetXRefObject, false);
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        // this must match the error value thrown by PdfRecursionGuard
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRef);
    }
}

// CVE-2018-8002, CVE-2021-30470
TEST_CASE("testNestedDictionaries")
{
    // test valid stream
    // generate an XRef stream with no deeply nested dictionaries
    ostringstream oss;
    size_t offsetStream;
    size_t offsetEndstream;
    size_t offsetXRefObject;
    string buffer;

    // XRef stream with 5 entries
    constexpr size_t lengthXRefObject = 58;

    offsetXRefObject = oss.str().length();
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
    oss << "01 0E8A 00\r\n";
    oss << "02 0002 00\r\n";
    oss << "02 0002 01\r\n";
    oss << "02 0002 02\r\n";
    oss << "02 0002 03\r\n";
    offsetEndstream = oss.str().length();
    oss << "endstream\r\n";
    oss << "endobj\r\n";
    REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

    // trailer
    oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
    oss << "startxref " << offsetXRefObject << "\r\n";
    oss << "%EOF";

    buffer = oss.str();

    {
        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, buffer);
        parser.ReadXRefStreamContents(offsetXRefObject, false);
        REQUIRE(true);
    }

    // CVE-2021-30470 - lots of <<<>>> brackets represent nested dictionaries which caused stack overflow
    try
    {
        // generate an XRef stream with deeply nested dictionaries
        oss.str("");

        const size_t maxNesting = getStackOverflowDepth(); // big enough to cause stack overflow 
        // XRef stream with 5 entries
        offsetXRefObject = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 5 ";
        oss << "/W [1 2 1] ";

        // output << << << /Test 0 >> >> >>
        for (size_t i = 0; i < maxNesting; i++)
        {
            oss << "<< ";
        }
        oss << " /Test 0";
        for (size_t i = 0; i < maxNesting; i++)
        {
            oss << " >>";
        }
        oss << " ";

        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 00\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";

        buffer = oss.str();

        PdfIndirectObjectList objects;
        PdfParserTest parser(objects, buffer);
        parser.ReadXRefStreamContents(offsetXRefObject, false);
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        // this must match the error value thrown by PdfRecursionGuard
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRef);
    }
}

// CVE-2021-30471
TEST_CASE("testNestedNameTree")
{
    // test for valid but deeply nested name tree
    // maxDepth must be less than GetMaxObjectCount otherwise PdfParser::ResizeOffsets
    // throws an error when reading the xref offsets table, and no outlines are read
    ostringstream oss;
    const size_t maxDepth = getStackOverflowDepth() - 6 - 1;
    const size_t numObjects = maxDepth + 6;
    vector<size_t> offsets(numObjects);
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
    for (size_t objNo = 6; objNo < numObjects; objNo++)
    {
        offsets[objNo] = oss.tellp();

        if (objNo < numObjects - 1)
            oss << objNo << " 0 obj<</Kids [" << objNo + 1 << " 0 R] /Limits [(A) (Z)]>>endobj ";
        else
            oss << objNo << " 0 obj<</Limits [(A) (Z)] /Names [ (A) (Avalue) (Z) (Zvalue) ] >>endobj ";
    }

    // output xref table
    oss << "\r\n";
    xrefOffset = oss.tellp();
    oss << "xref\r\n";
    oss << "0 " << numObjects << "\r\n";

    oss << "0000000000 65535 f\r\n";

    for (size_t objNo = 1; objNo < offsets.size(); objNo++)
    {
        // write xref entries like
        // "0000000010 00000 n\r\n"
        char refEntry[21];
        snprintf(refEntry, 21, "%010zu 00000 n\r\n", offsets[objNo]);

        oss << refEntry;
    }

    oss << "trailer<</Size " << numObjects << "/Root 1 0 R>>\r\n";
    oss << "startxref\r\n";
    oss << xrefOffset << "\r\n";
    oss << "%%EOF";

    auto buffer = oss.str();

    try
    {
        PdfMemDocument doc;
        doc.LoadFromBuffer(buffer);

        auto names = doc.GetNames();
        if (names != nullptr)
        {
            PdfDictionary dict;
            names->ToDictionary("Dests", dict);
        }

        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        // this must match the error value thrown by PdfRecursionGuard
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRef);
    }
}

// CVE-2021-30471
TEST_CASE("testLoopingNameTree")
{
    string strNoLoop =
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

    {
        PdfMemDocument doc;
        doc.LoadFromBuffer(strNoLoop);

        auto names = doc.GetNames();
        if (names != nullptr)
        {
            PdfDictionary dict;
            names->ToDictionary("Dests", dict);
        }
    }

    // CVE-2021-30471 /Dests points at pages tree root which has a /Kids entry loooping back to pages tree root
    string strSelfLoop =
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
        PdfMemDocument doc;
        doc.LoadFromBuffer(strSelfLoop);

        auto names = doc.GetNames();
        if (names != nullptr)
        {
            PdfDictionary dict;
            names->ToDictionary("Dests", dict);
        }

        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        // this must match the error value thrown by PdfRecursionGuard
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRef);
    }

    // CVE-2021-30471 /Dests points at pages tree which has a /Kids entry loooping back to ancestor (document root)
    string strAncestorLoop =
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
        PdfMemDocument doc;
        doc.LoadFromBuffer(strAncestorLoop);

        auto names = doc.GetNames();
        if (names != nullptr)
        {
            PdfDictionary dict;
            names->ToDictionary("Dests", dict);
        }

        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidDataType);
    }
}

// CVE-2021-30471
TEST_CASE("testNestedPageTree")
{
    // test for valid but deeply nested page tree
    // maxDepth must be less than GetMaxObjectCount otherwise PdfParser::ResizeOffsets
    // throws an error when reading the xref offsets table, and no outlines are read
    ostringstream oss;
    const size_t maxDepth = getStackOverflowDepth() - 4 - 1;
    const size_t numObjects = maxDepth + 4;
    vector<size_t> offsets(numObjects);
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
    for (size_t objNo = 4; objNo < numObjects; objNo++)
    {
        offsets[objNo] = oss.tellp();

        if (objNo < numObjects - 1)
            oss << objNo << " 0 obj<</Type/Pages /Kids [" << objNo + 1 << " 0 R] /Parent " << objNo - 1 << " 0 R /Count 1 >>endobj ";
        else
            oss << objNo << " 0 obj<</Type/Page  /Parent " << objNo - 1 << " 0 R >>endobj ";
    }

    // output xref table
    oss << "\r\n";
    xrefOffset = oss.tellp();
    oss << "xref\r\n";
    oss << "0 " << numObjects << "\r\n";

    oss << "0000000000 65535 f\r\n";

    for (size_t objNo = 1; objNo < offsets.size(); objNo++)
    {
        // write xref entries like
        // "0000000010 00000 n\r\n"
        char refEntry[21];
        snprintf(refEntry, 21, "%010zu 00000 n\r\n", offsets[objNo]);

        oss << refEntry;
    }

    oss << "trailer<</Size " << numObjects << "/Root 1 0 R>>\r\n";
    oss << "startxref\r\n";
    oss << xrefOffset << "\r\n";
    oss << "%%EOF";

    auto buffer = oss.str();
    try
    {
        PdfMemDocument doc;
        doc.LoadFromBuffer(buffer);

        auto& pages = doc.GetPages();
        for (unsigned pageNo = 0; pageNo < pages.GetCount(); pageNo++)
            (void)pages.GetPageAt(pageNo);

        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRef);
    }
}

// CVE-2021-30471
TEST_CASE("testLoopingPageTree")
{
    // test PDF without nested kids
    string strNoLoop =
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

    {
        PdfMemDocument doc;
        doc.LoadFromBuffer(strNoLoop);
        auto& pages = doc.GetPages();
        for (unsigned pageNo = 0; pageNo < doc.GetPages().GetCount(); pageNo++)
            (void)pages.GetPageAt(pageNo);
        REQUIRE(true);
    }

    // CVE-2021-30471 test for pages tree /Kids array that refer back to pages tree root
    string strSelfLoop =
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
        PdfMemDocument doc;
        doc.LoadFromBuffer(strSelfLoop);
        auto& pages = doc.GetPages();
        for (unsigned pageNo = 0; pageNo < pages.GetCount(); pageNo++)
            (void)pages.GetPageAt(pageNo);

        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::PageNotFound);
    }

    // CVE-2021-30471 test for pages tree /Kids array that refer back to an ancestor (document root object)
    string strAncestorLoop =
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
        PdfMemDocument doc;
        doc.LoadFromBuffer(strAncestorLoop);
        auto& pages = doc.GetPages();
        pages.GetPageAt(0);
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::PageNotFound);
    }
}

// CVE-2020-18971
TEST_CASE("testNestedOutlines")
{
    // test for valid but deeply nested outlines
    // maxDepth must be less than GetMaxObjectCount otherwise PdfParser::ResizeOffsets
    // throws an error when reading the xref offsets table, and no outlines are read
    ostringstream oss;
    const size_t maxDepth = getStackOverflowDepth() - 4 - 1;
    const size_t numObjects = maxDepth + 4;
    vector<size_t> offsets(numObjects);
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
    for (size_t objNo = 4; objNo < numObjects; objNo++)
    {
        offsets[objNo] = oss.tellp();

        if (objNo < numObjects - 1)
            oss << objNo << " 0 obj<</Title (Outline Item) /First " << objNo + 1 << " 0 R /Last " << objNo + 1 << " 0 R>>endobj ";
        else
            oss << objNo << " 0 obj<</Title (Outline Item)>>endobj ";
    }

    // output xref table
    oss << "\r\n";
    xrefOffset = oss.tellp();
    oss << "xref\r\n";
    oss << "0 " << numObjects << "\r\n";

    oss << "0000000000 65535 f\r\n";

    for (size_t objNo = 1; objNo < offsets.size(); objNo++)
    {
        // write xref entries like
        // "0000000010 00000 n\r\n"
        char szXrefEntry[21];
        snprintf(szXrefEntry, 21, "%010zu 00000 n\r\n", offsets[objNo]);

        oss << szXrefEntry;
    }

    oss << "trailer<</Size " << numObjects << "/Root 1 0 R>>\r\n";
    oss << "startxref\r\n";
    oss << xrefOffset << "\r\n";
    oss << "%%EOF";

    auto buffer = oss.str();
    try
    {
        PdfMemDocument doc;
        doc.LoadFromBuffer(buffer);

        // load should succeed, then GetOutlines goes recursive due to /Outlines deep nesting
        (void)doc.GetOutlines();
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRef);
    }
}

// CVE-2020-18971
TEST_CASE("testLoopingOutlines")
{
    // CVE-2020-18971 - PdfOutlineItem /Next refers a preceding sibling
    string strNextLoop =
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
        PdfMemDocument doc;
        doc.LoadFromBuffer(strNextLoop);

        // load should succeed, then GetOutlines goes recursive due to /Outlines loop
        (void)doc.GetOutlines();
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRef);
    }

    // https://sourceforge.net/p/podofo/tickets/25/
    string strSelfLoop =
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
        PdfMemDocument doc;
        doc.LoadFromBuffer(strNextLoop);

        // load should succeed, then GetOutlines goes recursive due to /Outlines loop
        (void)doc.GetOutlines();
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidXRef);
    }
}

string generateXRefEntries(size_t count)
{
    string strXRefEntries;

    // generates a block of 20-byte xref entries
    // 0000000000 65535 f\r\n
    // 0000000120 00000 n\r\n    
    // 0000000120 00000 n\r\n
    // 0000000120 00000 n\r\n
    try
    {
        strXRefEntries.reserve(count * 20);
        for (size_t i = 0; i < count; i++)
        {
            if (i == 0)
                strXRefEntries.append("0000000000 65535 f\r\n");
            else
                strXRefEntries.append("0000000120 00000 n\r\n");
        }
    }
    catch (exception&)
    {
        // if this fails it's a bug in the unit tests and not PoDoFo
        FAIL("generateXRefEntries memory allocation failure");
    }

    return strXRefEntries;
}

bool canOutOfMemoryKillUnitTests()
{
    // test if out of memory conditions will kill the unit test process
    // which prevents tests completing

#if defined(_WIN32)
    // on Windows 32/64 allocations close to size of VM address space always fail gracefully
    bool canTerminateProcess = false;
#elif defined( __APPLE__ )
    // on macOS/iOS allocations close to size of VM address space fail gracefully
    // unless Address Sanitizer (ASAN) is enabled
#if __has_feature(address_sanitizer)
    // ASAN terminates the process if alloc fails - and using allocator_may_return_null=1
    // to continue after an allocation doesn't work in C++ because new returns null which is
    // forbidden by the C++ spec and terminates process when 'this' is dereferenced in constructor
    // see https://github.com/google/sanitizers/issues/295
    bool canTerminateProcess = true;
#else
    // if alloc fails following message is logged
    // *** mach_vm_map failed (error code=3)
    // *** error: can't allocate region
    // *** set a breakpoint in malloc_error_break to debug
    bool canTerminateProcess = false;
#endif
#elif defined( __linux__ )
    // TODO do big allocs succeed then trigger OOM-killer fiasco??
    bool canTerminateProcess = false;
#else
    // other systems - assume big allocs faily gracefully and throw bad_alloc
    bool canTerminateProcess = false;
#endif
    return canTerminateProcess;
}

size_t getStackOverflowDepth()
{
    // calculate stack overflow depth - need to do this because a value that consistently overflows a 64-bit stack
    // doesn't work on 32-bit systems because they run out of heap in ReadObjects before they get a chance to overflow stack
    // this is because sizeof(PdfParserObject) = 472 bytes (and there's one of these for every object read)
    constexpr size_t parserObjectSize = sizeof(PdfParserObject);

#if defined(_WIN64)
    // 1 MB default stack size, 64-bit address space, Windows x64 ABI
    // each stack frame has at least 4 64-bit stack params, 4 64-bit register params, plus 64-bit return address
    // stack frame size increases if function contains local variables or more than 4 parameters
    // see https://docs.microsoft.com/en-us/cpp/build/stack-usage?view=msvc-170
    constexpr size_t stackSize = 1 * 1024 * 1024;
    constexpr size_t frameSize = sizeof(void*) * (4 + 4 + 1); // 4 stack params + 4 register params + return address
    constexpr size_t maxFrames = stackSize / frameSize; // overflows at 14,563 recursive calls (or sooner if functions contain local variables)
#elif defined(_WIN32)
    // 1 MB default stack size, 32-bit address space (can't allocate more than 2GB), Windows x86 thiscall calling convention
    // each stack frame has at least 32-bit EBP and return address
    // stack frame size increases if function contains local variables or any parameters
    constexpr size_t stackSize = 1 * 1024 * 1024;
    constexpr size_t frameSize = sizeof(void*) * (1 + 1); // EBP and return address
    constexpr size_t maxFrames = stackSize / frameSize; // overflows at 131,072 recursive calls (or sooner if functions contain local variables or has parameters)
#else
    // assume 8MB macOS / Linux default stack size, 64-bit address space, System V AMD64 ABI
    // each stack frame has at least 64-bit EBP and return address
    // stack frame size increases if function contains local variables or any parameters
    constexpr size_t stackSize = 8 * 1024 * 1024;
    constexpr size_t frameSize = sizeof(void*) * (1 + 1); // EBP and return address
    constexpr size_t maxFrames = stackSize / frameSize; // overflows at 524,288 recursive calls (or sooner if functions contain local variables or has parameters)
#endif

    // add a few frames to sure we go beyond end of stack
    constexpr size_t overflowDepth = maxFrames + 1000;

    // overflowDepth must be less than GetMaxObjectCount otherwise PdfParser::ResizeOffsets
    // throws an error when reading the xref offsets table, and no recursive calls are made
    // must also be allocate less than half of address space to prevent out-of-memory exceptions
    REQUIRE(overflowDepth < PdfParser::GetMaxObjectCount());
    REQUIRE(overflowDepth * parserObjectSize < numeric_limits<size_t>::max() / 2);

    return overflowDepth;
}

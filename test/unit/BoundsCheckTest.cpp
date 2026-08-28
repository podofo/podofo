 // SPDX-FileCopyrightText: 2026 PoDoFo contributors
 // SPDX-License-Identifier: MIT-0

#include <PdfTest.h>

using namespace std;
using namespace PoDoFo;

// Subclass PdfTokenizer to expose protected ReadString / ReadHexString
class TestableTokenizer : public PdfTokenizer
{
public:
    using PdfTokenizer::ReadString;
    using PdfTokenizer::ReadHexString;
};

// ---------------------------------------------------------------------------
// 1) Unterminated regular string – must be rejected when it exceeds the
//    maximum allowed string length instead of growing until OOM.
// ---------------------------------------------------------------------------
TEST_CASE("TestStringGrowthIsBounded")
{
    // Build a '(' followed by data exceeding MaxStringLength with no
    // closing ')'. After the fix the size cap rejects this; before the
    // fix m_charBuffer grows without limit.
    constexpr size_t payloadSize = PdfTokenizer::MaxStringLength + 1;
    string input(payloadSize + 1, 'A');
    input[0] = '('; // opening paren, never closed

    SpanStreamDevice device(input);
    TestableTokenizer tokenizer;
    PdfVariant variant;

    // Consume the '(' token first
    string_view token;
    PdfTokenType tokenType;
    REQUIRE(tokenizer.TryReadNextToken(device, token, tokenType));
    REQUIRE(tokenType == PdfTokenType::ParenthesisLeft);

    // ReadString should throw ValueOutOfRange because the string exceeds
    // the maximum allowed length
    try
    {
        tokenizer.ReadString(device, variant, nullptr, { true, false });
        FAIL("Expected PdfError to be thrown");
    }
    catch (const PdfError& e)
    {
        REQUIRE(e.GetCode() == PdfErrorCode::ValueOutOfRange);
    }
}

// ---------------------------------------------------------------------------
// 2) Unterminated hex string – same pattern, different code path.
// ---------------------------------------------------------------------------
TEST_CASE("TestHexStringGrowthIsBounded")
{
    constexpr size_t payloadSize = PdfTokenizer::MaxStringLength + 1;
    string input(payloadSize + 1, 'A'); // 'A' is a valid hex digit
    input[0] = '<'; // opening angle bracket, never closed

    SpanStreamDevice device(input);
    TestableTokenizer tokenizer;
    PdfVariant variant;

    string_view token;
    PdfTokenType tokenType;
    REQUIRE(tokenizer.TryReadNextToken(device, token, tokenType));
    REQUIRE(tokenType == PdfTokenType::AngleBracketLeft);

    try
    {
        tokenizer.ReadHexString(device, variant, nullptr, { true, false });
        FAIL("Expected PdfError to be thrown");
    }
    catch (const PdfError& e)
    {
        REQUIRE(e.GetCode() == PdfErrorCode::ValueOutOfRange);
    }
}

// ---------------------------------------------------------------------------
// 3) Object stream with offset beyond buffer – readObjectsFromStream must
//    reject first+offset that exceeds the decompressed buffer length.
//    We test via PdfObjectStreamParser indirectly by crafting a minimal PDF
//    with an ObjStm whose /First is larger than the stream content.
//
//    This test builds a complete XRef-stream-based PDF so the parser
//    actually enters the object stream parsing code path.
// ---------------------------------------------------------------------------
TEST_CASE("TestObjectStreamOffsetBeyondBuffer")
{
    // Strategy: Build a PDF 1.5 document with:
    //   obj 1 = catalog  (normal object at known offset)
    //   obj 2 = ObjStm   with /First 99999, but stream content is tiny
    //   obj 3 = compressed inside obj 2 (referenced via XRef stream type-2)
    //   obj 4 = XRef stream (the cross-reference itself)
    //
    // The XRef stream tells the parser that obj 3 lives inside obj 2,
    // triggering PdfObjectStreamParser::Parse which should now fail
    // because /First (99999) exceeds the stream buffer.

    // --- Object 1: catalog ---
    ostringstream oss;
    oss << "%PDF-1.5\n";

    size_t obj1Pos = oss.str().size();
    oss << "1 0 obj\n"
        << "<< /Type /Catalog /Pages 1 0 R >>\n"
        << "endobj\n";

    // --- Object 2: ObjStm with bad /First ---
    // Stream content: "3 0\n42" means object #3 is at offset 0 from /First, value=42
    string objStmContent = "3 0\n42";
    size_t obj2Pos = oss.str().size();
    oss << "2 0 obj\n"
        << "<< /Type /ObjStm /N 1 /First 99999 /Length "
        << objStmContent.size() << " >>\n"
        << "stream\n"
        << objStmContent
        << "\nendstream\n"
        << "endobj\n";

    // --- Object 4: XRef stream ---
    // W = [1 2 2]: type(1 byte), field2(2 bytes), field3(2 bytes)
    // Entries (5 total: obj 0-4):
    //   obj 0: type=0 (free),   next=0,       gen=65535
    //   obj 1: type=1 (normal), offset=obj1Pos, gen=0
    //   obj 2: type=1 (normal), offset=obj2Pos, gen=0
    //   obj 3: type=2 (compressed), stream_obj=2, index=0
    //   obj 4: type=1 (normal), offset=obj4Pos, gen=0   <-- self
    size_t obj4Pos = oss.str().size();

    // We need to encode the xref stream binary content.
    // We'll use ASCIIHexDecode so we can write hex in the stream.
    // Entry format with W=[1 2 2]:
    //   obj 0: 00 0000 FFFF
    //   obj 1: 01 XXXX 0000   (XXXX = obj1Pos as 2 bytes)
    //   obj 2: 01 YYYY 0000   (YYYY = obj2Pos as 2 bytes)
    //   obj 3: 02 0002 0000   (compressed in obj 2, index 0)
    //   obj 4: 01 ZZZZ 0000   (ZZZZ = obj4Pos as 2 bytes)

    ostringstream xrefData;
    xrefData << utls::Format("{:02X}{:04X}{:04X}", 0, 0, 65535);
    xrefData << utls::Format("{:02X}{:04X}{:04X}", 1, (unsigned)obj1Pos, 0);
    xrefData << utls::Format("{:02X}{:04X}{:04X}", 1, (unsigned)obj2Pos, 0);
    xrefData << utls::Format("{:02X}{:04X}{:04X}", 2, 2, 0);
    xrefData << utls::Format("{:02X}{:04X}{:04X}", 1, (unsigned)obj4Pos, 0);
    string xrefHex = xrefData.str();

    oss << "4 0 obj\n"
        << "<< /Type /XRef /Size 5 /W [1 2 2]"
        << " /Root 1 0 R"
        << " /Filter /ASCIIHexDecode"
        << " /Length " << xrefHex.size()
        << " >>\n"
        << "stream\n"
        << xrefHex
        << "\nendstream\n"
        << "endobj\n";

    // NOTE: No trailing newline, so the document is loadable in strict parsing as well
    oss << "startxref\n" << obj4Pos << "\n%%EOF";

    string pdfBuf = oss.str();

    // Accessing obj 3 triggers the decoding of the ObjStm (obj 2), which
    // must fail because /First=99999 exceeds the stream content. The bounds
    // are checked explicitly, so the error is BrokenFile and not the
    // incidental ValueOutOfRange of SpanStreamDevice::Seek
    {
        PdfMemDocument doc;
        doc.LoadFromBuffer(pdfBuf, PdfLoadOptions::StrictParsing);
        auto obj = doc.GetObjects().GetObject(PdfReference(3, 0));
        REQUIRE(obj != nullptr);
        ASSERT_THROW_WITH_ERROR_CODE(obj->GetDataType(), PdfErrorCode::BrokenFile);
    }

    // With lenient parsing the document is still usable and
    // the unreadable compressed object resolves to null
    {
        PdfMemDocument doc;
        doc.LoadFromBuffer(pdfBuf);
        auto obj = doc.GetObjects().GetObject(PdfReference(3, 0));
        REQUIRE(obj != nullptr);
        REQUIRE(obj->IsNull());
    }
}



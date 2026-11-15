// SPDX-FileCopyrightText: 2007 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2021 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: MIT-0

#include <PdfTest.h>
#include <podofo/private/PdfFilterFactory.h>

using namespace std;
using namespace PoDoFo;

static void testFilter(PdfFilterType filterType, const bufferview& buffer);

static string_view s_testBuffer1 = "Man is distinguished, not only by his reason, but by this singular passion from other animals, which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable generation of knowledge, exceeds the short vehemence of any carnal pleasure.";

const char s_testBuffer2[] = {
    0x01, 0x64, 0x65, static_cast<char>(0xFE), 0x6B, static_cast<char>(0x80), 0x45, 0x32, static_cast<char>(0x88), 0x12, static_cast<char>(0x71), static_cast<char>(0xEA), 0x01,
    0x01, 0x64, 0x65, static_cast<char>(0xFE), 0x6B, static_cast<char>(0x80), 0x45, 0x32, static_cast<char>(0x88), 0x12, static_cast<char>(0x71), static_cast<char>(0xEA), 0x03,
    0x01, 0x64, 0x65, static_cast<char>(0xFE), 0x6B, static_cast<char>(0x80), 0x45, 0x32, static_cast<char>(0x88), 0x12, static_cast<char>(0x71), static_cast<char>(0xEA), 0x02,
    0x01, 0x64, 0x65, static_cast<char>(0xFE), 0x6B, static_cast<char>(0x80), 0x45, 0x32, static_cast<char>(0x88), 0x12, static_cast<char>(0x71), static_cast<char>(0xEA), 0x00,
    0x01, 0x64, 0x65, static_cast<char>(0xFE), 0x6B, static_cast<char>(0x80), 0x45, 0x32, static_cast<char>(0x88), 0x12, static_cast<char>(0x71), static_cast<char>(0xEA), 0x00,
    0x00, 0x00, 0x00, 0x00, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

TEST_CASE("TestFilters")
{
    for (unsigned i = 0; i <= (unsigned)PdfFilterType::Crypt; i++)
    {
        testFilter(static_cast<PdfFilterType>(i), { s_testBuffer1.data(), s_testBuffer1.length() });
        testFilter(static_cast<PdfFilterType>(i), { s_testBuffer2, std::size(s_testBuffer2) });
    }
}

// RunLengthDecode can't encode, so it's never exercised by TestFilters above:
// decode a stream combining a literal run, a repeat run and a single-byte
// literal run (control byte 0) directly instead
TEST_CASE("TestRunLengthDecodeFilter")
{
    const char input[] = {
        0x02, 'A', 'B', 'C',
        static_cast<char>(0xFE), 'Z',
        0x00, 'Q',
        static_cast<char>(0x80),
    };

    unique_ptr<PdfFilter> filter;
    REQUIRE(PdfFilterFactory::TryCreate(PdfFilterType::RunLengthDecode, filter));

    charbuff decoded;
    filter->DecodeTo(decoded, bufferview(input, std::size(input)));
    REQUIRE(decoded == "ABCZZZQ");
}

TEST_CASE("TestHexDecodeFilter")
{
    // Invalid characters are skipped, an odd digit count is completed with 0
    // and nothing is decoded past the end marker, also in a following block
    auto filter = PdfFilterFactory::Create(PdfFilterType::ASCIIHexDecode);
    charbuff decoded;
    BufferStreamDevice device(decoded);
    filter->BeginDecode(device);
    filter->DecodeBlock(string_view("48 6x5\n6C6C 6F 7>4142"));
    filter->DecodeBlock(string_view("4344"));
    filter->EndDecode();
    REQUIRE(decoded == "Hellop");
}

TEST_CASE("TestAscii85DecodeFilter")
{
    // The end marker is split between two blocks: nothing is decoded past it
    auto filter = PdfFilterFactory::Create(PdfFilterType::ASCII85Decode);
    charbuff decoded;
    BufferStreamDevice device(decoded);
    filter->BeginDecode(device);
    filter->DecodeBlock(string_view("87cURDZ~"));
    filter->DecodeBlock(string_view(">87cUR"));
    filter->EndDecode();
    REQUIRE(decoded == "Hello");
}

void testFilter(PdfFilterType filterType, const bufferview& view)
{
    charbuff encoded;
    charbuff decoded;

    unique_ptr<PdfFilter> filter;
    if (!PdfFilterFactory::TryCreate(filterType, filter))
    {
        INFO(utls::Format("!!! Filter {} not implemented.\n", (int)filterType));
        return;
    }

    INFO(utls::Format("Testing Algorithm {}:", (int)filterType));
    INFO("\t-> Testing Encoding");
    try
    {
        filter->EncodeTo(encoded, view);
    }
    catch (PdfError& e)
    {
        if (e == PdfErrorCode::UnsupportedFilter)
        {
            INFO(utls::Format("\t-> Encoding not supported for filter {}.", (unsigned)filterType));
            return;
        }

        throw;
    }

    INFO("\t-> Testing Decoding");
    try
    {
        filter->DecodeTo(decoded, encoded);
    }
    catch (PdfError& e)
    {
        if (e == PdfErrorCode::UnsupportedFilter)
        {
            INFO(utls::Format("\t-> Decoding not supported for filter {}", (int)filterType));
            return;
        }

        throw;
    }

    INFO(utls::Format("\t-> Original Data Length: {}", view.size()));
    INFO(utls::Format("\t-> Encoded  Data Length: {}", encoded.size()));
    INFO(utls::Format("\t-> Decoded  Data Length: {}", decoded.size()));

    REQUIRE(decoded == view);

    INFO("\t-> Test succeeded!");
}

TEST_CASE("TestFilterChainTooLong")
{
    // The decode chain is flushed and destroyed recursively, one frame per
    // filter: a /Filter array beyond the supported length can't be decoded
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Contents 4 0 R>>endobj
4 0 obj<</Length 11/Filter[/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx/AHx]>>stream
48656C6C6F>
endstream
endobj
trailer<</Root 1 0 R/Size 5>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& objStream = doc.GetObjects().MustGetObject(PdfReference(4, 0)).MustGetStream();
    ASSERT_THROW_WITH_ERROR_CODE(objStream.GetCopy(), PdfErrorCode::UnsupportedFilter);
}

TEST_CASE("TestPredictorHugeColumns")
{
    // The predictor row buffers are sized from the file parameters and
    // allocated for every filter of the chain: unreasonable values are rejected
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Contents 4 0 R>>endobj
4 0 obj<</Length 1/Filter/FlateDecode/DecodeParms<</Predictor 12/Colors 4/BitsPerComponent 8/Columns 500000000>>>>stream
x
endstream
endobj
trailer<</Root 1 0 R/Size 5>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    charbuff buffer;
    ASSERT_THROW_WITH_ERROR_CODE(doc.GetObjects().MustGetObject(PdfReference(4, 0)).MustGetStream().CopyTo(buffer),
        PdfErrorCode::ValueOutOfRange);
}

TEST_CASE("TestPredictorNone")
{
    // With no prediction the data is passed through, whatever the other parameters
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Contents 4 0 R>>endobj
4 0 obj<</Length 27/Filter[/AHx/Fl]/DecodeParms[null<</Predictor 1/BitsPerComponent 1/Columns 500000000>>]>>stream
789CF348CDC9C90700058C01F5>
endstream
endobj
trailer<</Root 1 0 R/Size 5>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    REQUIRE(doc.GetObjects().MustGetObject(PdfReference(4, 0)).MustGetStream().GetCopy() == "Hello");
}

TEST_CASE("TestLZWTableFull")
{
    // A stream may defer the clear code after the table is full: the codes stay
    // 12 bits wide and no entry is added, so the table stays bounded. Each code
    // adds one entry to the 257 initial ones: the 'A' codes fill the table up to
    // the last entry, which 'B' sets to "AB", and the code after it is the first
    // one with the table full
    vector<uint32_t> codes(4095 - 257, 'A');
    codes.push_back('B');
    codes.push_back('C');
    codes.push_back(4095);
    codes.push_back(0x101);

    charbuff payload;
    uint32_t bits = 0;
    unsigned bitCount = 0;
    unsigned codeLen = 9;
    size_t tableSize = 257;
    for (uint32_t code : codes)
    {
        bits = (bits << codeLen) | code;
        bitCount += codeLen;
        while (bitCount >= 8)
        {
            payload.push_back((char)(bits >> (bitCount - 8)));
            bitCount -= 8;
        }

        // Grow the code length as the decoder does
        if (tableSize < 4096)
            tableSize++;
        if (tableSize == 511 || tableSize == 1023 || tableSize == 2047)
            codeLen++;
    }
    if (bitCount != 0)
        payload.push_back((char)(bits << (8 - bitCount)));

    auto filter = PdfFilterFactory::Create(PdfFilterType::LZWDecode);
    charbuff decoded;
    filter->DecodeTo(decoded, bufferview(payload.data(), payload.size()));
    REQUIRE(decoded == string(4095 - 257, 'A') + "BCAB");
}

TEST_CASE("TestAbandonedDecodeStreamTeardown")
{
    // A read stopped before the end leaves the filters live: on teardown the
    // hex stage flushes its dangling digit, the 8 of 8C, into the ASCII85
    // stage as 0x80, which is rejected. The error must not escape the destructor.
    // The read consumes 8 raw bytes at a time, the blank makes the digit count
    // odd at every stop
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Contents 4 0 R>>endobj
4 0 obj<</Length 32/Filter[/ASCIIHexDecode/ASCII85Decode]>>stream
414243444546474 8494A4B8C4D4E4F>
endstream
endobj
trailer<</Root 1 0 R/Size 5>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& obj = doc.GetObjects().MustGetObject(PdfReference(4, 0));
    REQUIRE_NOTHROW([&]() {
        auto input = obj.MustGetStream().GetInputStream();
        char buffer[8];
        bool eof;
        (void)input.Read(buffer, sizeof(buffer), eof);
    }());
}

TEST_CASE("TestDecodeStreamReadAfterError")
{
    // The invalid zlib header fails the first read and drops the filter: a
    // following read must report the end of the data instead of using it
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Contents 4 0 R>>endobj
4 0 obj<</Length 32/Filter/FlateDecode>>stream
ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ
endstream
endobj
trailer<</Root 1 0 R/Size 5>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto input = doc.GetObjects().MustGetObject(PdfReference(4, 0)).MustGetStream().GetInputStream();
    char buffer[8];
    bool eof;
    ASSERT_THROW_WITH_ERROR_CODE(input.Read(buffer, sizeof(buffer), eof), PdfErrorCode::FlateError);
    REQUIRE(input.Read(buffer, sizeof(buffer), eof) == 0);
    REQUIRE(eof);
}

TEST_CASE("TestEncodeStreamUnsupportedFilter")
{
    // Filters that can't encode are rejected when the stream is created,
    // instead of failing at the first block and again on destruction
    PdfMemDocument doc;
    auto& obj = doc.GetObjects().CreateDictionaryObject();
    ASSERT_THROW_WITH_ERROR_CODE(obj.GetOrCreateStream().SetData(bufferview("abc", 3), { PdfFilterType::RunLengthDecode }),
        PdfErrorCode::UnsupportedFilter);
}

TEST_CASE("TestDecodedStreamSizeLimit")
{
    // The bytes produced by every filter count: the first one produces 40
    // blanks and "48656C6C6F", 50 bytes, the second one discards the blanks
    // and produces "Hello", 5 bytes
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Contents 4 0 R>>endobj
4 0 obj<</Length 100/Filter[/AHx/AHx]>>stream
2020202020202020202020202020202020202020202020202020202020202020202020202020202034383635364336433646
endstream
endobj
trailer<</Root 1 0 R/Size 5>>
startxref
999999
%%EOF)";

    struct LimitRestore
    {
        size_t Previous = PdfCommon::GetMaxDecodedStreamSize();
        ~LimitRestore() { PdfCommon::SetMaxDecodedStreamSize(Previous); }
    } restore;

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& objStream = doc.GetObjects().MustGetObject(PdfReference(4, 0)).MustGetStream();
    PdfCommon::SetMaxDecodedStreamSize(54);
    ASSERT_THROW_WITH_ERROR_CODE(objStream.GetCopy(), PdfErrorCode::ValueOutOfRange);
    PdfCommon::SetMaxDecodedStreamSize(55);
    REQUIRE(objStream.GetCopy() == "Hello");
}

namespace
{
    class ThrowingOutputStream final : public OutputStream
    {
    protected:
        void writeBuffer(const char*, size_t) override
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::IOError);
        }
    };
}

TEST_CASE("TestEncodeStreamSinkFailure")
{
    // The sink rejects the encoded bytes when the stream is destroyed: the
    // failure must not escape the destructor nor enter the failed filter again
    auto encode = PdfFilterFactory::CreateEncodeStream(std::make_shared<ThrowingOutputStream>(),
        { PdfFilterType::FlateDecode });
    encode->Write("abc", 3);
    REQUIRE_NOTHROW(encode.reset());
}

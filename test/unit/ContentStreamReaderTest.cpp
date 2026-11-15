// SPDX-FileCopyrightText: 2026 PoDoFo contributors
// SPDX-License-Identifier: MIT-0

#include <PdfTest.h>

using namespace std;
using namespace PoDoFo;

TEST_CASE("TestContentStreamReaderTerminates")
{
    // Regression test for infinite loop in PdfContentStreamReader.
    //
    // Input "+\n\tH" (and similar short inputs like "-\0TT", ".\n\t\xf6",
    // ".///") causes an infinite loop when fed into the content stream reader.
    //
    // Root cause: PdfTokenizer::DetermineDataType classifies a bare "+"
    // (or "-", ".") as a candidate Number/Real, but std::from_chars fails
    // to parse it. The recovery path re-enqueues the same token via
    // EnqueueToken, then returns Unknown. On the next call to TryReadNextToken
    // the enqueued token is dequeued, DetermineDataType fails again and
    // re-enqueues it — creating an infinite loop.
    //
    // Inputs are taken from libFuzzer timeout artifacts (4 bytes each).
    const string_view inputs[] = {
        "+\n\tH"sv,
        { "-\x00TT", 4 },
        ".\n\t\xf6"sv,
        ".///"sv,
        { "\x00-(\xff", 4 },
    };

    for (auto& input : inputs)
    {
        INFO("Input bytes: " << input.size());

        auto device = std::make_shared<SpanStreamDevice>(input);
        PdfContentReaderArgs args;
        args.Flags = PdfContentReaderFlags::SkipFollowFormXObjects
                   | PdfContentReaderFlags::SkipHandleNonFormXObjects;

        PdfContentStreamReader reader(device, args);
        PdfContent content;

        // Must terminate — before the fix this loops forever.
        // A generous iteration cap ensures the test fails fast if
        // the bug regresses, without relying on wall-clock timeouts.
        unsigned iterations = 0;
        constexpr unsigned maxIterations = 100;
        try
        {
            while (reader.TryReadNext(content) && iterations < maxIterations)
                iterations++;
        }
        catch (PdfError&)
        {
        }
        catch (std::exception&)
        {
        }

        REQUIRE(iterations < maxIterations);
    }
}

TEST_CASE("TestContentStreamReaderFormXObjectLimit")
{
    // Each form invokes the next one 16 times: without a limit
    // the page would follow 1 + 16 + 256 + 4096 + 65536 forms
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Resources<</XObject<</F 5 0 R>>>>/Contents 4 0 R>>endobj
4 0 obj<</Length 5>>stream
/F Do
endstream
endobj
5 0 obj<</Type/XObject/Subtype/Form/BBox[0 0 9 9]/Resources<</XObject<</F 6 0 R>>>>/Length 80>>stream
/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do
endstream
endobj
6 0 obj<</Type/XObject/Subtype/Form/BBox[0 0 9 9]/Resources<</XObject<</F 7 0 R>>>>/Length 80>>stream
/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do
endstream
endobj
7 0 obj<</Type/XObject/Subtype/Form/BBox[0 0 9 9]/Resources<</XObject<</F 8 0 R>>>>/Length 80>>stream
/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do
endstream
endobj
8 0 obj<</Type/XObject/Subtype/Form/BBox[0 0 9 9]/Resources<</XObject<</F 9 0 R>>>>/Length 80>>stream
/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do/F Do
endstream
endobj
9 0 obj<</Type/XObject/Subtype/Form/BBox[0 0 9 9]/Length 0>>stream
endstream
endobj
trailer<</Root 1 0 R/Size 10>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    PdfContentStreamReader reader(doc.GetPages().GetPageAt(0));
    PdfContent content;
    unsigned followed = 0;
    unsigned skipped = 0;
    while (reader.TryReadNext(content))
    {
        if (content.GetType() == PdfContentType::BeginFormXObject)
            followed++;
        else if ((content.GetWarnings() & PdfContentWarnings::FormXObjectLimitReached) != PdfContentWarnings::None)
            skipped++;
    }

    // The first form and 15 of its subtrees of 4369 forms reach
    // the limit, so only the last invocation of the first form is skipped
    REQUIRE(followed == 65536);
    REQUIRE(skipped == 1);
}

TEST_CASE("TestContentStreamReaderOperandLimit")
{
    // Operands past the limit don't grow the stack, and the
    // operator still gets the operands right before it
    string input;
    for (unsigned i = 0; i < 70000; i++)
        input.append("0 ");
    input.append("1 2 m");

    PdfContentStreamReader reader(std::make_shared<SpanStreamDevice>(input));
    PdfContent content;
    REQUIRE(reader.TryReadNext(content));
    REQUIRE(content.GetType() == PdfContentType::Operator);
    REQUIRE(content->Operator == PdfOperator::m);
    REQUIRE((content.GetWarnings() & PdfContentWarnings::OperandLimitReached) != PdfContentWarnings::None);
    REQUIRE(content->Stack.GetSize() < 65536);
    REQUIRE(content->Stack[1].GetNumber() == 1);
    REQUIRE(content->Stack[0].GetNumber() == 2);
}

TEST_CASE("TestContentStreamReaderInlineImageKeyLimit")
{
    // The keys of an inline image dictionary past the limit of 256 are dropped
    string input("BI");
    for (unsigned i = 0; i < 300; i++)
    {
        input.append(" /K");
        input.append(std::to_string(i));
        input.append(" 0");
    }
    input.append(" ID x EI");

    PdfContentStreamReader reader(std::make_shared<SpanStreamDevice>(input));
    PdfContent content;
    REQUIRE(reader.TryReadNext(content));
    REQUIRE(content.GetType() == PdfContentType::ImageDictionary);
    REQUIRE((content.GetWarnings() & PdfContentWarnings::InvalidImageDictionaryContent) != PdfContentWarnings::None);
    REQUIRE(content.GetInlineImageDictionary().GetSize() == 256);
}

TEST_CASE("TestContentStreamReaderFailedStream")
{
    // The second content stream can't be decoded: the error is reported
    // once and the reader is at the end afterwards, instead of reading
    // through the stale window of the previous buffer
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 100 100]/Contents[4 0 R 5 0 R]>>endobj
4 0 obj<</Length 19>>stream
q 1 0 0 1 0 0 cm Q
endstream
endobj
5 0 obj<</Length 4/Filter/DCTDecode>>stream
AAAA
endstream
endobj
trailer<</Root 1 0 R/Size 6>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    PdfContentStreamReader reader(doc.GetPages().GetPageAt(0));
    PdfContent content;
    vector<PdfOperator> operators;
    try
    {
        while (reader.TryReadNext(content))
            operators.push_back(content->Operator);

        FAIL("The second content stream must fail");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::UnsupportedFilter);
    }

    REQUIRE(operators == vector<PdfOperator>{ PdfOperator::q, PdfOperator::cm, PdfOperator::Q });
    REQUIRE(!reader.TryReadNext(content));
}

TEST_CASE("TestContentStreamReaderFailedStreamLoad")
{
    // With strict parsing the third content stream, which has no /Length,
    // fails to load after an empty one: the reader is at the end afterwards
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 100 100]/Contents[4 0 R 5 0 R 6 0 R]>>endobj
4 0 obj<</Length 19>>stream
q 1 0 0 1 0 0 cm Q
endstream
endobj
5 0 obj<</Length 0>>stream
endstream
endobj
6 0 obj<<>>stream
q
endstream
endobj
trailer<</Root 1 0 R/Size 7>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf, PdfLoadOptions::StrictParsing);
    PdfContentStreamReader reader(doc.GetPages().GetPageAt(0));
    PdfContent content;
    vector<PdfOperator> operators;
    try
    {
        while (reader.TryReadNext(content))
            operators.push_back(content->Operator);

        FAIL("The third content stream must fail to load");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetCode() == PdfErrorCode::InvalidStream);
    }

    REQUIRE(operators == vector<PdfOperator>{ PdfOperator::q, PdfOperator::cm, PdfOperator::Q });
    REQUIRE(!reader.TryReadNext(content));
}

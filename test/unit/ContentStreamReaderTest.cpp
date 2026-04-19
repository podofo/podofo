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

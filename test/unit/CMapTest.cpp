// SPDX-FileCopyrightText: 2025 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: MIT-0

#include <PdfTest.h>

#include <podofo/private/XMPUtils.h>

using namespace std;
using namespace PoDoFo;

// UB: getCodeFromVariant did `code << i * 8` with int (32-bit).
// When a PdfString operand in a CMap section had raw length >= 5, the shift
// exponent reached 32+ which is undefined behavior (caught by UBSan as
// "shift exponent 32 is too large for 32-bit type 'int'").
TEST_CASE("TestCMapShiftOverflow")
{
    // A beginbfchar section where the source code is a 5-byte hex string.
    // getCodeFromVariant iterates over each byte with `code << i*8`;
    // at i==4 the shift is 32 which is UB for a 32-bit int.
    string_view bfcharInput =
        "1 beginbfchar\n"
        "<0102030405> <0041>\n"
        "endbfchar\n"sv;

    SpanStreamDevice device1(bfcharInput);
    REQUIRE_NOTHROW(PdfCMapEncoding::Parse(device1));

    // A begincidrange section with a 5-byte hex string source code.
    string_view cidrangeInput =
        "1 begincidrange\n"
        "<0102030405> <0102030405> 0\n"
        "endcidrange\n"sv;

    SpanStreamDevice device2(cidrangeInput);
    REQUIRE_NOTHROW(PdfCMapEncoding::Parse(device2));
}

// Bug: getCodeFromVariant used arithmetic right-shift on a signed int64_t
// to count code bytes: `do { codeSize++; num >>= 8; } while (num != 0)`.
// When the parsed number was negative (e.g. -1), arithmetic right-shift
// propagates the sign bit, so num stays -1 forever → infinite loop.
TEST_CASE("TestCMapNegativeCodeNoHang")
{
    // A beginbfrange with -1 as the source-code-lo operand.
    // Before the fix this hangs in getCodeFromVariant's byte-counting loop.
    string_view input =
        "-1 beginbfrange\n"
        "-1 -1 (A)\n"
        "endbfrange\n"sv;

    SpanStreamDevice device(input);
    REQUIRE_NOTHROW(PdfCMapEncoding::Parse(device));
}

// Bug: PdfCharCodeMap::PushRange called std::prev(inserted.first) when a
// duplicate range key was re-inserted with a larger size. If the duplicate
// was the first element in the set (m_Ranges.begin()), std::prev walked
// before begin() - undefined behavior that crashed under UBSan as a
// misaligned pointer access in libc++'s __tree.
// Trigger: two beginbfrange sections with the same srcCodeLo where the
// second has a larger span, and no smaller key exists in the map.
TEST_CASE("TestCMapDuplicateRangeAtBegin")
{
    // Two ranges starting at <01> - the second is wider (size 3 vs 2).
    // PushRange will find the duplicate key, enter the "update size" branch,
    // and call std::prev on begin().
    string_view input =
        "1 beginbfrange\n"
        "<01> <02> <0041>\n"
        "endbfrange\n"
        "1 beginbfrange\n"
        "<01> <03> <0041>\n"
        "endbfrange\n"sv;

    SpanStreamDevice device(input);
    REQUIRE_NOTHROW(PdfCMapEncoding::Parse(device));
}

TEST_CASE("TestCodeSpaceRange")
{
    // Testing the begincodespacerange section for the same CMap tested in the
    // Adobe CMap specification, pages 48-50:
    // https://adobe-type-tools.github.io/font-tech-notes/pdfs/5014.CIDFont_Spec.pdf

    auto cmap = PdfCMapEncoding::Parse(TestUtils::GetTestInputFilePath("83pv-RKSJ-H"));
    auto ranges = cmap.GetCharMap().GetCodeSpaceRanges();
    REQUIRE(ranges.size() == 5);
    REQUIRE((ranges[0].CodeLo == 32 && ranges[0].CodeHi == 128 && ranges[0].CodeSpaceSize == 1));
    REQUIRE((ranges[1].CodeLo == 33088 && ranges[1].CodeHi == 40956 && ranges[1].CodeSpaceSize == 2));
    REQUIRE((ranges[2].CodeLo == 160 && ranges[2].CodeHi == 223 && ranges[2].CodeSpaceSize == 1));
    REQUIRE((ranges[3].CodeLo == 57408 && ranges[3].CodeHi == 61180 && ranges[3].CodeSpaceSize == 2));
    REQUIRE((ranges[4].CodeLo == 253 && ranges[4].CodeHi == 255 && ranges[4].CodeSpaceSize == 1));
}

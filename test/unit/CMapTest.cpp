// SPDX-FileCopyrightText: 2025 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: MIT-0

#include <PdfTest.h>

#include <podofo/private/XMPUtils.h>
#include <thread>
#include <atomic>

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

TEST_CASE("TestCMapHugeRangeReverseMap")
{
    // A range spanning nearly the whole 4 byte code space is expanded one entry
    // per code when the reverse map is built, which stops at the Unicode code point
    // count. The bfchar defeats the identity shortcut, as in GHSA-jhhv-w9v7-j258
    string_view input =
        "1 beginbfrange\n"
        "<00000001> <FFFFFF00> <0041>\n"
        "endbfrange\n"
        "1 beginbfchar\n"
        "<20> <0042>\n"
        "endbfchar\n"sv;

    SpanStreamDevice device(input);
    auto cmap = PdfCMapEncoding::Parse(device);
    auto& map = cmap.GetCharMap();

    // The forward lookup is not truncated
    CodePointSpan codePoints;
    REQUIRE(map.TryGetCodePoints(PdfCharCode(0x20000, 4), codePoints));
    REQUIRE(*codePoints == (char32_t)(0x41 + 0x1FFFF));

    // The bfchar entry and the first 0x10FFFF codes of the range fill the reverse map
    PdfCharCode code;
    REQUIRE(map.TryGetCharCode(U'A', code));
    REQUIRE(code == PdfCharCode(1, 4));
    REQUIRE(map.TryGetCharCode((char32_t)(0x41 + 0x10FFFE), code));
    REQUIRE(code == PdfCharCode(0x10FFFF, 4));
    REQUIRE(!map.TryGetCharCode((char32_t)(0x41 + 0x10FFFF), code));
}

TEST_CASE("TestCMapRangeAtCodeSpaceEnd")
{
    // A range reaching the last 4 byte code stops before it, so that the end
    // past its last code fits 32 bits for the lookup and the overlap fixing
    auto parse = [](const string_view& input)
    {
        SpanStreamDevice device(input);
        return PdfCMapEncoding::Parse(device);
    };

    CodePointSpan codePoints;
    PdfCharCode code;

    // The later range cuts the previous one where they overlap
    auto cmap1 = parse(
        "2 beginbfrange\n"
        "<FFFFFF00> <FFFFFFFF> <0041>\n"
        "<FFFFFF80> <FFFFFF8F> <0061>\n"
        "endbfrange\n"sv);
    auto& map1 = cmap1.GetCharMap();
    REQUIRE(map1.TryGetCodePoints(PdfCharCode(0xFFFFFF10, 4), codePoints));
    REQUIRE(*codePoints == (char32_t)(0x41 + 0x10));
    REQUIRE(map1.TryGetCodePoints(PdfCharCode(0xFFFFFF85, 4), codePoints));
    REQUIRE(*codePoints == (char32_t)(0x61 + 0x5));
    REQUIRE(!map1.TryGetCharCode((char32_t)(0x41 + 0x85), code));

    // The later range drops the next range it covers
    auto cmap2 = parse(
        "2 beginbfrange\n"
        "<FFFFFF80> <FFFFFF8F> <0061>\n"
        "<FFFFFF00> <FFFFFFFF> <0041>\n"
        "endbfrange\n"sv);
    auto& map2 = cmap2.GetCharMap();
    REQUIRE(map2.TryGetCodePoints(PdfCharCode(0xFFFFFF85, 4), codePoints));
    REQUIRE(*codePoints == (char32_t)(0x41 + 0x85));
    REQUIRE(map2.TryGetCodePoints(PdfCharCode(0xFFFFFFFE, 4), codePoints));
    REQUIRE(*codePoints == (char32_t)(0x41 + 0xFE));
    REQUIRE(!map2.TryGetCodePoints(PdfCharCode(0xFFFFFFFF, 4), codePoints));
}

TEST_CASE("TestCMapHugeBfcharDestination")
{
    // The /ToUnicode of the font maps code 0x41 to a 100000 code points
    // destination, which would become a ligature chain of one node per code
    // point in the reverse map, walked recursively. The destination is cut
    // to the 512 bytes the specification allows
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("Fonts", "HugeBfcharDestination.pdf"));
    auto& resources = doc.GetPages().GetPageAt(0).GetResources();
    unique_ptr<PdfFont> font;
    REQUIRE(PdfFont::TryCreateFromObject(*resources.GetResource(PdfResourceType::Font, "F1"), font));
    unsigned gid;
    (void)font->TryGetGID(U'A', PdfGlyphAccess::FontProgram, gid);
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

TEST_CASE("TestCMapCIDsExceedingMaxCID")
{
    // The range is truncated at CID 1114111 (0x10FFFF), the char
    // mapping past it is dropped
    string_view input =
        "1 begincidrange\n"
        "<0000> <00FF> 1114100\n"
        "endcidrange\n"
        "1 begincidchar\n"
        "<FFFF> 1114112\n"
        "endcidchar\n"sv;

    SpanStreamDevice device(input);
    auto cmap = PdfCMapEncoding::Parse(device);
    auto& map = cmap.GetCharMap();
    CodePointSpan cids;
    REQUIRE(map.TryGetCodePoints(PdfCharCode(0x000B, 2), cids));
    REQUIRE(*cids == 1114111);
    REQUIRE(!map.TryGetCodePoints(PdfCharCode(0x000C, 2), cids));
    REQUIRE(!map.TryGetCodePoints(PdfCharCode(0xFFFF, 2), cids));
}

TEST_CASE("TestPredefinedCMapConcurrentReverseLookup")
{
    // The reverse map of a shared predefined CMap is built by the first lookup
    auto& cmap = PdfEncodingMapFactory::GetPredefinedCMapInstance("UniKS-UCS2-H");
    atomic<unsigned> failures(0);
    vector<thread> threads;
    for (unsigned i = 0; i < 4; i++)
    {
        threads.emplace_back([&]
        {
            for (unsigned ch = 0x41; ch <= 0x5A; ch++)
            {
                CodePointSpan cids;
                PdfCharCode reverse;
                CodePointSpan reverseCids;
                if (!cmap.GetCharMap().TryGetCodePoints(PdfCharCode(ch, 2), cids) || cids.GetSize() != 1
                    || !cmap.GetCharMap().TryGetCharCode(*cids, reverse)
                    || !cmap.GetCharMap().TryGetCodePoints(reverse, reverseCids)
                    || reverseCids.GetSize() != 1 || *reverseCids != *cids)
                {
                    failures++;
                }
            }
        });
    }

    for (auto& thread : threads)
        thread.join();

    REQUIRE(failures.load() == 0);
}

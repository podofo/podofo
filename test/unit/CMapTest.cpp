/**
 * SPDX-FileCopyrightText: (C) 2025 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: MIT-0
 */


#include <PdfTest.h>

#include <podofo/private/XMPUtils.h>

using namespace std;
using namespace PoDoFo;

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

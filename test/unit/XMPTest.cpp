/**
 * Copyright (C) 2022 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>

#include <podofo/private/XMPUtils.h>

using namespace std;
using namespace PoDoFo;

static string removeFirstAndLastLine(const string& str);

static void TestNormalizeXMP(string_view filename)
{
    string sourceXmp;
    TestUtils::ReadTestInputFile(string(filename) + ".xml", sourceXmp);

    unique_ptr<PdfXMPPacket> packet;
    auto metadata = PoDoFo::GetXMPMetadata(sourceXmp, packet);
    auto normalizedXmp = packet->ToString();

    string expectedXmp;
    TestUtils::ReadTestInputFile(string(filename) + "-Expected.xml", expectedXmp);

    // NOTE: This just fixes tests with last resources as in recent PoDoFo versions
    // we do better normalization of the XMP packet begin/end tags
    normalizedXmp = removeFirstAndLastLine(normalizedXmp);
    expectedXmp = removeFirstAndLastLine(expectedXmp);

    REQUIRE(normalizedXmp == expectedXmp);
}

TEST_CASE("TestNormalizeXMP")
{
    TestNormalizeXMP("TestXMP1");
    TestNormalizeXMP("TestXMP5");
    TestNormalizeXMP("TestXMP7");
}

string removeFirstAndLastLine(const string& str)
{
    // Trim trailing newlines first
    size_t end = str.size();
    while (end > 0 && (str[end - 1] == '\n' || str[end - 1] == '\r'))
        --end;

    // Work on the trimmed view [0, end)
    size_t firstNewline = str.find('\n');
    if (firstNewline == string::npos || firstNewline >= end)
        return ""; // Only one (non-empty) line

    size_t lastNewline = str.rfind('\n', end - 1);

    if (firstNewline == lastNewline)
        return ""; // Only two lines, nothing left after removing both

    return str.substr(firstNewline + 1, lastNewline - firstNewline - 1);
}

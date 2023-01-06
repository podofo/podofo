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

static void TestNormalizeXMP(string_view filename)
{
    string sourceXmp;
    TestUtils::ReadTestInputFileTo(sourceXmp, string(filename) + ".xml");

    unique_ptr<PdfXMPPacket> packet;
    auto metadata = PoDoFo::GetXMPMetadata(sourceXmp, packet);
    auto normalizedXmp = packet->ToString();

    string expectedXmp;
    TestUtils::ReadTestInputFileTo(expectedXmp, string(filename) + "-Expected.xml");

    REQUIRE(normalizedXmp == expectedXmp);
}

TEST_CASE("TestNormalizeXMP")
{
    TestNormalizeXMP("TestXMP1");
    TestNormalizeXMP("TestXMP5");
    TestNormalizeXMP("TestXMP7");
}

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
    TestUtils::ReadTestInputFile(string(filename) + ".xml", sourceXmp);

    unique_ptr<PdfXMPPacket> packet;
    auto metadata = PoDoFo::GetXMPMetadata(sourceXmp, packet);
    auto normalizedXmp = packet->ToString();

    string expectedXmp;
    TestUtils::ReadTestInputFile(string(filename) + "-Expected.xml", expectedXmp);

    REQUIRE(normalizedXmp == expectedXmp);
}

TEST_CASE("TestAdditionalXMPMetatadata")
{
    string sourceXmp;
    TestUtils::ReadTestInputFile("TestXMP5.xml", sourceXmp);

    unique_ptr<PdfXMPPacket> packet;
    auto metadata = PoDoFo::GetXMPMetadata(sourceXmp, packet);

    REQUIRE(metadata.PdfaLevel == PdfALevel::L1B);
    REQUIRE(metadata.PdfuaVersion == PdfUAVersion::V1);
    REQUIRE(*metadata.GetMetadata(PdfAdditionalMetadata::PdfAIdCorr) == "2:2011");
}

TEST_CASE("TestNormalizeXMP")
{
    TestNormalizeXMP("TestXMP1");
    TestNormalizeXMP("TestXMP5");
    TestNormalizeXMP("TestXMP7");
}

TEST_CASE("TestPDFA1_PDFUA1")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("blank-pdfa.pdf"));
    doc.GetMetadata().SetPdfUAVersion(PdfUAVersion::V1);
    doc.Save(TestUtils::GetTestOutputFilePath("TestPDFA1_PDFUA1.pdf"));
}

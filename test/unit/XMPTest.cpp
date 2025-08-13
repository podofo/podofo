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

static void testPruneInvalid(const fs::path& path, PdfALevel level, const fs::path& refFolder, charbuff& buff1, charbuff& buff2);

static void TestNormalizeXMP(string_view filename)
{
    string sourceXmp;
    TestUtils::ReadTestInputFile(string(filename) + ".xml", sourceXmp);

    auto packet = PdfXMPPacket::Create(sourceXmp);
    auto metadata = packet->GetMetadata();
    auto normalizedXmp = packet->ToString();

    string expectedXmp;
    TestUtils::ReadTestInputFile(string(filename) + "-Expected.xml", expectedXmp);

    REQUIRE(normalizedXmp == expectedXmp);
}

TEST_CASE("TestAdditionalXMPMetatadata")
{
    string sourceXmp;
    TestUtils::ReadTestInputFile("TestXMP5.xml", sourceXmp);

    auto packet = PdfXMPPacket::Create(sourceXmp);
    auto metadata = packet->GetMetadata();

    REQUIRE(metadata.PdfaLevel == PdfALevel::L1B);
    REQUIRE(metadata.PdfuaLevel == PdfUALevel::L1);
    REQUIRE(*metadata.GetMetadata(PdfAdditionalMetadata::PdfAIdCorr) == "2:2011");
}

TEST_CASE("TestNormalizeXMP")
{
    TestNormalizeXMP("TestXMP1");
    TestNormalizeXMP("TestXMP5");
    TestNormalizeXMP("TestXMP7");
}

#ifdef PODOFO_HAVE_RNG_VALIDATION_RECOVERY

TEST_CASE("TestPruneInvalid")
{
    vector<string> warnings;
    auto reportWarnings = [&warnings](string_view name) {
        warnings.push_back(string(name));
    };

    string sourceXmp;
    TestUtils::ReadTestInputFile("TestXMP1.xml", sourceXmp);

    auto packet = PdfXMPPacket::Create(sourceXmp);
    packet->PruneInvalidProperties(PdfALevel::L1B, reportWarnings);
    REQUIRE(warnings.size() == 0);
    warnings.clear();
    packet = PdfXMPPacket::Create(sourceXmp);
    packet->PruneInvalidProperties(PdfALevel::L2B, reportWarnings);
    REQUIRE(warnings.size() == 0);
    warnings.clear();
    packet = PdfXMPPacket::Create(sourceXmp);
    packet->PruneInvalidProperties(PdfALevel::L4, reportWarnings);
    REQUIRE(warnings.size() == 0);

    sourceXmp.clear();
    TestUtils::ReadTestInputFile("TestXMP1_PDFA4.xml", sourceXmp);
    packet = PdfXMPPacket::Create(sourceXmp);

    warnings.clear();
    packet = PdfXMPPacket::Create(sourceXmp);
    packet->PruneInvalidProperties(PdfALevel::L1B, reportWarnings);
    REQUIRE(warnings.size() == 1);
    warnings.clear();
    packet = PdfXMPPacket::Create(sourceXmp);
    packet->PruneInvalidProperties(PdfALevel::L2B, reportWarnings);
    REQUIRE(warnings.size() == 1);
    warnings.clear();
    packet = PdfXMPPacket::Create(sourceXmp);
    packet->PruneInvalidProperties(PdfALevel::L4, reportWarnings);
    REQUIRE(warnings.size() == 0);
}

#endif // PODOFO_HAVE_RNG_VALIDATION_RECOVERY

TEST_CASE("TestPDFA1_PDFUA1")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("blank-pdfa.pdf"));
    doc.GetMetadata().SetPdfUALevel(PdfUALevel::L1);
    doc.Save(TestUtils::GetTestOutputFilePath("TestPDFA1_PDFUA1.pdf"));
}

TEST_CASE("TestPruneInvalidDataset")
{
    charbuff buff1;
    charbuff buff2;
    auto srcPath = TestUtils::GetTestInputPath() / "XMP";
    auto refPath = srcPath / "Ref";
    fs::create_directories(refPath);
    for (const auto& entry : fs::directory_iterator(TestUtils::GetTestInputFilePath("XMP")))
    {
        testPruneInvalid(entry.path(), PdfALevel::L1B, refPath, buff1, buff2);
        testPruneInvalid(entry.path(), PdfALevel::L2B, refPath, buff1, buff2);
    }
}

void testPruneInvalid(const fs::path& path, PdfALevel level, const fs::path& refFolder, charbuff& buff1, charbuff& buff2)
{
    if (path.filename() == "Ref")
        return;

    utls::ReadTo(buff1, path.u8string());
    auto packet = PdfXMPPacket::Create(buff1);
    constexpr bool WriteNormalized = false;
    if (WriteNormalized)
    {
        auto normalizedPath = refFolder / path.stem().u8string().append("_Normalized").append(".xmp");
        buff1.clear();
        packet->ToString(buff1);
        utls::WriteTo(normalizedPath.u8string(), buff1);
    }

    packet->PruneInvalidProperties(level);
    buff1.clear();
    packet->ToString(buff1);

    auto refPath = refFolder / path.stem().u8string().append("_").append(PoDoFo::ToString(level)).append(".xmp");
    if (fs::exists(refPath))
    {
        utls::ReadTo(buff2, refPath.u8string());
        REQUIRE(buff1 == buff2);
    }
    else
    {
        utls::WriteTo(refPath.u8string(), buff1);
    }
}

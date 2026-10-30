// SPDX-FileCopyrightText: 2022 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: MIT-0

#include <PdfTest.h>

#include <podofo/private/XMPUtils.h>

using namespace std;
using namespace PoDoFo;

static void TestNormalizeXMP(string_view filename)
{
    string sourceXmp;
    TestUtils::ReadTestInputFileTo(sourceXmp, string(filename) + ".xml");

    auto packet = PdfXMPPacket::Create(sourceXmp);
    auto metadata = packet->GetMetadata();
    auto normalizedXmp = packet->ToString();

    string expectedXmp;
    TestUtils::ReadTestInputFileTo(expectedXmp, string(filename) + "-Expected.xml");

    REQUIRE(normalizedXmp == expectedXmp);
}

TEST_CASE("TestAdditionalXMPMetatadata")
{
    string sourceXmp;
    TestUtils::ReadTestInputFileTo(sourceXmp, "TestXMP5.xml");

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

TEST_CASE("TestPDFA1_PDFUA1")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("blank-pdfa.pdf"));
    doc.GetMetadata().SetPdfUALevel(PdfUALevel::L1);
    doc.Save(TestUtils::GetTestOutputFilePath("TestPDFA1_PDFUA1.pdf"));
}

TEST_CASE("TestXMPDefaultNamespaceElement")
{
    // The element is in a default namespace, which has no prefix
    string_view xmp = R"(<x:xmpmeta xmlns:x="adobe:ns:meta/"><rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"><rdf:Description rdf:about=""><Producer xmlns="http://ns.adobe.com/pdf/1.3/">P</Producer></rdf:Description></rdf:RDF></x:xmpmeta>)";
    auto packet = PdfXMPPacket::Create(xmp);
    auto metadata = packet->GetMetadata();
    REQUIRE(*metadata.Producer == "P");
}

TEST_CASE("TestXMPExtensionBagEmptyItem")
{
    // Setting the PDF/UA level on a PDF/A-1 packet rewrites the extension
    // schemas bag, which holds an empty item
    string_view xmp = R"(<x:xmpmeta xmlns:x="adobe:ns:meta/"><rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"><rdf:Description rdf:about="" xmlns:pdfaid="http://www.aiim.org/pdfa/ns/id/" xmlns:pdfuaid="http://www.aiim.org/pdfua/ns/id/" xmlns:pdfaExtension="http://www.aiim.org/pdfa/ns/extension/"><pdfaid:part>1</pdfaid:part><pdfaid:conformance>B</pdfaid:conformance><pdfuaid:part>1</pdfuaid:part><pdfaExtension:schemas><rdf:Bag><rdf:li/></rdf:Bag></pdfaExtension:schemas></rdf:Description></rdf:RDF></x:xmpmeta>)";
    auto packet = PdfXMPPacket::Create(xmp);
    auto metadata = packet->GetMetadata();
    REQUIRE(metadata.PdfaLevel == PdfALevel::L1B);
    REQUIRE(metadata.PdfuaLevel == PdfUALevel::L1);
    packet->SetMetadata(metadata);
}

TEST_CASE("TestXMPAttributeEntityNotExpanded")
{
    // The attribute value decodes to an entity reference: converting it
    // to an element must keep it verbatim instead of expanding it
    string_view xmp = R"(<?xml version="1.0"?><!DOCTYPE x [<!ENTITY a "aaaa">]><x:xmpmeta xmlns:x="adobe:ns:meta/"><rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"><rdf:Description rdf:about="" xmlns:pdf="http://ns.adobe.com/pdf/1.3/" pdf:Producer="&amp;a;"/></rdf:RDF></x:xmpmeta>)";
    auto packet = PdfXMPPacket::Create(xmp);
    auto metadata = packet->GetMetadata();
    REQUIRE(*metadata.Producer == "&a;");
}

#if LIBXML_VERSION >= 21500

TEST_CASE("TestPruneInvalid")
{
    struct FailedProp
    {
        string Name;
        bool IsDuplicated;
        bool HasInvalidPrefix;
    };

    vector<FailedProp> warnings;
    auto reportWarnings = [&warnings](const PdfXMPProperty& prop) {
        warnings.push_back(FailedProp{ prop.GetPrefixedName(),
            prop.IsDuplicated(), prop.HasInvalidPrefix() });
    };

    string xmp;
    TestUtils::ReadTestInputFileTo(xmp, "TestXMP1.xml");

    auto packet = PdfXMPPacket::Create(xmp);
    packet->PruneAndValidate(PdfALevel::L1B, reportWarnings);
    REQUIRE(warnings.size() == 0);
    warnings.clear();
    packet = PdfXMPPacket::Create(xmp);
    packet->PruneAndValidate(PdfALevel::L2B, reportWarnings);
    REQUIRE(warnings.size() == 0);
    warnings.clear();
    packet = PdfXMPPacket::Create(xmp);
    packet->PruneAndValidate(PdfALevel::L4, reportWarnings);
    REQUIRE(warnings.size() == 0);

    xmp.clear();
    TestUtils::ReadTestInputFileTo(xmp, "TestXMP1_PDFA4.xml");

    warnings.clear();
    packet = PdfXMPPacket::Create(xmp);
    packet->PruneAndValidate(PdfALevel::L1B, reportWarnings);
    REQUIRE(warnings.size() == 1);
    warnings.clear();
    packet = PdfXMPPacket::Create(xmp);
    packet->PruneAndValidate(PdfALevel::L2B, reportWarnings);
    REQUIRE(warnings.size() == 1);
    warnings.clear();
    packet = PdfXMPPacket::Create(xmp);
    packet->PruneAndValidate(PdfALevel::L4, reportWarnings);
    REQUIRE(warnings.size() == 0);

    xmp.clear();
    TestUtils::ReadTestInputFileTo(xmp, "TestXMP1_PDFA4_Invalid1.xml");
    packet = PdfXMPPacket::Create(xmp);
    packet->PruneAndValidate(PdfALevel::L4, reportWarnings);
    REQUIRE(warnings.size() == 1);
    REQUIRE(warnings[0].Name == "pdf:Trapped");
    REQUIRE(warnings[0].IsDuplicated);

    xmp.clear();
    TestUtils::ReadTestInputFileTo(xmp, "TestXMP1_PDFA4_Invalid2.xml");
    warnings.clear();
    packet = PdfXMPPacket::Create(xmp);
    packet->PruneAndValidate(PdfALevel::L4, reportWarnings);
    REQUIRE(warnings.size() == 2);
    REQUIRE(warnings[0].Name == "mypdfaid:part");
    REQUIRE(warnings[0].HasInvalidPrefix);

    xmp.clear();
    TestUtils::ReadTestInputFileTo(xmp, "TestXMP8.xml");
    warnings.clear();
    packet = PdfXMPPacket::Create(xmp);
    packet->PruneAndValidate(PdfALevel::L2B, reportWarnings);
    REQUIRE(warnings.size() == 0);

    string expectedXmp;
    TestUtils::ReadTestInputFileTo(expectedXmp, "TestXMP8-Expected.xml");
    packet->ToString(xmp);
    REQUIRE(xmp == expectedXmp);
}

static void testPruneInvalid(const fs::path& path, PdfALevel level, const fs::path& refFolder, charbuff& buff1, charbuff& buff2);

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

    packet->PruneAndValidate(level);
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

#endif // LIBXML_VERSION >= 21500

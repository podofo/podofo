/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>
#include <podofo/optional/PdfNames.h>

using namespace std;
using namespace PoDoFo;

/** This class tests the basic integer and other types PoDoFo uses
 *  to make sure they satisfy its requirements for behaviour, size, etc.
 */

TEST_CASE("NullableTest")
{
    int intval = 15;
    nullable<int&> nullint1(intval);
    nullable<int&> nullint2(&intval);
    nullable<int&> nullint3;
    REQUIRE(nullint1 == &intval);
    REQUIRE(&intval == nullint2);
    REQUIRE(nullint1 != nullptr);
    REQUIRE(nullptr != nullint2);
    REQUIRE(nullint1 == 15);
    REQUIRE(16 != nullint1);
    REQUIRE(nullint3 == nullptr);
    REQUIRE(nullptr == nullint3);
    REQUIRE(nullint3 != &intval);
    REQUIRE(&intval != nullint3);
}

TEST_CASE("BasicTypeTest")
{
    REQUIRE(std::numeric_limits<uint64_t>::max() >= 9999999999);
}

TEST_CASE("TestIterations")
{
    PdfMemDocument doc;
    PdfPage* pages[] = {
        &doc.GetPages().CreatePage(PdfPageSize::A4),
        &doc.GetPages().CreatePage(PdfPageSize::A4),
        &doc.GetPages().CreatePage(PdfPageSize::A4),
    };

    unsigned i = 0;
    for (auto page : doc.GetPages())
    {
        REQUIRE(page == pages[i]);
        i++;
    }

    auto& page1 = *pages[0];
    PdfAnnotation* annots[] = {
        &page1.GetAnnotations().CreateAnnot<PdfAnnotationWatermark>(Rect()),
        &page1.GetAnnotations().CreateAnnot<PdfAnnotationWatermark>(Rect()),
        &page1.GetAnnotations().CreateAnnot<PdfAnnotationWatermark>(Rect()),
    };

    i = 0;
    for (auto annot : page1.GetAnnotations())
    {
        REQUIRE(annot == annots[i]);
        i++;
    }
}

TEST_CASE("TestIterations2")
{
    PdfMemDocument doc;
    vector<PdfField*> fields;
    for (auto field : doc.GetFieldsIterator())
    {
        fields.push_back(field);
    }

    REQUIRE(fields.size() == 0);

    doc.GetPages().CreatePage(Rect(0, 0, 300, 300));
    for (auto field : doc.GetPages().GetPageAt(0).GetFieldsIterator())
    {
        fields.push_back(field);
    }
    REQUIRE(fields.size() == 0);

    doc.Load(TestUtils::GetTestInputFilePath("Hierarchies1.pdf"));
    for (auto field : doc.GetFieldsIterator())
    {
        fields.push_back(field);
    }

    REQUIRE(fields.size() == 25);

    fields.clear();
    for (auto field : doc.GetPages().GetPageAt(0).GetFieldsIterator())
    {
        fields.push_back(field);
    }

    REQUIRE(fields.size() == 23);
}

TEST_CASE("ErrorFilePath")
{
    try
    {
        PdfObject test;
        test.GetString();
    }
    catch (const PdfError& err)
    {
        auto path = err.GetCallStack().front().GetFilePath();
        REQUIRE(fs::u8path(path) == fs::u8path("main") / "PdfVariant.cpp");
    }
}

TEST_CASE("TestMetadataSet")
{
    PdfMemDocument doc;
    auto& metadata = doc.GetMetadata();
    metadata.SetTitle(PdfString("TestTitle"));
    REQUIRE(metadata.GetTitle()->GetString() == "TestTitle");
    metadata.SetTitle(PdfString("TestTitle2"));
    REQUIRE(metadata.GetTitle()->GetString() == "TestTitle2");
    metadata.SetTitle(nullptr);
    REQUIRE(metadata.GetTitle() == nullptr);
}

TEST_CASE("TestNormalizeRangeRotations")
{
    ASSERT_EQUAL(utls::NormalizeCircularRange(370, 0, 360), 10);
    ASSERT_EQUAL(utls::NormalizeCircularRange(-370, 0, 360), 350);
    ASSERT_EQUAL(utls::NormalizeCircularRange(360, 0, 360), 0);
    ASSERT_EQUAL(utls::NormalizeCircularRange(0, 0, 360), 0);
    ASSERT_EQUAL(utls::NormalizeCircularRange(10, 0, 360), 10);
    ASSERT_EQUAL(utls::NormalizeCircularRange(-190, -180, 180), 170);
    ASSERT_EQUAL(utls::NormalizeCircularRange(190, -180, 180), -170);
    ASSERT_EQUAL(utls::NormalizeCircularRange(180, -180, 180), -180);
    ASSERT_EQUAL(utls::NormalizeCircularRange(0, -180, 180), 0);
    ASSERT_EQUAL(utls::NormalizeCircularRange(10, -180, 180), 10);
    ASSERT_EQUAL(utls::NormalizeCircularRange(-10, -180, 180), -10);

    // The following page rotation normalizations have
    // been verified in Adobe Reader 2024.002.20759
    ASSERT_EQUAL(utls::NormalizePageRotation(0), 0);
    ASSERT_EQUAL(utls::NormalizePageRotation(90), 90);
    ASSERT_EQUAL(utls::NormalizePageRotation(180), 180);
    ASSERT_EQUAL(utls::NormalizePageRotation(270), 270);
    ASSERT_EQUAL(utls::NormalizePageRotation(360), 0);
    ASSERT_EQUAL(utls::NormalizePageRotation(0.1), 0);
    ASSERT_EQUAL(utls::NormalizePageRotation(0.499999999), 0);
    ASSERT_EQUAL(utls::NormalizePageRotation(0.5), 90);
    ASSERT_EQUAL(utls::NormalizePageRotation(360.00000001), 0);
    ASSERT_EQUAL(utls::NormalizePageRotation(360.499999999), 0);
    ASSERT_EQUAL(utls::NormalizePageRotation(360.5), 90);
    ASSERT_EQUAL(utls::NormalizePageRotation(359.499999999), 270);
    ASSERT_EQUAL(utls::NormalizePageRotation(359.5), 0);
    ASSERT_EQUAL(utls::NormalizePageRotation(179.499999999), 90);
    ASSERT_EQUAL(utls::NormalizePageRotation(179.5), 180);
    ASSERT_EQUAL(utls::NormalizePageRotation(180.499999999), 180);
    ASSERT_EQUAL(utls::NormalizePageRotation(180.5), 270);
}

TEST_CASE("TestFileSpecAttachment")
{
    PdfMemDocument doc;
    doc.GetPages().CreatePage();
    shared_ptr<PdfFileSpec> fs = doc.CreateFileSpec();
    fs->SetFilename(PdfString("Test.xml"));
    fs->SetEmbeddedData(charbuff(string("<?xml version=\"1.0\"?><catalog></catalog>")));
    auto& names = doc.GetOrCreateNames();
    auto& embeddedFiles = names.GetOrCreateTree<PdfEmbeddedFiles>();
    REQUIRE(embeddedFiles.GetValue(*fs->GetFilename()) == nullptr);
    REQUIRE(!embeddedFiles.HasKey(*fs->GetFilename()));
    embeddedFiles.AddValue(*fs->GetFilename(), fs);
    REQUIRE(embeddedFiles.HasKey(*fs->GetFilename()));
    REQUIRE(embeddedFiles.GetValue(*fs->GetFilename()) == fs.get());

    PdfNameTree<PdfFileSpec>::Map map;
    embeddedFiles.ToDictionary(map);
    REQUIRE(map.size() == 1);
    REQUIRE(map[*fs->GetFilename()] == fs);

    doc.Save(TestUtils::GetTestOutputFilePath("TestFileSpecAttachment.pdf"));
}

TEST_CASE("TestPdfNames")
{
    // Just include and use "optional/PdfNames.h"
    PdfDictionary dict;
    dict.AddKey(PdfNames::Length, PdfObject(static_cast<int64_t >(100)));
}

TEST_CASE("TestMoveSemantics")
{
    string_view refs1 = "S1";
    string_view refs2 = "S2";
    PdfString s1 = refs1;
    REQUIRE(s1 == refs1);
    PdfString s2 = refs2;
    REQUIRE(s2 == refs2);
    s2 = std::move(s1);
    REQUIRE(s2 == refs1);
    REQUIRE(s1 == "");

    string_view refn1 = "N1";
    string_view refn2 = "N2";
    PdfName n1 = refn1;
    REQUIRE(n1 == refn1);
    PdfName n2 = refn2;
    REQUIRE(n2 == refn2);
    n2 = std::move(n1);
    REQUIRE(n2 == refn1);
    REQUIRE(n1 == "");
}

TEST_CASE("TestAssignObjects")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("Hierarchies1.pdf"));

    {
        auto& annotsObj = doc.GetObjects().MustGetObject(PdfReference(100, 0));
        REQUIRE(!annotsObj.IsDirty());
        annotsObj.GetArray()[15] = PdfObject(nullptr);
        REQUIRE(annotsObj.IsDirty());

        auto& pageObj = doc.GetObjects().MustGetObject(PdfReference(39, 0));
        REQUIRE(!pageObj.IsDirty());
        auto& resx = pageObj.GetDictionary().MustGetKey("Contents");
        resx = PdfObject(nullptr);
        REQUIRE(pageObj.IsDirty());
    }

    auto outputFilePath = TestUtils::GetTestOutputFilePath("TestAssignObjects.pdf");
    doc.Save(outputFilePath);

    doc.Load(outputFilePath);
    {
        auto& annotsObj = doc.GetObjects().MustGetObject(PdfReference(100, 0));
        REQUIRE(annotsObj.GetArray()[15].IsNull());

        auto& pageObj = doc.GetObjects().MustGetObject(PdfReference(39, 0));
        auto& resx = pageObj.GetDictionary().MustGetKey("Contents");
        REQUIRE(resx.IsNull());
    }
}

TEST_CASE("TestObjectAdapter")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("blank.pdf"));
    const auto& info = doc.GetTrailer().GetDictionary().FindKeyAs<PdfDictionary>("Info");
    REQUIRE(info.GetKeyAs<PdfString>("Producer") == "PoDoFo - http://podofo.sf.net");
    REQUIRE(info.GetKeyAsSafe<PdfString>("Prod", "fallback") == "fallback");
    REQUIRE(info.FindKeyAs<PdfString>("Producer") == "PoDoFo - http://podofo.sf.net");
    REQUIRE(info.FindKeyAsSafe<PdfString>("Prod", "fallback") == "fallback");
    REQUIRE(info.FindKeyParentAs<PdfString>("Producer") == "PoDoFo - http://podofo.sf.net");
    REQUIRE(info.FindKeyParentAsSafe<PdfString>("Prod", "fallback") == "fallback");
}

/**
 * Copyright (C) 2009 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>

using namespace std;
using namespace PoDoFo;

TEST_CASE("TestEmptyContentsStream")
{
    PdfMemDocument doc;
    auto& page1 = doc.GetPages().CreatePage(PdfPageSize::A4);
    REQUIRE(page1.GetDictionary().MustGetKey("Parent").GetReference() == doc.GetPages().GetObject().GetIndirectReference());
    auto& annot1 = page1.GetAnnotations().CreateAnnot<PdfAnnotationPopup>(Rect(300.0, 20.0, 250.0, 50.0));
    string_view title = "Author: Dominik Seichter";
    annot1.SetContents(PdfString(title));
    annot1.SetOpen(true);

    string filename = TestUtils::GetTestOutputFilePath("testEmptyContentsStream.pdf");
    doc.Save(filename);

    // Read annotation again
    PdfMemDocument doc2;
    doc2.Load(filename);
    REQUIRE(doc2.GetPages().GetCount() == 1);
    auto& page2 = doc2.GetPages().GetPageAt(0);
    REQUIRE(page2.GetAnnotations().GetCount() == 1);
    auto& annot2 = page2.GetAnnotations().GetAnnotAt(0);
    REQUIRE(*annot2.GetContents() == title);

    auto& pageObj = page2.GetObject();
    REQUIRE(!pageObj.GetDictionary().HasKey("Contents"));
}

TEST_CASE("TestRotations")
{
    // The two documents are rotated but still portrait
    PdfMemDocument doc;
    {
        doc.Load(TestUtils::GetTestInputFilePath("blank-rotated-90.pdf"));
        auto& page = doc.GetPages().GetPageAt(0);
        REQUIRE(page.GetRect() == Rect(0, 0, 595, 842));
        REQUIRE(page.GetRectRaw() == Rect(0, 0, 842, 595));
        auto& annot = page.GetAnnotations().CreateAnnot<PdfAnnotationWatermark>(Rect(100, 600, 80, 20));
        REQUIRE(annot.GetRect() == Rect(100, 600, 80, 20));
        REQUIRE(annot.GetRectRaw() == Rect(222, 99.999999999999986, 20, 79.999999999999986));
        page.SetRect(Rect(0, 0, 500, 800));
        REQUIRE(page.GetRect() == Rect(0, 0, 500, 800));
        REQUIRE(page.GetRectRaw() == Rect(0, 0, 800, 500));
    }

    {
        doc.Load(TestUtils::GetTestInputFilePath("blank-rotated-270.pdf"));
        auto& page = doc.GetPages().GetPageAt(0);
        REQUIRE(page.GetRect() == Rect(0, 0, 595, 842));
        REQUIRE(page.GetRectRaw() == Rect(0, 0, 842, 595));
        auto& annot = page.GetAnnotations().CreateAnnot<PdfAnnotationWatermark>(Rect(100, 600, 80, 20));
        REQUIRE(annot.GetRect() == Rect(100, 600, 80, 20));
        REQUIRE(annot.GetRectRaw() == Rect(600.00000000000011, 415, 20, 80));
        annot.SetRect(Rect(100, 500, 100, 30));
        REQUIRE(annot.GetRect() == Rect(100, 500.00000000000006, 100, 29.999999999999943));
        REQUIRE(annot.GetRectRaw() == Rect(500.00000000000011, 395, 30, 100));
    }
}

TEST_CASE("TestFlattening")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TechDocs", "pdf_implementation.pdf"));
    doc.GetPages().FlattenStructure();
    auto pageRootRef = doc.GetPages().GetObject().GetIndirectReference();
    auto& dict = doc.GetPages().GetDictionary();
    REQUIRE(dict.GetKey("Count")->GetNumber() == 11);
    auto& kidsArr = dict.MustFindKey("Kids").GetArray();
    REQUIRE(kidsArr.GetSize() == 11);
    for (unsigned i = 0; i < dict.GetSize(); i++)
    {
        auto& child = kidsArr.MustFindAt(i);
        REQUIRE(child.GetDictionary().MustGetKey("Type").GetName() == "Page");
        REQUIRE(child.GetDictionary().MustGetKey("Parent").GetReference() == pageRootRef);
    }
}

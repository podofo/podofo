// SPDX-FileCopyrightText: 2009 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2021 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: MIT-0

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

// FillFromPage must write BBox in form space (content-stream coordinates, pre-rotation
// MediaBox) and compute Matrix translation pivots from those content-space dimensions.
// Before the fix, BBox was written in post-rotation visual dimensions and the Matrix
// pivot values (e, f) were derived from the wrong dimension, causing displaced and
// clipped content when importing rotated source pages as Form XObjects.
struct FillFromPageExpected
{
    double bboxX1, bboxY1, bboxX2, bboxY2;
    double matA, matB, matC, matD, matE, matF;
};

static void testFillFromPage(const Rect& mediaBox, int rotation,
    const FillFromPageExpected& expected)
{
    PdfMemDocument srcDoc;
    auto& srcPage = srcDoc.GetPages().CreatePage(mediaBox);
    if (rotation != 0)
        srcPage.SetRotation(rotation);

    PdfMemDocument dstDoc;
    auto xobj = dstDoc.CreateXObjectForm(Rect());
    xobj->FillFromPage(srcPage);

    auto& bboxArr = xobj->GetDictionary().FindKeyAs<PdfArray>("BBox");
    REQUIRE(bboxArr[0].GetReal() == Catch::Detail::Approx(expected.bboxX1));
    REQUIRE(bboxArr[1].GetReal() == Catch::Detail::Approx(expected.bboxY1));
    REQUIRE(bboxArr[2].GetReal() == Catch::Detail::Approx(expected.bboxX2));
    REQUIRE(bboxArr[3].GetReal() == Catch::Detail::Approx(expected.bboxY2));

    auto& matrixArr = xobj->GetDictionary().FindKeyAs<PdfArray>("Matrix");
    REQUIRE(matrixArr[0].GetReal() == Catch::Detail::Approx(expected.matA).margin(1e-10));
    REQUIRE(matrixArr[1].GetReal() == Catch::Detail::Approx(expected.matB).margin(1e-10));
    REQUIRE(matrixArr[2].GetReal() == Catch::Detail::Approx(expected.matC).margin(1e-10));
    REQUIRE(matrixArr[3].GetReal() == Catch::Detail::Approx(expected.matD).margin(1e-10));
    REQUIRE(matrixArr[4].GetReal() == Catch::Detail::Approx(expected.matE).margin(1e-6));
    REQUIRE(matrixArr[5].GetReal() == Catch::Detail::Approx(expected.matF).margin(1e-6));
}

TEST_CASE("FillFromPage rotation at origin", "[PdfXObjectForm]")
{
    Rect mediaBox(0, 0, 175, 72);

    SECTION("0 degrees") {
        testFillFromPage(mediaBox, 0,
            { 0, 0, 175, 72,  1, 0, 0, 1, 0, 0 });
    }
    SECTION("90 degrees") {
        testFillFromPage(mediaBox, 90,
            { 0, 0, 175, 72,  0, -1, 1, 0, 0, 175 });
    }
    SECTION("180 degrees") {
        testFillFromPage(mediaBox, 180,
            { 0, 0, 175, 72,  -1, 0, 0, -1, 175, 72 });
    }
    SECTION("270 degrees") {
        testFillFromPage(mediaBox, 270,
            { 0, 0, 175, 72,  0, 1, -1, 0, 72, 0 });
    }
}

TEST_CASE("FillFromPage rotation with non-zero origin", "[PdfXObjectForm]")
{
    // Non-zero origins appear after cropping or merge operations
    Rect mediaBox(10, 20, 175, 72);

    SECTION("0 degrees") {
        testFillFromPage(mediaBox, 0,
            { 10, 20, 185, 92,  1, 0, 0, 1, -10, -20 });
    }
    SECTION("90 degrees") {
        testFillFromPage(mediaBox, 90,
            { 10, 20, 185, 92,  0, -1, 1, 0, -20, 185 });
    }
    SECTION("180 degrees") {
        testFillFromPage(mediaBox, 180,
            { 10, 20, 185, 92,  -1, 0, 0, -1, 185, 92 });
    }
    SECTION("270 degrees") {
        testFillFromPage(mediaBox, 270,
            { 10, 20, 185, 92,  0, 1, -1, 0, 92, -10 });
    }
}

TEST_CASE("FillFromPage rotation with negative origin", "[PdfXObjectForm]")
{
    // Negative origins appear in some PDF generators and after certain transformations
    Rect mediaBox(-5, -10, 175, 72);

    SECTION("0 degrees") {
        testFillFromPage(mediaBox, 0,
            { -5, -10, 170, 62,  1, 0, 0, 1, 5, 10 });
    }
    SECTION("90 degrees") {
        testFillFromPage(mediaBox, 90,
            { -5, -10, 170, 62,  0, -1, 1, 0, 10, 170 });
    }
    SECTION("180 degrees") {
        testFillFromPage(mediaBox, 180,
            { -5, -10, 170, 62,  -1, 0, 0, -1, 170, 62 });
    }
    SECTION("270 degrees") {
        testFillFromPage(mediaBox, 270,
            { -5, -10, 170, 62,  0, 1, -1, 0, 62, 5 });
    }
}

TEST_CASE("FillFromPage rotation with square page", "[PdfXObjectForm]")
{
    // Square pages are a degenerate case where the W/H swap is a no-op
    Rect mediaBox(0, 0, 200, 200);

    SECTION("0 degrees") {
        testFillFromPage(mediaBox, 0,
            { 0, 0, 200, 200,  1, 0, 0, 1, 0, 0 });
    }
    SECTION("90 degrees") {
        testFillFromPage(mediaBox, 90,
            { 0, 0, 200, 200,  0, -1, 1, 0, 0, 200 });
    }
    SECTION("180 degrees") {
        testFillFromPage(mediaBox, 180,
            { 0, 0, 200, 200,  -1, 0, 0, -1, 200, 200 });
    }
    SECTION("270 degrees") {
        testFillFromPage(mediaBox, 270,
            { 0, 0, 200, 200,  0, 1, -1, 0, 200, 0 });
    }
}

TEST_CASE("FillFromPage rotation with offset square page", "[PdfXObjectForm]")
{
    // Catches bugs only visible when both the square W==H no-op swap and origin normalization interact
    Rect mediaBox(15, 25, 200, 200);

    SECTION("0 degrees") {
        testFillFromPage(mediaBox, 0,
            { 15, 25, 215, 225,  1, 0, 0, 1, -15, -25 });
    }
    SECTION("90 degrees") {
        testFillFromPage(mediaBox, 90,
            { 15, 25, 215, 225,  0, -1, 1, 0, -25, 215 });
    }
    SECTION("180 degrees") {
        testFillFromPage(mediaBox, 180,
            { 15, 25, 215, 225,  -1, 0, 0, -1, 215, 225 });
    }
    SECTION("270 degrees") {
        testFillFromPage(mediaBox, 270,
            { 15, 25, 215, 225,  0, 1, -1, 0, 225, -15 });
    }
}

TEST_CASE("FillFromPage rotation with standard page sizes", "[PdfXObjectForm]")
{
    SECTION("A4 portrait at 90 degrees") {
        testFillFromPage(Rect(0, 0, 595, 842), 90,
            { 0, 0, 595, 842,  0, -1, 1, 0, 0, 595 });
    }
    SECTION("US Letter portrait at 270 degrees") {
        testFillFromPage(Rect(0, 0, 612, 792), 270,
            { 0, 0, 612, 792,  0, 1, -1, 0, 792, 0 });
    }
}

TEST_CASE("FillFromPage round-trip preserves content", "[PdfXObjectForm]")
{
    // Guards against serialization silently dropping or corrupting BBox/Matrix
    PdfMemDocument srcDoc;
    auto& srcPage = srcDoc.GetPages().CreatePage(Rect(0, 0, 175, 72));
    srcPage.SetRotation(270);

    {
        PdfPainter painter;
        painter.SetCanvas(srcPage);
        painter.GraphicsState.SetStrokingColor(PdfColor(1.0, 0.0, 0.0));
        painter.DrawLine(0, 0, 175, 72);
        painter.FinishDrawing();
    }

    PdfMemDocument dstDoc;
    auto& dstPage = dstDoc.GetPages().CreatePage(Rect(0, 0, 400, 400));
    auto xobj = dstDoc.CreateXObjectForm(Rect());
    xobj->FillFromPage(srcPage);

    string outputPath = TestUtils::GetTestOutputFilePath("fillFromPageRoundTrip.pdf");
    {
        PdfPainter painter;
        painter.SetCanvas(dstPage);
        painter.DrawXObject(*xobj, 50, 50);
        painter.FinishDrawing();
    }
    dstDoc.Save(outputPath);

    PdfMemDocument reloaded;
    reloaded.Load(outputPath);
    REQUIRE(reloaded.GetPages().GetCount() == 1);
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

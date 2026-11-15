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

TEST_CASE("TestAnnotationFromObject")
{
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
    auto& square = page.GetAnnotations().CreateAnnot<PdfAnnotationSquare>(Rect(100, 600, 80, 20));
    auto& highlight = page.GetAnnotations().CreateAnnot<PdfAnnotationHighlight>(Rect(100, 500, 80, 20));

    unique_ptr<PdfAnnotationTextMarkupBase> markup;
    REQUIRE(!PdfAnnotation::TryCreateFromObject<PdfAnnotationTextMarkupBase>(square.GetObject(), markup));
    REQUIRE(markup == nullptr);
    REQUIRE(PdfAnnotation::TryCreateFromObject<PdfAnnotationTextMarkupBase>(highlight.GetObject(), markup));

    unique_ptr<const PdfAnnotationTextMarkupBase> constMarkup;
    REQUIRE(!PdfAnnotation::TryCreateFromObject<PdfAnnotationTextMarkupBase>(std::as_const(square.GetObject()), constMarkup));
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

TEST_CASE("TestFlattenInheritedAttributes")
{
    // The intermediate node holds the attributes the page inherits: they
    // must be copied to the page when flattening, and the resources must
    // survive the garbage collection of the node
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Pages/Parent 2 0 R/Kids[4 0 R]/Count 1/Resources<</Font<</F1 5 0 R>>>>/MediaBox[0 0 200 100]/Rotate 90>>endobj
4 0 obj<</Type/Page/Parent 3 0 R>>endobj
5 0 obj<</Type/Font/Subtype/Type1/BaseFont/Helvetica>>endobj
trailer<</Root 1 0 R/Size 6>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& page = doc.GetPages().GetPageAt(0);
    doc.GetPages().CreatePage(PdfPageSize::A4);
    doc.CollectGarbage();

    auto& dict = page.GetDictionary();
    REQUIRE(dict.MustGetKey("Parent").GetReference() == doc.GetPages().GetObject().GetIndirectReference());
    REQUIRE(dict.HasKey("Resources"));
    REQUIRE(dict.HasKey("MediaBox"));
    REQUIRE(dict.MustFindKey("Rotate").GetNumber() == 90);
    REQUIRE(page.GetResources().GetResource(PdfResourceType::Font, "F1") != nullptr);
    REQUIRE(dict.MustFindKey("MediaBox").GetArray()[2].GetReal() == 200);
}

TEST_CASE("TestCreatePageDoesntInheritRootAttributes")
{
    // The root attributes must stay on the existing page only
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1/CropBox[0 0 100 100]/Rotate 90>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 200 200]>>endobj
trailer<</Root 1 0 R/Size 4>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& pages = doc.GetPages();
    auto& newPage = pages.CreatePage(PdfPageSize::A4);

    double rotation;
    REQUIRE(!newPage.TryGetRotationRaw(rotation));
    REQUIRE(newPage.GetCropBox().Width == newPage.GetMediaBox().Width);
    auto& page = pages.GetPageAt(0);
    REQUIRE(page.GetRotation() == 90);
    REQUIRE(page.GetCropBox().Width == 100);
}

TEST_CASE("TestFlattenInlineKids")
{
    // The page and the intermediate node inline in /Kids must survive
    // its replacement when flattening, and the page must become indirect
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Count 2/Kids[<</Type/Pages/Count 1/MediaBox[0 0 200 100]/Kids[<</Type/Page/Resources<</Font<</F1 4 0 R>>>>>>]>> 3 0 R]>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 200 200]>>endobj
4 0 obj<</Type/Font/Subtype/Type1/BaseFont/Helvetica>>endobj
trailer<</Root 1 0 R/Size 5>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& pages = doc.GetPages();
    auto& page = pages.GetPageAt(0);
    REQUIRE(page.GetResources().GetResource(PdfResourceType::Font, "F1") != nullptr);
    pages.CreatePage(PdfPageSize::A4);

    REQUIRE(pages.GetCount() == 3);
    REQUIRE(page.GetObject().IsIndirect());
    REQUIRE(pages.GetDictionary().MustFindKey("Kids").GetArray()[0].GetReference() == page.GetObject().GetIndirectReference());
    REQUIRE(page.GetDictionary().MustFindKey("MediaBox").GetArray()[3].GetReal() == 100);
    REQUIRE(page.GetResources().GetResource(PdfResourceType::Font, "F1") != nullptr);
}

TEST_CASE("TestFlattenFailureKeepsCollectionConsistent")
{
    // The /Parent cycle makes the flattening fail while looking up the
    // inherited attributes: the collection must not publish a partial
    // /Kids array, so the following insertion doesn't write past it, and
    // the inline page moved to a new object must stay in the old /Kids
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Count 2/Kids[<</Type/Page/MediaBox[0 0 200 200]>> 3 0 R]/Parent 3 0 R>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/Resources<<>>/MediaBox[0 0 200 200]/Rotate 0>>endobj
trailer<</Root 1 0 R/Size 4>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& pages = doc.GetPages();
    ASSERT_THROW_WITH_ERROR_CODE(pages.CreatePage(PdfPageSize::A4), PdfErrorCode::MaxRecursionReached);
    ASSERT_THROW_WITH_ERROR_CODE(pages.CreatePage(PdfPageSize::A4), PdfErrorCode::MaxRecursionReached);
    REQUIRE(pages.GetCount() == 2);
    auto& page = pages.GetPageAt(0);
    REQUIRE(page.GetObject().IsIndirect());
    REQUIRE(pages.GetDictionary().MustFindKey("Kids").GetArray()[0].GetReference() == page.GetObject().GetIndirectReference());
}

TEST_CASE("TestInsertDocumentPageAtClampsIndex")
{
    PdfMemDocument src;
    src.GetPages().CreatePage(PdfPageSize::A4);

    PdfMemDocument dst;
    dst.GetPages().InsertDocumentPageAt(100, src, 0);
    REQUIRE(dst.GetPages().GetCount() == 1);
    REQUIRE(dst.GetPages().GetDictionary().MustFindKey("Kids").GetArray().GetSize() == 1);
}

TEST_CASE("TestFillFromPageCopiesTransparencyGroup")
{
    // Without the fix, FillFromPage dropped /Group, causing viewers to reset
    // compositing to defaults and ignore isolation/knockout flags (issue #337)
    PdfMemDocument sourceDoc;
    auto& sourcePage = sourceDoc.GetPages().CreatePage(PdfPageSize::A4);

    PdfDictionary groupDict;
    groupDict.AddKey("Type"_n, PdfName("Group"));
    groupDict.AddKey("S"_n, PdfName("Transparency"));
    groupDict.AddKey("I"_n, PdfObject(true));
    groupDict.AddKey("K"_n, PdfObject(false));
    groupDict.AddKey("CS"_n, PdfName("DeviceRGB"));
    sourcePage.GetDictionary().AddKey("Group"_n, groupDict);

    PdfMemDocument destDoc;
    auto xobj = destDoc.CreateXObjectForm(sourcePage.GetMediaBox());
    xobj->FillFromPage(sourcePage);

    auto* copiedGroup = xobj->GetDictionary().FindKey("Group");
    REQUIRE(copiedGroup != nullptr);
    REQUIRE(copiedGroup->GetDictionary().GetKey("S")->GetName() == "Transparency");
    REQUIRE(copiedGroup->GetDictionary().GetKey("I")->GetBool() == true);
    REQUIRE(copiedGroup->GetDictionary().GetKey("K")->GetBool() == false);
    REQUIRE(copiedGroup->GetDictionary().GetKey("CS")->GetName() == "DeviceRGB");
}

TEST_CASE("TestFillFromPageCopiesGroupSameDocument")
{
    // The same-document path skips object remapping, so it exercises a
    // different branch in FillXObjectFromPage than the cross-document test
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);

    PdfDictionary groupDict;
    groupDict.AddKey("Type"_n, PdfName("Group"));
    groupDict.AddKey("S"_n, PdfName("Transparency"));
    groupDict.AddKey("CS"_n, PdfName("DeviceCMYK"));
    page.GetDictionary().AddKey("Group"_n, groupDict);

    auto xobj = doc.CreateXObjectForm(page.GetMediaBox());
    xobj->FillFromPage(page);

    auto* copiedGroup = xobj->GetDictionary().FindKey("Group");
    REQUIRE(copiedGroup != nullptr);
    REQUIRE(copiedGroup->GetDictionary().GetKey("S")->GetName() == "Transparency");
    REQUIRE(copiedGroup->GetDictionary().GetKey("CS")->GetName() == "DeviceCMYK");
}

TEST_CASE("TestFillFromPageWithoutGroupDoesNotCreateOne")
{
    PdfMemDocument sourceDoc;
    auto& sourcePage = sourceDoc.GetPages().CreatePage(PdfPageSize::A4);

    PdfMemDocument destDoc;
    auto xobj = destDoc.CreateXObjectForm(sourcePage.GetMediaBox());
    xobj->FillFromPage(sourcePage);

    REQUIRE(xobj->GetDictionary().FindKey("Group") == nullptr);
}

TEST_CASE("TestFillFromPageWithoutGroupSameDocument")
{
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);

    auto xobj = doc.CreateXObjectForm(page.GetMediaBox());
    xobj->FillFromPage(page);

    REQUIRE(xobj->GetDictionary().FindKey("Group") == nullptr);
}

TEST_CASE("TestAnnotationsIterationSkipsInvalid")
{
    // The first annotation has no /Subtype: it is stored as a null
    // placeholder that the iterations must not hand out
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/AcroForm<</Fields[4 0 R]>>>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Annots[5 0 R 4 0 R]>>endobj
4 0 obj<</Type/Annot/Subtype/Widget/FT/Tx/T(Foo)/Rect[0 0 5 5]/P 3 0 R>>endobj
5 0 obj<</Type/Annot/Rect[0 0 5 5]>>endobj
trailer<</Root 1 0 R/Size 6>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& page = doc.GetPages().GetPageAt(0);
    auto& annots = page.GetAnnotations();
    REQUIRE(annots.GetCount() == 2);
    ASSERT_THROW_WITH_ERROR_CODE(annots.GetAnnotAt(0), PdfErrorCode::InvalidHandle);

    unsigned count = 0;
    for (auto annot : annots)
    {
        REQUIRE(annot->GetType() == PdfAnnotationType::Widget);
        count++;
    }
    REQUIRE(count == 1);

    count = 0;
    for (auto field : page.GetFieldsIterator())
    {
        REQUIRE(field->GetType() == PdfFieldType::TextBox);
        count++;
    }
    REQUIRE(count == 1);

    count = 0;
    for (auto field : doc.GetFieldsIterator())
    {
        REQUIRE(field->GetType() == PdfFieldType::TextBox);
        count++;
    }
    REQUIRE(count == 1);

    count = 0;
    for (auto field : *doc.GetAcroForm())
    {
        REQUIRE(field->GetType() == PdfFieldType::TextBox);
        count++;
    }
    REQUIRE(count == 1);

    auto form = doc.CreateXObjectForm(page.GetMediaBox());
    REQUIRE_NOTHROW(form->FillFromPage(page));
}

TEST_CASE("TestAnnotationsDuplicateReferences")
{
    // The same annotation is referenced twice: the duplicate is not wrapped
    // and the second removal must not erase a reference already gone from the map
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Annots[4 0 R 4 0 R]>>endobj
4 0 obj<</Type/Annot/Subtype/Text/Rect[0 0 5 5]>>endobj
trailer<</Root 1 0 R/Size 5>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& annots = doc.GetPages().GetPageAt(0).GetAnnotations();
    REQUIRE(annots.GetCount() == 2);
    REQUIRE(annots.GetAnnotAt(0).GetType() == PdfAnnotationType::Text);
    ASSERT_THROW_WITH_ERROR_CODE(annots.GetAnnotAt(1), PdfErrorCode::InvalidHandle);
    annots.RemoveAnnotAt(0);
    annots.RemoveAnnotAt(0);
    REQUIRE(annots.GetCount() == 0);
}

TEST_CASE("TestAnnotationsRemovePlaceholder")
{
    // The first annotation has no /Subtype and is a placeholder: removing it by
    // index must drop its reference too, so lookups by reference don't use a stale index
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Annots[4 0 R 5 0 R]>>endobj
4 0 obj<</Type/Annot/Rect[0 0 5 5]>>endobj
5 0 obj<</Type/Annot/Subtype/Text/Rect[0 0 5 5]>>endobj
trailer<</Root 1 0 R/Size 6>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& annots = doc.GetPages().GetPageAt(0).GetAnnotations();
    REQUIRE(annots.GetCount() == 2);
    annots.RemoveAnnotAt(0);
    REQUIRE(annots.GetCount() == 1);
    ASSERT_THROW_WITH_ERROR_CODE(annots.GetAnnot(PdfReference(4, 0)), PdfErrorCode::ValueOutOfRange);
    annots.RemoveAnnot(PdfReference(4, 0));
    REQUIRE(annots.GetCount() == 1);
    REQUIRE(annots.GetAnnot(PdfReference(5, 0)).GetType() == PdfAnnotationType::Text);
    annots.RemoveAnnot(PdfReference(5, 0));
    REQUIRE(annots.GetCount() == 0);
}

TEST_CASE("TestAnnotationsNonDictionaryEntries")
{
    // A number and a dangling reference in /Annots are placeholders and
    // must not leave the collection half initialized
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Annots[42 4 0 R 999 0 R]>>endobj
4 0 obj<</Type/Annot/Subtype/Text/Rect[0 0 5 5]>>endobj
trailer<</Root 1 0 R/Size 5>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& annots = doc.GetPages().GetPageAt(0).GetAnnotations();
    REQUIRE(annots.GetCount() == 3);
    ASSERT_THROW_WITH_ERROR_CODE(annots.GetAnnotAt(0), PdfErrorCode::InvalidHandle);
    REQUIRE(annots.GetAnnotAt(1).GetType() == PdfAnnotationType::Text);
    ASSERT_THROW_WITH_ERROR_CODE(annots.GetAnnotAt(2), PdfErrorCode::InvalidHandle);

    auto& created = annots.CreateAnnot(PdfAnnotationType::Square, Rect(0, 0, 5, 5));
    REQUIRE(annots.GetCount() == 4);
    REQUIRE(&annots.GetAnnot(created.GetObject().GetIndirectReference()) == &created);
}

TEST_CASE("TestAnnotationsInlineArrayKept")
{
    // Adding to an /Annots array inline in the page must not replace it with a copy
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
    auto& annots = page.GetAnnotations();
    annots.CreateAnnot(PdfAnnotationType::Text, Rect(0, 0, 5, 5));
    auto arr = &page.GetDictionary().MustGetKey("Annots").GetArray();
    annots.CreateAnnot(PdfAnnotationType::Text, Rect(0, 0, 5, 5));
    REQUIRE(&page.GetDictionary().MustGetKey("Annots").GetArray() == arr);
    REQUIRE(arr->GetSize() == 2);
}

TEST_CASE("TestAnnotationsSharedArrayRemoval")
{
    // Two pages reference the same /Annots array: a removal through one page
    // must not drop the annotation the other still wraps, which the garbage
    // collection on save would otherwise free
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R 4 0 R]/Count 2>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Annots 5 0 R>>endobj
4 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Annots 5 0 R>>endobj
5 0 obj[6 0 R]endobj
6 0 obj<</Type/Annot/Subtype/Text/Rect[1 1 5 5]>>endobj
trailer<</Root 1 0 R/Size 7>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& pages = doc.GetPages();
    auto& annots0 = pages.GetPageAt(0).GetAnnotations();
    auto& annots1 = pages.GetPageAt(1).GetAnnotations();
    REQUIRE(annots0.GetCount() == 1);
    REQUIRE(annots1.GetCount() == 1);
    annots0.RemoveAnnotAt(0);
    REQUIRE(annots0.GetCount() == 0);

    doc.Save(TestUtils::GetTestOutputFilePath("TestAnnotationsSharedArrayRemoval.pdf"));
    REQUIRE(annots1.GetCount() == 1);
    auto& annot = annots1.GetAnnotAt(0);
    REQUIRE(annot.GetRectRaw().GetNormalized().Width == 4);
    annot.SetFlags(PdfAnnotationFlags::Hidden);
    REQUIRE(annot.GetFlags() == PdfAnnotationFlags::Hidden);
}

TEST_CASE("TestAnnotationsDirectDictionaries")
{
    // Direct annotation dictionaries live in the array storage, which the
    // addition relocates: they must be moved to indirect objects and the
    // existing wrappers rebound to them
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Annots[<</Type/Annot/Subtype/Text/Rect[1 1 5 5]>><</Type/Annot/Subtype/Square/Rect[1 1 6 6]>><</Type/Annot/Subtype/Square/Rect[1 1 7 7]>><</Type/Annot/Subtype/Square/Rect[1 1 8 8]>>]>>endobj
trailer<</Root 1 0 R/Size 4>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& page = doc.GetPages().GetPageAt(0);
    auto& annots = page.GetAnnotations();
    REQUIRE(annots.GetCount() == 4);
    auto& first = annots.GetAnnotAt(0);
    auto& last = annots.GetAnnotAt(3);
    REQUIRE(first.GetType() == PdfAnnotationType::Text);

    annots.CreateAnnot(PdfAnnotationType::Link, Rect(0, 0, 5, 5));
    REQUIRE(annots.GetCount() == 5);
    REQUIRE(first.GetObject().IsIndirect());
    REQUIRE(first.GetRectRaw().GetNormalized().Width == 4);
    REQUIRE(&annots.GetAnnot(first.GetObject().GetIndirectReference()) == &first);
    auto& arr = page.GetDictionary().MustGetKey("Annots").GetArray();
    REQUIRE(arr[0].IsReference());
    REQUIRE(arr[3].GetReference() == last.GetObject().GetIndirectReference());

    annots.RemoveAnnotAt(0);
    REQUIRE(annots.GetCount() == 4);
    REQUIRE(last.GetRectRaw().GetNormalized().Width == 7);
    REQUIRE(&annots.GetAnnotAt(2) == &last);
}

TEST_CASE("TestFillFromPageNonDictionaryResources")
{
    // The form already wraps its /Resources: a page /Resources that is not a
    // dictionary must not be copied over it
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
    auto form = doc.CreateXObjectForm(page.GetMediaBox());
    form->GetOrCreateResources();
    page.GetDictionary().AddKey("Resources"_n, static_cast<int64_t>(42));
    form->FillFromPage(page);
    REQUIRE(form->GetDictionary().MustGetKey("Resources").IsDictionary());
    REQUIRE(&form->GetOrCreateResources().GetObject() == &form->GetDictionary().MustGetKey("Resources"));
}

TEST_CASE("TestFillFromPageParsedFormResources")
{
    // The parsed form wraps its indirect /Resources, which the fill replaces
    // with a copy of the page one: the wrapper must follow, as the orphaned
    // object is collected on save
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Resources<</XObject<</Fm0 4 0 R>>>>>>endobj
4 0 obj<</Type/XObject/Subtype/Form/BBox[0 0 9 9]/Resources 5 0 R/Length 0>>stream

endstream
endobj
5 0 obj<</ProcSet[/PDF]>>endobj
trailer<</Root 1 0 R/Size 6>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& page = doc.GetPages().GetPageAt(0);
    unique_ptr<PdfXObjectForm> form;
    REQUIRE(PdfXObject::TryCreateFromObject<PdfXObjectForm>(
        *page.GetResources().GetResource(PdfResourceType::XObject, "Fm0"), form));
    form->FillFromPage(page);
    REQUIRE(&form->GetOrCreateResources().GetObject() == &form->GetDictionary().MustGetKey("Resources"));

    doc.Save(TestUtils::GetTestOutputFilePath("TestFillFromPageParsedFormResources.pdf"));
    auto img = doc.CreateImage();
    static_cast<PdfResourceOperations&>(form->GetOrCreateResources()).AddResource(PdfResourceType::XObject, img->GetObject());
    REQUIRE(form->GetDictionary().MustGetKey("Resources").GetDictionary().HasKey("XObject"));
}

TEST_CASE("TestSetAppearanceStreamImageUnrotatedPage")
{
    // A non-form XObject on an unrotated page gets a trampoline form
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
    auto img = doc.CreateImage();
    PdfImageInfo info;
    info.Width = 2;
    info.Height = 2;
    info.ColorSpace = PdfColorSpaceType::DeviceGray;
    info.BitsPerComponent = 8;
    img->SetDataRaw(bufferview("\x00\xFF\xFF\x00", 4), info);
    auto& annot = page.GetAnnotations().CreateAnnot<PdfAnnotationStamp>(Rect(10, 10, 100, 100));
    annot.SetAppearanceStream(*img);

    unique_ptr<PdfXObjectForm> form;
    REQUIRE(PdfXObject::TryCreateFromObject<PdfXObjectForm>(*annot.GetAppearanceStream(), form));
    REQUIRE(form->GetResources()->GetResource(PdfResourceType::XObject, "XOb1") == &img->GetObject());
}

TEST_CASE("TestAnnotationSetActionSelf")
{
    // Setting the cached action instance back
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
    auto& link = page.GetAnnotations().CreateAnnot<PdfAnnotationLink>(Rect(10, 10, 100, 20));
    auto action = doc.CreateAction<PdfActionURI>();
    action->SetURI(PdfString("https://example.com"));
    link.SetAction(*action);
    link.SetAction(*link.GetAction());
    REQUIRE(&link.GetAction()->GetObject() == &action->GetObject());
}

TEST_CASE("TestLinkSetDestinationSelf")
{
    // Setting the cached destination instance back
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
    auto& link = page.GetAnnotations().CreateAnnot<PdfAnnotationLink>(Rect(10, 10, 100, 20));
    auto dest = doc.CreateDestination();
    dest->SetDestination(page);
    link.SetDestination(*dest);
    link.SetDestination(*link.GetDestination());
    REQUIRE(link.GetDestination()->GetPage() == &page);
}

TEST_CASE("TestSetActionForeignDocument")
{
    // A failed insertion of the action must leave the cached one in place
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
    auto& link = page.GetAnnotations().CreateAnnot<PdfAnnotationLink>(Rect(10, 10, 100, 20));
    auto action = doc.CreateAction<PdfActionURI>();
    link.SetAction(*action);
    auto& item = doc.GetOrCreateOutlines().CreateRoot(PdfString("Root"));
    item.SetAction(*action);

    PdfMemDocument foreignDoc;
    auto foreignAction = foreignDoc.CreateAction<PdfActionURI>();
    ASSERT_THROW_WITH_ERROR_CODE(link.SetAction(*foreignAction), PdfErrorCode::InvalidHandle);
    REQUIRE(&link.GetAction()->GetObject() == &action->GetObject());
    ASSERT_THROW_WITH_ERROR_CODE(item.SetAction(*foreignAction), PdfErrorCode::InvalidHandle);
    REQUIRE(&item.GetAction()->GetObject() == &action->GetObject());
}

TEST_CASE("TestResourcesAddProperties")
{
    // The resource id counters cover all the resource types, Properties included
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
    auto& resources = static_cast<PdfResourceOperations&>(page.GetResources());
    auto& obj1 = doc.GetObjects().CreateDictionaryObject();
    auto& obj2 = doc.GetObjects().CreateDictionaryObject();
    auto name1 = resources.AddResource(PdfResourceType::Properties, obj1);
    auto name2 = resources.AddResource(PdfResourceType::Properties, obj2);
    REQUIRE(name1 != name2);
    REQUIRE(resources.GetResource(PdfResourceType::Properties, name1.GetString()) == &obj1);
    REQUIRE(resources.GetResource(PdfResourceType::Properties, name2.GetString()) == &obj2);
}

TEST_CASE("TestRelocationMapAfterGarbageCollection")
{
    // Mapped objects removed from the destination must be copied again instead of reused
    PdfMemDocument src;
    auto& srcPage = src.GetPages().CreatePage(PdfPageSize::A4);
    auto form = src.CreateXObjectForm(Rect(0, 0, 10, 10));
    static_cast<PdfResourceOperations&>(srcPage.GetResources()).AddResource(PdfResourceType::XObject, "Fm0"_n, form->GetObject());

    PdfMemDocument dst;
    PdfObjectRelocationMap map;
    dst.GetPages().AppendDocumentPages(src, 0, 1, &map);
    dst.GetPages().RemovePageAt(0);
    dst.CollectGarbage();
    dst.GetPages().AppendDocumentPages(src, 0, 1, &map);
    REQUIRE(dst.GetPages().GetCount() == 1);
    REQUIRE(dst.GetPages().GetPageAt(0).GetResources().GetResource(PdfResourceType::XObject, "Fm0") != nullptr);
}

TEST_CASE("TestLinkAliasedDestsRetyped")
{
    // The PDF 1.1 /Dests table aliases the annotation dictionary: SetFlags
    // writes the key the named destination selects and retypes the array
    constexpr string_view pdf = R"(%PDF-1.4
1 0 obj<</Type/Catalog/Pages 2 0 R/Dests 4 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Annots[4 0 R]>>endobj
4 0 obj<</Type/Annot/Subtype/Link/Rect[0 0 9 9]/Dest/F/F[3 0 R/Fit]>>endobj
trailer<</Root 1 0 R/Size 5>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& link = static_cast<PdfAnnotationLink&>(doc.GetPages().GetPageAt(0).GetAnnotations().GetAnnotAt(0));
    REQUIRE(link.GetDestination()->GetType() == PdfDestinationType::Fit);
    link.SetFlags(PdfAnnotationFlags::Hidden);
    REQUIRE(link.GetDestination() == nullptr);
}

TEST_CASE("TestLinkDestinationAfterGarbageCollection")
{
    // The cache must not wrap the caller's unreferenced destination object
    PdfMemDocument doc;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
    auto& link = page.GetAnnotations().CreateAnnot<PdfAnnotationLink>(Rect(10, 10, 100, 20));
    auto dest = doc.CreateDestination();
    dest->SetDestination(page);
    link.SetDestination(*dest);
    REQUIRE(&link.GetDestination()->GetObject() != &dest->GetObject());
    doc.CollectGarbage();
    REQUIRE(link.GetDestination()->GetPage() == &page);
    REQUIRE(link.GetDestination()->GetType() == PdfDestinationType::Fit);
}

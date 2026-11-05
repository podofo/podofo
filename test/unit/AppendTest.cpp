// SPDX-FileCopyrightText: 2026 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: MIT-0

#include <PdfTest.h>

using namespace std;
using namespace PoDoFo;

TEST_CASE("TestAppendDocument")
{
    PdfMemDocument srcDoc;
    srcDoc.Load(TestUtils::GetTestInputFilePath("TechDocs", "Acrobat_SignatureCreationQuickKeyAll.pdf"));

    PdfMemDocument dstDoc;
    dstDoc.Load(TestUtils::GetTestInputFilePath("TechDocs", "Acrobat_DigitalSignatures_in_PDF.pdf"));

    unsigned dstPageCount = dstDoc.GetPages().GetCount();
    unsigned srcPageCount = srcDoc.GetPages().GetCount();

    dstDoc.GetPages().AppendDocumentPages(srcDoc);

    REQUIRE(dstDoc.GetPages().GetCount() == dstPageCount + srcPageCount);

    string outputPath = TestUtils::GetTestOutputFilePath("TestAppendDocument.pdf");
    dstDoc.Save(outputPath);

    // Reload and verify
    PdfMemDocument reloaded;
    reloaded.Load(outputPath);
    REQUIRE(reloaded.GetPages().GetCount() == dstPageCount + srcPageCount);
}

TEST_CASE("TestFillXObjectMiniature")
{
    PdfMemDocument srcDoc;
    srcDoc.Load(TestUtils::GetTestInputFilePath("TechDocs", "Acrobat_SignatureCreationQuickKeyAll.pdf"));

    auto& srcPage = srcDoc.GetPages().GetPageAt(0);
    Rect srcBox = srcPage.GetMediaBox();

    PdfMemDocument dstDoc;

    // Create an XObject filled from the source page
    auto xobj = dstDoc.CreateXObjectForm(Rect());
    xobj->FillFromPage(srcPage);

    // Create a page sized to fit 4 miniatures (2x2 grid)
    double pageWidth = srcBox.Width;
    double pageHeight = srcBox.Height;
    auto& dstPage = dstDoc.GetPages().CreatePage(Rect(0, 0, pageWidth, pageHeight));

    // Scale to roughly a quarter of the page (half width, half height)
    double scale = 0.5;
    double miniHeight = srcBox.Height * scale;

    PdfPainter painter;
    painter.SetCanvas(dstPage);
    // Draw at top-left quadrant
    painter.DrawXObject(*xobj, 0, pageHeight - miniHeight, scale, scale);
    painter.FinishDrawing();

    string outputPath = TestUtils::GetTestOutputFilePath("TestFillXObjectMiniature.pdf");
    dstDoc.Save(outputPath);

    // Reload and verify basic structure
    PdfMemDocument reloaded;
    reloaded.Load(outputPath);
    REQUIRE(reloaded.GetPages().GetCount() == 1);
    Rect reloadedBox = reloaded.GetPages().GetPageAt(0).GetMediaBox();
    ASSERT_EQUAL(reloadedBox.Width, pageWidth);
    ASSERT_EQUAL(reloadedBox.Height, pageHeight);
}

TEST_CASE("TestAppendInheritedAttributes")
{
    // Start from a blank document and make its page inherit /Resources
    // from the /Pages tree root, with an indirect reference inside it
    PdfMemDocument srcDoc;
    srcDoc.Load(TestUtils::GetTestInputFilePath("blank.pdf"));

    auto& marker = srcDoc.GetObjects().CreateDictionaryObject();
    marker.GetDictionary().AddKey("MarkerTag"_n, PdfString("INHERITED_MARKER"));

    auto& xobjects = srcDoc.GetObjects().CreateDictionaryObject();
    xobjects.GetDictionary().AddKeyIndirect("XMarker"_n, marker);

    auto& resources = srcDoc.GetObjects().CreateDictionaryObject();
    resources.GetDictionary().AddKeyIndirect("XObject"_n, xobjects);

    srcDoc.GetPages().GetPageAt(0).GetObject().GetDictionary().RemoveKey("Resources");
    srcDoc.GetPages().GetObject().GetDictionary().AddKeyIndirect("Resources"_n, resources);

    // Append into a fresh destination so new object numbers overlap the
    // source ones, which would corrupt references if remapping happened twice
    PdfMemDocument dstDoc;
    dstDoc.GetPages().AppendDocumentPages(srcDoc);

    REQUIRE(dstDoc.GetPages().GetCount() == 1);

    auto& newPage = dstDoc.GetPages().GetPageAt(0);
    auto* res = newPage.GetDictionary().FindKeyParent("Resources");
    REQUIRE(res != nullptr);
    auto* xobjDict = res->GetDictionary().FindKey("XObject");
    REQUIRE(xobjDict != nullptr);
    auto* markerResolved = xobjDict->GetDictionary().FindKey("XMarker");
    REQUIRE(markerResolved != nullptr);
    auto* tag = markerResolved->GetDictionary().FindKey("MarkerTag");
    REQUIRE(tag != nullptr);
    REQUIRE(tag->GetString().GetString() == "INHERITED_MARKER");
}

TEST_CASE("TestAppendPageWithLongObjectChain")
{
    // The page references a chain of 100000 distinct indirect objects: the
    // copy of the referenced objects must not recurse once per object
    PdfMemDocument src;
    auto& page = src.GetPages().CreatePage();
    PdfObject* curr = &src.GetObjects().CreateArrayObject();
    page.GetDictionary().AddKeyIndirect("Chain"_n, *curr);
    for (unsigned i = 0; i < 100000; i++)
    {
        auto& next = src.GetObjects().CreateArrayObject();
        curr->GetArray().Add(PdfObject(next.GetIndirectReference()));
        curr = &next;
    }

    PdfMemDocument dst;
    dst.GetPages().AppendDocumentPages(src);

    unsigned length = 0;
    auto obj = dst.GetPages().GetPageAt(0).GetDictionary().FindKey("Chain");
    while (obj != nullptr && obj->GetArray().GetSize() != 0)
    {
        obj = obj->GetArray().FindAt(0);
        length++;
    }

    REQUIRE(length == 100000);
}

TEST_CASE("TestAppendPageWithSelfReferencingDecodeParms")
{
    // The /DecodeParms of the content stream is the stream object itself:
    // copying the stream must copy only its dictionary, not the stream again
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Contents 4 0 R>>endobj
4 0 obj<</Length 0/DecodeParms 4 0 R>>stream

endstream
endobj
trailer<</Root 1 0 R/Size 5>>
startxref
999999
%%EOF)";

    PdfMemDocument src;
    src.LoadFromBuffer(pdf);
    PdfMemDocument dst;
    dst.GetPages().AppendDocumentPages(src);
    auto& contents = dst.GetPages().GetPageAt(0).GetDictionary().MustFindKey("Contents");
    REQUIRE(contents.HasStream());
    auto& decodeParms = contents.GetDictionary().MustGetKey("DecodeParms");
    REQUIRE(!decodeParms.HasStream());
    REQUIRE(decodeParms.GetDictionary().MustFindKey("Length").GetNumber() == 0);
}

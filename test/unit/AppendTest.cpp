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

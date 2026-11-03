// SPDX-FileCopyrightText: 2008 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2021 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: MIT-0

#include <PdfTest.h>

#define TEST_PAGE_KEY "TestPageNumber"_n
constexpr unsigned TEST_NUM_PAGES = 100;

namespace PoDoFo
{
    class PdfPageTest
    {
    public:
        static std::vector<std::unique_ptr<PdfPage>> CreateSamplePages(PdfMemDocument& doc, unsigned pageCount);
        static PdfMemDocument CreateTestTreeCustom();
    };
}

using namespace std;
using namespace PoDoFo;

static bool isPageNumber(PdfPage& page, unsigned number);
static void createTestTree(PdfMemDocument& doc);
static void testGetPages(PdfMemDocument& doc);
static void testInsert(PdfMemDocument& doc);
static void testDeleteAll(PdfMemDocument& doc);
static void testGetPagesReverse(PdfMemDocument& doc);

TEST_CASE("TestEmptyDoc")
{
    PdfMemDocument doc;

    // Empty document must have page count == 0
    REQUIRE(doc.GetPages().GetCount() == 0);

    // Retrieving any page from an empty document must be NULL
    ASSERT_THROW_WITH_ERROR_CODE(doc.GetPages().GetPageAt(0), PdfErrorCode::ValueOutOfRange);
}

// GHSA-h2fg-3p95-qpgr: a tiny file with a huge /Pages /Count must not drive
// a multi-gigabyte reserve before the real /Kids tree is walked
TEST_CASE("TestHugePageCountReserve")
{
    static string_view hugeCountPdf = R"(%PDF-1.4
1 0 obj
<< /Type /Catalog /Pages 2 0 R >>
endobj
2 0 obj
<< /Type /Pages /Kids [] /Count 4000000000 >>
endobj
xref
0 3
0000000000 65535 f 
0000000009 00000 n 
0000000058 00000 n 
trailer
<< /Root 1 0 R /Size 3 >>
startxref
119
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(hugeCountPdf);
    REQUIRE(doc.GetPages().GetCount() == 0);
}

TEST_CASE("TestCreateDelete")
{
    PdfMemDocument doc;
    PdfPainter painter;

    // create font
    auto font = doc.GetFonts().SearchFont("LiberationSans");
    if (font == nullptr)
        FAIL("Could not find Arial font");

    {
        // write 1. page
        auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
        painter.SetCanvas(page);
        painter.TextState.SetFont(*font, 16.0);
        painter.DrawText("Page 1", 200, 200);
        painter.FinishDrawing();
        REQUIRE(doc.GetPages().GetCount() == 1);
    }

    {
        // write 2. page
        auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
        painter.SetCanvas(page);
        painter.TextState.SetFont(*font, 16.0);
        painter.DrawText("Page 2", 200, 200);
        painter.FinishDrawing();
        REQUIRE(doc.GetPages().GetCount() == 2);
    }

    // try to delete second page, index is 0 based 
    doc.GetPages().RemovePageAt(1);
    REQUIRE(doc.GetPages().GetCount() == 1);

    {
        // write 3. page
        auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
        painter.SetCanvas(page);
        painter.TextState.SetFont(*font, 16.0);
        painter.DrawText("Page 3", 200, 200);
        painter.FinishDrawing();
        REQUIRE(doc.GetPages().GetCount() == 2);
    }
}

TEST_CASE("TestGetPagesCustom")
{
    auto doc = PdfPageTest::CreateTestTreeCustom();
    testGetPages(doc);
}

TEST_CASE("TestGetPages")
{
    PdfMemDocument doc;
    createTestTree(doc);
    testGetPages(doc);
}

TEST_CASE("TestGetPagesReverseCustom")
{
    auto doc = PdfPageTest::CreateTestTreeCustom();
    testGetPagesReverse(doc);
}

TEST_CASE("TestGetPagesReverse")
{
    PdfMemDocument doc;
    createTestTree(doc);
    testGetPagesReverse(doc);
}

TEST_CASE("TestInsertCustom")
{
    auto doc = PdfPageTest::CreateTestTreeCustom();
    testInsert(doc);
}

TEST_CASE("TestInsert")
{
    PdfMemDocument doc;
    createTestTree(doc);
    testInsert(doc);
}

TEST_CASE("TestDeleteAllCustom")
{
    auto doc = PdfPageTest::CreateTestTreeCustom();
    testDeleteAll(doc);
}

TEST_CASE("TestDeleteAll")
{
    PdfMemDocument doc;
    createTestTree(doc);
    testDeleteAll(doc);
}

TEST_CASE("TestMovePage1")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TechDocs", "pdf_implementation.pdf"));

    vector<PdfReference> refs = {
        PdfReference(15, 0),
        PdfReference(82, 0),
        PdfReference(88, 0),
        PdfReference(83, 0),
        PdfReference(84, 0),
        PdfReference(85, 0),
        PdfReference(86, 0),
        PdfReference(87, 0),
        PdfReference(89, 0),
        PdfReference(90, 0),
        PdfReference(91, 0),
    };

    {
        auto& pages = doc.GetPages();

        {
            auto& page = pages.GetPageAt(7);
            REQUIRE(page.GetIndex() == 7);
            REQUIRE(!page.MoveTo(11));
            REQUIRE(page.MoveTo(2));
        }

        for (unsigned i = 0; i < pages.GetCount(); i++)
        {
            auto& page = pages.GetPageAt(i);
            REQUIRE(page.GetIndex() == i);
            REQUIRE(page.GetObject().GetIndirectReference() == refs[i]);
        }
    }

    string filename = TestUtils::GetTestOutputFilePath("TestMovePage1.pdf");
    doc.Save(filename);

    // Re-load the file to check again the references
    doc.Load(filename);
    auto& pages = doc.GetPages();
    for (unsigned i = 0; i < pages.GetCount(); i++)
        REQUIRE(pages.GetPageAt(i).GetObject().GetIndirectReference() == refs[i]);
}

TEST_CASE("TestMovePage2")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TechDocs", "pdf_implementation.pdf"));

    vector<PdfReference> refs = {
        PdfReference(82, 0),
        PdfReference(83, 0),
        PdfReference(84, 0),
        PdfReference(15, 0),
        PdfReference(85, 0),
        PdfReference(86, 0),
        PdfReference(87, 0),
        PdfReference(88, 0),
        PdfReference(89, 0),
        PdfReference(90, 0),
        PdfReference(91, 0),
    };

    {
        auto& pages = doc.GetPages();

        {
            auto& page = pages.GetPageAt(0);
            REQUIRE(page.GetIndex() == 0);
            REQUIRE(!page.MoveTo(0));
            REQUIRE(page.MoveTo(3));
        }

        for (unsigned i = 0; i < pages.GetCount(); i++)
        {
            auto& page = pages.GetPageAt(i);
            REQUIRE(page.GetIndex() == i);
            REQUIRE(page.GetObject().GetIndirectReference() == refs[i]);
        }
    }

    string filename = TestUtils::GetTestOutputFilePath("TestMovePage2.pdf");
    doc.Save(filename);

    // Re-load the file to check again the references
    doc.Load(filename);
    auto& pages = doc.GetPages();
    for (unsigned i = 0; i < pages.GetCount(); i++)
        REQUIRE(pages.GetPageAt(i).GetObject().GetIndirectReference() == refs[i]);
}

TEST_CASE("TestPageTreeDuplicatePage")
{
    // The same page is listed twice in /Kids: it must be wrapped once, as two
    // wrappers would share the inline /Annots array and one would free it
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R 3 0 R 5 0 R]/Count 3>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]/Annots[4 0 R]>>endobj
4 0 obj<</Type/Annot/Subtype/Square/Rect[0 0 5 5]>>endobj
5 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 9 9]>>endobj
trailer<</Root 1 0 R/Size 6>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& pages = doc.GetPages();
    REQUIRE(pages.GetCount() == 2);
    REQUIRE(pages.GetPageAt(1).GetObject().GetIndirectReference() == PdfReference(5, 0));
    auto& annots = pages.GetPageAt(0).GetAnnotations();
    annots.CreateAnnot(PdfAnnotationType::Square, Rect(0, 0, 5, 5));
    REQUIRE(annots.GetCount() == 2);
}

TEST_CASE("TestPageTreeDuplicatePageContents")
{
    // With two wrappers of the same page, appending content through one
    // orphaned the stream still wrapped by the other, which the garbage
    // collection on save then freed: the page must be wrapped once
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj
2 0 obj<</Type/Pages/Kids[3 0 R 3 0 R]/Count 2>>endobj
3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 100 100]/Contents 4 0 R>>endobj
4 0 obj<</Length 15>>stream
0 0 m 50 50 l S
endstream
endobj
trailer<</Root 1 0 R/Size 5>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    auto& pages = doc.GetPages();
    REQUIRE(pages.GetCount() == 1);
    {
        PdfPainter painter;
        painter.SetCanvas(pages.GetPageAt(0));
        painter.DrawLine(0, 0, 50, 50);
        painter.FinishDrawing();
    }

    doc.Save(TestUtils::GetTestOutputFilePath("TestPageTreeDuplicatePageContents.pdf"));
    vector<PdfTextEntry> entries;
    pages.GetPageAt(0).ExtractTextTo(entries);
    REQUIRE(entries.size() == 0);
}

void testGetPages(PdfMemDocument& doc)
{
    for (unsigned i = 0; i < TEST_NUM_PAGES; i++)
    {
        auto& page = doc.GetPages().GetPageAt(i);
        REQUIRE(isPageNumber(page, i));
    }

    // Now delete first page 
    doc.GetPages().RemovePageAt(0);

    for (unsigned i = 0; i < TEST_NUM_PAGES - 1; i++)
    {
        auto& page = doc.GetPages().GetPageAt(i);
        REQUIRE(isPageNumber(page, i + 1));
    }

    // Now delete any page
    constexpr unsigned DELETED_PAGE = 50;
    doc.GetPages().RemovePageAt(DELETED_PAGE);

    for (unsigned i = 0; i < TEST_NUM_PAGES - 2; i++)
    {
        auto& page = doc.GetPages().GetPageAt(i);
        if (i < DELETED_PAGE)
            REQUIRE(isPageNumber(page, i + 1));
        else
            REQUIRE(isPageNumber(page, i + 2));
    }
}

void testGetPagesReverse(PdfMemDocument& doc)
{
    for (int i = TEST_NUM_PAGES - 1; i >= 0; i--)
    {
        unsigned index = (unsigned)i;
        auto& page = doc.GetPages().GetPageAt(index);
        REQUIRE(isPageNumber(page, index));
    }

    // Now delete first page 
    doc.GetPages().RemovePageAt(0);

    for (int i = TEST_NUM_PAGES - 2; i >= 0; i--)
    {
        unsigned index = (unsigned)i;
        auto& page = doc.GetPages().GetPageAt(index);
        REQUIRE(isPageNumber(page, index + 1));
    }
}

void testInsert(PdfMemDocument& doc)
{
    const unsigned INSERTED_PAGE_FLAG = 1234;
    const unsigned INSERTED_PAGE_FLAG1 = 1234 + 1;
    const unsigned INSERTED_PAGE_FLAG2 = 1234 + 2;

    {
        auto& page = doc.GetPages().CreatePageAt(0, PdfPageSize::A4);
        page.GetDictionary().AddKey(TEST_PAGE_KEY,
            static_cast<int64_t>(INSERTED_PAGE_FLAG));
    }

    // Find inserted page (beginning)
    REQUIRE(isPageNumber(doc.GetPages().GetPageAt(0), INSERTED_PAGE_FLAG));

    // Find old first page
    REQUIRE(isPageNumber(doc.GetPages().GetPageAt(1), 0));

    {
        // Insert at end 
        auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
        page.GetDictionary().AddKey(TEST_PAGE_KEY,
            static_cast<int64_t>(INSERTED_PAGE_FLAG1));
    }

    REQUIRE(isPageNumber(doc.GetPages().GetPageAt(doc.GetPages().GetCount() - 1),
        INSERTED_PAGE_FLAG1));

    // Insert in middle
    const unsigned INSERT_POINT = 50;
    {
        auto& page = doc.GetPages().CreatePageAt(INSERT_POINT, PdfPageSize::A4);
        page.GetDictionary().AddKey(TEST_PAGE_KEY,
            static_cast<int64_t>(INSERTED_PAGE_FLAG2));
    }

    REQUIRE(isPageNumber(doc.GetPages().GetPageAt(INSERT_POINT), INSERTED_PAGE_FLAG2));
}

void testDeleteAll(PdfMemDocument& doc)
{
    for (unsigned i = 0; i < TEST_NUM_PAGES; i++)
    {
        doc.GetPages().RemovePageAt(0);
        REQUIRE(doc.GetPages().GetCount() == TEST_NUM_PAGES - (i + 1));
    }
    REQUIRE(doc.GetPages().GetCount() == 0);
}

void createTestTree(PdfMemDocument& doc)
{
    for (unsigned i = 0; i < TEST_NUM_PAGES; i++)
    {
        auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
        page.GetDictionary().AddKey(TEST_PAGE_KEY, static_cast<int64_t>(i));
        REQUIRE(doc.GetPages().GetCount() == i + 1);
    }
}

PdfMemDocument PdfPageTest::CreateTestTreeCustom()
{
    PdfMemDocument doc;

    constexpr unsigned COUNT = TEST_NUM_PAGES / 10;
    auto& root = doc.GetObjects().CreateDictionaryObject("Pages"_n);
    doc.GetCatalog().GetDictionary().AddKeyIndirect("Pages"_n, root);
    PdfArray rootKids;

    for (unsigned i = 0; i < COUNT; i++)
    {
        auto& node = doc.GetObjects().CreateDictionaryObject("Pages"_n);
        PdfArray nodeKids;

        for (unsigned j = 0; j < COUNT; j++)
        {
            unique_ptr<PdfPage> page(new PdfPage(doc, PdfPage::CreateStandardPageSize(PdfPageSize::A4)));
            page->SetIndex(j);
            page->GetDictionary().AddKey(TEST_PAGE_KEY,
                static_cast<int64_t>(i) * COUNT + j);

            nodeKids.Add(page->GetObject().GetIndirectReference());
        }

        node.GetDictionary().AddKey("Kids"_n, nodeKids);
        node.GetDictionary().AddKey("Count"_n, static_cast<int64_t>(COUNT));
        rootKids.Add(node.GetIndirectReference());
    }

    root.GetDictionary().AddKey("Kids"_n, rootKids);
    root.GetDictionary().AddKey("Count"_n, static_cast<int64_t>(TEST_NUM_PAGES));

    // NOTE: We must copy the document as the PdfPageCollection
    // in the source document is already initialized
    return PdfMemDocument(doc);
}

bool isPageNumber(PdfPage& page, unsigned number)
{
    int64_t pageNumber = page.GetDictionary().GetKeyAsSafe<int64_t>(TEST_PAGE_KEY, -1);

    if (pageNumber != static_cast<int64_t>(number))
    {
        INFO(utls::Format("PagesTreeTest: Expected page number {} but got {}", number, pageNumber));
        return false;
    }
    else
        return true;
}

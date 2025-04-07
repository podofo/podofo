/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

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

static void appendChildNode(PdfObject& parent, PdfObject& child);
static bool isPageNumber(PdfPage& page, unsigned number);

static vector<PdfObject*> createNodes(PdfMemDocument& doc, unsigned nodeCount);
static void createEmptyKidsTree(PdfMemDocument& doc);
static void createNestedArrayTree(PdfMemDocument& doc);
static void createTestTree(PdfMemDocument& doc);
static void createCyclicTree(PdfMemDocument& doc, bool createCycle);
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

TEST_CASE("TestCyclicTree")
{
    {
        PdfMemDocument doc;
        createCyclicTree(doc, false);
        for (unsigned pagenum = 0; pagenum < doc.GetPages().GetCount(); pagenum++)
        {
            // pass 0:
            // valid tree without cycles should yield all pages
            auto& page = doc.GetPages().GetPageAt(pagenum);
            REQUIRE(isPageNumber(page, pagenum));
        }
    }

    {
        PdfMemDocument doc;
        createCyclicTree(doc, true);
        // pass 1:
        // cyclic tree must throw exception to prevent infinite recursion
        ASSERT_THROW_WITH_ERROR_CODE(doc.GetPages().GetPageAt(0), PdfErrorCode::ValueOutOfRange);
    }
}

TEST_CASE("TestEmptyKidsTree")
{
    PdfMemDocument doc;
    createEmptyKidsTree(doc);
    //doc.Write("tree_zerokids.pdf");
    for (unsigned pagenum = 0; pagenum < doc.GetPages().GetCount(); pagenum++)
    {
        PdfPage& page = doc.GetPages().GetPageAt(pagenum);
        REQUIRE(isPageNumber(page, pagenum));
    }
}

TEST_CASE("TestNestedArrayTree")
{
    PdfMemDocument doc;
    createNestedArrayTree(doc);
    for (unsigned i = 0, count = doc.GetPages().GetCount(); i < count; i++)
        ASSERT_THROW_WITH_ERROR_CODE(doc.GetPages().GetPageAt(i), PdfErrorCode::ValueOutOfRange);
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

vector<unique_ptr<PdfPage>> PdfPageTest::CreateSamplePages(PdfMemDocument& doc, unsigned pageCount)
{
    // create font
    auto font = doc.GetFonts().SearchFont("LiberationSans");
    if (font == nullptr)
        FAIL("Could not find Arial font");

    vector<unique_ptr<PdfPage>> pages(pageCount);
    for (unsigned i = 0; i < pageCount; ++i)
    {
        pages[i].reset(new PdfPage(doc, PdfPage::CreateStandardPageSize(PdfPageSize::A4)));
        pages[i]->SetIndex(i);
        pages[i]->GetDictionary().AddKey(TEST_PAGE_KEY, static_cast<int64_t>(i));

        PdfPainter painter;
        painter.SetCanvas(*pages[i]);
        painter.TextState.SetFont(*font, 16.0);
        ostringstream os;
        os << "Page " << i + 1;
        painter.DrawText(os.str(), 200, 200);
        painter.FinishDrawing();
    }

    return pages;
}

vector<PdfObject*> createNodes(PdfMemDocument& doc, unsigned nodeCount)
{
    vector<PdfObject*> nodes(nodeCount);

    for (unsigned i = 0; i < nodeCount; ++i)
    {
        nodes[i] = &doc.GetObjects().CreateDictionaryObject("Pages"_n);
        // init required keys
        nodes[i]->GetDictionary().AddKey("Kids"_n, PdfArray());
        nodes[i]->GetDictionary().AddKey("Count"_n, PdfVariant(static_cast<int64_t>(0L)));
    }

    return nodes;
}

void createCyclicTree(PdfMemDocument& doc, bool createCycle)
{
    const unsigned COUNT = 3;

    auto pages = PdfPageTest::CreateSamplePages(doc, COUNT);
    auto nodes = createNodes(doc, 2);

    // manually insert pages into pagetree
    auto& root = doc.GetPages().GetObject();

    // tree layout (for !bCreateCycle):
    //
    //    root
    //    +-- node0
    //        +-- node1
    //        |   +-- page0
    //        |   +-- page1
    //        \-- page2

    // root node
    appendChildNode(root, *nodes[0]);

    // tree node 0
    appendChildNode(*nodes[0], *nodes[1]);
    appendChildNode(*nodes[0], pages[2]->GetObject());

    // tree node 1
    appendChildNode(*nodes[1], pages[0]->GetObject());
    appendChildNode(*nodes[1], pages[1]->GetObject());

    if (createCycle)
    {
        // invalid tree: Cycle!!!
        // was not detected in PdfPagesTree::GetPageNode() rev. 1937
        nodes[0]->GetDictionary().MustFindKey("Kids").GetArray()[0] = root.GetIndirectReference();
    }
}

void createEmptyKidsTree(PdfMemDocument& doc)
{
    const unsigned COUNT = 3;

    auto pages = PdfPageTest::CreateSamplePages(doc, COUNT);
    auto nodes = createNodes(doc, 3);

    // manually insert pages into pagetree
    auto& root = doc.GetPages().GetObject();

    // tree layout:
    //
    //    root
    //    +-- node0
    //    |   +-- page0
    //    |   +-- page1
    //    |   +-- page2
    //    +-- node1
    //    \-- node2

    // root node
    appendChildNode(root, *nodes[0]);
    appendChildNode(root, *nodes[1]);
    appendChildNode(root, *nodes[2]);

    // tree node 0
    appendChildNode(*nodes[0], pages[0]->GetObject());
    appendChildNode(*nodes[0], pages[1]->GetObject());
    appendChildNode(*nodes[0], pages[2]->GetObject());

    // tree node 1 and node 2 are left empty: this is completely valid
    // according to the PDF spec, i.e. the required keys may have the
    // values "/Kids [ ]" and "/Count 0"
}

void createNestedArrayTree(PdfMemDocument& doc)
{
    constexpr unsigned COUNT = 3;

    auto pages = PdfPageTest::CreateSamplePages(doc, COUNT);
    auto& root = doc.GetPages().GetObject();

    // create kids array
    PdfArray kids;
    for (unsigned i = 0; i < COUNT; i++)
    {
        kids.Add(pages[i]->GetObject().GetIndirectReference());
        pages[i]->GetDictionary().AddKey("Parent"_n, root.GetIndirectReference());
    }

    // create nested kids array
    PdfArray nested;
    nested.Add(kids);

    // manually insert pages into pagetree
    root.GetDictionary().AddKey("Count"_n, static_cast<int64_t>(COUNT));
    root.GetDictionary().AddKey("Kids"_n, nested);
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

void appendChildNode(PdfObject& parent, PdfObject& child)
{
    // 1. Add the reference of the new child to the kids array of parent
    PdfArray kids;
    PdfObject* oldKids = parent.GetDictionary().FindKey("Kids");
    if (oldKids != nullptr && oldKids->IsArray()) kids = oldKids->GetArray();
    kids.Add(child.GetIndirectReference());
    parent.GetDictionary().AddKey("Kids"_n, kids);

    // 2. If the child is a page (leaf node), increase count of every parent
    //    (which also includes pParent)
    if (child.GetDictionary().GetKeyAs<PdfName>("Type") == "Page")
    {
        PdfObject* node = &parent;
        while (node)
        {
            int64_t count = 0;
            if (node->GetDictionary().FindKey("Count")) count = node->GetDictionary().FindKey("Count")->GetNumber();
            count++;
            node->GetDictionary().AddKey("Count"_n, count);
            node = node->GetDictionary().FindKey("Parent");
        }
    }

    // 3. Add Parent key to the child
    child.GetDictionary().AddKey("Parent"_n, parent.GetIndirectReference());
}

// SPDX-FileCopyrightText: 2018 Amin Massad <a.massad@gmx.de>
// SPDX-FileCopyrightText: 2018 Clayton Wheeler <cwheeler@genomenon.com>
// SPDX-License-Identifier: GPL-2.0-or-later

// ---------------
// !!! WARNING !!!
// ---------------
// These tests are still GPLv2+ licensed and
// are not yet relicensed/rewritten to MIT-0

#ifdef PODOFO_ENABLE_GPL2_TESTS

#include <PdfTest.h>

#include <ostream>
#include <iostream>

using namespace std;
using namespace PoDoFo;

#define TEST_PAGE_KEY "TestPageNumber"_n

static vector<PdfObject*> createNodes(PdfMemDocument& doc, unsigned nodeCount);
static void createEmptyKidsTree(PdfMemDocument& doc);
static void createNestedArrayTree(PdfMemDocument& doc);
static void createCyclicTree(PdfMemDocument& doc, bool createCycle);
static void appendChildNode(PdfObject& parent, PdfObject& child);
static bool isPageNumber(PdfPage& page, unsigned number);

namespace PoDoFo
{
    class PdfPageTest
    {
    public:
        static std::vector<std::unique_ptr<PdfPage>> CreateSamplePages(PdfMemDocument& doc, unsigned pageCount);
        static PdfMemDocument CreateTestTreeCustom();
    };
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

// Clayton Wheeler, commit 2cddb84ae550afd612749cac831ae0bb330c6422
TEST_CASE("TestSaveIncrementalRoundTrip")
{
    ostringstream oss;
    oss << "%PDF-1.1\n";
    unsigned currObj = 1;
    streamoff objPos[20];

    // Pages

    unsigned pagesObj = currObj;
    objPos[currObj] = oss.tellp();
    oss << currObj++ << " 0 obj\n";
    oss << "<</Type /Pages /Count 0 /Kids []>>\n";
    oss << "endobj\n";

    // Root catalog

    unsigned rootObj = currObj;
    objPos[currObj] = oss.tellp();
    oss << currObj++ << " 0 obj\n";
    oss << "<</Type /Catalog /Pages " << pagesObj << " 0 R>>\n";
    oss << "endobj\n";

    // ID
    unsigned idObj = currObj;
    objPos[currObj] = oss.tellp();
    oss << currObj++ << " 0 obj\n";
    oss << "[<F1E375363A6314E3766EDF396D614748> <F1E375363A6314E3766EDF396D614748>]\n";
    oss << "endobj\n";

    streamoff xrefPos = oss.tellp();
    oss << "xref\n";
    oss << "0 " << currObj << "\n";
    oss << "0000000000 65535 f \n";
    for (unsigned i = 1; i < currObj; i++)
        oss << utls::Format("{:010d} 00000 n \n", objPos[i]);

    oss << "trailer <<\n"
        << "  /Size " << currObj << "\n"
        << "  /Root " << rootObj << " 0 R\n"
        << "  /ID " << idObj << " 0 R\n" // indirect ID
        << ">>\n"
        << "startxref\n"
        << xrefPos << "\n"
        << "%%EOF\n";

    string docBuff = oss.str();
    try
    {
        PdfMemDocument doc;
        // load for update
        doc.LoadFromBuffer(docBuff);

        StringStreamDevice outDev(docBuff);

        doc.SaveUpdate(outDev);
        doc.LoadFromBuffer(docBuff);
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
}

#endif // PODOFO_ENABLE_GPL2_TESTS

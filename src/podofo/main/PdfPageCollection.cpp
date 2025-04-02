/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfPageCollection.h"

#include <algorithm>

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfObject.h"
#include <podofo/auxiliary/OutputDevice.h>
#include "PdfPage.h"

using namespace std;
using namespace PoDoFo;

namespace
{
    enum class PdfPageTreeNodeType : uint8_t
    {
        Unknown,
        Node,
        Page
    };
}

static PdfPageTreeNodeType getPageTreeNodeType(const PdfObject& nodeObj);
static unsigned getChildCount(const PdfObject& nodeObj);

PdfPageCollection::PdfPageCollection(PdfDocument& doc)
    : PdfDictionaryElement(doc, "Pages"_n), m_initialized(true)
{
    m_kidsArray = &GetDictionary().AddKey("Kids"_n, PdfArray()).GetArray();
    GetDictionary().AddKey("Count"_n, static_cast<int64_t>(0));
}

PdfPageCollection::PdfPageCollection(PdfObject& pagesRoot)
    : PdfDictionaryElement(pagesRoot), m_initialized(false), m_kidsArray(nullptr)
{
}

PdfPageCollection::~PdfPageCollection()
{
    for (unsigned i = 0; i < m_Pages.size(); i++)
        delete m_Pages[i];
}

unsigned PdfPageCollection::GetCount() const
{
    const_cast<PdfPageCollection&>(*this).initPages();
    return (unsigned)m_Pages.size();
}

PdfPage& PdfPageCollection::GetPageAt(unsigned index)
{
    const_cast<PdfPageCollection&>(*this).initPages();
    if (index >= m_Pages.size())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Page with index {} not found", index);

    return *m_Pages[index];
}

const PdfPage& PdfPageCollection::GetPageAt(unsigned index) const
{
    const_cast<PdfPageCollection&>(*this).initPages();
    if (index >= m_Pages.size())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Page with index {} not found", index);

    return *m_Pages[index];
}

PdfPage& PdfPageCollection::GetPage(const PdfReference& ref)
{
    const_cast<PdfPageCollection&>(*this).initPages();
    return getPage(ref);
}

const PdfPage& PdfPageCollection::GetPage(const PdfReference& ref) const
{
    const_cast<PdfPageCollection&>(*this).initPages();
    return getPage(ref);
}

Rect PdfPageCollection::getActualRect(const nullable<Rect>& size)
{
    if (size == nullptr)
    {
        if (m_Pages.size() == 0)
            return PdfPage::CreateStandardPageSize(PdfPageSize::A4);
        else
            return m_Pages[m_Pages.size() - 1]->GetRect();
    }
    else
    {
        return *size;
    }
}

PdfPage& PdfPageCollection::getPage(const PdfReference& ref) const
{
    // We have to search through all pages,
    // as this is the only way
    // to instantiate the PdfPage with a correct list of parents
    for (unsigned i = 0; i < m_Pages.size(); i++)
    {
        auto& page = *m_Pages[i];
        if (page.GetObject().GetIndirectReference() == ref)
            return page;
    }

    PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);
}

PdfPageCollection::iterator PdfPageCollection::begin()
{
    return m_Pages.begin();
}

PdfPageCollection::iterator PdfPageCollection::end()
{
    return m_Pages.end();
}

PdfPageCollection::const_iterator PdfPageCollection::begin() const
{
    return m_Pages.begin();
}

PdfPageCollection::const_iterator PdfPageCollection::end() const
{
    return m_Pages.end();
}

void PdfPageCollection::InsertPageAt(unsigned atIndex, PdfPage& pageObj)
{
    FlattenStructure();
    vector<PdfPage*> objs = { &pageObj };
    insertPagesAt(atIndex, objs);
}

void PdfPageCollection::insertPageAt(unsigned atIndex, PdfPage& pageObj)
{
    vector<PdfPage*> objs = { &pageObj };
    insertPagesAt(atIndex, objs);
}

void PdfPageCollection::InsertPagesAt(unsigned atIndex, cspan<PdfPage*> pages)
{
    FlattenStructure();
    insertPagesAt(atIndex, pages);
}

bool PdfPageCollection::TryMovePageTo(unsigned atIndex, unsigned toIndex)
{
    PODOFO_ASSERT(atIndex < m_Pages.size() && atIndex != toIndex);
    if (toIndex >= m_Pages.size())
        return false;
    
    FlattenStructure();

    m_kidsArray->MoveTo(atIndex, toIndex);

    auto temp = m_Pages[atIndex];
    if (atIndex > toIndex)
    {
        for (unsigned i = atIndex; i > toIndex; i--)
        {
            m_Pages[i] = m_Pages[i - 1];
            m_Pages[i]->SetIndex(i);
        }
    }
    else
    {
        for (unsigned i = atIndex; i < toIndex; i++)
        {
            m_Pages[i] = m_Pages[i + 1];
            m_Pages[i]->SetIndex(i);
        }
    }

    m_Pages[toIndex] = temp;
    m_Pages[toIndex]->SetIndex(toIndex);

    return true;
}

void PdfPageCollection::insertPagesAt(unsigned atIndex, cspan<PdfPage*> pages)
{
    // Insert the pages and fix the indices
    m_Pages.insert(m_Pages.begin() + atIndex, pages.begin(), pages.end());
    for (unsigned i = atIndex; i < m_Pages.size(); i++)
        m_Pages[i]->SetIndex(i);

    // Update the actual /Kids array and set /Parent to the new pages
    vector<PdfObject> pageObjects;
    pageObjects.reserve(pages.size());
    for (unsigned i = 0; i < pages.size(); i++)
    {
        pageObjects.push_back(pages[i]->GetObject().GetIndirectReference());
        pages[i]->GetDictionary().AddKey("Parent"_n, GetObject().GetIndirectReference());
    }

    m_kidsArray->insert(m_kidsArray->begin() + atIndex, pageObjects.begin(), pageObjects.end());
    GetDictionary().AddKey("Count"_n, static_cast<int64_t>(m_Pages.size()));
}

PdfPage& PdfPageCollection::CreatePage(const nullable<Rect>& size_)
{
    FlattenStructure();
    auto size = getActualRect(size_);
    unique_ptr<PdfPage> page(new PdfPage(GetDocument(), size));
    insertPageAt((unsigned)m_Pages.size(), *page);
    return *page.release();
}

PdfPage& PdfPageCollection::CreatePage(PdfPageSize pageSize)
{
    return CreatePage(PdfPage::CreateStandardPageSize(pageSize));
}

PdfPage& PdfPageCollection::CreatePageAt(unsigned atIndex, const nullable<Rect>& size_)
{
    FlattenStructure();
    auto size = getActualRect(size_);

    unsigned pageCount = this->GetCount();
    if (atIndex > pageCount)
        atIndex = pageCount;

    unique_ptr<PdfPage> page(new PdfPage(GetDocument(), size));
    insertPageAt(atIndex, *page);
    return *page.release();
}

PdfPage& PdfPageCollection::CreatePageAt(unsigned atIndex, PdfPageSize pageSize)
{
    return CreatePageAt(atIndex, PdfPage::CreateStandardPageSize(pageSize));
}

void PdfPageCollection::CreatePagesAt(unsigned atIndex, unsigned count, const nullable<Rect>& size_)
{
    FlattenStructure();
    auto size = getActualRect(size_);

    unsigned pageCount = this->GetCount();
    if (atIndex > pageCount)
        atIndex = pageCount;

    vector<PdfPage*> pages(count);
    for (unsigned i = 0; i < count; i++)
        pages[i] = new PdfPage(GetDocument(), size);

    insertPagesAt(atIndex, pages);
}

void PdfPageCollection::CreatePagesAt(unsigned atIndex, unsigned count, PdfPageSize pageSize)
{
    CreatePagesAt(atIndex, count, PdfPage::CreateStandardPageSize(pageSize));
}

void PdfPageCollection::AppendDocumentPages(const PdfDocument& doc)
{
    return GetDocument().AppendDocumentPages(doc);
}

void PdfPageCollection::AppendDocumentPages(const PdfDocument& doc, unsigned pageIndex, unsigned pageCount)
{
    return GetDocument().AppendDocumentPages(doc, pageIndex, pageCount);
}

void PdfPageCollection::InsertDocumentPageAt(unsigned atIndex, const PdfDocument& doc, unsigned pageIndex)
{
    return GetDocument().InsertDocumentPageAt(atIndex, doc, pageIndex);
}

void PdfPageCollection::RemovePageAt(unsigned atIndex)
{
    FlattenStructure();
    if (atIndex >= m_Pages.size())
        return;

    auto page = m_Pages[atIndex];
    m_Pages.erase(m_Pages.begin() + atIndex);
    delete page;
    m_kidsArray->RemoveAt(atIndex);

    // Fix page indices
    for (unsigned i = atIndex; i < m_Pages.size(); i++)
        m_Pages[i]->SetIndex(i);

    GetDictionary().AddKey("Count"_n, static_cast<int64_t>(m_Pages.size()));

    // After removing the page the /OpenAction entry may be invalidated,
    // prompting an error using Acrobat. Remove it for safer behavior
    GetDocument().GetCatalog().GetDictionary().RemoveKey("OpenAction");
}

void PdfPageCollection::initPages()
{
    if (m_initialized)
        return;

    vector<PdfObject*> parents;
    unsigned count = getChildCount(GetObject());
    if (count != 0)
    {
        m_Pages.reserve(count);
        unordered_set<PdfObject*> visitedNodes;
        (void)traversePageTreeNode(GetObject(), count, parents, visitedNodes);
    }

    m_initialized = true;
}

// Returns the number of the remaining
unsigned PdfPageCollection::traversePageTreeNode(PdfObject& obj, unsigned count,
    vector<PdfObject*>& parents, unordered_set<PdfObject*>& visitedNodes)
{
    PODOFO_ASSERT(count != 0);
    utls::RecursionGuard guard;

    auto type = getPageTreeNodeType(obj);
    switch (type)
    {
        case PdfPageTreeNodeType::Node:
        {
            auto pair = visitedNodes.insert(&obj);
            if (!pair.second)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::BrokenFile, "The page structure tree has loops");

            auto kidsObj = obj.GetDictionary().FindKey("Kids");
            PdfArray* kidsArr;
            if (kidsObj == nullptr || !kidsObj->TryGetArray(kidsArr))
                return 0;

            parents.push_back(&obj);

            PdfReference ref;
            for (unsigned i = 0; i < kidsArr->GetSize(); i++)
            {
                auto child = &(*kidsArr)[i];
                if (child->TryGetReference(ref))
                    child = obj.MustGetDocument().GetObjects().GetObject(ref);

                if (child == nullptr)
                    continue;

                count = traversePageTreeNode(*child, count, parents, visitedNodes);
                if (count == 0)
                    break;
            }

            parents.pop_back();
            return count;
        }
        case PdfPageTreeNodeType::Page:
        {
            unsigned index = (unsigned)m_Pages.size();
            unique_ptr<PdfPage> page(new PdfPage(obj, vector<PdfObject*>(parents)));
            m_Pages.push_back(page.get());
            (*page.release()).SetIndex(index);
            return count - 1;
        }
        case PdfPageTreeNodeType::Unknown:
        default:
        {
            // NOTE: This is a degenerate case
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::BrokenFile, "The page structure tree has invalid nodes");
        }
    }
}

void PdfPageCollection::FlattenStructure()
{
    if (m_kidsArray != nullptr)
        return;

    initPages();

    // Flatten the document page structure by recreating a single /Pages
    // node and insert all pages there. This is allowed by PDF
    // Specification, see ISO 32000-2:2020, 7.7.3.2 Page tree nodes:
    // "PDF processors shall not be required to preserve the existing
    // structure of the page tree"
    auto& kidsObj = GetDocument().GetObjects().CreateArrayObject();
    GetDictionary().AddKeyIndirect("Kids"_n, kidsObj);
    m_kidsArray = &kidsObj.GetArray();
    m_kidsArray->reserve(m_Pages.size());
    for (unsigned i = 0; i < m_Pages.size(); i++)
    {
        auto page = m_Pages[i];
        page->FlattenStructure();

        // Fix pages parent and add them to /Kids
        page->GetDictionary().AddKey("Parent"_n, GetObject().GetIndirectReference());
        (*m_kidsArray).AddIndirect(page->GetObject());
    }
}

PdfPageTreeNodeType getPageTreeNodeType(const PdfObject& obj)
{
    const PdfName* name;
    if (!obj.GetDictionary().TryFindKeyAs("Type", name))
        return PdfPageTreeNodeType::Unknown;

    if (*name == "Page")
        return PdfPageTreeNodeType::Page;
    else if (*name == "Pages")
        return PdfPageTreeNodeType::Node;
    else
        return PdfPageTreeNodeType::Unknown;
}

unsigned getChildCount(const PdfObject& nodeObj)
{
    auto countObj = nodeObj.GetDictionary().FindKey("Count");
    int64_t num;
    if (countObj == nullptr || !countObj->TryGetNumber(num) || num < 0)
        return 1;

    return (unsigned)num;
}

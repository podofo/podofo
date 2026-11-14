// SPDX-FileCopyrightText: 2006 Dominik Seichter <domseichter@web.de>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfOutlines.h"

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfObject.h"
#include "PdfAction.h"
#include "PdfDestination.h"

using namespace std;
using namespace PoDoFo;

static void deleteItems(PdfOutlineItem* item);

PdfOutlineItem::PdfOutlineItem(PdfObject& obj, PdfOutlineItem* parentOutline, PdfOutlineItem* previous,
        unordered_set<const PdfObject*>* visited)
    : PdfDictionaryElement(obj), m_ParentOutline(parentOutline), m_Prev(previous),
    m_Next(nullptr), m_First(nullptr), m_Last(nullptr), m_Destination(nullptr), m_Action(nullptr)
{
    // NOTE: Only the nesting recurses, as the children are built
    // below in a loop along /Next, by the parent that owns them
    utls::RecursionGuard guard;

    // NOTE: The depth guard can't bound the count of items: if /First and
    // /Next of each object point to the same next object, every path to an
    // object would create an item, so an object linked twice is rejected,
    // which also ends a /Next cycle
    unique_ptr<unordered_set<const PdfObject*>> rootVisited;
    if (visited == nullptr)
    {
        rootVisited.reset(new unordered_set<const PdfObject*>());
        visited = rootVisited.get();
    }

    if (!visited->insert(&obj).second)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::BrokenFile, "The outline item {} is linked more than once", obj.GetIndirectReference().ToString());

    // NOTE: The chain is owned by a local guard until it's complete, as
    // the destructor doesn't run for a throwing constructor
    unique_ptr<PdfOutlineItem, void(*)(PdfOutlineItem*)> first(nullptr, deleteItems);
    auto& objects = GetDocument().GetObjects();
    auto childRef = GetDictionary().GetKey("First");
    while (childRef != nullptr)
    {
        auto child = new PdfOutlineItem(objects.MustGetObject(childRef->GetReference()), this, m_Last, visited);
        if (m_Last == nullptr)
            first.reset(child);
        else
            m_Last->m_Next = child;

        m_Last = child;
        childRef = child->GetDictionary().GetKey("Next");
    }

    m_First = first.release();
}

PdfOutlineItem::PdfOutlineItem(PdfDocument& doc, PdfOutlineItem* parentOutline)
    : PdfDictionaryElement(doc, "Outlines"_n), m_ParentOutline(parentOutline), m_Prev(nullptr),
    m_Next(nullptr), m_First(nullptr), m_Last(nullptr), m_Destination(nullptr), m_Action(nullptr)
{
    if (parentOutline != nullptr)
        GetDictionary().AddKey("Parent"_n, parentOutline->GetObject().GetIndirectReference());
}

PdfOutlineItem::~PdfOutlineItem()
{
    deleteItems(m_First);
}

PdfOutlineItem& PdfOutlineItem::CreateChild(const PdfString& title)
{
    unique_ptr<PdfOutlineItem> item(new PdfOutlineItem(GetDocument()));
    item->SetTitle(title);
    this->insertChildInternal(item.get(), false);
    return *item.release();
}

void PdfOutlineItem::InsertChild(unique_ptr<PdfOutlineItem> item)
{
    this->insertChildInternal(item.get(), true);
    (void)item.release();
}

/// @param checkSameTree Check if the given item is in the same tree as this item
void PdfOutlineItem::insertChildInternal(PdfOutlineItem* item, bool checkSameTree)
{
    if (item == nullptr)
        return;

    if (checkSameTree)
    {
        auto itemRoot = item;
        while (itemRoot->GetParentOutline() != nullptr)
            itemRoot = itemRoot->GetParentOutline();

        auto thisRoot = this;
        while (thisRoot->GetParentOutline() != nullptr)
            thisRoot = thisRoot->GetParentOutline();

        if (itemRoot == thisRoot) // item is in the same tree as this
            PODOFO_RAISE_ERROR(PdfErrorCode::ItemAlreadyPresent);
    }

    // The item belongs to this outline from now on
    item->m_ParentOutline = this;
    item->GetDictionary().AddKey("Parent"_n, GetObject().GetIndirectReference());

    if (m_Last != nullptr)
    {
        m_Last->setNext(item);
        item->setPrevious(m_Last);
    }

    m_Last = item;

    if (m_First == nullptr)
        m_First = m_Last;

    GetDictionary().AddKey("First"_n, m_First->GetObject().GetIndirectReference());
    GetDictionary().AddKey("Last"_n, m_Last->GetObject().GetIndirectReference());
}

PdfOutlineItem& PdfOutlineItem::CreateNext(const PdfString& title)
{
    // The siblings are owned by the parent, which the root doesn't have
    if (m_ParentOutline == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedOperation, "The outlines root can't have siblings");

    unique_ptr<PdfOutlineItem> item(new PdfOutlineItem(GetDocument(), m_ParentOutline));
    item->SetTitle(title);

    if (m_Next != nullptr)
    {
        m_Next->setPrevious(item.get());
        item->setNext(m_Next);
    }

    m_Next = item.get();
    item->setPrevious(this);

    GetDictionary().AddKey("Next"_n, item->GetObject().GetIndirectReference());

    if (item->Next() == nullptr)
        m_ParentOutline->setLast(item.get());

    return *item.release();
}

void PdfOutlineItem::setPrevious(PdfOutlineItem* item)
{
    m_Prev = item;
    if (m_Prev == nullptr)
        GetDictionary().RemoveKey("Prev");
    else
        GetDictionary().AddKey("Prev"_n, m_Prev->GetObject().GetIndirectReference());
}

void PdfOutlineItem::setNext(PdfOutlineItem* item)
{
    m_Next = item;
    if (m_Next == nullptr)
        GetDictionary().RemoveKey("Next");
    else
        GetDictionary().AddKey("Next"_n, m_Next->GetObject().GetIndirectReference());
}

void PdfOutlineItem::setLast(PdfOutlineItem* item)
{
    m_Last = item;
    if (m_Last == nullptr)
        GetDictionary().RemoveKey("Last");

    else
        GetDictionary().AddKey("Last"_n, m_Last->GetObject().GetIndirectReference());
}

void PdfOutlineItem::setFirst(PdfOutlineItem* item)
{
    m_First = item;
    if (m_First == nullptr)
        GetDictionary().RemoveKey("First");
    else
        GetDictionary().AddKey("First"_n, m_First->GetObject().GetIndirectReference());
}

void PdfOutlineItem::Erase()
{
    // The root is owned by the document and can't erase itself
    if (m_ParentOutline == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedOperation, "The outlines root can't be erased");

    while (m_First != nullptr)
    {
        // Erase will set a new first
        // if it has a next item
        m_First->Erase();
    }

    // Unlink from siblings
    if (m_Prev != nullptr)
        m_Prev->setNext(m_Next);

    if (m_Next != nullptr)
        m_Next->setPrevious(m_Prev);

    // Unlink from parent
    if (m_Prev == nullptr && m_ParentOutline != nullptr && m_ParentOutline->First() == this)
        m_ParentOutline->setFirst(m_Next);

    if (m_Next == nullptr && m_ParentOutline != nullptr && m_ParentOutline->Last() == this)
        m_ParentOutline->setLast(m_Prev);

    delete this;
}

void PdfOutlineItem::SetDestination(nullable<const PdfDestination&> destination)
{
    auto& dict = GetDictionary();
    if (destination == nullptr)
    {
        dict.RemoveKey("Dest");
        m_Destination *= nullptr;
    }
    else
    {
        // NOTE: The dictionary is updated first, as destination may be
        // the cached instance. The cache is then rebuilt from the stored
        // /Dest on access, as the caller's object is not the stored one
        destination->AddToDictionary(dict);
        dict.RemoveKey("A");
        m_Destination = { };
        m_Action *= nullptr;
    }
}

nullable<const PdfDestination&> PdfOutlineItem::GetDestination() const
{
    return const_cast<PdfOutlineItem&>(*this).getDestination();
}

nullable<PdfDestination&> PdfOutlineItem::GetDestination()
{
    return getDestination();
}

nullable<PdfDestination&> PdfOutlineItem::getDestination()
{
    // NOTE: /Dest is resolved on every access: a name tree insertion can relocate
    // the destination array, and a setter writing an aliased /Dests entry can retype it
    PdfObject* destObj;
    auto obj = GetDictionary().FindKey("Dest");
    if (obj == nullptr || (destObj = PdfDestination::FindDestinationObject(GetDocument(), *obj)) == nullptr)
    {
        m_Destination *= nullptr;
    }
    else if (!m_Destination.has_value() || *m_Destination == nullptr
        || &(*m_Destination)->GetObject() != destObj)
    {
        m_Destination = unique_ptr<PdfDestination>(new PdfDestination(*destObj));
    }

    // NOTE: The nullable has a value also when no destination was found
    if (*m_Destination == nullptr)
        return { };
    else
        return **m_Destination;
}

void PdfOutlineItem::SetAction(nullable<const PdfAction&> action)
{
    auto& dict = GetDictionary();
    if (action == nullptr)
    {
        dict.RemoveKey("A");
        m_Action *= nullptr;
    }
    else
    {
        // NOTE: The dictionary is updated first, as action may be the
        // cached instance replaced by the assignment below, and the
        // caches must not change when the insertion fails
        dict.AddKeyIndirect("A"_n, action->GetObject());
        dict.RemoveKey("Dest");
        m_Action = PdfAction::Create(*action);
        m_Destination *= nullptr;
    }
}

nullable<const PdfAction&> PdfOutlineItem::GetAction() const
{
    return const_cast<PdfOutlineItem&>(*this).getAction();
}

nullable<PdfAction&> PdfOutlineItem::GetAction()
{
    return getAction();
}

nullable<PdfAction&> PdfOutlineItem::getAction()
{
    if (m_Action == nullptr)
    {
        auto obj = GetDictionary().FindKey("A");
        if (obj == nullptr)
        {
            m_Action *= nullptr;
        }
        else
        {
            unique_ptr<PdfAction> action;
            if (PdfAction::TryCreateFromObject(*obj, action))
                m_Action = std::move(action);
        }
    }

    // NOTE: The nullable has a value also when no action was found
    if (*m_Action == nullptr)
        return { };
    else
        return **m_Action;
}

void PdfOutlineItem::SetTitle(const PdfString& title)
{
    GetDictionary().AddKey("Title"_n, title);
}

const PdfString& PdfOutlineItem::GetTitle() const
{
    return this->GetDictionary().MustFindKey("Title").GetString();
}

void PdfOutlineItem::SetTextFormat(PdfOutlineFormat format)
{
    GetDictionary().AddKey("F"_n, static_cast<int64_t>(format));
}

PdfOutlineFormat PdfOutlineItem::GetTextFormat() const
{
    auto fObj = GetDictionary().FindKey("F");
    if (fObj == nullptr)
        return PdfOutlineFormat::Default;

    return static_cast<PdfOutlineFormat>(fObj->GetNumber());
}

void PdfOutlineItem::SetTextColor(const PdfColor& color)
{
    auto rgb = color.ConvertToRGB();
    GetDictionary().AddKey("C"_n, rgb.ToArray());
}


PdfColor PdfOutlineItem::GetTextColor() const
{
    PdfColor color;
    auto colorObj = GetDictionary().FindKey("C");
    if (colorObj == nullptr
        || !PdfColor::TryCreateFromObject(*colorObj, color))
    {
        return PdfColor(0, 0, 0);
    }

    return color;
}

PdfOutlines::PdfOutlines(PdfDocument& doc)
    : PdfOutlineItem(doc)
{
}

PdfOutlines::PdfOutlines(PdfObject& obj)
    : PdfOutlineItem(obj, nullptr, nullptr)
{
}

PdfOutlineItem& PdfOutlines::CreateRoot(const PdfString& title)
{
    return this->CreateChild(title);
}

// Delete a chain of sibling items in a loop, as each item owns only its children
void deleteItems(PdfOutlineItem* item)
{
    while (item != nullptr)
    {
        auto next = item->Next();
        delete item;
        item = next;
    }
}

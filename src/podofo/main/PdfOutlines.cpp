/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

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

PdfOutlineItem::PdfOutlineItem(PdfObject& obj, PdfOutlineItem* parentOutline, PdfOutlineItem* previous)
    : PdfDictionaryElement(obj), m_ParentOutline(parentOutline), m_Prev(previous),
    m_Next(nullptr), m_First(nullptr), m_Last(nullptr), m_Destination(nullptr), m_Action(nullptr)
{
    utls::RecursionGuard guard;
    PdfReference first, next;

    if (GetDictionary().HasKey("First"))
    {
        first = GetDictionary().GetKey("First")->GetReference();
        m_First = new PdfOutlineItem(obj.GetDocument()->GetObjects().MustGetObject(first), this, nullptr);
    }

    if (GetDictionary().HasKey("Next"))
    {
        next = GetDictionary().GetKey("Next")->GetReference();
        m_Next = new PdfOutlineItem(obj.GetDocument()->GetObjects().MustGetObject(next), parentOutline, this);
    }
}

PdfOutlineItem::PdfOutlineItem(PdfDocument& doc, PdfOutlineItem* parentOutline)
    : PdfDictionaryElement(doc, "Outlines"_n), m_ParentOutline(nullptr), m_Prev(nullptr),
    m_Next(nullptr), m_First(nullptr), m_Last(nullptr), m_Destination(nullptr), m_Action(nullptr)
{
    if (parentOutline != nullptr)
        GetDictionary().AddKey("Parent"_n, parentOutline->GetObject().GetIndirectReference());
}

PdfOutlineItem::~PdfOutlineItem()
{
    delete m_Next;
    delete m_First;
    m_Next = nullptr;
    m_First = nullptr;
}

PdfOutlineItem& PdfOutlineItem::CreateChild(const PdfString& title)
{
    unique_ptr< PdfOutlineItem> item(new PdfOutlineItem(GetDocument(), this));
    item->SetTitle(title);
    this->insertChildInternal(item.get(), false);
    return *item.release();
}

void PdfOutlineItem::InsertChild(unique_ptr<PdfOutlineItem> item)
{
    this->insertChildInternal(item.get(), true);
    (void)item.release();
}

void PdfOutlineItem::insertChildInternal(PdfOutlineItem* item, bool checkParent)
{
    PdfOutlineItem* itemToCheckParent = item;
    PdfOutlineItem* root = nullptr;
    PdfOutlineItem* rootOfThis = nullptr;

    if (itemToCheckParent == nullptr)
        return;

    if (checkParent)
    {
        while (itemToCheckParent != nullptr)
        {
            while (itemToCheckParent->GetParentOutline())
                itemToCheckParent = itemToCheckParent->GetParentOutline();

            if (itemToCheckParent == item) // item can't have a parent
            {
                root = item; // needed later, "root" can mean "standalone" here
                break;       // for performance in standalone or doc-merge case
            }

            if (root == nullptr)
            {
                rootOfThis = itemToCheckParent;
                itemToCheckParent = nullptr;
            }
            else
            {
                root = itemToCheckParent;
                itemToCheckParent = this;
            }
        }

        if (root == rootOfThis) // later nullptr if check skipped for performance
            PODOFO_RAISE_ERROR(PdfErrorCode::ItemAlreadyPresent);
    }

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

    if (m_ParentOutline != nullptr && item->Next() == nullptr)
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
    while (m_First != nullptr)
    {
        // erase will set a new first
        // if it has a next item
        m_First->Erase();
    }

    if (m_Prev != nullptr)
    {
        m_Prev->setNext(m_Next);
    }

    if (m_Next != nullptr)
    {
        m_Next->setPrevious(m_Prev);
    }

    if (m_Prev == nullptr && m_ParentOutline != nullptr && this == m_ParentOutline->First())
        m_ParentOutline->setFirst(m_Next);

    if (m_Next == nullptr && m_ParentOutline != nullptr && this == m_ParentOutline->Last())
        m_ParentOutline->setLast(m_Prev);

    m_Next = nullptr;
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
        m_Destination = unique_ptr<PdfDestination>(new PdfDestination(*destination));
        m_Action *= nullptr;
        destination->AddToDictionary(dict);
        dict.RemoveKey("A");
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
    if (m_Destination == nullptr)
    {
        auto obj = GetDictionary().FindKey("Dest");
        unique_ptr<PdfDestination> dest;
        if (obj == nullptr || !PdfDestination::TryCreateFromObject(*obj, dest))
            m_Destination *= nullptr;
        else
            m_Destination = std::move(dest);
    }

    if (m_Destination == nullptr)
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
        m_Action = PdfAction::Create(*action);
        m_Destination *= nullptr;
        dict.RemoveKey("Dest");
        dict.AddKeyIndirect("A"_n, action->GetObject());
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

    if (m_Action == nullptr)
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
    auto& ret = this->CreateChild(title);
    ret.SetDestination(*GetDocument().CreateDestination());
    return ret;
}

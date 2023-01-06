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

PdfOutlineItem::PdfOutlineItem(PdfDocument& doc, const PdfString& title, const shared_ptr<PdfDestination>& dest,
    PdfOutlineItem* parentOutline) :
    PdfDictionaryElement(doc),
    m_ParentOutline(parentOutline), m_Prev(nullptr), m_Next(nullptr),
    m_First(nullptr), m_Last(nullptr), m_destination(nullptr), m_action(nullptr)
{
    if (parentOutline != nullptr)
        this->GetObject().GetDictionary().AddKey("Parent", parentOutline->GetObject().GetIndirectReference());

    this->SetTitle(title);
    this->SetDestination(dest);
}

PdfOutlineItem::PdfOutlineItem(PdfDocument& doc, const PdfString& title, const shared_ptr<PdfAction>& action,
    PdfOutlineItem* parentOutline) :
    PdfDictionaryElement(doc),
    m_ParentOutline(parentOutline), m_Prev(nullptr), m_Next(nullptr),
    m_First(nullptr), m_Last(nullptr), m_destination(nullptr), m_action(nullptr)
{
    if (parentOutline != nullptr)
        this->GetObject().GetDictionary().AddKey("Parent", parentOutline->GetObject().GetIndirectReference());

    this->SetTitle(title);
    this->SetAction(action);
}

PdfOutlineItem::PdfOutlineItem(PdfObject& obj, PdfOutlineItem* parentOutline, PdfOutlineItem* previous)
    : PdfDictionaryElement(obj), m_ParentOutline(parentOutline), m_Prev(previous),
    m_Next(nullptr), m_First(nullptr), m_Last(nullptr), m_destination(nullptr), m_action(nullptr)
{
    utls::RecursionGuard guard;
    PdfReference first, next;

    if (this->GetObject().GetDictionary().HasKey("First"))
    {
        first = this->GetObject().GetDictionary().GetKey("First")->GetReference();
        m_First = new PdfOutlineItem(obj.GetDocument()->GetObjects().MustGetObject(first), this, nullptr);
    }

    if (this->GetObject().GetDictionary().HasKey("Next"))
    {
        next = this->GetObject().GetDictionary().GetKey("Next")->GetReference();
        m_Next = new PdfOutlineItem(obj.GetDocument()->GetObjects().MustGetObject(next), parentOutline, this);
    }
}

PdfOutlineItem::PdfOutlineItem(PdfDocument& doc)
    : PdfDictionaryElement(doc, "Outlines"), m_ParentOutline(nullptr), m_Prev(nullptr),
    m_Next(nullptr), m_First(nullptr), m_Last(nullptr), m_destination(nullptr), m_action(nullptr)
{
}

PdfOutlineItem::~PdfOutlineItem()
{
    delete m_Next;
    delete m_First;
}

PdfOutlineItem* PdfOutlineItem::CreateChild(const PdfString& title, const shared_ptr<PdfDestination>& dest)
{
    PdfOutlineItem* item = new PdfOutlineItem(*this->GetObject().GetDocument(), title, dest, this);
    this->InsertChildInternal(item, false);
    return item;
}

void PdfOutlineItem::InsertChild(PdfOutlineItem* item)
{
    this->InsertChildInternal(item, true);
}

void PdfOutlineItem::InsertChildInternal(PdfOutlineItem* item, bool checkParent)
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
                break;         // for performance in standalone or doc-merge case
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
            PODOFO_RAISE_ERROR(PdfErrorCode::OutlineItemAlreadyPresent);
    }

    if (m_Last != nullptr)
    {
        m_Last->SetNext(item);
        item->SetPrevious(m_Last);
    }

    m_Last = item;

    if (m_First == nullptr)
        m_First = m_Last;

    this->GetObject().GetDictionary().AddKey("First", m_First->GetObject().GetIndirectReference());
    this->GetObject().GetDictionary().AddKey("Last", m_Last->GetObject().GetIndirectReference());
}

PdfOutlineItem* PdfOutlineItem::CreateNext(const PdfString& title, const shared_ptr<PdfDestination>& dest)
{
    PdfOutlineItem* item = new PdfOutlineItem(*this->GetObject().GetDocument(), title, dest, m_ParentOutline);

    if (m_Next != nullptr)
    {
        m_Next->SetPrevious(item);
        item->SetNext(m_Next);
    }

    m_Next = item;
    m_Next->SetPrevious(this);

    this->GetObject().GetDictionary().AddKey("Next", m_Next->GetObject().GetIndirectReference());

    if (m_ParentOutline != nullptr && m_Next->Next() == nullptr)
        m_ParentOutline->SetLast(m_Next);

    return m_Next;
}

PdfOutlineItem* PdfOutlineItem::CreateNext(const PdfString& title, const shared_ptr<PdfAction>& action)
{
    PdfOutlineItem* item = new PdfOutlineItem(*this->GetObject().GetDocument(), title, action, m_ParentOutline);

    if (m_Next != nullptr)
    {
        m_Next->SetPrevious(item);
        item->SetNext(m_Next);
    }

    m_Next = item;
    m_Next->SetPrevious(this);

    this->GetObject().GetDictionary().AddKey("Next", m_Next->GetObject().GetIndirectReference());

    if (m_ParentOutline != nullptr && m_Next->Next() == nullptr)
        m_ParentOutline->SetLast(m_Next);

    return m_Next;
}

void PdfOutlineItem::SetPrevious(PdfOutlineItem* item)
{
    m_Prev = item;
    if (m_Prev == nullptr)
        this->GetObject().GetDictionary().RemoveKey("Prev");
    else
        this->GetObject().GetDictionary().AddKey("Prev", m_Prev->GetObject().GetIndirectReference());
}

void PdfOutlineItem::SetNext(PdfOutlineItem* item)
{
    m_Next = item;
    if (m_Next == nullptr)
        this->GetObject().GetDictionary().RemoveKey("Next");
    else
        this->GetObject().GetDictionary().AddKey("Next", m_Next->GetObject().GetIndirectReference());
}

void PdfOutlineItem::SetLast(PdfOutlineItem* item)
{
    m_Last = item;
    if (m_Last == nullptr)
        this->GetObject().GetDictionary().RemoveKey("Last");

    else
        this->GetObject().GetDictionary().AddKey("Last", m_Last->GetObject().GetIndirectReference());
}

void PdfOutlineItem::SetFirst(PdfOutlineItem* item)
{
    m_First = item;
    if (m_First == nullptr)
        this->GetObject().GetDictionary().RemoveKey("First");
    else
        this->GetObject().GetDictionary().AddKey("First", m_First->GetObject().GetIndirectReference());
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
        m_Prev->SetNext(m_Next);
    }

    if (m_Next != nullptr)
    {
        m_Next->SetPrevious(m_Prev);
    }

    if (m_Prev == nullptr && m_ParentOutline != nullptr && this == m_ParentOutline->First())
        m_ParentOutline->SetFirst(m_Next);

    if (m_Next == nullptr && m_ParentOutline != nullptr && this == m_ParentOutline->Last())
        m_ParentOutline->SetLast(m_Prev);

    m_Next = nullptr;
    delete this;
}

void PdfOutlineItem::SetDestination(const shared_ptr<PdfDestination>& dest)
{
    dest->AddToDictionary(this->GetObject().GetDictionary());
    m_destination = dest;
}

shared_ptr<PdfDestination> PdfOutlineItem::GetDestination() const
{
    return const_cast<PdfOutlineItem&>(*this).getDestination();
}

shared_ptr<PdfDestination> PdfOutlineItem::getDestination()
{
    if (m_destination == nullptr)
    {
        PdfObject* obj = this->GetObject().GetDictionary().FindKey("Dest");
        if (obj == nullptr)
            return nullptr;

        m_destination = PdfDestination::Create(*obj);
    }

    return m_destination;
}

void PdfOutlineItem::SetAction(const shared_ptr<PdfAction>& action)
{
    action->AddToDictionary(this->GetObject().GetDictionary());
    m_action = action;
}

shared_ptr<PdfAction> PdfOutlineItem::GetAction() const
{
    return const_cast<PdfOutlineItem&>(*this).getAction();
}

shared_ptr<PdfAction> PdfOutlineItem::getAction()
{
    if (m_action == nullptr)
    {
        PdfObject* obj = this->GetObject().GetDictionary().FindKey("A");
        if (obj == nullptr)
            return nullptr;

        m_action.reset(new PdfAction(*obj));
    }

    return m_action;
}

void PdfOutlineItem::SetTitle(const PdfString& title)
{
    this->GetObject().GetDictionary().AddKey("Title", title);
}

const PdfString& PdfOutlineItem::GetTitle() const
{
    return this->GetObject().GetDictionary().MustFindKey("Title").GetString();
}

void PdfOutlineItem::SetTextFormat(PdfOutlineFormat eFormat)
{
    this->GetObject().GetDictionary().AddKey("F", static_cast<int64_t>(eFormat));
}

PdfOutlineFormat PdfOutlineItem::GetTextFormat() const
{
    if (this->GetObject().GetDictionary().HasKey("F"))
        return static_cast<PdfOutlineFormat>(this->GetObject().GetDictionary().MustFindKey("F").GetNumber());

    return PdfOutlineFormat::Default;
}

void PdfOutlineItem::SetTextColor(double r, double g, double b)
{
    PdfArray color;
    color.Add(r);
    color.Add(g);
    color.Add(b);

    this->GetObject().GetDictionary().AddKey("C", color);
}


double PdfOutlineItem::GetTextColorRed() const
{
    if (this->GetObject().GetDictionary().HasKey("C"))
        return this->GetObject().GetDictionary().MustFindKey("C").GetArray()[0].GetReal();

    return 0.0;
}

double PdfOutlineItem::GetTextColorGreen() const
{
    if (this->GetObject().GetDictionary().HasKey("C"))
        return this->GetObject().GetDictionary().MustFindKey("C").GetArray()[1].GetReal();

    return 0.0;
}

double PdfOutlineItem::GetTextColorBlue() const
{
    if (this->GetObject().GetDictionary().HasKey("C"))
        return this->GetObject().GetDictionary().MustFindKey("C").GetArray()[2].GetReal();

    return 0.0;
}

PdfOutlines::PdfOutlines(PdfDocument& doc)
    : PdfOutlineItem(doc)
{
}

PdfOutlines::PdfOutlines(PdfObject& obj)
    : PdfOutlineItem(obj, nullptr, nullptr)
{
}

PdfOutlineItem* PdfOutlines::CreateRoot(const PdfString& title)
{
    return this->CreateChild(title, std::make_shared<PdfDestination>(*GetObject().GetDocument()));
}

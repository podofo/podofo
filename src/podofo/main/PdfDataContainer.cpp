// SPDX-FileCopyrightText: 2006 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfDataContainer.h"

#include "PdfDocument.h"
#include "PdfObject.h"
#include "PdfIndirectObjectList.h"

using namespace PoDoFo;

PdfDataContainer::PdfDataContainer()
    : m_Owner(nullptr), m_Document(nullptr)
{
}

PdfDataContainer::~PdfDataContainer() { }

void PdfDataContainer::SetOwner(PdfObject& owner, PdfDocument* document)
{
    m_Owner = &owner;
    m_Document = document;
    setChildrenParent();
}

void PdfDataContainer::ResetDirty()
{
    resetDirty();
}

PdfObject* PdfDataContainer::GetIndirectObject(const PdfReference& ref) const
{
    if (m_Owner == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Object is a reference but does not have an owner");

    if (m_Document == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Object owner is not part of any document");

    return m_Document->GetObjects().GetObject(ref);
}

void PdfDataContainer::SetDirty()
{
    if (m_Owner != nullptr)
        m_Owner->SetDirty();
}

bool PdfDataContainer::IsIndirectReferenceAllowed(const PdfObject& obj)
{
    PdfDocument* objDocument;
    if (obj.IsIndirect()
        && (objDocument = obj.GetDocument()) != nullptr
        && m_Owner != nullptr
        && objDocument == m_Document)
    {
        return true;
    }

    return false;
}

void PdfDataContainer::AssertMutable() const
{
    if (m_Owner != nullptr && m_Owner->IsImmutable())
        PODOFO_RAISE_ERROR(PdfErrorCode::ChangeOnImmutable);
}


PdfIndirectIterableBase::PdfIndirectIterableBase()
    : m_Objects(nullptr) { }

PdfIndirectIterableBase::PdfIndirectIterableBase(PdfDataContainer& container)
{
    auto owner = container.GetOwner();
    if (owner == nullptr)
    {
        m_Objects = nullptr;
    }
    else
    {
        auto document = owner->GetDocument();
        if (document == nullptr)
            m_Objects = nullptr;
        else
            m_Objects = &document->GetObjects();
    }
}

PdfObject* PdfIndirectIterableBase::GetObject(const PdfIndirectObjectList& list, const PdfReference& ref)
{
    return list.GetObject(ref);
}

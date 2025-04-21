/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFieldChildrenCollection.h"
#include "PdfField.h"
#include "PdfArray.h"
#include "PdfDictionary.h"

using namespace std;
using namespace PoDoFo;

PdfFieldChildrenCollectionBase::PdfFieldChildrenCollectionBase(PdfField& field)
    : m_field(&field), m_kidsArray(nullptr)
{
    initFields();
}

PdfField& PdfFieldChildrenCollectionBase::CreateChild()
{
    return AddChild(m_field->CreateChild());
}

PdfField& PdfFieldChildrenCollectionBase::CreateChild(PdfPage& page, const Rect& rect)
{
    return AddChild(m_field->CreateChild(page, rect));
}

PdfField& PdfFieldChildrenCollectionBase::GetFieldAt(unsigned index)
{
    return getFieldAt(index);
}

const PdfField& PdfFieldChildrenCollectionBase::GetFieldAt(unsigned index) const
{
    return getFieldAt(index);
}

PdfField& PdfFieldChildrenCollectionBase::GetField(const PdfReference& ref)
{
    return getField(ref);
}

const PdfField& PdfFieldChildrenCollectionBase::GetField(const PdfReference& ref) const
{
    return getField(ref);
}

void PdfFieldChildrenCollectionBase::RemoveFieldAt(unsigned index)
{
    if (index >= m_Fields.size())
        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

    if (m_Fields[index] != nullptr)
    {
        // It may be null if the annotation is invalid
        m_fieldMap.erase(m_fieldMap.find(m_Fields[index]->GetObject().GetIndirectReference()));
    }

    m_kidsArray->RemoveAt(index);
    m_Fields.erase(m_Fields.begin() + index);
    fixIndices(index);

    // NOTE: No need to remove the object from the document
    // indirect object list: it will be garbage collected
}

void PdfFieldChildrenCollectionBase::RemoveField(const PdfReference& ref)
{
    auto found = m_fieldMap.find(ref);
    if (found == m_fieldMap.end())
        return;

    unsigned index = found->second;
    m_Fields.erase(m_Fields.begin() + index);
    m_kidsArray->RemoveAt(index);
    m_fieldMap.erase(found);
    fixIndices(index);

    // NOTE: No need to remove the object from the document
    // indirect object list: it will be garbage collected
}

unsigned PdfFieldChildrenCollectionBase::GetCount() const
{
    return (unsigned)m_Fields.size();
}

bool PdfFieldChildrenCollectionBase::HasKidsArray() const
{
    return m_kidsArray != nullptr;
}

PdfFieldChildrenCollectionBase::iterator PdfFieldChildrenCollectionBase::begin()
{
    return m_Fields.begin();
}

PdfFieldChildrenCollectionBase::iterator PdfFieldChildrenCollectionBase::end()
{
    return m_Fields.end();
}

PdfFieldChildrenCollectionBase::const_iterator PdfFieldChildrenCollectionBase::begin() const
{
    return m_Fields.begin();
}

PdfFieldChildrenCollectionBase::const_iterator PdfFieldChildrenCollectionBase::end() const
{
    return m_Fields.end();
}

PdfField& PdfFieldChildrenCollectionBase::AddChild(shared_ptr<PdfField> field)
{
    PODOFO_ASSERT(field != nullptr);
    if (m_kidsArray == nullptr)
        m_kidsArray = &m_field->GetDictionary().AddKey("Kids"_n, PdfArray()).GetArray();

    m_fieldMap[field->GetObject().GetIndirectReference()] = m_kidsArray->GetSize();
    m_kidsArray->AddIndirectSafe(field->GetObject());
    auto ret = field.get();
    m_Fields.push_back(std::move(field));
    return *ret;
}

PdfArray* PdfFieldChildrenCollectionBase::getKidsArray() const
{
    auto obj = const_cast<PdfFieldChildrenCollectionBase&>(*this).m_field->GetDictionary().FindKey("Kids");
    if (obj == nullptr)
        return nullptr;

    return &obj->GetArray();
}

PdfField& PdfFieldChildrenCollectionBase::getFieldAt(unsigned index) const
{
    if (index >= m_Fields.size())
        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

    return *m_Fields[index];
}

PdfField& PdfFieldChildrenCollectionBase::getField(const PdfReference& ref) const
{
    return *m_Fields[m_fieldMap.at(ref)];
}

void PdfFieldChildrenCollectionBase::initFields()
{
    m_kidsArray = getKidsArray();
    if (m_kidsArray == nullptr)
        return;

    m_Fields.reserve(m_kidsArray->size());
    unique_ptr<PdfField> field;
    unsigned i = 0;
    for (auto obj : m_kidsArray->GetIndirectIterator())
    {
        m_fieldMap[obj->GetIndirectReference()] = i;
        // The annotation may be invalid. In that case we push a placeholder
        if (PdfField::TryCreateFromObject(*obj, field))
        {
            field->SetParent(m_field->GetPtr());
            m_Fields.push_back(std::move(field));
        }
        else
        {
            m_Fields.push_back(nullptr);
        }

        i++;
    }
}

void PdfFieldChildrenCollectionBase::fixIndices(unsigned index)
{
    for (auto& pair : m_fieldMap)
    {
        // Decrement indices where needed
        if (pair.second > index)
            pair.second--;
    }
}

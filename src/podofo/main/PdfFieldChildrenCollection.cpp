// SPDX-FileCopyrightText: 2022 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

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

    inlineKidsArray();
    promoteDirectEntries();
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
    PODOFO_ASSERT(index < m_Fields.size());
    inlineKidsArray();
    promoteDirectEntries();
    m_kidsArray->RemoveAt(index);
    m_Fields.erase(m_Fields.begin() + index);
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
    return iterator(m_Fields.begin(), m_Fields.end());
}

PdfFieldChildrenCollectionBase::iterator PdfFieldChildrenCollectionBase::end()
{
    return iterator(m_Fields.end(), m_Fields.end());
}

PdfFieldChildrenCollectionBase::const_iterator PdfFieldChildrenCollectionBase::begin() const
{
    return const_iterator(m_Fields.begin(), m_Fields.end());
}

PdfFieldChildrenCollectionBase::const_iterator PdfFieldChildrenCollectionBase::end() const
{
    return const_iterator(m_Fields.end(), m_Fields.end());
}

PdfField& PdfFieldChildrenCollectionBase::AddChild(shared_ptr<PdfField> field)
{
    PODOFO_ASSERT(field != nullptr);
    if (m_kidsArray == nullptr)
    {
        m_kidsArray = &m_field->GetDictionary().AddKey("Kids"_n, PdfArray()).GetArray();
    }
    else
    {
        inlineKidsArray();
        promoteDirectEntries();
    }

    m_fieldMap[field->GetObject().GetIndirectReference()] = (unsigned)m_Fields.size();
    m_kidsArray->AddIndirectSafe(field->GetObject());

    // A widget bound field is co-owned by its widget, which
    // unlinks itself from the field when destroyed
    auto widget = field->GetWidget();
    if (widget != nullptr && widget->GetFieldPtr() == nullptr)
        widget->SetField(shared_ptr<PdfField>(field));

    auto ret = field.get();
    m_Fields.push_back(std::move(field));
    return *ret;
}

shared_ptr<PdfField> PdfFieldChildrenCollectionBase::GetFieldPtr(const PdfReference& ref) const
{
    auto found = m_fieldMap.find(ref);
    if (found == m_fieldMap.end())
        return nullptr;

    unsigned index = found->second;
    PODOFO_ASSERT(index < m_Fields.size());
    return m_Fields[index];
}

PdfArray* PdfFieldChildrenCollectionBase::getKidsArray() const
{
    auto obj = const_cast<PdfFieldChildrenCollectionBase&>(*this).m_field->GetDictionary().FindKey("Kids");
    if (obj == nullptr)
        return nullptr;

    return &obj->GetArray();
}

// A /Kids array stored in its own object may be shared with other fields:
// it's copied inline in the field before a modification
void PdfFieldChildrenCollectionBase::inlineKidsArray()
{
    if (!m_kidsArray->GetOwner()->IsIndirect())
        return;

    m_kidsArray = &m_field->GetDictionary().AddKey("Kids"_n, PdfArray(*m_kidsArray)).GetArray();
}

PdfField& PdfFieldChildrenCollectionBase::getFieldAt(unsigned index) const
{
    if (index >= m_Fields.size())
        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

    auto ret = m_Fields[index].get();
    if (ret == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The field at index {} is invalid", index);

    return *ret;
}

PdfField& PdfFieldChildrenCollectionBase::getField(const PdfReference& ref) const
{
    auto found = m_fieldMap.find(ref);
    if (found == m_fieldMap.end())
        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

    unsigned index = found->second;
    PODOFO_ASSERT(index < m_Fields.size());
    auto ret = m_Fields[index].get();
    if (ret == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The field at index {} is invalid", index);

    return *ret;
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
        auto& ref = obj->GetIndirectReference();
        // NOTE: Direct fields all share the 0 0 R reference
        if (!m_fieldMap.try_emplace(ref, i).second && ref.IsIndirect())
        {
            // A repeated reference is wrapped only once: the duplicate is a placeholder
            m_Fields.push_back(nullptr);
        }
        else if (PdfField::TryCreateFromObject(*obj, field))
        {
            field->SetParent(m_field->GetPtr());
            m_Fields.push_back(std::move(field));
        }
        else
        {
            // The field may be invalid. In that case we push a placeholder
            m_Fields.push_back(nullptr);
        }

        i++;
    }
}

// Fields from direct dictionaries point into the array storage, which a
// modification can relocate or free: such entries are first moved to indirect
// objects, as the specification requires, and the elements are rebound to them
void PdfFieldChildrenCollectionBase::promoteDirectEntries()
{
    PODOFO_ASSERT(m_kidsArray->GetSize() == m_Fields.size());
    bool promoted = false;
    for (unsigned i = 0; i < m_kidsArray->GetSize(); i++)
    {
        auto& elem = (*m_kidsArray)[i];
        if (elem.IsDictionary())
        {
            // It's a field stored directly in the array
            auto& obj = PoDoFo::MoveToIndirect(elem);
            if (m_Fields[i] != nullptr)
                m_Fields[i]->SetObject(obj);

            promoted = true;
        }
    }

    if (!promoted)
        return;

    // Key the map again, as the promoted entries have their own references now
    m_fieldMap.clear();
    unsigned i = 0;
    for (auto obj : m_kidsArray->GetIndirectIterator())
    {
        (void)m_fieldMap.try_emplace(obj->GetIndirectReference(), i);
        i++;
    }
}

void PdfFieldChildrenCollectionBase::fixIndices(unsigned index)
{
    for (auto it = m_fieldMap.begin(); it != m_fieldMap.end(); )
    {
        if (it->second == index)
        {
            // The entry of the removed slot: it exists also for placeholders
            it = m_fieldMap.erase(it);
            continue;
        }

        // Decrement indices where needed
        if (it->second > index)
            it->second--;

        it++;
    }
}

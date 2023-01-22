/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfDictionary.h"

#include "PdfOutputDevice.h"

using namespace std;
using namespace PoDoFo;

PdfDictionary::PdfDictionary() { }

PdfDictionary::PdfDictionary(const PdfDictionary& rhs)
    : m_Map(rhs.m_Map)
{
    setChildrenParent();
}

PdfDictionary::PdfDictionary(PdfDictionary&& rhs) noexcept
    : m_Map(std::move(rhs.m_Map))
{
    setChildrenParent();
}

PdfDictionary& PdfDictionary::operator=(const PdfDictionary& rhs)
{
    m_Map = rhs.m_Map;
    setChildrenParent();
    return *this;
}

PdfDictionary& PdfDictionary::operator=(PdfDictionary&& rhs) noexcept
{
    m_Map = std::move(rhs.m_Map);
    setChildrenParent();
    return *this;
}

bool PdfDictionary::operator==(const PdfDictionary& rhs) const
{
    if (this == &rhs)
        return true;

    // We don't check owner
    return m_Map == rhs.m_Map;
}

bool PdfDictionary::operator!=(const PdfDictionary& rhs) const
{
    if (this != &rhs)
        return true;

    // We don't check owner
    return m_Map != rhs.m_Map;
}

void PdfDictionary::Clear()
{
    if (!m_Map.empty())
    {
        m_Map.clear();
        SetDirty();
    }
}

PdfObject& PdfDictionary::AddKey(const PdfName& key, const PdfObject& obj)
{
    return addKey(key, PdfObject(obj));
}

PdfObject& PdfDictionary::AddKey(const PdfName& key, PdfObject&& obj)
{
    return addKey(key, std::move(obj));
}

void PdfDictionary::AddKeyIndirect(const PdfName& key, const PdfObject& obj)
{
    if (IsIndirectReferenceAllowed(obj))
        (void)addKey(key, obj.GetIndirectReference());
    else
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Given object shall allow indirect insertion");
}

PdfObject& PdfDictionary::AddKeyIndirectSafe(const PdfName& key, const PdfObject& obj)
{
    if (IsIndirectReferenceAllowed(obj))
        return addKey(key, obj.GetIndirectReference());
    else
        return addKey(key, PdfObject(obj));
}

PdfObject& PdfDictionary::addKey(const PdfName& key, PdfObject&& obj)
{
    auto added = AddKey(key, std::move(obj), false);
    if (added.second)
        SetDirty();

    return added.first->second;
}

pair<PdfDictionaryMap::iterator, bool> PdfDictionary::AddKey(const PdfName& key, PdfObject&& obj, bool noDirtySet)
{
    // NOTE: Empty PdfNames are legal according to the PDF specification.
    // Don't check for it

    pair<PdfDictionaryMap::iterator, bool> inserted = m_Map.try_emplace(key, std::move(obj));
    if (!inserted.second)
    {
        if (noDirtySet)
            inserted.first->second.Assign(obj);
        else
            inserted.first->second = obj;
    }

    inserted.first->second.SetParent(*this);
    return inserted;
}

PdfObject* PdfDictionary::getKey(const string_view& key) const
{
    // NOTE: Empty PdfNames are legal according to the PDF,
    // specification don't check for it
    auto it = m_Map.find(key);
    if (it == m_Map.end())
        return nullptr;

    return &const_cast<PdfObject&>(it->second);
}

PdfObject* PdfDictionary::findKey(const string_view& key) const
{
    PdfObject* obj = getKey(key);
    if (obj == nullptr)
        return nullptr;

    if (obj->IsReference())
        return GetIndirectObject(obj->GetReference());
    else
        return obj;

    return nullptr;
}

PdfObject* PdfDictionary::findKeyParent(const string_view& key) const
{
    PdfObject* obj = findKey(key);
    if (obj == nullptr)
    {
        PdfObject* parent = findKey("Parent");
        if (parent == nullptr)
        {
            return nullptr;
        }
        else
        {
            if (parent->IsDictionary())
                return parent->GetDictionary().findKeyParent(key);
            else
                return nullptr;
        }
    }
    else
    {
        return obj;
    }
}

bool PdfDictionary::HasKey(const string_view& key) const
{
    // NOTE: Empty PdfNames are legal according to the PDF,
    // specification don't check for it
    return m_Map.find(key) != m_Map.end();
}

bool PdfDictionary::RemoveKey(const string_view& key)
{
    PdfDictionaryMap::iterator found = m_Map.find(key);
    if (found == m_Map.end())
        return false;

    m_Map.erase(found);
    SetDirty();
    return true;
}

void PdfDictionary::Write(OutputStreamDevice& device, PdfWriteFlags writeMode,
    const PdfStatefulEncrypt& encrypt, charbuff& buffer) const
{
    if ((writeMode & PdfWriteFlags::Clean) == PdfWriteFlags::Clean)
        device.Write("<<\n");
    else
        device.Write("<<");

    if (this->HasKey(PdfName::KeyType))
    {
        // Type has to be the first key in any dictionary
        if ((writeMode & PdfWriteFlags::Clean) == PdfWriteFlags::Clean)
            device.Write("/Type ");
        else
            device.Write("/Type");

        this->getKey(PdfName::KeyType)->GetVariant().Write(device, writeMode, encrypt, buffer);

        if ((writeMode & PdfWriteFlags::Clean) == PdfWriteFlags::Clean)
            device.Write('\n');
    }

    for (auto& pair : m_Map)
    {
        if (pair.first != PdfName::KeyType)
        {
            pair.first.Write(device, writeMode, encrypt, buffer);
            if ((writeMode & PdfWriteFlags::Clean) == PdfWriteFlags::Clean)
                device.Write(' '); // write a separator

            pair.second.GetVariant().Write(device, writeMode, encrypt, buffer);
            if ((writeMode & PdfWriteFlags::Clean) == PdfWriteFlags::Clean)
                device.Write('\n');
        }
    }

    device.Write(">>");
}

void PdfDictionary::ResetDirtyInternal()
{
    // Propagate state to all sub objects
    for (auto& pair : m_Map)
        pair.second.ResetDirty();
}

void PdfDictionary::setChildrenParent()
{
    // Set parent for all children
    for (auto& pair : m_Map)
        pair.second.SetParent(*this);
}

const PdfObject* PdfDictionary::GetKey(const string_view& key) const
{
    return getKey(key);
}

PdfObject* PdfDictionary::GetKey(const string_view& key)
{
    return getKey(key);
}

const PdfObject* PdfDictionary::FindKey(const string_view& key) const
{
    return findKey(key);
}

PdfObject* PdfDictionary::FindKey(const string_view& key)
{
    return findKey(key);
}

const PdfObject& PdfDictionary::MustFindKey(const string_view& key) const
{
    auto obj = findKey(key);
    if (obj == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::NoObject);

    return *obj;
}

PdfObject& PdfDictionary::MustFindKey(const string_view& key)
{
    auto obj = findKey(key);
    if (obj == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::NoObject);

    return *obj;
}

const PdfObject* PdfDictionary::FindKeyParent(const string_view& key) const
{
    return findKeyParent(key);
}

PdfObject* PdfDictionary::FindKeyParent(const string_view& key)
{
    return findKeyParent(key);
}

const PdfObject& PdfDictionary::MustFindKeyParent(const string_view& key) const
{
    auto obj = findKeyParent(key);
    if (obj == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::NoObject);

    return *obj;
}

PdfObject& PdfDictionary::MustFindKeyParent(const string_view& key)
{
    auto obj = findKeyParent(key);
    if (obj == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::NoObject);

    return *obj;
}

unsigned PdfDictionary::GetSize() const
{
    return (unsigned)m_Map.size();
}

PdfDictionaryIndirectIterable PdfDictionary::GetIndirectIterator()
{
    return PdfDictionaryIndirectIterable(*this);
}

PdfDictionaryConstIndirectIterable PdfDictionary::GetIndirectIterator() const
{
    return PdfDictionaryConstIndirectIterable(const_cast<PdfDictionary&>(*this));
}

const PdfObject& PdfDictionary::MustGetKey(const string_view& key) const
{
    auto obj = getKey(key);
    if (obj == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::NoObject);

    return *obj;
}

PdfObject& PdfDictionary::MustGetKey(const string_view& key)
{
    auto obj = getKey(key);
    if (obj == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::NoObject);

    return *obj;
}

PdfDictionary::iterator PdfDictionary::begin()
{
    return m_Map.begin();
}

PdfDictionary::iterator PdfDictionary::end()
{
    return m_Map.end();
}

PdfDictionary::const_iterator PdfDictionary::begin() const
{
    return m_Map.begin();
}

PdfDictionary::const_iterator PdfDictionary::end() const
{
    return m_Map.end();
}

size_t PdfDictionary::size() const
{
    return m_Map.size();
}

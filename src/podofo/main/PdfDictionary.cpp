/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfDictionary.h"

#include <podofo/auxiliary/OutputDevice.h>
#include <podofo/auxiliary/StreamDevice.h>

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
    rhs.SetDirty();
}

PdfDictionary& PdfDictionary::operator=(const PdfDictionary& rhs)
{
    AssertMutable();
    m_Map = rhs.m_Map;
    setChildrenParent();
    return *this;
}

PdfDictionary& PdfDictionary::operator=(PdfDictionary&& rhs) noexcept
{
    AssertMutable();
    m_Map = std::move(rhs.m_Map);
    setChildrenParent();
    rhs.SetDirty();
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
    AssertMutable();
    if (!m_Map.empty())
    {
        m_Map.clear();
        SetDirty();
    }
}

PdfObject& PdfDictionary::AddKey(const PdfName& key, const PdfObject& obj)
{
    AssertMutable();
    return addKey(key, PdfObject(obj));
}

PdfObject& PdfDictionary::AddKey(const PdfName& key, PdfObject&& obj)
{
    AssertMutable();
    auto& ret = addKey(key, std::move(obj));
    // NOTE: Manually make obj dirty, as "addKey" doesn't do it
    obj.SetDirty();
    return ret;
}

void PdfDictionary::AddKeyIndirect(const PdfName& key, const PdfObject& obj)
{
    AssertMutable();
    if (IsIndirectReferenceAllowed(obj))
        (void)addKey(key, obj.GetIndirectReference());
    else
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Given object shall allow indirect insertion");
}

PdfObject& PdfDictionary::AddKeyIndirectSafe(const PdfName& key, const PdfObject& obj)
{
    AssertMutable();
    if (IsIndirectReferenceAllowed(obj))
        return addKey(key, obj.GetIndirectReference());
    else
        return addKey(key, PdfObject(obj));
}

// Add key with the "obj" value.
// NOTE: It doesn't set dirty moved "obj:
PdfObject& PdfDictionary::addKey(const PdfName& key, PdfObject&& obj)
{
    // NOTE: Empty PdfNames are legal. Don't check for it
    pair<iterator, bool> inserted = m_Map.try_emplace(key, std::move(obj));
    if (inserted.second)
    {
        SetDirty();
    }
    else
    {
        // Manually setting dirty on the assigned object will
        // implicity make this container dirty, but won't make
        // dirty the moved "obj"
        inserted.first->second.AssignNoDirtySet(std::move(obj));
        inserted.first->second.SetDirty();
    }

    inserted.first->second.SetParent(*this);
    return inserted.first->second;
}

void PdfDictionary::AddKeyNoDirtySet(const PdfName& key, PdfVariant&& var)
{
    // NOTE: Empty PdfNames are legal. Don't check for it
    pair<iterator, bool> inserted = m_Map.try_emplace(key, std::move(var));
    if (!inserted.second)
        inserted.first->second.AssignNoDirtySet(std::move(var));

    inserted.first->second.SetParent(*this);
}

void PdfDictionary::AddKeyNoDirtySet(const PdfName& key, PdfObject&& obj)
{
    // NOTE: Empty PdfNames are legal. Don't check for it
    pair<iterator, bool> inserted = m_Map.try_emplace(key, std::move(obj));
    if (!inserted.second)
        inserted.first->second.AssignNoDirtySet(std::move(obj));

    inserted.first->second.SetParent(*this);
}

void PdfDictionary::RemoveKeyNoDirtySet(const string_view& key)
{
    auto found = m_Map.find(key);
    if (found == m_Map.end())
        return;

    m_Map.erase(found);
}

PdfObject& PdfDictionary::EmplaceNoDirtySet(const PdfName& key)
{
    return m_Map.emplace(key, nullptr).first->second;
}

PdfObject* PdfDictionary::getKey(const string_view& key) const
{
    // NOTE: Empty PdfNames are legal. Don't check for it
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

    PdfReference ref;
    if (obj->TryGetReference(ref))
        return GetIndirectObject(ref);
    else
        return obj;
}

PdfObject* PdfDictionary::findKeyParent(const string_view& key) const
{
    utls::RecursionGuard guard;
    auto obj = findKey(key);
    if (obj == nullptr)
    {
        auto parent = findKey("Parent");
        if (parent == nullptr || parent->GetIndirectReference() == GetOwner()->GetIndirectReference())
        {
            return nullptr;
        }
        else
        {
            PdfDictionary* parentDict;
            if (parent->TryGetDictionary(parentDict))
                return parentDict->findKeyParent(key);
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
    // NOTE: Empty PdfNames are legal. Don't check for it
    return m_Map.find(key) != m_Map.end();
}

bool PdfDictionary::RemoveKey(const string_view& key)
{
    AssertMutable();
    iterator found = m_Map.find(key);
    if (found == m_Map.end())
        return false;

    m_Map.erase(found);
    SetDirty();

    return true;
}

void PdfDictionary::Write(OutputStream& device, PdfWriteFlags writeMode,
    const PdfStatefulEncrypt* encrypt, charbuff& buffer) const
{
    bool addDelimiters = (writeMode & PdfWriteFlags::SkipDelimiters) == PdfWriteFlags::None;
    // It doesn't make sense to propagate SkipDelimiters flag
    writeMode &= ~PdfWriteFlags::SkipDelimiters;
    return write(device, writeMode, addDelimiters, encrypt, buffer);
}

void PdfDictionary::write(OutputStream& device, PdfWriteFlags writeMode, bool addDelimiters,
    const PdfStatefulEncrypt* encrypt, charbuff& buffer) const
{
    if (addDelimiters)
    {
        if ((writeMode & PdfWriteFlags::Clean) == PdfWriteFlags::Clean)
            device.Write("<<\n");
        else
            device.Write("<<");
    }

    if (this->HasKey("Type"))
    {
        // Type has to be the first key in any dictionary
        if ((writeMode & PdfWriteFlags::Clean) == PdfWriteFlags::Clean)
            device.Write("/Type ");
        else
            device.Write("/Type");

        this->getKey("Type")->GetVariant().Write(device, writeMode, encrypt, buffer);

        if ((writeMode & PdfWriteFlags::Clean) == PdfWriteFlags::Clean)
            device.Write('\n');
    }

    for (auto& pair : m_Map)
    {
        if (pair.first != "Type")
        {
            pair.first.Write(device, writeMode, encrypt, buffer);
            if ((writeMode & PdfWriteFlags::Clean) == PdfWriteFlags::Clean)
                device.Write(' '); // write a separator

            pair.second.GetVariant().Write(device, writeMode, encrypt, buffer);
            if ((writeMode & PdfWriteFlags::Clean) == PdfWriteFlags::Clean)
                device.Write('\n');
        }
    }

    if (addDelimiters)
        device.Write(">>");
}

void PdfDictionary::resetDirty()
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
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ObjectNotFound, "No object with key /{} found", key);

    return *obj;
}

PdfObject& PdfDictionary::MustFindKey(const string_view& key)
{
    auto obj = findKey(key);
    if (obj == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ObjectNotFound, "No object with key /{} found", key);

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
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ObjectNotFound, "No object with key /{} found", key);

    return *obj;
}

PdfObject& PdfDictionary::MustFindKeyParent(const string_view& key)
{
    auto obj = findKeyParent(key);
    if (obj == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ObjectNotFound, "No object with key /{} found", key);

    return *obj;
}

unsigned PdfDictionary::GetSize() const
{
    return (unsigned)m_Map.size();
}

PdfDictionaryIndirectIterable PdfDictionary::GetIndirectIterator()
{
    AssertMutable();
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
        PODOFO_RAISE_ERROR(PdfErrorCode::ObjectNotFound);

    return *obj;
}

PdfObject& PdfDictionary::MustGetKey(const string_view& key)
{
    auto obj = getKey(key);
    if (obj == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::ObjectNotFound);

    return *obj;
}

PdfDictionary::iterator PdfDictionary::begin()
{
    AssertMutable();
    return m_Map.begin();
}

PdfDictionary::iterator PdfDictionary::end()
{
    AssertMutable();
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

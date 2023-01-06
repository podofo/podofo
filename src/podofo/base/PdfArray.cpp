/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfArray.h"
#include "PdfOutputDevice.h"

using namespace std;
using namespace PoDoFo;

PdfArray::PdfArray() { }

PdfArray::PdfArray(const PdfArray& rhs)
    : m_Objects(rhs.m_Objects)
{
    setChildrenParent();
}

PdfArray::PdfArray(PdfArray&& rhs) noexcept
    : m_Objects(std::move(rhs.m_Objects))
{
    setChildrenParent();
}

void PdfArray::RemoveAt(unsigned idx)
{
    // TODO: Set dirty only if really removed
    if (idx >= m_Objects.size())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index is out of bounds");

    m_Objects.erase(m_Objects.begin() + idx);
    SetDirty();
}

const PdfObject* PdfArray::FindAt(unsigned idx) const
{
    return findAt(idx);
}

PdfObject* PdfArray::FindAt(unsigned idx)
{
    return findAt(idx);
}

const PdfObject& PdfArray::MustFindAt(unsigned idx) const
{
    auto obj = findAt(idx);
    if (obj == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::NoObject);

    return *obj;
}

PdfObject& PdfArray::MustFindAt(unsigned idx)
{
    auto obj = findAt(idx);
    if (obj == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::NoObject);

    return *obj;
}

PdfArray& PdfArray::operator=(const PdfArray& rhs)
{
    m_Objects = rhs.m_Objects;
    setChildrenParent();
    return *this;
}

PdfArray& PdfArray::operator=(PdfArray&& rhs) noexcept
{
    m_Objects = std::move(rhs.m_Objects);
    setChildrenParent();
    return *this;
}

unsigned PdfArray::GetSize() const
{
    return (unsigned)m_Objects.size();
}

bool PdfArray::IsEmpty() const
{
    return m_Objects.empty();
}

PdfObject& PdfArray::Add(const PdfObject& obj)
{
    auto& ret = add(PdfObject(obj));
    SetDirty();
    return ret;
}

PdfObject& PdfArray::Add(PdfObject&& obj)
{
    auto& ret = add(std::move(obj));
    SetDirty();
    return ret;
}

void PdfArray::AddIndirect(const PdfObject& obj)
{
    if (IsIndirectReferenceAllowed(obj))
        add(obj.GetIndirectReference());
    else
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Given object shall allow indirect insertion");

    SetDirty();
}

PdfObject& PdfArray::AddIndirectSafe(const PdfObject& obj)
{
    auto& ret = IsIndirectReferenceAllowed(obj)
        ? add(obj.GetIndirectReference())
        : add(PdfObject(obj));
    SetDirty();
    return ret;
}

PdfObject& PdfArray::SetAt(unsigned idx, const PdfObject& obj)
{
    if (idx >= m_Objects.size())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index is out of bounds");

    auto& ret = m_Objects[idx];
    ret = obj;
    // NOTE: No dirty set! The container itself is not modified
    return ret;
}

PdfObject& PdfArray::SetAt(unsigned idx, PdfObject&& obj)
{
    if (idx >= m_Objects.size())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index is out of bounds");

    auto& ret = m_Objects[idx];
    ret = std::move(obj);
    // NOTE: No dirty set! The container itself is not modified
    return ret;
}

void PdfArray::SetAtIndirect(unsigned idx, const PdfObject* obj)
{
    if (idx >= m_Objects.size())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index is out of bounds");

    if (IsIndirectReferenceAllowed(*obj))
        m_Objects[idx] = obj->GetIndirectReference();
    else
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Given object shall allow indirect insertion");

    // NOTE: No dirty set! The container itself is not modified
}

PdfObject& PdfArray::SetAtIndirectSafe(unsigned idx, const PdfObject& obj)
{
    if (idx >= m_Objects.size())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index is out of bounds");

    auto& ret = m_Objects[idx];
    if (IsIndirectReferenceAllowed(obj))
        ret = obj.GetIndirectReference();
    else
        ret = PdfObject(obj);

    // NOTE: No dirty set! The container itself is not modified
    return ret;
}

PdfArrayIndirectIterable PdfArray::GetIndirectIterator()
{
    return PdfArrayIndirectIterable(*this);
}

PdfArrayConstIndirectIterable PdfArray::GetIndirectIterator() const
{
    return PdfArrayConstIndirectIterable(const_cast<PdfArray&>(*this));
}

void PdfArray::Clear()
{
    if (m_Objects.size() == 0)
        return;

    m_Objects.clear();
    SetDirty();
}

void PdfArray::Write(OutputStreamDevice& device, PdfWriteFlags writeMode,
    const PdfStatefulEncrypt& encrypt, charbuff& buffer) const
{
    auto it = m_Objects.begin();

    int count = 1;

    if ((writeMode & PdfWriteFlags::Clean) == PdfWriteFlags::Clean)
        device.Write("[ ");
    else
        device.Write('[');

    while (it != m_Objects.end())
    {
        it->GetVariant().Write(device, writeMode, encrypt, buffer);
        if ((writeMode & PdfWriteFlags::Clean) == PdfWriteFlags::Clean)
        {
            device.Write((count % 10 == 0) ? '\n' : ' ');
        }

        it++;
        count++;
    }

    device.Write(']');
}

void PdfArray::ResetDirtyInternal()
{
    // Propagate state to all subclasses
    for (auto& obj : m_Objects)
        obj.ResetDirty();
}

void PdfArray::setChildrenParent()
{
    // Set parent for all children
    for (auto& obj : m_Objects)
        obj.SetParent(*this);
}

PdfObject& PdfArray::add(PdfObject&& obj)
{
    return *insertAt(m_Objects.end(), std::move(obj));
}

PdfArray::iterator PdfArray::insertAt(const iterator& pos, PdfObject&& obj)
{
    auto ret = m_Objects.emplace(pos, std::move(obj));
    ret->SetParent(*this);
    return ret;
}

PdfObject& PdfArray::getAt(unsigned idx) const
{
    if (idx >= (unsigned)m_Objects.size())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index is out of bounds");

    return const_cast<PdfArray&>(*this).m_Objects[idx];
}

PdfObject* PdfArray::findAt(unsigned idx) const
{
    if (idx >= (unsigned)m_Objects.size())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index is out of bounds");

    auto& obj = const_cast<PdfArray&>(*this).m_Objects[idx];
    if (obj.IsReference())
        return GetIndirectObject(obj.GetReference());
    else
        return &obj;
}

size_t PdfArray::size() const
{
    return m_Objects.size();
}

PdfArray::iterator PdfArray::insert(const iterator& pos, const PdfObject& obj)
{
    auto it = insertAt(pos, PdfObject(obj));
    SetDirty();
    return it;
}

PdfArray::iterator PdfArray::insert(const iterator& pos, PdfObject&& obj)
{
    auto it = insertAt(pos, std::move(obj));
    SetDirty();
    return it;
}

void PdfArray::erase(const iterator& pos)
{
    // TODO: Set dirty only if really removed
    m_Objects.erase(pos);
    SetDirty();
}

void PdfArray::erase(const iterator& first, const iterator& last)
{
    // TODO: Set dirty only if really removed
    m_Objects.erase(first, last);
    SetDirty();
}

void PdfArray::Resize(unsigned count, const PdfObject& val)
{
    size_t currentSize = m_Objects.size();
    m_Objects.resize(count, val);
    for (size_t i = currentSize; i < count; i++)
    {
        auto& obj = m_Objects[i];
        obj.SetParent(*this);
    }

    if (currentSize != count)
        SetDirty();
}

void PdfArray::Reserve(unsigned n)
{
    m_Objects.reserve(n);
}

PdfObject& PdfArray::operator[](size_type idx)
{
    return getAt((unsigned)idx);
}

const PdfObject& PdfArray::operator[](size_type idx) const
{
    return getAt((unsigned)idx);
}

PdfArray::iterator PdfArray::begin()
{
    return m_Objects.begin();
}

PdfArray::const_iterator PdfArray::begin() const
{
    return m_Objects.begin();
}

PdfArray::iterator PdfArray::end()
{
    return m_Objects.end();
}

PdfArray::const_iterator PdfArray::end() const
{
    return m_Objects.end();
}

PdfArray::reverse_iterator PdfArray::rbegin()
{
    return m_Objects.rbegin();
}

PdfArray::const_reverse_iterator PdfArray::rbegin() const
{
    return m_Objects.rbegin();
}

PdfArray::reverse_iterator PdfArray::rend()
{
    return m_Objects.rend();
}

PdfArray::const_reverse_iterator PdfArray::rend() const
{
    return m_Objects.rend();
}

void PdfArray::resize(size_t size)
{
#ifndef NDEBUG
    if (size > numeric_limits<unsigned>::max())
        throw length_error("Too big size");
#endif
    m_Objects.resize(size);
}

void PdfArray::reserve(size_t size)
{
#ifndef NDEBUG
    if (size > numeric_limits<unsigned>::max())
        throw length_error("Too big size");
#endif
    m_Objects.reserve(size);
}

PdfObject& PdfArray::front()
{
    return m_Objects.front();
}

const PdfObject& PdfArray::front() const
{
    return m_Objects.front();
}

PdfObject& PdfArray::back()
{
    return m_Objects.back();
}

const PdfObject& PdfArray::back() const
{
    return m_Objects.back();
}

bool PdfArray::operator==(const PdfArray& rhs) const
{
    if (this == &rhs)
        return true;

    // We don't check owner
    return m_Objects == rhs.m_Objects;
}

bool PdfArray::operator!=(const PdfArray& rhs) const
{
    if (this == &rhs)
        return false;

    // We don't check owner
    return m_Objects != rhs.m_Objects;
}

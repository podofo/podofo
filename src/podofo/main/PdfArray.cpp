/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfArray.h"

#include <podofo/auxiliary/OutputDevice.h>

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
    rhs.SetDirty();
}

void PdfArray::RemoveAt(unsigned idx)
{
    AssertMutable();
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
        PODOFO_RAISE_ERROR(PdfErrorCode::ObjectNotFound);

    return *obj;
}

PdfObject& PdfArray::MustFindAt(unsigned idx)
{
    auto obj = findAt(idx);
    if (obj == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::ObjectNotFound);

    return *obj;
}

PdfArray PdfArray::FromBools(cspan<bool> bools)
{
    PdfArray arr;
    arr.reserve(bools.size());
    for (unsigned i = 0; i < bools.size(); i++)
        arr.Add(PdfObject(bools[i]));

    return arr;
}

PdfArray& PdfArray::operator=(const PdfArray& rhs)
{
    AssertMutable();
    m_Objects = rhs.m_Objects;
    setChildrenParent();
    return *this;
}

PdfArray& PdfArray::operator=(PdfArray&& rhs) noexcept
{
    AssertMutable();
    m_Objects = std::move(rhs.m_Objects);
    setChildrenParent();
    rhs.SetDirty();
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
    AssertMutable();
    auto& ret = add(PdfObject(obj));
    SetDirty();
    return ret;
}

PdfObject& PdfArray::Add(PdfObject&& obj)
{
    AssertMutable();
    auto& ret = add(std::move(obj));
    obj.SetDirty();
    SetDirty();
    return ret;
}

void PdfArray::AddIndirect(const PdfObject& obj)
{
    AssertMutable();
    if (IsIndirectReferenceAllowed(obj))
        add(obj.GetIndirectReference());
    else
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Given object shall allow indirect insertion");

    SetDirty();
}

PdfObject& PdfArray::AddIndirectSafe(const PdfObject& obj)
{
    AssertMutable();
    auto& ret = IsIndirectReferenceAllowed(obj)
        ? add(obj.GetIndirectReference())
        : add(PdfObject(obj));
    SetDirty();
    return ret;
}

PdfObject& PdfArray::SetAt(unsigned idx, const PdfObject& obj)
{
    AssertMutable();
    if (idx >= m_Objects.size())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index is out of bounds");

    auto& ret = m_Objects[idx];
    ret = obj;
    // NOTE: No dirty set! The container itself is not modified
    return ret;
}

PdfObject& PdfArray::SetAt(unsigned idx, PdfObject&& obj)
{
    AssertMutable();
    if (idx >= m_Objects.size())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index is out of bounds");

    auto& ret = m_Objects[idx];
    // NOTE: Assignment will implicitly make this container dirty
    ret = std::move(obj);
    return ret;
}

void PdfArray::SetAtIndirect(unsigned idx, const PdfObject* obj)
{
    AssertMutable();
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
    AssertMutable();
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
    AssertMutable();
    return PdfArrayIndirectIterable(*this);
}

PdfArrayConstIndirectIterable PdfArray::GetIndirectIterator() const
{
    return PdfArrayConstIndirectIterable(const_cast<PdfArray&>(*this));
}

void PdfArray::Clear()
{
    AssertMutable();
    if (m_Objects.size() == 0)
        return;

    m_Objects.clear();
    SetDirty();
}

void PdfArray::Write(OutputStream& stream, PdfWriteFlags writeMode,
    const PdfStatefulEncrypt* encrypt, charbuff& buffer) const
{
    bool addDelimiters = (writeMode & PdfWriteFlags::SkipDelimiters) == PdfWriteFlags::None;
    // It doesn't make sense to propagate SkipDelimiters flag
    writeMode &= ~PdfWriteFlags::SkipDelimiters;
    return write(stream, writeMode, addDelimiters, encrypt, buffer);
}

void PdfArray::write(OutputStream& stream, PdfWriteFlags writeMode, bool addDelimiters, const PdfStatefulEncrypt* encrypt, charbuff& buffer) const
{
    if (addDelimiters)
    {
        if ((writeMode & PdfWriteFlags::Clean) == PdfWriteFlags::Clean)
            stream.Write("[ ");
        else
            stream.Write('[');
    }

    auto it = m_Objects.begin();
    unsigned count = 1;
    while (it != m_Objects.end())
    {
        it->GetVariant().Write(stream, writeMode, encrypt, buffer);
        if ((writeMode & PdfWriteFlags::Clean) == PdfWriteFlags::Clean)
        {
            stream.Write((count % 10 == 0) ? '\n' : ' ');
        }

        it++;
        count++;
    }

    if (addDelimiters)
        stream.Write(']');
}

void PdfArray::resetDirty()
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

PdfObject& PdfArray::EmplaceBackNoDirtySet()
{
    auto& ret = m_Objects.emplace_back(nullptr);
    ret.SetParent(*this);
    return ret;
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
    AssertMutable();
    auto it = insertAt(pos, PdfObject(obj));
    SetDirty();
    return it;
}

PdfArray::iterator PdfArray::insert(const iterator& pos, PdfObject&& obj)
{
    AssertMutable();
    auto it = insertAt(pos, std::move(obj));
    obj.SetDirty();
    SetDirty();
    return it;
}

void PdfArray::erase(const iterator& pos)
{
    AssertMutable();
    // TODO: Set dirty only if really removed
    m_Objects.erase(pos);
    SetDirty();
}

void PdfArray::erase(const iterator& first, const iterator& last)
{
    AssertMutable();
    // TODO: Set dirty only if really removed
    m_Objects.erase(first, last);
    SetDirty();
}

void PdfArray::Resize(unsigned count, const PdfObject& val)
{
    AssertMutable();
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
    AssertMutable();
    m_Objects.reserve(n);
}

void PdfArray::SwapAt(unsigned atIndex, unsigned toIndex)
{
    AssertMutable();
    if (atIndex >= m_Objects.size() || toIndex >= m_Objects.size())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "atIndex or toIndex is out of bounds");

    if (atIndex == toIndex)
        return;

    PdfObject temp = m_Objects[toIndex];
    m_Objects[toIndex].AssignNoDirtySet(std::move(m_Objects[atIndex]));
    m_Objects[atIndex].AssignNoDirtySet(std::move(temp));
    SetDirty();
}

void PdfArray::MoveTo(unsigned atIndex, unsigned toIndex)
{
    AssertMutable();
    if (atIndex >= m_Objects.size() || toIndex >= m_Objects.size())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "atIndex or toIndex is out of bounds");

    if (atIndex == toIndex)
        return;

    PdfObject temp(m_Objects[atIndex]);
    if (atIndex > toIndex)
    {
        for (unsigned i = atIndex; i > toIndex; i--)
            m_Objects[i].AssignNoDirtySet(std::move(m_Objects[i - 1]));
    }
    else
    {
        for (unsigned i = atIndex; i < toIndex; i++)
            m_Objects[i].AssignNoDirtySet(std::move(m_Objects[i + 1]));
    }

    m_Objects[toIndex].AssignNoDirtySet(std::move(temp));
    SetDirty();
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
    AssertMutable();
    return m_Objects.begin();
}

PdfArray::const_iterator PdfArray::begin() const
{
    return m_Objects.begin();
}

PdfArray::iterator PdfArray::end()
{
    AssertMutable();
    return m_Objects.end();
}

PdfArray::const_iterator PdfArray::end() const
{
    return m_Objects.end();
}

PdfArray::reverse_iterator PdfArray::rbegin()
{
    AssertMutable();
    return m_Objects.rbegin();
}

PdfArray::const_reverse_iterator PdfArray::rbegin() const
{
    return m_Objects.rbegin();
}

PdfArray::reverse_iterator PdfArray::rend()
{
    AssertMutable();
    return m_Objects.rend();
}

PdfArray::const_reverse_iterator PdfArray::rend() const
{
    return m_Objects.rend();
}

void PdfArray::resize(size_t size)
{
    AssertMutable();
#ifndef NDEBUG
    if (size > numeric_limits<unsigned>::max())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Too big size");
#endif
    // TODO: Check other checks PdfArray::Resize(...)
    m_Objects.resize(size);
}

void PdfArray::reserve(size_t size)
{
    AssertMutable();
#ifndef NDEBUG
    if (size > numeric_limits<unsigned>::max())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Too big size");
#endif
    m_Objects.reserve(size);
}

PdfObject& PdfArray::front()
{
    AssertMutable();
    return m_Objects.front();
}

const PdfObject& PdfArray::front() const
{
    return m_Objects.front();
}

PdfObject& PdfArray::back()
{
    AssertMutable();
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

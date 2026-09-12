// SPDX-FileCopyrightText: 2006 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfArray.h"

#include <podofo/auxiliary/OutputDevice.h>

using namespace std;
using namespace PoDoFo;

PdfArray::PdfArray()
    : m_data(nullptr), m_size(0), m_capacity(0) { }

PdfArray::PdfArray(const PdfArray& rhs)
    : m_data(nullptr), m_size(0), m_capacity(0)
{
    copyFrom(rhs);
    setChildrenParent();
}

PdfArray::PdfArray(PdfArray&& rhs) noexcept
    : m_data(nullptr), m_size(0), m_capacity(0)
{
    moveFrom(std::move(rhs));
    setChildrenParent();
    rhs.SetDirty();
}

PdfArray::~PdfArray()
{
    destroyAll();
}

void PdfArray::RemoveAt(unsigned idx)
{
    AssertMutable();
    // TODO: Set dirty only if really removed
    if (idx >= m_size)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index is out of bounds");

    eraseAt(idx, 1);
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
    if (&rhs == this)
        return *this;

    destroyAll();
    copyFrom(rhs);
    setChildrenParent();
    SetDirty();
    return *this;
}

PdfArray& PdfArray::operator=(PdfArray&& rhs) noexcept
{
    AssertMutable();
    destroyAll();
    moveFrom(std::move(rhs));
    setChildrenParent();
    rhs.SetDirty();
    SetDirty();
    return *this;
}

unsigned PdfArray::GetSize() const
{
    return (unsigned)m_size;
}

bool PdfArray::IsEmpty() const
{
    return m_size == 0;
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
    if (idx >= m_size)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index is out of bounds");

    auto& ret = m_data[idx];
    ret = obj;
    // NOTE: No dirty set! The container itself is not modified
    return ret;
}

PdfObject& PdfArray::SetAt(unsigned idx, PdfObject&& obj)
{
    AssertMutable();
    if (idx >= m_size)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index is out of bounds");

    auto& ret = m_data[idx];
    // NOTE: Assignment will implicitly make this container dirty
    ret = std::move(obj);
    return ret;
}

void PdfArray::SetAtIndirect(unsigned idx, const PdfObject* obj)
{
    AssertMutable();
    if (idx >= m_size)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index is out of bounds");

    if (IsIndirectReferenceAllowed(*obj))
        m_data[idx] = obj->GetIndirectReference();
    else
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Given object shall allow indirect insertion");

    // NOTE: No dirty set! The container itself is not modified
}

PdfObject& PdfArray::SetAtIndirectSafe(unsigned idx, const PdfObject& obj)
{
    AssertMutable();
    if (idx >= m_size)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index is out of bounds");

    auto& ret = m_data[idx];
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
    if (m_size == 0)
        return;

    eraseAt(0, m_size);
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

    auto it = m_data;
    unsigned count = 1;
    auto end = m_data + m_size;
    while (it != end)
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
    for (unsigned i = 0; i < m_size; i++)
        m_data[i].ResetDirty();
}

void PdfArray::setChildrenParent()
{
    // Set parent for all children
    for (unsigned i = 0; i < m_size; i++)
        m_data[i].SetParent(*this);
}

PdfObject& PdfArray::EmplaceBackNoDirtySet()
{
    ensureCapacity((size_t)m_size + 1);
    auto& ret = *new(m_data + m_size)PdfObject(*this, nullptr);
    m_size++;
    return ret;
}

PdfObject& PdfArray::add(PdfObject&& obj)
{
    return *insertAt(m_data + m_size, std::move(obj));
}

PdfArray::iterator PdfArray::insertAt(const iterator& pos, PdfObject&& obj)
{
    // NOTE: The index must be taken before growing, as a relocation
    // invalidates the given position
    unsigned index = (unsigned)(pos - m_data);
    PODOFO_INVARIANT(index <= m_size);
    ensureCapacity((size_t)m_size + 1);
    auto ret = m_data + index;
    if (index < m_size)
    {
        // The tail elements are relocated one position forward
        // NOTE: Cast to void* is required to silence -Wclass-memaccess in gcc 
        std::memmove((void*)(ret + 1), (void*)ret, (size_t)(m_size - index) * sizeof(PdfObject));
        relocateBackPointers(ret + 1, m_size - index);
    }

    new(ret)PdfObject(*this, std::move(obj));
    m_size++;
    return ret;
}

PdfObject& PdfArray::getAt(unsigned idx) const
{
    if (idx >= (unsigned)m_size)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index is out of bounds");

    return const_cast<PdfArray&>(*this).m_data[idx];
}

PdfObject* PdfArray::findAt(unsigned idx) const
{
    if (idx >= (unsigned)m_size)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index is out of bounds");

    auto& obj = const_cast<PdfArray&>(*this).m_data[idx];
    if (obj.IsReference())
        return GetIndirectObject(obj.GetReference());
    else
        return &obj;
}

size_t PdfArray::size() const
{
    return m_size;
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
    eraseAt((unsigned)(pos - m_data), 1);
    SetDirty();
}

void PdfArray::erase(const iterator& first, const iterator& last)
{
    AssertMutable();
    // TODO: Set dirty only if really removed
    eraseAt((unsigned)(first - m_data), (unsigned)(last - first));
    SetDirty();
}

void PdfArray::Resize(unsigned count, const PdfObject& val)
{
    AssertMutable();
    unsigned currentSize = m_size;
    if (count < currentSize)
    {
        eraseAt(count, currentSize - count);
    }
    else
    {
        ensureCapacity(count);
        while (m_size < count)
            addAt(m_size, val);
    }

    if (currentSize != count)
        SetDirty();
}

void PdfArray::Reserve(unsigned n)
{
    AssertMutable();
    if (n > m_capacity)
        reallocate(n);
}

void PdfArray::SwapAt(unsigned atIndex, unsigned toIndex)
{
    AssertMutable();
    if (atIndex >= m_size || toIndex >= m_size)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "atIndex or toIndex is out of bounds");

    if (atIndex == toIndex)
        return;

    PdfObject temp = m_data[toIndex];
    m_data[toIndex].AssignNoDirtySet(std::move(m_data[atIndex]));
    m_data[atIndex].AssignNoDirtySet(std::move(temp));
    SetDirty();
}

void PdfArray::MoveTo(unsigned atIndex, unsigned toIndex)
{
    AssertMutable();
    if (atIndex >= m_size || toIndex >= m_size)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "atIndex or toIndex is out of bounds");

    if (atIndex == toIndex)
        return;

    PdfObject temp(m_data[atIndex]);
    if (atIndex > toIndex)
    {
        for (unsigned i = atIndex; i > toIndex; i--)
            m_data[i].AssignNoDirtySet(std::move(m_data[i - 1]));
    }
    else
    {
        for (unsigned i = atIndex; i < toIndex; i++)
            m_data[i].AssignNoDirtySet(std::move(m_data[i + 1]));
    }

    m_data[toIndex].AssignNoDirtySet(std::move(temp));
    SetDirty();
}

void PdfArray::copyFrom(const PdfArray& rhs)
{
    PODOFO_INVARIANT(m_size == 0);
    if (rhs.m_size == 0)
        return;

    reallocate(rhs.m_size);
    try
    {
        for (unsigned i = 0; i < rhs.m_size; i++)
        {
            new(m_data + i)PdfObject(rhs.m_data[i]);
            m_size++;
        }
    }
    catch (...)
    {
        destroyAll();
        throw;
    }
}

void PdfArray::moveFrom(PdfArray&& rhs)
{
    PODOFO_INVARIANT(m_size == 0 && m_data == nullptr);
    m_data = rhs.m_data;
    m_size = rhs.m_size;
    m_capacity = rhs.m_capacity;
    rhs.m_data = nullptr;
    rhs.m_size = 0;
    rhs.m_capacity = 0;
}

void PdfArray::destroyAll()
{
    for (unsigned i = 0; i < m_size; i++)
        m_data[i].~PdfObject();

    ::operator delete(m_data);
    m_data = nullptr;
    m_size = 0;
    m_capacity = 0;
}

void PdfArray::reallocate(unsigned capacity)
{
    PODOFO_INVARIANT(capacity >= m_size);
    PdfObject* data;
    if (capacity == 0)
    {
        data = nullptr;
    }
    else
    {
        data = (PdfObject*)::operator new((size_t)capacity * sizeof(PdfObject));
        // NOTE: This is a relocation, not a move. The elements stay attached to
        // this container, which didn't move itself, so the whole block is taken
        // over at once and the previous one is released without destroying the
        // elements in it. Only the back pointers to the new addresses are fixed.
        // Cast to void* is required to silence -Wclass-memaccess in gcc 
        std::memcpy((void*)data, (void*)m_data, (size_t)m_size * sizeof(PdfObject));
        relocateBackPointers(data, m_size);
    }

    ::operator delete(m_data);
    m_data = data;
    m_capacity = capacity;
}

void PdfArray::ensureCapacity(size_t size)
{
    if (size > (numeric_limits<unsigned>::max)())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Too big size");

    if (size <= m_capacity)
        return;

    constexpr unsigned MinCapacity = 4;
    unsigned capacity = std::max(m_capacity, MinCapacity);
    while (capacity < size)
    {
        if (capacity > (numeric_limits<unsigned>::max)() / 2)
        {
            capacity = (unsigned)size;
            break;
        }

        capacity *= 2;
    }

    reallocate(capacity);
}

void PdfArray::eraseAt(unsigned index, unsigned count)
{
    PODOFO_INVARIANT(index + count <= m_size);
    for (unsigned i = 0; i < count; i++)
        m_data[index + i].~PdfObject();

    unsigned tail = m_size - index - count;
    if (tail != 0)
    {
        // The shifted elements are relocated, not moved
        // NOTE: Cast to void* is required to silence -Wclass-memaccess in gcc 
        std::memmove((void*)(m_data + index), (void*)(m_data + index + count), (size_t)tail * sizeof(PdfObject));
        relocateBackPointers(m_data + index, tail);
    }

    m_size -= count;
}

void PdfArray::relocateBackPointers(PdfObject* data, unsigned count)
{
    for (unsigned i = 0; i < count; i++)
        data[i].RelocateBackPointers();
}

void PdfArray::addAt(unsigned index, const PdfObject& obj)
{
    new(m_data + index)PdfObject(*this, obj);
    m_size++;
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
    return m_data;
}

PdfArray::const_iterator PdfArray::begin() const
{
    return m_data;
}

PdfArray::iterator PdfArray::end()
{
    AssertMutable();
    return m_data + m_size;
}

PdfArray::const_iterator PdfArray::end() const
{
    return m_data + m_size;
}

PdfArray::reverse_iterator PdfArray::rbegin()
{
    AssertMutable();
    return reverse_iterator(m_data + m_size);
}

PdfArray::const_reverse_iterator PdfArray::rbegin() const
{
    return const_reverse_iterator(m_data + m_size);
}

PdfArray::reverse_iterator PdfArray::rend()
{
    AssertMutable();
    return reverse_iterator(m_data);
}

PdfArray::const_reverse_iterator PdfArray::rend() const
{
    return const_reverse_iterator(m_data);
}

void PdfArray::resize(size_t size)
{
    AssertMutable();
#ifndef NDEBUG
    if (size > numeric_limits<unsigned>::max())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Too big size");
#endif
    // TODO: Check other checks PdfArray::Resize(...)
    unsigned count = (unsigned)size;
    if (count < m_size)
    {
        eraseAt(count, m_size - count);
        return;
    }

    ensureCapacity(count);
    while (m_size < count)
    {
        auto& obj = *new(m_data + m_size)PdfObject();
        m_size++;
        obj.SetParent(*this);
    }
}

void PdfArray::reserve(size_t size)
{
    AssertMutable();
#ifndef NDEBUG
    if (size > numeric_limits<unsigned>::max())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Too big size");
#endif
    if (size > m_capacity)
        reallocate((unsigned)size);
}

PdfObject& PdfArray::front()
{
    AssertMutable();
    return *m_data;
}

const PdfObject& PdfArray::front() const
{
    return *m_data;
}

PdfObject& PdfArray::back()
{
    AssertMutable();
    return m_data[m_size - 1];
}

const PdfObject& PdfArray::back() const
{
    return m_data[m_size - 1];
}

bool PdfArray::operator==(const PdfArray& rhs) const
{
    if (this == &rhs)
        return true;

    // We don't check owner
    if (m_size != rhs.m_size)
        return false;

    for (unsigned i = 0; i < m_size; i++)
    {
        if (m_data[i] != rhs.m_data[i])
            return false;
    }

    return true;
}

bool PdfArray::operator!=(const PdfArray& rhs) const
{
    if (this == &rhs)
        return false;

    // We don't check owner
    return !(*this == rhs);
}

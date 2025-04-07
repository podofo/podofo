/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_ARRAY_H
#define PDF_ARRAY_H

#include "PdfDeclarations.h"
#include "PdfDataContainer.h"

namespace PoDoFo {

class PdfArray;
using PdfArrayList = std::vector<PdfObject>;

/**
 * Helper class to iterate through array indirect objects
 */
template <typename TObject, typename TListIterator>
class PdfArrayIndirectIterableBase final : public PdfIndirectIterableBase
{
    friend class PdfArray;

public:
    PdfArrayIndirectIterableBase();

private:
    PdfArrayIndirectIterableBase(PdfArray& arr);

public:
    class Iterator final
    {
        friend class PdfArrayIndirectIterableBase;
    public:
        using difference_type = void;
        using value_type = TObject*;
        using pointer = void;
        using reference = void;
        using iterator_category = std::forward_iterator_tag;
    public:
        Iterator();
    private:
        Iterator(TListIterator&& iterator, PdfIndirectObjectList* objects);
    public:
        Iterator(const Iterator&) = default;
        Iterator& operator=(const Iterator&) = default;
        bool operator==(const Iterator& rhs) const;
        bool operator!=(const Iterator& rhs) const;
        Iterator& operator++();
        Iterator operator++(int);
        value_type operator*();
        value_type operator->();
    private:
        value_type resolve();
    private:
        TListIterator m_iterator;
        PdfIndirectObjectList* m_objects;
    };

public:
    Iterator begin() const;
    Iterator end() const;

private:
    PdfArray* m_arr;
};

using PdfArrayIndirectIterable = PdfArrayIndirectIterableBase<PdfObject, PdfArrayList::iterator>;
using PdfArrayConstIndirectIterable = PdfArrayIndirectIterableBase<const PdfObject, PdfArrayList::const_iterator>;

/** This class represents a PdfArray
 *  Use it for all arrays that are written to a PDF file.
 *
 *  A PdfArray can hold any PdfVariant.
 *
 *  \see PdfVariant
 */
class PODOFO_API PdfArray final : public PdfDataContainer
{
    friend class PdfObject;
    friend class PdfTokenizer;

public:
    using size_type = size_t;
    using value_type = PdfObject;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = PdfArrayList::iterator;
    using const_iterator = PdfArrayList::const_iterator;
    using reverse_iterator = PdfArrayList::reverse_iterator;
    using const_reverse_iterator = PdfArrayList::const_reverse_iterator;

    /** Create an empty array
     */
    PdfArray();

    /** Deep copy an existing PdfArray
     *
     *  \param rhs the array to copy
     */
    PdfArray(const PdfArray& rhs);
    PdfArray(PdfArray&& rhs) noexcept;

    template <typename TReal, typename = std::enable_if_t<std::is_floating_point_v<TReal>>>
    static PdfArray FromReals(cspan<TReal> reals);

    template <typename TInt, typename = std::enable_if_t<std::is_integral_v<TInt>>>
    static PdfArray FromNumbers(cspan<TInt> numbers);

    static PdfArray FromBools(cspan<bool> bools);

    /** assignment operator
     *
     *  \param rhs the array to assign
     */
    PdfArray& operator=(const PdfArray& rhs);
    PdfArray& operator=(PdfArray&& rhs) noexcept;

    /**
     *  \returns the size of the array
     */
    unsigned GetSize() const;

    /**
     *  \returns true if is empty
     */
    bool IsEmpty() const;

    /** Remove all elements from the array
     */
    void Clear();

    void Write(OutputStream& stream, PdfWriteFlags writeMode,
        const PdfStatefulEncrypt* encrypt, charbuff& buffer) const override;

    template <typename T>
    const typename ObjectAdapter<T>::TRet GetAtAs(unsigned idx) const;

    template <typename T>
    typename ObjectAdapter<T>::TRet GetAtAs(unsigned idx);

    template <typename T>
    const typename ObjectAdapter<T>::TRet GetAtAsSafe(unsigned idx, const std::common_type_t<T>& fallback = { }) const;

    template <typename T>
    typename ObjectAdapter<T>::TRet GetAtAsSafe(unsigned idx, const std::common_type_t<T>& fallback = { });

    template <typename T>
    bool TryGetAtAs(unsigned idx, T& value) const;

    template <typename T>
    bool TryGetAtAs(unsigned idx, T& value);

    /** Get the object at the given index out of the array.
     *
     * Lookup in the indirect objects as well, if the shallow object was a reference.
     * The returned value is a pointer to the internal object in the dictionary
     * so it MUST not be deleted.
     *
     *  \param idx
     *  \returns pointer to the found value. nullptr if the index was out of the boundaries
     */
    const PdfObject* FindAt(unsigned idx) const;
    PdfObject* FindAt(unsigned idx);

    const PdfObject& MustFindAt(unsigned idx) const;
    PdfObject& MustFindAt(unsigned idx);

    template <typename T>
    const typename ObjectAdapter<T>::TRet FindAtAs(unsigned idx) const;

    template <typename T>
    typename ObjectAdapter<T>::TRet FindAtAs(unsigned idx);

    template <typename T>
    const typename ObjectAdapter<T>::TRet FindAtAsSafe(unsigned idx, const std::common_type_t<T>& fallback = { }) const;

    template <typename T>
    typename ObjectAdapter<T>::TRet FindAtAsSafe(unsigned idx, const std::common_type_t<T>& fallback = { });

    template <typename T>
    bool TryFindAtAs(unsigned idx, T& value) const;

    template <typename T>
    bool TryFindAtAs(unsigned idx, T& value);

    void RemoveAt(unsigned idx);

    PdfObject& Add(const PdfObject& obj);

    PdfObject& Add(PdfObject&& obj);

    void AddIndirect(const PdfObject& obj);

    PdfObject& AddIndirectSafe(const PdfObject& obj);

    PdfObject& SetAt(unsigned idx, const PdfObject& obj);

    PdfObject& SetAt(unsigned idx, PdfObject&& obj);

    void SetAtIndirect(unsigned idx, const PdfObject* obj);

    PdfObject& SetAtIndirectSafe(unsigned idx, const PdfObject& obj);

    PdfArrayIndirectIterable GetIndirectIterator();

    PdfArrayConstIndirectIterable GetIndirectIterator() const;

    /**
     * Resize the internal vector.
     * \param count new size
     * \param value reference value
     */
    void Resize(unsigned count, const PdfObject& val = PdfObject());

    void Reserve(unsigned n);

    void SwapAt(unsigned atIndex, unsigned toIndex);

    void MoveTo(unsigned atIndex, unsigned toIndex);

public:
    /**
     *  \returns the size of the array
     */
    size_t size() const;

    PdfObject& operator[](size_type idx);
    const PdfObject& operator[](size_type idx) const;

    /**
     *  Returns a read/write iterator that points to the first
     *  element in the array.  Iteration is done in ordinary
     *  element order.
     */
    iterator begin();

    /**
     *  Returns a read-only (constant) iterator that points to the
     *  first element in the array.  Iteration is done in ordinary
     *  element order.
     */
    const_iterator begin() const;

    /**
     *  Returns a read/write iterator that points one past the last
     *  element in the array.  Iteration is done in ordinary
     *  element order.
     */
    iterator end();

    /**
     *  Returns a read-only (constant) iterator that points one past
     *  the last element in the array.  Iteration is done in
     *  ordinary element order.
     */
    const_iterator end() const;

    /**
     *  Returns a read/write reverse iterator that points to the
     *  last element in the array.  Iteration is done in reverse
     *  element order.
     */
    reverse_iterator rbegin();

    /**
     *  Returns a read-only (constant) reverse iterator that points
     *  to the last element in the array.  Iteration is done in
     *  reverse element order.
     */
    const_reverse_iterator rbegin() const;

    /**
     *  Returns a read/write reverse iterator that points to one
     *  before the first element in the array.  Iteration is done
     *  in reverse element order.
     */
    reverse_iterator rend();

    /**
     *  Returns a read-only (constant) reverse iterator that points
     *  to one before the first element in the array.  Iteration
     *  is done in reverse element order.
     */
    const_reverse_iterator rend() const;

    void resize(size_t size);

    void reserve(size_t size);

    iterator insert(const iterator& pos, const PdfObject& obj);
    iterator insert(const iterator& pos, PdfObject&& obj);

    template<typename InputIterator>
    inline void insert(const iterator& pos, const InputIterator& first, const InputIterator& last);

    void erase(const iterator& pos);
    void erase(const iterator& first, const iterator& last);

    /**
     *  \returns a read/write reference to the data at the first
     *           element of the array.
     */
    reference front();

    /**
     *  \returns a read-only (constant) reference to the data at the first
     *           element of the array.
     */
    const_reference front() const;

    /**
     *  \returns a read/write reference to the data at the last
     *           element of the array.
     */
    reference back();

    /**
     *  \returns a read-only (constant) reference to the data at the
     *           last element of the array.
     */
    const_reference back() const;

public:
    bool operator==(const PdfArray& rhs) const;
    bool operator!=(const PdfArray& rhs) const;

protected:
    void resetDirty() override;
    void setChildrenParent() override;

private:
    // Append a new "null" object to the back
    PdfObject& EmplaceBackNoDirtySet();

private:
    PdfObject& add(PdfObject&& obj);
    iterator insertAt(const iterator& pos, PdfObject&& obj);
    PdfObject& getAt(unsigned idx) const;
    PdfObject* findAt(unsigned idx) const;
    void write(OutputStream& stream, PdfWriteFlags writeMode, bool addDelimiters,
        const PdfStatefulEncrypt* encrypt, charbuff& buffer) const;

private:
    PdfArrayList m_Objects;
};

template<typename TReal, typename>
PdfArray PdfArray::FromReals(cspan<TReal> reals)
{
    PdfArray arr;
    arr.reserve(reals.size());
    for (unsigned i = 0; i < reals.size(); i++)
        arr.Add(PdfObject(static_cast<double>(reals[i])));

    return arr;
}

template<typename TInt, typename>
PdfArray PdfArray::FromNumbers(cspan<TInt> numbers)
{
    PdfArray arr;
    arr.reserve(numbers.size());
    for (unsigned i = 0; i < numbers.size(); i++)
        arr.Add(PdfObject(static_cast<int64_t>(numbers[i])));

    return arr;
}

template<typename T>
const typename ObjectAdapter<T>::TRet PdfArray::GetAtAs(unsigned idx) const
{
    return ObjectAdapter<T>::Get(const_cast<const PdfObject&>(getAt(idx)));
}

template<typename T>
typename ObjectAdapter<T>::TRet PdfArray::GetAtAs(unsigned idx)
{
    return ObjectAdapter<T>::Get(getAt(idx));
}

template<typename T>
const typename ObjectAdapter<T>::TRet PdfArray::GetAtAsSafe(unsigned idx, const std::common_type_t<T>& fallback) const
{
    return ObjectAdapter<T>::Get(const_cast<const PdfObject&>(getAt(idx)), fallback);
}

template<typename T>
typename ObjectAdapter<T>::TRet PdfArray::GetAtAsSafe(unsigned idx, const std::common_type_t<T>& fallback)
{
    return ObjectAdapter<T>::Get(getAt(idx), fallback);
}

template<typename T>
bool PdfArray::TryGetAtAs(unsigned idx, T& value) const
{
    if (ObjectAdapter<T>::TryGet(const_cast<const PdfObject&>(getAt(idx)), value))
    {
        return true;
    }
    else
    {
        value = { };
        return false;
    }
}

template<typename T>
bool PdfArray::TryGetAtAs(unsigned idx, T& value)
{
    if (ObjectAdapter<T>::TryGet(getAt(idx), value))
    {
        return true;
    }
    else
    {
        value = { };
        return false;
    }
}

template<typename T>
const typename ObjectAdapter<T>::TRet PdfArray::FindAtAs(unsigned idx) const
{
    return ObjectAdapter<T>::Get(MustFindAt(idx));
}

template<typename T>
inline typename ObjectAdapter<T>::TRet PdfArray::FindAtAs(unsigned idx)
{
    return ObjectAdapter<T>::Get(MustFindAt(idx));
}

template<typename T>
const typename ObjectAdapter<T>::TRet PdfArray::FindAtAsSafe(unsigned idx, const std::common_type_t<T>& fallback) const
{
    auto obj = findAt(idx);
    if (obj == nullptr)
        return fallback;
    else
        return ObjectAdapter<T>::Get(const_cast<const PdfObject&>(*obj), fallback);
}

template<typename T>
inline typename ObjectAdapter<T>::TRet PdfArray::FindAtAsSafe(unsigned idx, const std::common_type_t<T>& fallback)
{
    auto obj = findAt(idx);
    if (obj == nullptr)
        return fallback;
    else
        return ObjectAdapter<T>::Get(*obj, fallback);
}

template<typename T>
bool PdfArray::TryFindAtAs(unsigned idx, T& value) const
{
    auto obj = findAt(idx);
    if (obj != nullptr && ObjectAdapter<T>::TryGet(const_cast<const PdfObject&>(*obj), value))
    {
        return true;
    }
    else
    {
        value = { };
        return false;
    }
}

template<typename T>
inline bool PdfArray::TryFindAtAs(unsigned idx, T& value)
{
    auto obj = findAt(idx);
    if (obj != nullptr && ObjectAdapter<T>::TryGet(*obj, value))
    {
        return true;
    }
    else
    {
        value = { };
        return false;
    }
}

template<typename InputIterator>
void PdfArray::insert(const PdfArray::iterator& pos,
    const InputIterator& first,
    const InputIterator& last)
{
    AssertMutable();
    auto document = GetObjectDocument();
    InputIterator it1 = first;
    iterator it2 = pos;
    for (; it1 != last; it1++, it2++)
    {
        it2 = m_Objects.insert(it2, *it1);
        it2->SetDocument(document);
    }

    SetDirty();
}

template<typename TObject, typename TListIterator>
PdfArrayIndirectIterableBase<TObject, TListIterator>::PdfArrayIndirectIterableBase()
    : m_arr(nullptr) { }

template<typename TObject, typename TListIterator>
PdfArrayIndirectIterableBase<TObject, TListIterator>::PdfArrayIndirectIterableBase(PdfArray& arr)
    : PdfIndirectIterableBase(arr), m_arr(&arr) { }

template<typename TObject, typename TListIterator>
typename PdfArrayIndirectIterableBase<TObject, TListIterator>::Iterator PdfArrayIndirectIterableBase<TObject, TListIterator>::begin() const
{
    if (m_arr == nullptr)
        return Iterator();
    else
        return Iterator(m_arr->begin(), GetObjects());
}

template<typename TObject, typename TListIterator>
typename PdfArrayIndirectIterableBase<TObject, TListIterator>::Iterator PdfArrayIndirectIterableBase<TObject, TListIterator>::end() const
{
    if (m_arr == nullptr)
        return Iterator();
    else
        return Iterator(m_arr->end(), GetObjects());
}

template<typename TObject, typename TListIterator>
PdfArrayIndirectIterableBase<TObject, TListIterator>::Iterator::Iterator() : m_objects(nullptr) { }

template<typename TObject, typename TListIterator>
PdfArrayIndirectIterableBase<TObject, TListIterator>::Iterator::Iterator(TListIterator&& iterator, PdfIndirectObjectList* objects)
    : m_iterator(std::move(iterator)), m_objects(objects) { }

template<typename TObject, typename TListIterator>
bool PdfArrayIndirectIterableBase<TObject, TListIterator>::Iterator::operator==(const Iterator& rhs) const
{
    return m_iterator == rhs.m_iterator;
}

template<typename TObject, typename TListIterator>
bool PdfArrayIndirectIterableBase<TObject, TListIterator>::Iterator::operator!=(const Iterator& rhs) const
{
    return m_iterator != rhs.m_iterator;
}

template<typename TObject, typename TListIterator>
typename PdfArrayIndirectIterableBase<TObject, TListIterator>::Iterator& PdfArrayIndirectIterableBase<TObject, TListIterator>::Iterator::operator++()
{
    m_iterator++;
    return *this;
}

template<typename TObject, typename TListIterator>
typename PdfArrayIndirectIterableBase<TObject, TListIterator>::Iterator PdfArrayIndirectIterableBase<TObject, TListIterator>::Iterator::operator++(int)
{
    auto copy = *this;
    m_iterator++;
    return copy;
}

template<typename TObject, typename TListIterator>
typename PdfArrayIndirectIterableBase<TObject, TListIterator>::Iterator::value_type PdfArrayIndirectIterableBase<TObject, TListIterator>::Iterator::operator*()
{
    return resolve();
}

template<typename TObject, typename TListIterator>
typename PdfArrayIndirectIterableBase<TObject, TListIterator>::Iterator::value_type PdfArrayIndirectIterableBase<TObject, TListIterator>::Iterator::operator->()
{
    return resolve();
}

template<typename TObject, typename TListIterator>
typename PdfArrayIndirectIterableBase<TObject, TListIterator>::Iterator::value_type PdfArrayIndirectIterableBase<TObject, TListIterator>::Iterator::resolve()
{
    TObject& robj = *m_iterator;
    TObject* indirectobj;
    PdfReference ref;
    if (m_objects != nullptr
        && robj.TryGetReference(ref)
        && ref.IsIndirect()
        && (indirectobj = GetObject(*m_objects, ref)) != nullptr)
    {
       return indirectobj;
    }
    else
    {
        return &robj;
    }
}

};

#endif // PDF_ARRAY_H

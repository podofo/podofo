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
    class iterator final
    {
        friend class PdfArrayIndirectIterableBase;
    public:
        using difference_type = void;
        using value_type = TObject*;
        using pointer = void;
        using reference = void;
        using iterator_category = std::forward_iterator_tag;
    public:
        iterator();
    private:
        iterator(TListIterator&& iterator, PdfIndirectObjectList* objects);
    public:
        iterator(const iterator&) = default;
        iterator& operator=(const iterator&) = default;
        bool operator==(const iterator& rhs) const;
        bool operator!=(const iterator& rhs) const;
        iterator& operator++();
        value_type operator*();
        value_type operator->();
    private:
        value_type resolve();
    private:
        TListIterator m_iterator;
        PdfIndirectObjectList* m_objects;
    };

public:
    iterator begin() const;
    iterator end() const;

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

    void Write(OutputStreamDevice& device, PdfWriteFlags writeMode,
        const PdfStatefulEncrypt& encrypt, charbuff& buffer) const override;

    template <typename T>
    T GetAtAs(unsigned idx) const;

    template <typename T>
    T GetAtAsSafe(unsigned idx, const std::common_type_t<T>& defvalue = { }) const;

    template <typename T>
    bool TryGetAtAs(unsigned idx, T& value) const;

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
    T FindAtAs(unsigned idx, const std::common_type_t<T>& defvalue = { }) const;

    template <typename T>
    T FindAtAsSafe(unsigned idx, const std::common_type_t<T>& defvalue = { }) const;

    template <typename T>
    bool TryFindAtAs(unsigned idx, T& value) const;

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
     * \param value refernce value
     */
    void Resize(unsigned count, const PdfObject& val = PdfObject());

    void Reserve(unsigned n);

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
    void ResetDirtyInternal() override;
    void setChildrenParent() override;

private:
    PdfObject& add(PdfObject&& obj);
    iterator insertAt(const iterator& pos, PdfObject&& obj);
    PdfObject& getAt(unsigned idx) const;
    PdfObject* findAt(unsigned idx) const;

private:
    PdfArrayList m_Objects;
};

template<typename T>
T PdfArray::GetAtAs(unsigned idx) const
{
    return Object<T>::Get(getAt(idx));
}

template<typename T>
T PdfArray::GetAtAsSafe(unsigned idx, const std::common_type_t<T>& defvalue) const
{
    T value;
    if (Object<T>::TryGet(getAt(idx), value))
        return value;
    else
        return defvalue;
}

template<typename T>
bool PdfArray::TryGetAtAs(unsigned idx, T& value) const
{
    if (Object<T>::TryGet(getAt(idx), value))
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
T PdfArray::FindAtAs(unsigned idx, const std::common_type_t<T>& defvalue) const
{
    auto obj = findAt(idx);
    if (obj == nullptr)
        return defvalue;

    return Object<T>::Get(*obj);
}

template<typename T>
T PdfArray::FindAtAsSafe(unsigned idx, const std::common_type_t<T>& defvalue) const
{
    T value;
    auto obj = findAt(idx);
    if (obj != nullptr && Object<T>::TryGet(*obj, value))
        return value;
    else
        return defvalue;
}

template<typename T>
bool PdfArray::TryFindAtAs(unsigned idx, T& value) const
{
    auto obj = findAt(idx);
    if (obj != nullptr && Object<T>::TryGet(*obj, value))
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
typename PdfArrayIndirectIterableBase<TObject, TListIterator>::iterator PdfArrayIndirectIterableBase<TObject, TListIterator>::begin() const
{
    if (m_arr == nullptr)
        return iterator();
    else
        return iterator(m_arr->begin(), GetObjects());
}

template<typename TObject, typename TListIterator>
typename PdfArrayIndirectIterableBase<TObject, TListIterator>::iterator PdfArrayIndirectIterableBase<TObject, TListIterator>::end() const
{
    if (m_arr == nullptr)
        return iterator();
    else
        return iterator(m_arr->end(), GetObjects());
}

template<typename TObject, typename TListIterator>
PdfArrayIndirectIterableBase<TObject, TListIterator>::iterator::iterator() : m_objects(nullptr) { }

template<typename TObject, typename TListIterator>
PdfArrayIndirectIterableBase<TObject, TListIterator>::iterator::iterator(TListIterator&& iterator, PdfIndirectObjectList* objects)
    : m_iterator(std::move(iterator)), m_objects(objects) { }

template<typename TObject, typename TListIterator>
bool PdfArrayIndirectIterableBase<TObject, TListIterator>::iterator::operator==(const iterator& rhs) const
{
    return m_iterator == rhs.m_iterator;
}

template<typename TObject, typename TListIterator>
bool PdfArrayIndirectIterableBase<TObject, TListIterator>::iterator::operator!=(const iterator& rhs) const
{
    return m_iterator != rhs.m_iterator;
}

template<typename TObject, typename TListIterator>
typename PdfArrayIndirectIterableBase<TObject, TListIterator>::iterator& PdfArrayIndirectIterableBase<TObject, TListIterator>::iterator::operator++()
{
    m_iterator++;
    return *this;
}

template<typename TObject, typename TListIterator>
typename PdfArrayIndirectIterableBase<TObject, TListIterator>::iterator::value_type PdfArrayIndirectIterableBase<TObject, TListIterator>::iterator::operator*()
{
    return resolve();
}

template<typename TObject, typename TListIterator>
typename PdfArrayIndirectIterableBase<TObject, TListIterator>::iterator::value_type PdfArrayIndirectIterableBase<TObject, TListIterator>::iterator::operator->()
{
    return resolve();
}

template<typename TObject, typename TListIterator>
typename PdfArrayIndirectIterableBase<TObject, TListIterator>::iterator::value_type PdfArrayIndirectIterableBase<TObject, TListIterator>::iterator::resolve()
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

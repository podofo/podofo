/**
 * SPDX-FileCopyrightText: (C) 2011 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_DICTIONARY_H
#define PDF_DICTIONARY_H

#include "PdfDeclarations.h"
#include "PdfDataContainer.h"

namespace PoDoFo {

class PdfDictionary;

// Compartor to enable heterogeneous lookup in
// PdfDictionary with both PdfName and string_view
// See https://stackoverflow.com/a/31924435/213871
struct PdfDictionaryComparator final
{
    using is_transparent = std::true_type;
    bool operator()(const PdfName& lhs, const PdfName& rhs) const
    {
        return lhs < rhs;
    }
    bool operator()(const PdfName& lhs, const std::string_view& rhs) const
    {
        return lhs.GetRawData() < rhs;
    }
    bool operator()(const std::string_view& lhs, const PdfName& rhs) const
    {
        return lhs < rhs.GetRawData();
    }
};

using PdfDictionaryMap = std::map<PdfName, PdfObject, PdfDictionaryComparator>;

/**
 * Helper class to iterate through indirect objects
 */
template <typename TObject, typename TMapIterator>
class PdfDictionaryIndirectIterableBase final : public PdfIndirectIterableBase
{
    friend class PdfDictionary;

public:
    PdfDictionaryIndirectIterableBase();

private:
    PdfDictionaryIndirectIterableBase(PdfDictionary& dict);

public:
    class iterator final
    {
        friend class PdfDictionaryIndirectIterableBase;
    public:
        using difference_type = void;
        using value_type = std::pair<PdfName, TObject*>;
        using pointer = const value_type*;
        using reference = const value_type&;
        using iterator_category = std::forward_iterator_tag;
    public:
        iterator();
    private:
        iterator(TMapIterator&& iterator, PdfIndirectObjectList* objects);
    public:
        iterator(const iterator&) = default;
        iterator& operator=(const iterator&) = default;
        bool operator==(const iterator& rhs) const;
        bool operator!=(const iterator& rhs) const;
        iterator& operator++();
        reference operator*();
        pointer operator->();
    private:
        void resolve();
    private:
        TMapIterator m_iterator;
        PdfIndirectObjectList* m_objects;
        value_type m_pair;
    };

public:
    iterator begin() const;
    iterator end() const;

private:
    PdfDictionary* m_dict;
};

using PdfDictionaryIndirectIterable = PdfDictionaryIndirectIterableBase<PdfObject, PdfDictionaryMap::iterator>;
using PdfDictionaryConstIndirectIterable = PdfDictionaryIndirectIterableBase<const PdfObject, PdfDictionaryMap::const_iterator>;

/** The PDF dictionary data type of PoDoFo (inherits from PdfDataContainer,
 * the base class for such representations)
 * Note: manipulation function accepts PdfName for the key,
 * while getters accept std::string_view. This is an optimization
 * since we do lookup with both types. We also assume doing
 * lookups with strings will only use characters compatible
 * with PdfDocEncoding
 */
class PODOFO_API PdfDictionary final : public PdfDataContainer
{
    friend class PdfObject;
    friend class PdfTokenizer;

public:
    /** Create a new, empty dictionary
     */
    PdfDictionary();

    /** Deep copy a dictionary
     *  \param rhs the PdfDictionary to copy
     */
    PdfDictionary(const PdfDictionary& rhs);
    PdfDictionary(PdfDictionary&& rhs) noexcept;

    /** Assignment operator.
     *  Assign another PdfDictionary to this dictionary. This is a deep copy;
     *  all elements of the source dictionary are duplicated.
     *
     *  \param rhs the PdfDictionary to copy.
     *
     *  \return this PdfDictionary
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    PdfDictionary& operator=(const PdfDictionary& rhs);
    PdfDictionary& operator=(PdfDictionary&& rhs) noexcept;

    /**
     * Comparison operator. If this dictionary contains all the same keys
     * as the other dictionary, and for each key the values compare equal,
     * the dictionaries are considered equal.
     */
    bool operator==(const PdfDictionary& rhs) const;

    /**
     * \see operator==
     */
    bool operator!=(const PdfDictionary& rhs) const;

    /** Removes all keys from the dictionary
     */
    void Clear();

    /** Add a key to the dictionary.
     *  If an existing key of this name exists, its value is replaced and
     *  the old value object will be deleted. The given object is copied.
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     *
     *  \param key the key is identified by this name in the dictionary
     *  \param obj object containing the data. The object is copied.
     */
    PdfObject& AddKey(const PdfName& key, const PdfObject& obj);
    PdfObject& AddKey(const PdfName& key, PdfObject&& obj);

    /** Add a key to the dictionary.
     *  If an existing key of this name exists, its value is replaced and
     *  the old value object will be deleted. The object must be indirect
     *  and the object reference will be added instead to the dictionary
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     *
     *  \param key the key is identified by this name in the dictionary
     *  \param obj object containing the data
     *  \throws PdfError::InvalidHandle on nullptr obj or if the object can't
     *  be added as an indirect reference
     */
    void AddKeyIndirect(const PdfName& key, const PdfObject& obj);

    /** Add a key to the dictionary.
     *  If an existing key of this name exists, its value is replaced and
     *  the old value object will be deleted. If the object is indirect
     *  the object reference will be added instead to the dictionary,
     *  otherwise the object is copied
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     *
     *  \param key the key is identified by this name in the dictionary
     *  \param obj a variant object containing the data
     *  \throws PdfError::InvalidHandle on nullptr obj
     */
    PdfObject& AddKeyIndirectSafe(const PdfName& key, const PdfObject& obj);

    /** Get the key's value out of the dictionary.
     *
     * The returned value is a pointer to the internal object in the dictionary
     * so it MUST not be deleted.
     *
     *  \param key look for the key named key in the dictionary
     *
     *  \returns pointer to the found value, or 0 if the key was not found.
     */
    const PdfObject* GetKey(const std::string_view& key) const;

    /** Get the key's value out of the dictionary.  This is an overloaded member
     * function.
     *
     * The returned value is a pointer to the internal object in the dictionary.
     * It may be modified but is still owned by the dictionary so it MUST not
     * be deleted.
     *
     *  \param key look for the key named key in the dictionary
     *
     *  \returns the found value, or 0 if the key was not found.
     */
    PdfObject* GetKey(const std::string_view& key);

    /** Get the keys value out of the dictionary
     *
     * Lookup in the indirect objects as well, if the shallow object was a reference.
     * The returned value is a pointer to the internal object in the dictionary
     * so it MUST not be deleted.
     *
     *  \param key look for the key names key in the dictionary
     *  \returns pointer to the found value or 0 if the key was not found.
     */
    const PdfObject* FindKey(const std::string_view& key) const;
    PdfObject* FindKey(const std::string_view& key);
    const PdfObject& MustFindKey(const std::string_view& key) const;
    PdfObject& MustFindKey(const std::string_view& key);

    /** Get the keys value out of the dictionary
     *
     * Lookup in the indirect objects as well, if the shallow object was a reference.
     * Also lookup the parent objects, if /Parent key is found in the dictionary.
     * The returned value is a pointer to the internal object in the dictionary
     * so it MUST not be deleted.
     *
     *  \param key look for the key names key in the dictionary
     *  \returns pointer to the found value or 0 if the key was not found.
     */
    const PdfObject* FindKeyParent(const std::string_view& key) const;
    PdfObject* FindKeyParent(const std::string_view& key);
    const PdfObject& MustFindKeyParent(const std::string_view& key) const;
    PdfObject& MustFindKeyParent(const std::string_view& key);

    /** Get the key's value out of the dictionary.
     *
     * The returned value is a reference to the internal object in the dictionary
     * so it MUST not be deleted. If the key is not found, this throws a PdfError
     * exception with error code PdfErrorCode::NoObject, instead of returning.
     * This is intended to make code more readable by sparing (especially multiple)
     * nullptr checks.
     *
     *  \param key look for the key named key in the dictionary
     *
     *  \returns reference to the found value (never 0).
     *  \throws PdfError(PdfErrorCode::NoObject).
     */
    const PdfObject& MustGetKey(const std::string_view& key) const;
    PdfObject& MustGetKey(const std::string_view& key);

    template <typename T>
    T GetKeyAs(const std::string_view& key, const std::common_type_t<T>& defvalue = { }) const;

    template <typename T>
    T FindKeyAs(const std::string_view& key, const std::common_type_t<T>& defvalue = { }) const;

    template <typename T>
    T FindKeyParentAs(const std::string_view& key, const std::common_type_t<T>& defvalue = { }) const;

    template <typename T>
    T FindKeyAsSafe(const std::string_view& key, const std::common_type_t<T>& defvalue = { }) const;

    template <typename T>
    T FindKeyParentAsSafe(const std::string_view& key, const std::common_type_t<T>& defvalue = { }) const;

    template <typename T>
    bool TryFindKeyAs(const std::string_view& key, T& value) const;

    template <typename T>
    bool TryFindKeyParentAs(const std::string_view& key, T& value) const;

    /** Allows to check if a dictionary contains a certain key.
     * \param key look for the key named key.Name() in the dictionary
     *
     *  \returns true if the key is part of the dictionary, otherwise false.
     */
    bool HasKey(const std::string_view& key) const;

    /** Remove a key from this dictionary.  If the key does not exist, this
     * function does nothing.
     *
     *  \param key the name of the key to delete
     *
     *  \returns true if the key was found in the object and was removed.
     *  If there was no key with this name, false is returned.
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    bool RemoveKey(const std::string_view& key);

    void Write(OutputStreamDevice& device, PdfWriteFlags writeMode,
        const PdfStatefulEncrypt& encrypt, charbuff& buffer) const override;

    /**
     * \returns the size of the internal map
     */
    unsigned GetSize() const;

    PdfDictionaryIndirectIterable GetIndirectIterator();

    PdfDictionaryConstIndirectIterable GetIndirectIterator() const;

public:
    using iterator = PdfDictionaryMap::iterator;
    using const_iterator = PdfDictionaryMap::const_iterator;

public:
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    size_t size() const;

protected:
    void ResetDirtyInternal() override;
    void setChildrenParent() override;

private:
    std::pair<iterator, bool> AddKey(const PdfName& key, PdfObject&& obj, bool noDirtySet);

private:
    PdfObject& addKey(const PdfName& key, PdfObject&& obj);
    PdfObject* getKey(const std::string_view& key) const;
    PdfObject* findKey(const std::string_view& key) const;
    PdfObject* findKeyParent(const std::string_view& key) const;

private:
    PdfDictionaryMap m_Map;
};

template<typename T>
T PdfDictionary::GetKeyAs(const std::string_view& key, const std::common_type_t<T>& defvalue) const
{
    auto obj = getKey(key);
    if (obj == nullptr)
        return defvalue;

    return Object<T>::Get(*obj);
}

template<typename T>
T PdfDictionary::FindKeyAs(const std::string_view& key, const std::common_type_t<T>& defvalue) const
{
    auto obj = findKey(key);
    if (obj == nullptr)
        return defvalue;

    return Object<T>::Get(*obj);
}

template<typename T>
T PdfDictionary::FindKeyParentAs(const std::string_view& key, const std::common_type_t<T>& defvalue) const
{
    auto obj = findKeyParent(key);
    T ret{ };
    if (obj == nullptr)
        return defvalue;

    return Object<T>::Get(*obj);
}

template<typename T>
T PdfDictionary::FindKeyAsSafe(const std::string_view& key, const std::common_type_t<T>& defvalue) const
{
    T value;
    auto obj = findKey(key);
    if (obj != nullptr && Object<T>::TryGet(*obj, value))
        return value;
    else
        return defvalue;
}

template<typename T>
T PdfDictionary::FindKeyParentAsSafe(const std::string_view& key, const std::common_type_t<T>& defvalue) const
{
    T value;
    auto obj = findKeyParent(key);
    if (obj != nullptr && Object<T>::TryGet(*obj, value))
        return value;
    else
        return defvalue;
}

template <typename T>
bool PdfDictionary::TryFindKeyAs(const std::string_view& key, T& value) const
{
    auto obj = findKey(key);
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

template <typename T>
bool PdfDictionary::TryFindKeyParentAs(const std::string_view& key, T& value) const
{
    auto obj = findKeyParent(key);
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

template <typename TObject, typename TMapIterator>
PdfDictionaryIndirectIterableBase<TObject, TMapIterator>::PdfDictionaryIndirectIterableBase()
    : m_dict(nullptr) { }

template <typename TObject, typename TMapIterator>
PdfDictionaryIndirectIterableBase<TObject, TMapIterator>::PdfDictionaryIndirectIterableBase(PdfDictionary& dict)
    : PdfIndirectIterableBase(dict), m_dict(&dict) { }

template <typename TObject, typename TMapIterator>
typename PdfDictionaryIndirectIterableBase<TObject, TMapIterator>::iterator PdfDictionaryIndirectIterableBase<TObject, TMapIterator>::begin() const
{
    if (m_dict == nullptr)
        return iterator();
    else
        return iterator(m_dict->begin(), GetObjects());
}

template <typename TObject, typename TMapIterator>
typename PdfDictionaryIndirectIterableBase<TObject, TMapIterator>::iterator PdfDictionaryIndirectIterableBase<TObject, TMapIterator>::end() const
{
    if (m_dict == nullptr)
        return iterator();
    else
        return iterator(m_dict->end(), GetObjects());
}


template<typename TObject, typename TMapIterator>
PdfDictionaryIndirectIterableBase<TObject, TMapIterator>::iterator::iterator() : m_objects(nullptr) { }

template<typename TObject, typename TMapIterator>
PdfDictionaryIndirectIterableBase<TObject, TMapIterator>::iterator::iterator(TMapIterator&& iterator, PdfIndirectObjectList* objects)
    : m_iterator(std::move(iterator)), m_objects(objects) { }

template<typename TObject, typename TMapIterator>
bool PdfDictionaryIndirectIterableBase<TObject, TMapIterator>::iterator::operator==(const iterator& rhs) const
{
    return m_iterator == rhs.m_iterator;
}

template<typename TObject, typename TMapIterator>
bool PdfDictionaryIndirectIterableBase<TObject, TMapIterator>::iterator::operator!=(const iterator& rhs) const
{
    return m_iterator != rhs.m_iterator;
}

template<typename TObject, typename TMapIterator>
typename PdfDictionaryIndirectIterableBase<TObject, TMapIterator>::iterator& PdfDictionaryIndirectIterableBase<TObject, TMapIterator>::iterator::operator++()
{
    m_iterator++;
    return *this;
}

template<typename TObject, typename TMapIterator>
typename PdfDictionaryIndirectIterableBase<TObject, TMapIterator>::iterator::reference PdfDictionaryIndirectIterableBase<TObject, TMapIterator>::iterator::operator*()
{
    resolve();
    return m_pair;
}

template<typename TObject, typename TMapIterator>
typename PdfDictionaryIndirectIterableBase<TObject, TMapIterator>::iterator::pointer PdfDictionaryIndirectIterableBase<TObject, TMapIterator>::iterator::operator->()
{
    resolve();
    return &m_pair;
}

template<typename TObject, typename TMapIterator>
void PdfDictionaryIndirectIterableBase<TObject, TMapIterator>::iterator::resolve()
{
    TObject& robj = m_iterator->second;
    TObject* indirectobj;
    PdfReference ref;
    if (m_objects != nullptr
        && robj.TryGetReference(ref)
        && ref.IsIndirect()
        && (indirectobj = GetObject(*m_objects, ref)) != nullptr)
    {
        m_pair = value_type(m_iterator->first, indirectobj);
    }
    else
    {
        m_pair = value_type(m_iterator->first, &robj);
    }
}

}

#endif // PDF_DICTIONARY_H

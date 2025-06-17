/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2024 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PdfDeclarationsPrivate.h"
#include <podofo/main/PdfDocument.h>

namespace PoDoFo
{
    template <typename TKey>
    struct PdfTreeKeyAccess
    {
        using TLookup = void*;

        static bool TryGetKey(const PdfObject& obj, TLookup key)
        {
            (void)obj;
            (void)key;
            static_assert(always_false<TKey>, "Unsupported type");
            return false;
        }

        static bool Equals(const PdfObject& obj, TLookup key)
        {
            (void)obj;
            (void)key;
            static_assert(always_false<TKey>, "Unsupported type");
            return false;
        }

        static bool GreaterThan(const PdfObject& obj, TLookup key)
        {
            (void)obj;
            (void)key;
            static_assert(always_false<TKey>, "Unsupported type");
            return false;
        }

        static bool LessThan(const PdfObject& obj, TLookup key)
        {
            (void)obj;
            (void)key;
            static_assert(always_false<TKey>, "Unsupported type");
            return false;
        }

        static PdfName GetKeyStoreName()
        {
            static_assert(always_false<TKey>, "Unsupported type");
            return { };
        }

        static std::string_view GetKeyStoreNameStr()
        {
            static_assert(always_false<TKey>, "Unsupported type");
            return { };
        }
    };

    template <>
    struct PdfTreeKeyAccess<PdfString>
    {
        using TLookup = std::string_view;

        static bool TryGetKey(const PdfObject& obj, PdfString& key)
        {
            return obj.TryGetString(key);
        }

        static bool Equals(const PdfObject& obj, std::string_view key)
        {
            return obj.GetString().GetString() == key;
        }

        static bool GreaterThan(const PdfObject& obj, std::string_view key)
        {
            return obj.GetString().GetString() > key;
        }

        static bool LessThan(const PdfObject& obj, std::string_view key)
        {
            return obj.GetString().GetString() < key;
        }

        static PdfName GetKeyStoreName()
        {
            return "Names"_n;
        }

        static std::string_view GetKeyStoreNameStr()
        {
            return "Names";
        }
    };

    template <>
    struct PdfTreeKeyAccess<int64_t>
    {
        using TLookup = std::int64_t;

        static bool TryGetKey(const PdfObject& obj, int64_t& key)
        {
            return obj.TryGetNumber(key);
        }

        static bool Equals(const PdfObject& obj, int64_t key)
        {
            return obj.GetNumber() == key;
        }

        static bool GreaterThan(const PdfObject& obj, int64_t key)
        {
            return obj.GetNumber() > key;
        }

        static bool LessThan(const PdfObject& obj, int64_t key)
        {
            return obj.GetNumber() < key;
        }

        static PdfName GetKeyStoreName()
        {
            return "Nums"_n;
        }

        static std::string_view GetKeyStoreNameStr()
        {
            return "Nums";
        }
    };

    template <typename TKey>
    class PdfTreeNode : PdfDictionaryElement
    {
        static constexpr unsigned BalanceTreeMax = 65;

    public:
        PdfTreeNode(PdfTreeNode* parent, PdfObject& obj)
            : PdfDictionaryElement(obj), m_Parent(parent)
        {
            m_HasKids = GetDictionary().HasKey("Kids");
        }

        PdfObject* GetValue(typename PdfTreeKeyAccess<TKey>::TLookup key);

        bool AddValue(const TKey& key, const PdfObject& value);

    public:
        class iterator final
        {
            friend class PdfTreeNode;

        public:
            iterator()
                : m_arr(nullptr), m_index(0), m_Pair{ } { }

        private:
            iterator(PdfArray& arr, unsigned index, const TKey& key, PdfObject& value)
                : m_arr(&arr), m_index(index), m_Pair(key, &value) { }

        public:
            const std::pair<TKey, PdfObject*>& operator*() const
            {
                return m_Pair;
            }

            const std::pair<TKey, PdfObject*>* operator->() const
            {
                return &m_Pair;
            }

            bool operator==(const iterator& it)
            {
                return m_arr == it.m_arr && m_index == it.m_index;
            }

            bool operator!=(const iterator& it)
            {
                return m_arr != it.m_arr || m_index != it.m_index;
            }

        private:
            PdfArray* m_arr;
            unsigned m_index;
            std::pair<TKey, PdfObject*> m_Pair;
        };

        iterator begin() const { return GetFirst(); }
        iterator end() const { return iterator(); }
        iterator GetFirst() const;
        iterator GetLast() const;

    private:
        enum class PdfNameLimits : uint8_t
        {
            Before = 0,
            Inside,
            After
        };

        void setLimits();

        bool rebalance();

        static PdfObject* getKeyValue(PdfObject& obj, typename PdfTreeKeyAccess<TKey>::TLookup key, const PdfIndirectObjectList& objects);

        static iterator getLeftMost(PdfDictionary& dict);

        static iterator getRightMost(PdfDictionary& dict);

        static PdfNameLimits checkLimits(const PdfObject& obj, typename PdfTreeKeyAccess<TKey>::TLookup key);

    private:
        PdfTreeNode* m_Parent;
        bool m_HasKids;
    };

    template<typename TKey>
    PdfObject* PdfTreeNode<TKey>::GetValue(typename PdfTreeKeyAccess<TKey>::TLookup key)
    {
        return getKeyValue(GetObject(), key, GetDocument().GetObjects());
    }

    template <typename TKey>
    bool PdfTreeNode<TKey>::AddValue(const TKey& key, const PdfObject& value)
    {
        if (!value.IsIndirect())
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidObject, "Input object must be indirect");

        if (m_HasKids)
        {
            auto& kids = GetDictionary().MustFindKey("Kids").GetArray();
            PdfObject* childObj = nullptr;
            PdfNameLimits limits = PdfNameLimits::Before;

            unsigned i = 0;
            for (; i < kids.GetSize(); i++)
            {
                childObj = kids.FindAt(i);
                if (childObj == nullptr)
                    PODOFO_RAISE_ERROR(PdfErrorCode::ObjectNotFound);

                limits = checkLimits(*childObj, key);
                if ((limits == PdfNameLimits::Before) ||
                    (limits == PdfNameLimits::Inside))
                {
                    break;
                }
            }

            if (i == kids.GetSize())
            {
                // not added, so add to last child
                childObj = kids.FindAt(i - 1);
                if (childObj == nullptr)
                    PODOFO_RAISE_ERROR(PdfErrorCode::ObjectNotFound);

                limits = PdfNameLimits::After;
            }

            PODOFO_ASSERT(childObj != nullptr);
            PdfTreeNode child(this, *childObj);
            if (child.AddValue(key, value))
            {
                // if a child insert the key in a way that the limits
                // are changed, we have to change our limits as well!
                // our parent has to change his parents too!
                if (limits != PdfNameLimits::Inside)
                    this->setLimits();

                this->rebalance();
                return true;
            }
            else
                return false;
        }
        else
        {
            bool rebalance = false;
            PdfArray limits;

            auto namesObj = GetDictionary().FindKey(PdfTreeKeyAccess<TKey>::GetKeyStoreNameStr());
            if (namesObj != nullptr)
            {
                auto& arr = namesObj->GetArray();
                PdfArray::iterator it = arr.begin();
                while (it != arr.end())
                {
                    if (PdfTreeKeyAccess<TKey>::Equals(*it, key))
                    {
                        // no need to write the key as it is anyways the same
                        it++;
                        // write the value
                        *it = value.GetIndirectReference();
                        break;
                    }
                    else if (PdfTreeKeyAccess<TKey>::GreaterThan(*it, key))
                    {
                        it = arr.insert(it, value.GetIndirectReference()); // arr.insert invalidates the iterator
                        it = arr.insert(it, key);
                        break;
                    }

                    it += 2;
                }

                if (it == arr.end())
                {
                    arr.Add(key);
                    arr.Add(value.GetIndirectReference());
                }

                limits.Add(*arr.begin());
                limits.Add(*(arr.end() - 2));
                rebalance = true;
            }
            else
            {
                // we create a completely new node
                PdfArray arr;
                arr.Add(key);
                arr.Add(value.GetIndirectReference());

                limits.Add(key);
                limits.Add(key);

                // create a child object
                auto& child = GetDocument().GetObjects().CreateDictionaryObject();
                child.GetDictionary().AddKey(PdfTreeKeyAccess<TKey>::GetKeyStoreName(), arr);
                child.GetDictionary().AddKey("Limits"_n, limits);

                PdfArray kids;
                kids.Add(child.GetIndirectReference());
                GetDictionary().AddKey("Kids"_n, kids);
                m_HasKids = true;
            }

            if (m_Parent != nullptr)
            {
                // Root node is not allowed to have a limits key!
                GetDictionary().AddKey("Limits"_n, limits);
            }

            if (rebalance)
                this->rebalance();

            return true;
        }
    }

    template<typename TKey>
    typename PdfTreeNode<TKey>::iterator PdfTreeNode<TKey>::GetFirst() const
    {
        return getLeftMost(const_cast<PdfTreeNode&>(*this).GetDictionary());
    }

    template<typename TKey>
    typename PdfTreeNode<TKey>::iterator PdfTreeNode<TKey>::GetLast() const
    {
        return getRightMost(const_cast<PdfTreeNode&>(*this).GetDictionary());
    }

    template <typename TKey>
    void PdfTreeNode<TKey>::setLimits()
    {
        PdfArray limits;
        PdfArray* arr;
        if (m_HasKids)
        {
            PdfDictionary* childDict;
            if (GetDictionary().TryFindKeyAs("Kids", arr))
            {
                PODOFO_ASSERT(arr->GetSize() != 0);

                PdfObject* limitsObj = nullptr;
                if (arr->TryFindAtAs(0, childDict)
                    && (limitsObj = childDict->FindKey("Limits")) != nullptr
                    && limitsObj->IsArray())
                {
                    limits.Add(limitsObj->GetArray().front());
                }

                if (arr->TryFindAtAs(arr->GetSize() - 1, childDict)
                    && (limitsObj = childDict->FindKey("Limits")) != nullptr
                    && limitsObj->IsArray())
                {
                    limits.Add(limitsObj->GetArray().back());
                }
            }
            else
            {
                PoDoFo::LogMessage(PdfLogSeverity::Error,
                    "Object {} {} R does not have Kids array",
                    GetObject().GetIndirectReference().ObjectNumber(),
                    GetObject().GetIndirectReference().GenerationNumber());
            }
        }
        else // has "Names
        {
            auto namesObj = GetDictionary().FindKey(PdfTreeKeyAccess<TKey>::GetKeyStoreNameStr());
            if (namesObj != nullptr && namesObj->TryGetArray(arr))
            {
                limits.Add(*arr->begin());
                limits.Add(*(arr->end() - 2));
            }
            else
                PoDoFo::LogMessage(PdfLogSeverity::Error,
                    "Object {} {} R does not have Names array",
                    GetObject().GetIndirectReference().ObjectNumber(),
                    GetObject().GetIndirectReference().GenerationNumber());
        }

        if (m_Parent != nullptr)
        {
            // Root node is not allowed to have a limits key!
            GetDictionary().AddKey("Limits"_n, limits);
        }
    }

    template <typename TKey>
    bool PdfTreeNode<TKey>::rebalance()
    {
        PdfArray& arr = m_HasKids
            ? GetDictionary().MustFindKey("Kids").GetArray()
            : GetDictionary().MustFindKey(PdfTreeKeyAccess<TKey>::GetKeyStoreNameStr()).GetArray();
        PdfName key = m_HasKids ? "Kids"_n : PdfTreeKeyAccess<TKey>::GetKeyStoreName();
        const unsigned arrLength = m_HasKids ? BalanceTreeMax : BalanceTreeMax * 2;

        if (arr.size() > arrLength)
        {
            PdfArray first;
            PdfArray second;
            PdfArray kids;

            first.insert(first.end(), arr.begin(), arr.begin() + (arrLength / 2) + 1);
            second.insert(second.end(), arr.begin() + (arrLength / 2) + 1, arr.end());

            PdfObject* child1;
            if (m_Parent == nullptr)
            {
                m_HasKids = true;
                child1 = &GetDocument().GetObjects().CreateDictionaryObject();
                GetDictionary().RemoveKey(PdfTreeKeyAccess<TKey>::GetKeyStoreNameStr());
            }
            else
            {
                child1 = &GetObject();
                kids = GetDictionary().MustFindKey("Kids").GetArray();
            }

            auto child2 = &GetDocument().GetObjects().CreateDictionaryObject();

            child1->GetDictionary().AddKey(key, first);
            child2->GetDictionary().AddKey(key, second);

            PdfArray::iterator it = kids.begin();
            while (it != kids.end())
            {
                if (it->GetReference() == child1->GetIndirectReference())
                {
                    it++;
                    it = kids.insert(it, child2->GetIndirectReference());
                    break;
                }

                it++;
            }

            if (it == kids.end())
            {
                kids.Add(child1->GetIndirectReference());
                kids.Add(child2->GetIndirectReference());
            }

            if (m_Parent == nullptr)
                GetDictionary().AddKey("Kids"_n, kids);
            else
                m_Parent->GetDictionary().AddKey("Kids"_n, kids);

            // Important is to the the limits
            // of the children first,
            // because SetLimits( parent )
            // depends on the /Limits key of all its children!
            PdfTreeNode(m_Parent != nullptr ? m_Parent : this, *child1).setLimits();
            PdfTreeNode(this, *child2).setLimits();

            // limits do only change if splitting name arrays
            if (m_HasKids)
                this->setLimits();
            else if (m_Parent != nullptr)
                m_Parent->setLimits();

            return true;
        }

        return false;
    }

    // Tests whether a key is in the range of a limits entry of a name tree node
    // \returns PdfNameLimits::Inside if the key is inside of the range
    // \returns PdfNameLimits::After if the key is greater than the specified range
    // \returns PdfNameLimits::Before if the key is smalelr than the specified range
    template <typename TKey>
    typename PdfTreeNode<TKey>::PdfNameLimits PdfTreeNode<TKey>::checkLimits(const PdfObject& obj, typename PdfTreeKeyAccess<TKey>::TLookup key)
    {
        auto limitsObj = obj.GetDictionary().FindKey("Limits");
        if (limitsObj != nullptr)
        {
            auto& limits = limitsObj->GetArray();

            if (PdfTreeKeyAccess<TKey>::GreaterThan(limits[0], key))
                return PdfNameLimits::Before;

            if (PdfTreeKeyAccess<TKey>::LessThan(limits[1], key))
                return PdfNameLimits::After;
        }
        else
        {
            PoDoFo::LogMessage(PdfLogSeverity::Debug, "Name tree object {} {} R does not have a limits key!",
                obj.GetIndirectReference().ObjectNumber(),
                obj.GetIndirectReference().GenerationNumber());
        }

        return PdfNameLimits::Inside;
    }

    // Recursively walk through the name tree and find the value for key.
    // \param obj the name tree
    // \param key the key to find a value for
    // \return the value for the key or nullptr if it was not found
    template <typename TKey>
    PdfObject* PdfTreeNode<TKey>::getKeyValue(PdfObject& obj, typename PdfTreeKeyAccess<TKey>::TLookup key, const PdfIndirectObjectList& objects)
    {
        if (checkLimits(obj, key) != PdfNameLimits::Inside)
            return nullptr;

        auto kidsObj = obj.GetDictionary().FindKey("Kids");
        if (kidsObj != nullptr)
        {
            auto& kids = kidsObj->GetArray();
            for (auto& child : kids)
            {
                auto childObj = objects.GetObject(child.GetReference());
                if (childObj == nullptr)
                {
                    PoDoFo::LogMessage(PdfLogSeverity::Debug, "Object {} {} R is child of nametree but was not found!",
                        child.GetReference().ObjectNumber(),
                        child.GetReference().GenerationNumber());
                }
                else
                {
                    auto result = getKeyValue(*childObj, key, objects);
                    if (result != nullptr)
                    {
                        // If recursive call returns nullptr, continue with
                        // the next element in the kids array.
                        return result;
                    }
                }
            }
        }
        else
        {
            PdfArray* namesArr;
            if (obj.GetDictionary().TryFindKeyAs(PdfTreeKeyAccess<TKey>::GetKeyStoreNameStr(), namesArr))
            {
                PdfArray::iterator it = namesArr->begin();

                // a names array is a set of PdfString/PdfObject pairs
                // so we loop in sets of two - getting each pair
                while (it != namesArr->end())
                {
                    if (PdfTreeKeyAccess<TKey>::Equals(*it, key))
                    {
                        it++;
                        if (it->IsReference())
                            return objects.GetObject((*it).GetReference());

                        return &(*it);
                    }

                    it += 2;
                }
            }
        }

        return nullptr;
    }

    template<typename TKey>
    typename PdfTreeNode<TKey>::iterator PdfTreeNode<TKey>::getLeftMost(PdfDictionary& dict)
    {
        auto kidsArr = dict.FindKeyAsSafe<PdfArray*>("Kids");
        if (kidsArr == nullptr)
        {
            auto valuesArr = dict.FindKeyAsSafe<PdfArray*>(PdfTreeKeyAccess<TKey>::GetKeyStoreNameStr());
            TKey key;
            PdfObject* obj;
            if (valuesArr == nullptr
                || valuesArr->GetSize() <= 1
                || (obj = valuesArr->FindAt(0)) == nullptr
                || !PdfTreeKeyAccess<TKey>::TryGetKey(*obj, key)
                || (obj = valuesArr->FindAt(1)) == nullptr)
            {
                return iterator();
            }

            return iterator(*valuesArr, 0, key, *obj);
        }
        else
        {
            PdfDictionary* childDict;
            if (kidsArr->GetSize() == 0
                || (childDict = kidsArr->FindAtAsSafe<PdfDictionary*>(0)) == nullptr)
            {
                return iterator();
            }

            return getLeftMost(*childDict);
        }
    }

    template<typename TKey>
    typename PdfTreeNode<TKey>::iterator PdfTreeNode<TKey>::getRightMost(PdfDictionary& dict)
    {
        auto kidsArr = dict.FindKeyAsSafe<PdfArray*>("Kids");
        unsigned size;
        if (kidsArr == nullptr)
        {
            auto valuesArr = dict.FindKeyAsSafe<PdfArray*>(PdfTreeKeyAccess<TKey>::GetKeyStoreNameStr());
            TKey key;
            PdfObject* obj;
            if (valuesArr == nullptr
                || (size = valuesArr->GetSize()) == 0
                || size % 2 == 1
                || (obj = valuesArr->FindAt(size - 2)) == nullptr
                || !PdfTreeKeyAccess<TKey>::TryGetKey(*obj, key)
                || (obj = valuesArr->FindAt(size - 1)) == nullptr)
            {
                return iterator();
            }

            return iterator(*valuesArr, (size / 2) - 1, key, *obj);
        }
        else
        {
            PdfDictionary* childDict;
            if ((size = kidsArr->GetSize()) == 0
                || (childDict = kidsArr->FindAtAsSafe<PdfDictionary*>(size - 1)) == nullptr)
            {
                return iterator();
            }

            return getRightMost(*childDict);
        }
    }

    // ISO 32000-2:2020 7.9.6 "Name trees"
    using PdfNameTreeNode = PdfTreeNode<PdfString>;

    // ISO 32000-2:2020 7.9.7 "Number trees"
    using PdfNumberTreeNode = PdfTreeNode<int64_t>;
}

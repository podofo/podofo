/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2024 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfNameTrees.h"

#include <podofo/auxiliary/OutputDevice.h>

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfFileSpec.h"
#include "PdfDestination.h"

using namespace PoDoFo;
using namespace std;

constexpr unsigned BalanceTreeMax = 65;

enum class PdfNameLimits
{
    Before = 0,
    Inside,
    After
};

static PdfNameLimits checkLimits(const PdfObject& obj, const string_view& key);
static PdfName getNameTreeTypeName(PdfKnownNameTree type);
static void enumerateValues(PdfObject& obj, const PdfIndirectObjectList& objects,
    const function<void(const PdfString&, PdfObject&)>& handleValue);
static PdfObject* getKeyValue(PdfObject& obj, const PdfString& key, const PdfIndirectObjectList& objects);

class PdfNameTreeNode : PdfDictionaryElement
{
public:
    PdfNameTreeNode(PdfNameTreeNode* parent, PdfObject& obj)
        : PdfDictionaryElement(obj), m_Parent(parent)
    {
        m_HasKids = GetDictionary().HasKey("Kids");
    }

    bool AddValue(const PdfString& key, const PdfObject& value);

    void SetLimits();

private:
    bool rebalance();

private:
    PdfNameTreeNode* m_Parent;
    bool m_HasKids;
};

bool PdfNameTreeNode::AddValue(const PdfString& key, const PdfObject& value)
{
    if (m_HasKids)
    {
        const PdfArray& kids = GetDictionary().MustFindKey("Kids").GetArray();
        auto it = kids.begin();
        PdfObject* childObj = nullptr;
        PdfNameLimits limits = PdfNameLimits::Before; // RG: TODO Compiler complains that this variable should be initialised

        while (it != kids.end())
        {
            childObj = GetDocument().GetObjects().GetObject((*it).GetReference());
            if (childObj == nullptr)
                PODOFO_RAISE_ERROR(PdfErrorCode::ObjectNotFound);

            limits = checkLimits(*childObj, key);
            if ((limits == PdfNameLimits::Before) ||
                (limits == PdfNameLimits::Inside))
            {
                break;
            }

            it++;
        }

        if (it == kids.end())
        {
            // not added, so add to last child
            childObj = GetDocument().GetObjects().GetObject(kids.back().GetReference());
            if (childObj == nullptr)
                PODOFO_RAISE_ERROR(PdfErrorCode::ObjectNotFound);

            limits = PdfNameLimits::After;
        }

        PODOFO_ASSERT(childObj != nullptr);
        PdfNameTreeNode child(this, *childObj);
        if (child.AddValue(key, value))
        {
            // if a child insert the key in a way that the limits
            // are changed, we have to change our limits as well!
            // our parent has to change his parents too!
            if (limits != PdfNameLimits::Inside)
                this->SetLimits();

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

        auto namesObj = GetDictionary().FindKey("Names");
        if (namesObj != nullptr)
        {
            auto& arr = namesObj->GetArray();
            PdfArray::iterator it = arr.begin();
            while (it != arr.end())
            {
                if (it->GetString() == key)
                {
                    // no need to write the key as it is anyways the same
                    it++;
                    // write the value
                    *it = value;
                    break;
                }
                else if (it->GetString().GetString() > key.GetString())
                {
                    it = arr.insert(it, value); // arr.insert invalidates the iterator
                    it = arr.insert(it, key);
                    break;
                }

                it += 2;
            }

            if (it == arr.end())
            {
                arr.Add(key);
                arr.Add(value);
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
            arr.Add(value);

            limits.Add(key);
            limits.Add(key);

            // create a child object
            auto& child = GetDocument().GetObjects().CreateDictionaryObject();
            child.GetDictionary().AddKey("Names"_n, arr);
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

void PdfNameTreeNode::SetLimits()
{
    PdfArray limits;

    if (m_HasKids)
    {
        auto kidsObj = GetDictionary().FindKey("Kids");
        if (kidsObj != nullptr && kidsObj->IsArray())
        {
            auto& kidsArr = kidsObj->GetArray();
            auto refFirst = kidsArr.front().GetReference();
            auto child = GetDocument().GetObjects().GetObject(refFirst);
            PdfObject* limitsObj = nullptr;
            if (child != nullptr
                && (limitsObj = child->GetDictionary().FindKey("Limits")) != nullptr
                && limitsObj->IsArray())
            {
                limits.Add(limitsObj->GetArray().front());
            }

            auto refLast = kidsArr.back().GetReference();
            child = GetDocument().GetObjects().GetObject(refLast);
            if (child != nullptr
                && (limitsObj = child->GetDictionary().FindKey("Limits")) != nullptr
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
        auto namesObj = GetDictionary().FindKey("Names");
        if (namesObj != nullptr && namesObj->IsArray())
        {
            auto& namesArr = namesObj->GetArray();
            limits.Add(*namesArr.begin());
            limits.Add(*(namesArr.end() - 2));
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

bool PdfNameTreeNode::rebalance()
{
    PdfArray& arr = m_HasKids
        ? GetDictionary().MustFindKey("Kids").GetArray()
        : GetDictionary().MustFindKey("Names").GetArray();
    PdfName key = m_HasKids ? "Kids"_n : "Names"_n;
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
            GetDictionary().RemoveKey("Names");
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
        PdfNameTreeNode(m_Parent != nullptr ? m_Parent : this, *child1).SetLimits();
        PdfNameTreeNode(this, *child2).SetLimits();

        // limits do only change if splitting name arrays
        if (m_HasKids)
            this->SetLimits();
        else if (m_Parent != nullptr)
            m_Parent->SetLimits();

        return true;
    }

    return false;
}


// NOTE: The NamesTree dict does NOT have a /Type key!
PdfNameTrees::PdfNameTrees(PdfDocument& doc)
    : PdfDictionaryElement(doc)
{
}

PdfNameTrees::PdfNameTrees(PdfObject& obj)
    : PdfDictionaryElement(obj)
{
}

void PdfNameTrees::AddValue(PdfKnownNameTree tree, const PdfString& key, const PdfObject& value)
{
    AddValue(getNameTreeTypeName(tree), key, value);
}

void PdfNameTrees::AddValue(const PdfName& treeName, const PdfString& key, const PdfObject& value)
{
    PdfNameTreeNode root(nullptr, this->getOrCreateRootNode(treeName));
    if (!root.AddValue(key, value))
        PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
}

const PdfObject* PdfNameTrees::GetValue(PdfKnownNameTree tree, const string_view& key) const
{
    return getValue(getNameTreeTypeName(tree), key);
}

const PdfObject* PdfNameTrees::GetValue(const string_view& treeName, const string_view& key) const
{
    return getValue(treeName, key);
}

PdfObject* PdfNameTrees::GetValue(PdfKnownNameTree tree, const string_view& key)
{
    return getValue(getNameTreeTypeName(tree), key);
}

PdfObject* PdfNameTrees::GetValue(const string_view& treeName, const string_view& key)
{
    return getValue(treeName, key);
}

bool PdfNameTrees::HasKey(PdfKnownNameTree tree, const string_view& key) const
{
    return getValue(getNameTreeTypeName(tree), key) != nullptr;
}

bool PdfNameTrees::HasKey(const string_view& treeName, const string_view& key) const
{
    return getValue(treeName, key) != nullptr;
}

void PdfNameTrees::ToDictionary(PdfKnownNameTree tree, PdfStringMap<PdfObject>& dict, bool skipClear) const
{
    ToDictionary(getNameTreeTypeName(tree), dict, skipClear);
}

void PdfNameTrees::ToDictionary(const string_view& treeName, PdfStringMap<PdfObject>& dict, bool skipClear) const
{
    if (!skipClear)
        dict.clear();

    auto obj = this->getRootNode(treeName);
    auto handleValue = [&dict](const PdfString& name, PdfObject& obj)
    {
        dict[name] = obj;
    };
    if (obj != nullptr)
        enumerateValues(*obj, GetDocument().GetObjects(), handleValue);
}

// Recursively walk through the name tree and find the value for key.
// \param obj the name tree
// \param key the key to find a value for
// \return the value for the key or nullptr if it was not found
PdfObject* getKeyValue(PdfObject& obj, const PdfString& key, const PdfIndirectObjectList& objects)
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
        if (obj.GetDictionary().TryFindKeyAs("Names", namesArr))
        {
            PdfArray::iterator it = namesArr->begin();

            // a names array is a set of PdfString/PdfObject pairs
            // so we loop in sets of two - getting each pair
            while (it != namesArr->end())
            {
                if (it->GetString() == key)
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

PdfNameTreeBase* PdfNameTrees::getNameTree(PdfKnownNameTree tree) const
{
    if (m_Trees[(unsigned)tree - 1] != nullptr)
        return (*const_cast<PdfNameTrees&>(*this).m_Trees[(unsigned)tree - 1]).get();

    switch (tree)
    {
        case PdfKnownNameTree::Dests:
        {
            auto destsObj = const_cast<PdfNameTrees&>(*this).GetDictionary().FindKey("Dests");
            if (destsObj == nullptr)
            {
                const_cast<PdfNameTrees&>(*this).m_Trees[(unsigned)tree - 1] = unique_ptr<PdfDestinations>();
                return nullptr;
            }
            else
            {
                auto ret = new PdfDestinations(*destsObj);
                const_cast<PdfNameTrees&>(*this).m_Trees[(unsigned)tree - 1] = unique_ptr<PdfDestinations>(ret);
                return ret;
            }

            break;
        }
        case PdfKnownNameTree::EmbeddedFiles:
        {
            auto embeddedFilesObj = const_cast<PdfNameTrees&>(*this).GetDictionary().FindKey("EmbeddedFiles");
            if (embeddedFilesObj == nullptr)
            {
                const_cast<PdfNameTrees&>(*this).m_Trees[(unsigned)tree - 1] = unique_ptr<PdfEmbeddedFiles>();
                return nullptr;
            }
            else
            {
                auto ret = new PdfEmbeddedFiles(*embeddedFilesObj);
                const_cast<PdfNameTrees&>(*this).m_Trees[(unsigned)tree - 1] = unique_ptr<PdfEmbeddedFiles>(ret);
                return ret;
            }

            break;
        }
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

PdfNameTreeBase& PdfNameTrees::mustGetNameTree(PdfKnownNameTree tree) const
{
    auto ret = getNameTree(tree);
    if (ret == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Destinations are not present");

    return *ret;
}

PdfNameTreeBase& PdfNameTrees::getOrCreateNameTree(PdfKnownNameTree type)
{
    auto ret = getNameTree(type);
    if (ret != nullptr)
        return *ret;

    switch (type)
    {
        case PdfKnownNameTree::Dests:
        {
            auto tree = new PdfDestinations(GetDocument());
            m_Trees[(unsigned)type - 1] = unique_ptr<PdfDestinations>(tree);
            GetDictionary().AddKey("Dests"_n, tree->GetObject().GetIndirectReference());
            return *tree;
        }
        case PdfKnownNameTree::EmbeddedFiles:
        {
            auto tree = new PdfEmbeddedFiles(GetDocument());
            m_Trees[(unsigned)type - 1] = unique_ptr<PdfEmbeddedFiles>(tree);
            GetDictionary().AddKey("EmbeddedFiles"_n, tree->GetObject().GetIndirectReference());
            return *tree;
        }
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

PdfObject* PdfNameTrees::getRootNode(const string_view& treeName) const
{
    return const_cast<PdfNameTrees&>(*this).GetDictionary().FindKey(treeName);
}

PdfObject& PdfNameTrees::getOrCreateRootNode(const PdfName& treeName)
{
    auto rootNode = GetDictionary().FindKey(treeName);
    if (rootNode == nullptr)
    {
        rootNode = &GetDocument().GetObjects().CreateDictionaryObject();
        GetDictionary().AddKey(treeName, rootNode->GetIndirectReference());
    }

    return *rootNode;
}

PdfObject* PdfNameTrees::getValue(const string_view& name, const string_view& key) const
{
    auto obj = this->getRootNode(name);
    if (obj == nullptr)
        return nullptr;

    return getKeyValue(*obj, key, GetDocument().GetObjects());
}

PdfNameTreeOperations::PdfNameTreeOperations() { }

PdfNameTreeBase::PdfNameTreeBase(PdfDocument& doc)
    : PdfDictionaryElement(doc)
{
}

PdfNameTreeBase::PdfNameTreeBase(PdfObject& obj)
    : PdfDictionaryElement(obj)
{
}

void PdfNameTreeBase::AddValue(const PdfString& key, shared_ptr<PdfElement>&& value)
{
    if (value == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The value must be non null");

    PdfNameTreeNode root(nullptr, GetObject());
    if (!root.AddValue(key, value->GetObject()))
        PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);

    m_cache[key] = std::move(value);
}

PdfElement* PdfNameTreeBase::GetValue(const string_view& key) const
{
    auto found = m_cache.find(key);
    if (found != m_cache.end())
        return found->second.get();

    auto valueObj = getKeyValue(const_cast<PdfNameTreeBase&>(*this).GetObject(), key, GetDocument().GetObjects());
    if (valueObj == nullptr)
        return nullptr;

    auto element = createElement(*valueObj);
    auto ret = element.get();
    const_cast<PdfNameTreeBase&>(*this).m_cache[key] = std::move(element);
    return ret;
}

bool PdfNameTreeBase::HasKey(const string_view& key) const
{
    auto found = m_cache.find(key);
    if (found != m_cache.end())
        return true;

    return getKeyValue(const_cast<PdfNameTreeBase&>(*this).GetObject(), key, GetDocument().GetObjects()) != nullptr;
}

void PdfNameTreeBase::ToDictionary(PdfStringMap<shared_ptr<PdfElement>>& dict, bool skipClear)
{
    if (!skipClear)
        dict.clear();

    auto handleValue = [&](const PdfString& name, PdfObject& obj)
    {
        auto found = m_cache.find(name);
        if (found != m_cache.end())
        {
            dict[name] = found->second;
            return;
        }

        dict[name] = createElement(obj);
    };

    enumerateValues(GetObject(), GetDocument().GetObjects(), handleValue);
}

unique_ptr<PdfElement> PdfNameTreeBase::createElement(PdfObject& obj) const
{
    switch (GetType())
    {
        case PdfKnownNameTree::EmbeddedFiles:
            return unique_ptr<PdfElement>(new PdfFileSpec(obj));
        case PdfKnownNameTree::Dests:
            return unique_ptr<PdfElement>(new PdfDestination(obj));
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Unsupported type");
    }
}

void enumerateValues(PdfObject& obj, const PdfIndirectObjectList& objects,
    const function<void(const PdfString&, PdfObject&)>& handleValue)
{
    utls::RecursionGuard guard;
    auto kidsObj = obj.GetDictionary().FindKey("Kids");
    PdfObject* namesObj;
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
                enumerateValues(*childObj, objects, handleValue);
            }
        }
    }
    else if ((namesObj = obj.GetDictionary().FindKey("Names")) != nullptr)
    {
        auto& names = namesObj->GetArray();
        auto it = names.begin();

        // a names array is a set of PdfString/PdfObject pairs
        // so we loop in sets of two - getting each pair
        while (it != names.end())
        {
            // convert all strings into names
            auto& name = it->GetString();
            it++;
            if (it == names.end())
            {
                PoDoFo::LogMessage(PdfLogSeverity::Warning,
                    "No reference in /Names array last element in "
                    "object {} {} R, possible exploit attempt!",
                    obj.GetIndirectReference().ObjectNumber(),
                    obj.GetIndirectReference().GenerationNumber());
                break;
            }

            PdfObject* found = nullptr;
            if (it->IsReference())
                found = objects.GetObject((*it).GetReference());

            if (found == nullptr)
                handleValue(name, *it);
            else
                handleValue(name, *found);

            it++;
        }
    }
}

// Tests whether a key is in the range of a limits entry of a name tree node
// \returns PdfNameLimits::Inside if the key is inside of the range
// \returns PdfNameLimits::After if the key is greater than the specified range
// \returns PdfNameLimits::Before if the key is smalelr than the specified range
PdfNameLimits checkLimits(const PdfObject& obj, const string_view& key)
{
    auto limitsObj = obj.GetDictionary().FindKey("Limits");
    if (limitsObj != nullptr)
    {
        auto& limits = limitsObj->GetArray();

        if (limits[0].GetString().GetString() > key)
            return PdfNameLimits::Before;

        if (limits[1].GetString().GetString() < key)
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

PdfName getNameTreeTypeName(PdfKnownNameTree type)
{
    switch (type)
    {
        case PdfKnownNameTree::Dests:
            return "Dests"_n;
        case PdfKnownNameTree::AP:
            return "AP"_n;
        case PdfKnownNameTree::JavaScript:
            return "JavaScript"_n;
        case PdfKnownNameTree::Pages:
            return "Pages"_n;
        case PdfKnownNameTree::Templates:
            return "Templates"_n;
        case PdfKnownNameTree::IDS:
            return "IDS"_n;
        case PdfKnownNameTree::URLS:
            return "URLS"_n;
        case PdfKnownNameTree::EmbeddedFiles:
            return "EmbeddedFiles"_n;
        case PdfKnownNameTree::AlternatePresentations:
            return "AlternatePresentations"_n;
        case PdfKnownNameTree::Renditions:
            return "Renditions"_n;
        case PdfKnownNameTree::Unknown:
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

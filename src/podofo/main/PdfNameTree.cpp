/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfNameTree.h"

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfOutputDevice.h"

using namespace PoDoFo;

#define BALANCE_TREE_MAX 65

/*
#define BALANCE_TREE_MAX 9
*/

class PdfNameTreeNode
{
public:
    PdfNameTreeNode(PdfNameTreeNode* parent, PdfObject* obj)
        : m_Parent(parent), m_Object(obj)
    {
        m_HasKids = m_Object->GetDictionary().HasKey("Kids");
    }

    bool AddValue(const PdfString& key, const PdfObject& value);

    void SetLimits();

    inline PdfObject* GetObject() { return m_Object; }

private:
    bool Rebalance();

private:
    PdfNameTreeNode* m_Parent;
    PdfObject* m_Object;

    bool m_HasKids;
};

bool PdfNameTreeNode::AddValue(const PdfString& key, const PdfObject& value)
{
    if (m_HasKids)
    {
        const PdfArray& kids = this->GetObject()->GetDictionary().MustFindKey("Kids").GetArray();
        auto it = kids.begin();
        PdfObject* childObj = nullptr;
        PdfNameLimits limits = PdfNameLimits::Before; // RG: TODO Compiler complains that this variable should be initialised

        while (it != kids.end())
        {
            childObj = this->GetObject()->GetDocument()->GetObjects().GetObject((*it).GetReference());
            if (childObj == nullptr)
                PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

            limits = PdfNameTree::CheckLimits(*childObj, key);
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
            childObj = this->GetObject()->GetDocument()->GetObjects().GetObject(kids.back().GetReference());
            if (childObj == nullptr)
                PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

            limits = PdfNameLimits::After;
        }

        PdfNameTreeNode child(this, childObj);
        if (child.AddValue(key, value))
        {
            // if a child insert the key in a way that the limits
            // are changed, we have to change our limits as well!
            // our parent has to change his parents too!
            if (limits != PdfNameLimits::Inside)
                this->SetLimits();

            this->Rebalance();
            return true;
        }
        else
            return false;
    }
    else
    {
        bool rebalance = false;
        PdfArray limits;

        if (this->GetObject()->GetDictionary().HasKey("Names"))
        {
            auto& array = this->GetObject()->GetDictionary().MustFindKey("Names").GetArray();
            PdfArray::iterator it = array.begin();
            while (it != array.end())
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
                    it = array.insert(it, value); // array.insert invalidates the iterator
                    it = array.insert(it, key);
                    break;
                }

                it += 2;
            }

            if (it == array.end())
            {
                array.Add(key);
                array.Add(value);
            }

            limits.Add(*array.begin());
            limits.Add(*(array.end() - 2));
            rebalance = true;
        }
        else
        {
            // we create a completely new node
            PdfArray array;
            array.Add(key);
            array.Add(value);

            limits.Add(key);
            limits.Add(key);

            // create a child object
            auto& child = this->GetObject()->GetDocument()->GetObjects().CreateDictionaryObject();
            child.GetDictionary().AddKey("Names", array);
            child.GetDictionary().AddKey("Limits", limits);

            PdfArray kids;
            kids.Add(child.GetIndirectReference());
            this->GetObject()->GetDictionary().AddKey("Kids", kids);
            m_HasKids = true;
        }

        if (m_Parent != nullptr)
        {
            // Root node is not allowed to have a limits key!
            this->GetObject()->GetDictionary().AddKey("Limits", limits);
        }

        if (rebalance)
            this->Rebalance();

        return true;
    }
}

void PdfNameTreeNode::SetLimits()
{
    PdfArray limits;

    if (m_HasKids)
    {
        auto kidsObj = this->GetObject()->GetDictionary().FindKey("Kids");
        if (kidsObj != nullptr && kidsObj->IsArray())
        {
            auto& kidsArr = kidsObj->GetArray();
            auto refFirst = kidsArr.front().GetReference();
            auto child = this->GetObject()->GetDocument()->GetObjects().GetObject(refFirst);
            PdfObject* limitsObj = nullptr;
            if (child != nullptr
                && (limitsObj = child->GetDictionary().FindKey("Limits")) != nullptr
                && limitsObj->IsArray())
            {
                limits.Add(limitsObj->GetArray().front());
            }

            auto refLast = kidsArr.back().GetReference();
            child = this->GetObject()->GetDocument()->GetObjects().GetObject(refLast);
            if (child != nullptr && child->GetDictionary().HasKey("Limits")
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
                this->GetObject()->GetIndirectReference().ObjectNumber(),
                this->GetObject()->GetIndirectReference().GenerationNumber());
        }
    }
    else // has "Names"
    {
        auto namesObj = this->GetObject()->GetDictionary().FindKey("Names");
        if (namesObj != nullptr && namesObj->IsArray())
        {
            auto& namesArr = namesObj->GetArray();
            limits.Add(*namesArr.begin());
            limits.Add(*(namesArr.end() - 2));
        }
        else
            PoDoFo::LogMessage(PdfLogSeverity::Error,
                "Object {} {} R does not have Names array",
                this->GetObject()->GetIndirectReference().ObjectNumber(),
                this->GetObject()->GetIndirectReference().GenerationNumber());
    }

    if (m_Parent != nullptr)
    {
        // Root node is not allowed to have a limits key!
        this->GetObject()->GetDictionary().AddKey("Limits", limits);
    }
}

bool PdfNameTreeNode::Rebalance()
{
    PdfArray& arr = m_HasKids
        ? this->GetObject()->GetDictionary().MustFindKey("Kids").GetArray()
        : this->GetObject()->GetDictionary().MustFindKey("Names").GetArray();
    PdfName key = m_HasKids ? PdfName("Kids") : PdfName("Names");
    const unsigned arrLength = m_HasKids ? BALANCE_TREE_MAX : BALANCE_TREE_MAX * 2;

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
            child1 = &this->GetObject()->GetDocument()->GetObjects().CreateDictionaryObject();
            this->GetObject()->GetDictionary().RemoveKey("Names");
        }
        else
        {
            child1 = this->GetObject();
            kids = m_Parent->GetObject()->GetDictionary().MustFindKey("Kids").GetArray();
        }

        auto child2 = &this->GetObject()->GetDocument()->GetObjects().CreateDictionaryObject();

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
            this->GetObject()->GetDictionary().AddKey("Kids", kids);
        else
            m_Parent->GetObject()->GetDictionary().AddKey("Kids", kids);

        // Important is to the the limits
        // of the children first,
        // because SetLimits( parent )
        // depends on the /Limits key of all its children!
        PdfNameTreeNode(m_Parent != nullptr ? m_Parent : this, child1).SetLimits();
        PdfNameTreeNode(this, child2).SetLimits();

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
PdfNameTree::PdfNameTree(PdfDocument& doc)
    : PdfDictionaryElement(doc)
{
}

PdfNameTree::PdfNameTree(PdfObject& obj)
    : PdfDictionaryElement(obj)
{
}

void PdfNameTree::AddValue(const PdfName& tree, const PdfString& key, const PdfObject& value)
{
    PdfNameTreeNode root(nullptr, this->GetRootNode(tree, true));
    if (!root.AddValue(key, value))
        PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
}

PdfObject* PdfNameTree::GetValue(const PdfName& tree, const PdfString& key) const
{
    auto obj = this->GetRootNode(tree);
    PdfObject* result = nullptr;

    if (obj != nullptr)
    {
        result = this->GetKeyValue(*obj, key);
        if (result != nullptr && result->IsReference())
            result = this->GetObject().GetDocument()->GetObjects().GetObject(result->GetReference());
    }

    return result;
}

PdfObject* PdfNameTree::GetKeyValue(PdfObject& obj, const PdfString& key) const
{
    if (PdfNameTree::CheckLimits(obj, key) != PdfNameLimits::Inside)
        return nullptr;

    if (obj.GetDictionary().HasKey("Kids"))
    {
        auto& kids = obj.GetDictionary().MustFindKey("Kids").GetArray();
        for (auto& child : kids)
        {
            auto childObj = this->GetObject().GetDocument()->GetObjects().GetObject(child.GetReference());
            if (childObj == nullptr)
            {
                PoDoFo::LogMessage(PdfLogSeverity::Debug, "Object {} {} R is child of nametree but was not found!",
                    child.GetReference().ObjectNumber(),
                    child.GetReference().GenerationNumber());
            }
            else
            {
                auto result = GetKeyValue(*childObj, key);
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
        auto& names = obj.GetDictionary().MustFindKey("Names").GetArray();
        PdfArray::iterator it = names.begin();

        // a names array is a set of PdfString/PdfObject pairs
        // so we loop in sets of two - getting each pair
        while (it != names.end())
        {
            if (it->GetString() == key)
            {
                it++;
                if (it->IsReference())
                    return this->GetObject().GetDocument()->GetObjects().GetObject((*it).GetReference());

                return &(*it);
            }

            it += 2;
        }

    }

    return nullptr;
}

PdfObject* PdfNameTree::GetRootNode(const PdfName& name, bool create) const
{
    auto& obj = const_cast<PdfNameTree&>(*this).GetObject();
    auto rootNode = obj.GetDictionary().FindKey(name);
    if (rootNode == nullptr && create)
    {
        rootNode = &obj.GetDocument()->GetObjects().CreateDictionaryObject();
        obj.GetDictionary().AddKey(name, rootNode->GetIndirectReference());
    }

    return rootNode;
}

bool PdfNameTree::HasValue(const PdfName& tree, const PdfString& key) const
{
    return this->GetValue(tree, key) != nullptr;
}

PdfNameLimits PdfNameTree::CheckLimits(const PdfObject& obj, const PdfString& key)
{
    if (obj.GetDictionary().HasKey("Limits"))
    {
        auto& limits = obj.GetDictionary().MustFindKey("Limits").GetArray();

        if (limits[0].GetString().GetString() > key.GetString())
            return PdfNameLimits::Before;

        if (limits[1].GetString().GetString() < key.GetString())
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

void PdfNameTree::ToDictionary(const PdfName& tree, PdfDictionary& dict)
{
    dict.Clear();
    auto obj = this->GetRootNode(tree);
    if (obj != nullptr)
        AddToDictionary(*obj, dict);
}

void PdfNameTree::AddToDictionary(PdfObject& obj, PdfDictionary& dict)
{
    utls::RecursionGuard guard;
    if (obj.GetDictionary().HasKey("Kids"))
    {
        auto& kids = obj.GetDictionary().MustFindKey("Kids").GetArray();
        for (auto& child : kids)
        {
            auto childObj = this->GetObject().GetDocument()->GetObjects().GetObject(child.GetReference());
            if (childObj == nullptr)
            {
                PoDoFo::LogMessage(PdfLogSeverity::Debug, "Object {} {} R is child of nametree but was not found!",
                    child.GetReference().ObjectNumber(),
                    child.GetReference().GenerationNumber());
            }
            else
            {
                this->AddToDictionary(*childObj, dict);
            }
        }
    }
    else if (obj.GetDictionary().HasKey("Names"))
    {
        auto& names = obj.GetDictionary().MustFindKey("Names").GetArray();
        auto it = names.begin();

        // a names array is a set of PdfString/PdfObject pairs
        // so we loop in sets of two - getting each pair
        while (it != names.end())
        {
            // convert all strings into names 
            PdfName name(it->GetString().GetString());
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
            dict.AddKey(name, *it);
            it++;
        }
    }
}

PdfObject* PdfNameTree::GetJavaScriptNode(bool create) const
{
    return this->GetRootNode("JavaScript", create);
}

PdfObject* PdfNameTree::GetDestsNode(bool create) const
{
    return this->GetRootNode("Dests", create);
}

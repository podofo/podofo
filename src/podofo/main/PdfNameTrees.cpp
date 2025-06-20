/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2024 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfNameTrees.h"

#include <podofo/auxiliary/OutputDevice.h>
#include <podofo/private/PdfTreeNode.h>

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfFileSpec.h"
#include "PdfDestination.h"

using namespace PoDoFo;
using namespace std;

static PdfName getNameTreeTypeName(PdfKnownNameTree type);
static void enumerateValues(PdfObject& obj, const PdfIndirectObjectList& objects,
    const function<void(const PdfString&, PdfObject&)>& handleValue);

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
                const_cast<PdfNameTrees&>(*this).m_Trees[(unsigned)tree - 1] *= nullptr;
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
                const_cast<PdfNameTrees&>(*this).m_Trees[(unsigned)tree - 1] *= nullptr;
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

    PdfNameTreeNode node(nullptr, *obj);
    return node.GetValue(key);
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

    PdfNameTreeNode node(nullptr, const_cast<PdfNameTreeBase&>(*this).GetObject());
    auto valueObj = node.GetValue(key);
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

    PdfNameTreeNode node(nullptr, const_cast<PdfNameTreeBase&>(*this).GetObject());
    return node.GetValue(key) != nullptr;
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

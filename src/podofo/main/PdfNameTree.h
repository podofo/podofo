/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2024 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_NAME_TREE_H
#define PDF_NAME_TREE_H

#include "PdfElement.h"

namespace PoDoFo {

class PdfDocument;
class PdfFileSpec;
class PdfDestination;

class PODOFO_API PdfNameTreeBase : public PdfDictionaryElement
{
    template <typename TElement>
    friend class PdfNameTree;

private:
    PdfNameTreeBase(PdfDocument& doc);
    PdfNameTreeBase(PdfObject& obj);

public:
    bool HasKey(const std::string_view& key) const;

protected:
    void AddValue(const PdfString& key, std::shared_ptr<PdfElement>&& value);

    PdfElement* GetValue(const std::string_view& key) const;

    void ToDictionary(PdfStringMap<std::shared_ptr<PdfElement>>& dict, bool skipClear);

    virtual PdfKnownNameTree GetType() const = 0;

private:
    std::unique_ptr<PdfElement> createElement(PdfObject& obj) const;

private:
    PdfStringHashMap<std::shared_ptr<PdfElement>> m_cache;
};

template <typename TElement>
class PdfNameTree final : public PdfNameTreeBase
{
    friend class PdfNameTrees;

private:
    PdfNameTree(PdfDocument& doc)
        : PdfNameTreeBase(doc) { }

    PdfNameTree(PdfObject& obj)
        : PdfNameTreeBase(obj) { }

public:
    using Map = PdfStringMap<std::shared_ptr<TElement>>;

public:
    void AddValue(const PdfString& key, std::shared_ptr<TElement> value)
    {
        PdfNameTreeBase::AddValue(key, std::move(reinterpret_cast<std::shared_ptr<PdfElement>&>(value)));
    }

    TElement* GetValue(const std::string_view& key)
    {
        return static_cast<TElement*>(PdfNameTreeBase::GetValue(key));
    }

    const TElement* GetValue(const std::string_view& key) const
    {
        return static_cast<const TElement*>(PdfNameTreeBase::GetValue(key));
    }

    void ToDictionary(Map& dict, bool skipClear = false)
    {
        PdfNameTreeBase::ToDictionary(reinterpret_cast<PdfStringMap<std::shared_ptr<PdfElement>>&>(dict), skipClear);
    }

protected:
    PdfKnownNameTree GetType() const override
    {
        return getType();
    }

private:
    static constexpr PdfKnownNameTree getType()
    {
        if (std::is_same_v<TElement, PdfFileSpec>)
            return PdfKnownNameTree::EmbeddedFiles;
        else if (std::is_same_v<TElement, PdfDestination>)
            return PdfKnownNameTree::Dests;
        else
            return PdfKnownNameTree::Unknown;
    }
};

// TODO: Add more trees
using PdfDestinations = PdfNameTree<PdfDestination>;
using PdfEmbeddedFiles = PdfNameTree<PdfFileSpec>;

};

#endif // PDF_NAME_TREE_H

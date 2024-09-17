/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2024 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_NAME_TREES_H
#define PDF_NAME_TREES_H

#include "PdfNameTree.h"
#include "PdfNameTreeOperations.h"

namespace PoDoFo {

/**
 * Interface to access names trees in the document
 * \remarks Prefer accessing trees through GetNameTree<TNameTree>
 * or similars. You can cast the instance to PdfNameTreeOperations
 * to access low level mutable operations
 */
class PODOFO_API PdfNameTrees final : public PdfDictionaryElement, public PdfNameTreeOperations
{
    friend class PdfDocument;

    template <typename TNameTree>
    struct TreeGetter
    {
        static PdfKnownNameTree GetType()
        {
            static_assert(always_false<TNameTree>, "Unsupported type");
            return PdfKnownNameTree::Unknown;
        }
    };

    template <>
    struct TreeGetter<PdfDestinations>
    {
        static PdfKnownNameTree GetType()
        {
            return PdfKnownNameTree::Dests;
        }
    };

    template <>
    struct TreeGetter<PdfEmbeddedFiles>
    {
        static PdfKnownNameTree GetType()
        {
            return PdfKnownNameTree::EmbeddedFiles;
        }
    };

private:
    /** Create a new PdfNameTrees object
     *  \param parent parent of this action
     */
    PdfNameTrees(PdfDocument& doc);

    /** Create a PdfNameTrees object from an existing PdfObject
     *	\param obj the object to create from
     *  \param pCatalog the Catalog dictionary of the owning PDF
     */
    PdfNameTrees(PdfObject& obj);

public:
    template <typename TNameTree>
    TNameTree* GetTree()
    {
        return static_cast<TNameTree*>(getNameTree(TreeGetter<TNameTree>::GetType()));
    }

    template <typename TNameTree>
    const TNameTree* GetTree() const
    {
        return static_cast<const TNameTree*>(getNameTree(TreeGetter<TNameTree>::GetType()));
    }

    template <typename TNameTree>
    TNameTree& MustGetTree()
    {
        return static_cast<TNameTree&>(mustGetNameTree(TreeGetter<TNameTree>::GetType()));
    }

    template <typename TNameTree>
    const TNameTree& MustGetTree() const
    {
        return static_cast<const TNameTree&>(mustGetNameTree(TreeGetter<TNameTree>::GetType()));
    }

    template <typename TNameTree>
    TNameTree& GetOrCreateTree()
    {
        return static_cast<TNameTree&>(getOrCreateNameTree(TreeGetter<TNameTree>::GetType()));
    }

public:
    const PdfObject* GetValue(PdfKnownNameTree tree, const std::string_view& key) const override;
    PdfObject* GetValue(PdfKnownNameTree tree, const std::string_view& key) override;
    bool HasKey(PdfKnownNameTree tree, const std::string_view& key) const override;
    void ToDictionary(PdfKnownNameTree tree, PdfStringMap<PdfObject>& dict, bool skipClear = false) const override;

private:
    void AddValue(PdfKnownNameTree tree, const PdfString& key, const PdfObject& value) override;
    void AddValue(const PdfName& treeName, const PdfString& key, const PdfObject& value) override;
    const PdfObject* GetValue(const std::string_view& treeName, const std::string_view& key) const override;
    PdfObject* GetValue(const std::string_view& treeName, const std::string_view& key) override;
    bool HasKey(const std::string_view& treeName, const std::string_view& key) const override;
    void ToDictionary(const std::string_view& treeName, PdfStringMap<PdfObject>& dict, bool skipClear = false) const override;

private:
    /** Get a PdfNameTrees root node for a certain name.
     *  \param treeName that identifies a specific name tree.
     *         Valid names are:
     *            - Dests
     *            - AP
     *            - JavaScript
     *            - Pages
     *            - Templates
     *            - IDS
     *            - URLS
     *            - EmbeddedFiles
     *            - AlternatePresentations
     *            - Renditions
     *
     *  \param create if true the root node is created if it does not exists.
     *  \returns the root node of the tree or nullptr if it does not exists
     */
    PdfObject* getRootNode(const std::string_view& treeName) const;
    PdfObject& getOrCreateRootNode(const PdfName& treeName);

    PdfObject* getValue(const std::string_view& name, const std::string_view& key) const;

    PdfNameTreeBase* getNameTree(PdfKnownNameTree tree) const;

    PdfNameTreeBase& mustGetNameTree(PdfKnownNameTree tree) const;

    PdfNameTreeBase& getOrCreateNameTree(PdfKnownNameTree tree);

private:
    nullable<std::unique_ptr<PdfNameTreeBase>> m_Trees[(unsigned)PdfKnownNameTree::Renditions];
};

};

#endif // PDF_NAME_TREES_H

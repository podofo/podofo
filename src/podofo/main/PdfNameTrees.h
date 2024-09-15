/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2024 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_NAME_TREE_H
#define PDF_NAME_TREE_H

#include "PdfElement.h"

namespace PoDoFo {

class PdfDictionary;

enum class PdfKnownNameTree
{
    Unknown = 0,
    Dests,
    AP,
    JavaScript,
    Pages,
    Templates,
    IDS,
    URLS,
    EmbeddedFiles,
    AlternatePresentations,
    Renditions,
};

class PODOFO_API PdfNameTrees final : public PdfDictionaryElement
{
    friend class PdfDocument;

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
    /** Insert a key and value in one of the dictionaries of the name tree.
     *  \param tree name of the tree to search for the key.
     *  \param key the key to insert. If it exists, it will be overwritten.
     *  \param value the value to insert.
     */
    void AddValue(PdfKnownNameTree tree, const PdfString& key, const PdfObject& value);
    void AddValue(const PdfName& treeName, const PdfString& key, const PdfObject& value);

    /** Get the object referenced by a string key in one of the dictionaries
     *  of the name tree.
     *  \param tree name of the tree to search for the key.
     *  \param key the key to search for
     *  \returns the value of the key or nullptr if the key was not found.
     *           if the value is a reference, the object referenced by
     *           this reference is returned.
     */
    const PdfObject* GetValue(PdfKnownNameTree tree, const std::string_view& key) const;
    const PdfObject* GetValue(const std::string_view& treeName, const std::string_view& key) const;
    PdfObject* GetValue(PdfKnownNameTree tree, const std::string_view& key);
    PdfObject* GetValue(const std::string_view& treeName, const std::string_view& key);

    /** Tests whether a certain nametree has a value.
     *
     *  It is generally faster to use GetValue and check for nullptr
     *  as return value.
     *
     *  \param tree name of the tree to search for the key.
     *  \param key name of the key to look for
     *  \returns true if the dictionary has such a key.
     */
    bool HasValue(PdfKnownNameTree tree, const std::string_view& key) const;
    bool HasValue(const std::string_view& treeName, const std::string_view& key) const;

    /**
     * Adds all keys and values from a name tree to a dictionary.
     * Removes all keys that have been previously in the dictionary.
     *
     * \param tree the name of the tree to convert into a dictionary
     * \param dict add all keys and values to this dictionary
     * \param skipClear skip clearing the output dictionary
     */
    void ToDictionary(PdfKnownNameTree tree, PdfDictionary& dict, bool skipClear = false);
    void ToDictionary(const std::string_view& treeName, PdfDictionary& dict, bool skipClear = false);

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

    /** Recursively walk through the name tree and find the value for key.
     *  \param obj the name tree
     *  \param key the key to find a value for
     *  \return the value for the key or nullptr if it was not found
     */
    PdfObject* getKeyValue(PdfObject& obj, const PdfString& key) const;

    /**
     *  Add all keys and values from an object and its children to a dictionary.
     *  \param obj a pdf name tree node
     *  \param dict a dictionary
     */
    void addToDictionary(PdfObject& obj, PdfDictionary& dict);
};

};

#endif // PDF_NAME_TREE_H

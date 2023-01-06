/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_NAMES_TREE_H
#define PDF_NAMES_TREE_H

#include "PdfDeclarations.h"

#include "PdfElement.h"

namespace PoDoFo {

class PdfDictionary;
class PdfName;
class PdfObject;
class PdfString;
class PdfIndirectObjectList;

enum class PdfNameLimits
{
    Before,
    Inside,
    After
};

class PODOFO_API PdfNameTree final : public PdfDictionaryElement
{
public:
    /** Create a new PdfNameTree object
     *  \param parent parent of this action
     */
    PdfNameTree(PdfDocument& doc);

    /** Create a PdfNameTree object from an existing PdfObject
     *	\param obj the object to create from
     *  \param pCatalog the Catalog dictionary of the owning PDF
     */
    PdfNameTree(PdfObject& obj);

    /** Insert a key and value in one of the dictionaries of the name tree.
     *  \param tree name of the tree to search for the key.
     *  \param key the key to insert. If it exists, it will be overwritten.
     *  \param value the value to insert.
     */
    void AddValue(const PdfName& tree, const PdfString& key, const PdfObject& value);

    /** Get the object referenced by a string key in one of the dictionaries
     *  of the name tree.
     *  \param tree name of the tree to search for the key.
     *  \param key the key to search for
     *  \returns the value of the key or nullptr if the key was not found.
     *           if the value is a reference, the object referenced by
     *           this reference is returned.
     */
    PdfObject* GetValue(const PdfName& tree, const PdfString& key) const;

    /** Tests whether a certain nametree has a value.
     *
     *  It is generally faster to use GetValue and check for nullptr
     *  as return value.
     *
     *  \param tree name of the tree to search for the key.
     *  \param key name of the key to look for
     *  \returns true if the dictionary has such a key.
     */
    bool HasValue(const PdfName& tree, const PdfString& key) const;

    /** Tests whether a key is in the range of a limits entry of a name tree node
     *  \returns PdfNameLimits::Inside if the key is inside of the range
     *  \returns PdfNameLimits::After if the key is greater than the specified range
     *  \returns PdfNameLimits::Before if the key is smalelr than the specified range
     *
     *  Internal use only.
     */
    static PdfNameLimits CheckLimits(const PdfObject& obj, const PdfString& key);

    /**
     * Adds all keys and values from a name tree to a dictionary.
     * Removes all keys that have been previously in the dictionary.
     *
     * \param tree the name of the tree to convert into a dictionary
     * \param dict add all keys and values to this dictionary
     */
    void ToDictionary(const PdfName& dictionary, PdfDictionary& dict);

    /**
     * I have made it for access to "JavaScript" dictonary. This is "document-level javascript storage"
     *  \param create if true the javascript node is created if it does not exists.
     */
    PdfObject* GetJavaScriptNode(bool create = false) const;

    /**
     * I have made it for access to "Dest" dictionary. This is "document-level named destination storage"
     *  \param create if true the dests node is created if it does not exists.
     */
    PdfObject* GetDestsNode(bool create = false) const;

private:
    /** Get a PdfNameTrees root node for a certain name.
     *  \param name that identifies a specific name tree.
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
    PdfObject* GetRootNode(const PdfName& name, bool create = false) const;

    /** Recursively walk through the name tree and find the value for key.
     *  \param obj the name tree
     *  \param key the key to find a value for
     *  \return the value for the key or nullptr if it was not found
     */
    PdfObject* GetKeyValue(PdfObject& obj, const PdfString& key) const;

    /**
     *  Add all keys and values from an object and its children to a dictionary.
     *  \param obj a pdf name tree node
     *  \param dict a dictionary
     */
    void AddToDictionary(PdfObject& obj, PdfDictionary& dict);
};

};

#endif // PDF_NAMES_TREE_H

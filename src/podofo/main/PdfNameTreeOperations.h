/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2024 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_NAME_TREE_OPERATIONS_H
#define PDF_NAME_TREE_OPERATIONS_H

#include "PdfObject.h"

namespace PoDoFo {

/** A low level interface with operations to handle document name trees stored in the /Names element
 * \remarks Implemented by PdfNameTrees
 */
class PODOFO_API PdfNameTreeOperations
{
protected:
    PdfNameTreeOperations();

public:
    /** Insert a key and value in one of the dictionaries of the name tree.
     *  \param tree name of the tree to search for the key.
     *  \param key the key to insert. If it exists, it will be overwritten.
     *  \param value the value to insert.
     */
    virtual void AddValue(PdfKnownNameTree tree, const PdfString& key, const PdfObject& value) = 0;
    virtual void AddValue(const PdfName& treeName, const PdfString& key, const PdfObject& value) = 0;

    /** Get the object referenced by a string key in one of the dictionaries
     *  of the name tree.
     *  \param tree name of the tree to search for the key.
     *  \param key the key to search for
     *  \returns the value of the key or nullptr if the key was not found.
     *           if the value is a reference, the object referenced by
     *           this reference is returned.
     */
    virtual const PdfObject* GetValue(PdfKnownNameTree tree, const std::string_view& key) const = 0;
    virtual const PdfObject* GetValue(const std::string_view& treeName, const std::string_view& key) const = 0;
    virtual PdfObject* GetValue(PdfKnownNameTree tree, const std::string_view& key) = 0;
    virtual PdfObject* GetValue(const std::string_view& treeName, const std::string_view& key) = 0;

    /** Tests whether a certain nametree has a value.
     *
     *  It is generally faster to use GetValue and check for nullptr
     *  as return value.
     *
     *  \param tree name of the tree to search for the key.
     *  \param key name of the key to look for
     *  \returns true if the dictionary has such a key.
     */
    virtual bool HasKey(PdfKnownNameTree tree, const std::string_view& key) const = 0;
    virtual bool HasKey(const std::string_view& treeName, const std::string_view& key) const = 0;

    /**
     * Adds all keys and values from a name tree to a dictionary.
     * Removes all keys that have been previously in the dictionary.
     *
     * \param tree the name of the tree to convert into a dictionary
     * \param dict add all keys and values to this dictionary
     * \param skipClear skip clearing the output dictionary
     */
    virtual void ToDictionary(PdfKnownNameTree tree, PdfStringMap<PdfObject>& dict, bool skipClear = false) const = 0;
    virtual void ToDictionary(const std::string_view& treeName, PdfStringMap<PdfObject>& dict, bool skipClear = false) const = 0;

protected:
    PdfNameTreeOperations(const PdfNameTreeOperations&) = default;
    PdfNameTreeOperations& operator=(const PdfNameTreeOperations&) = default;
};

};

#endif // PDF_NAME_TREE_OPERATIONS_H

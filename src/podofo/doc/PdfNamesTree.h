/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#ifndef _PDF_NAMES_TREE_H_
#define _PDF_NAMES_TREE_H_

#include "podofo/base/PdfDefines.h"
#include "PdfElement.h"

namespace PoDoFo {

class PdfDictionary;
class PdfName;
class PdfObject;
class PdfString;
class PdfVecObjects;

enum EPdfNameLimits {
    ePdfNameLimits_Before,
    ePdfNameLimits_Inside,
    ePdfNameLimits_After
};


class PODOFO_DOC_API PdfNamesTree : public PdfElement {
 public:
    /** Create a new PdfNamesTree object
     *  \param pParent parent of this action
     */
    PdfNamesTree( PdfVecObjects* pParent );

    /** Create a PdfNamesTree object from an existing PdfObject
     *	\param pObject the object to create from
     *  \param pCatalog the Catalog dictionary of the owning PDF
     */
    PdfNamesTree( PdfObject* pObject, PdfObject* pCatalog );

    virtual ~PdfNamesTree() { }

    /** Insert a key and value in one of the dictionaries of the name tree.
     *  \param tree name of the tree to search for the key.
     *  \param key the key to insert. If it exists, it will be overwritten.
     *  \param rValue the value to insert.
     */
    void AddValue( const PdfName & tree, const PdfString & key, const PdfObject & rValue );

    /** Get the object referenced by a string key in one of the dictionaries
     *  of the name tree.
     *  \param tree name of the tree to search for the key.
     *  \param key the key to search for
     *  \returns the value of the key or NULL if the key was not found.
     *           if the value is a reference, the object referenced by 
     *           this reference is returned.
     */
    PdfObject* GetValue( const PdfName & tree, const PdfString & key ) const;

    /** Tests wether a certain nametree has a value.
     *
     *  It is generally faster to use GetValue and check for NULL
     *  as return value.
     *  
     *  \param tree name of the tree to search for the key.
     *  \param key name of the key to look for
     *  \returns true if the dictionary has such a key.
     */
    bool HasValue( const PdfName & tree, const PdfString & key ) const;

    /** Tests wether a key is in the range of a limits entry of a name tree node
     *  \returns ePdfNameLimits_Inside if the key is inside of the range
     *  \returns ePdfNameLimits_After if the key is greater than the specified range
     *  \returns ePdfNameLimits_Before if the key is smalelr than the specified range
     *
     *  Internal use only.
     */
    static EPdfNameLimits CheckLimits( const PdfObject* pObj, const PdfString & key );

    /** 
     * Adds all keys and values from a name tree to a dictionary.
     * Removes all keys that have been previously in the dictionary.
     * 
     * \param tree the name of the tree to convert into a dictionary
     * \param rDict add all keys and values to this dictionary
     */
    void ToDictionary( const PdfName & dictionary, PdfDictionary& rDict );

    /** Peter Petrov: 23 May 2008
     * I have made it for access to "JavaScript" dictonary. This is "document-level javascript storage"
     *  \param bCreate if true the javascript node is created if it does not exists.
     */
    inline PdfObject* GetJavaScriptNode(bool bCreate = false) const;

    /** Peter Petrov: 6 June 2008
     * I have made it for access to "Dest" dictionary. This is "document-level named destination storage"
     *  \param bCreate if true the dests node is created if it does not exists.
     */
    inline PdfObject* GetDestsNode(bool bCreate = false) const;

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
     *  \param bCreate if true the root node is created if it does not exists.
     *  \returns the root node of the tree or NULL if it does not exists
     */
    PdfObject* GetRootNode( const PdfName & name, bool bCreate = false ) const;

    /** Recursively walk through the name tree and find the value for key.
     *  \param pObj the name tree 
     *  \param key the key to find a value for
     *  \return the value for the key or NULL if it was not found
     */
    PdfObject* GetKeyValue( PdfObject* pObj, const PdfString & key ) const;

    /** 
     *  Add all keys and values from an object and its children to a dictionary.
     *  \param pObj a pdf name tree node
     *  \param rDict a dictionary
     */
    void AddToDictionary( PdfObject* pObj, PdfDictionary & rDict );

 private:
    PdfObject*	m_pCatalog;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfObject* PdfNamesTree::GetJavaScriptNode(bool bCreate) const
{
    return this->GetRootNode( PdfName("JavaScript"), bCreate );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfObject* PdfNamesTree::GetDestsNode(bool bCreate) const
{
    return this->GetRootNode( PdfName("Dests"), bCreate );
}

};

#endif // _PDF_NAMES_TREE_H_

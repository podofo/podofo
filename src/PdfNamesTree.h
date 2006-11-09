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
 ***************************************************************************/

#ifndef _PDF_NAMES_TREE_H_
#define _PDF_NAMES_TREE_H_

#include "PdfDefines.h"
#include "PdfElement.h"

namespace PoDoFo {

class PdfObject;
class PdfString;
class PdfVecObjects;
class PdfName;

/** An action that can be performed in a PDF document
 */
class PODOFO_API PdfNamesTree : public PdfElement {
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
     *  \param dictionary the name of the dictionary to search in.
     *  \param key the key to insert. If it exists, it will be overwritten.
     *  \param rValue the value to insert.
     */
    void AddValue( const PdfName & dictionary, const PdfString & key, const PdfObject & rValue );

    /** Get the object referenced by a string key in one of the dictionaries
     *  of the name tree.
     *  \param dictionary the name of the dictionary to search in.
     *  \param key the key to search for
     *  \returns the value of the key or NULL if the key was not found.
     *           if the value is a reference, the object referenced by 
     *           this reference is returned.
     */
    PdfObject* GetValue( const PdfName & dictionary, const PdfString & key ) const;

    /** Tests wether a certain nametree has a value.
     *  \param dictionary name of the dictionary to search for the key.
     *  \param key name of the key to look for
     *  \returns true if the dictionary has such a key.
     */
    bool HasValue( const PdfName & dictionary, const PdfString & key ) const;

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
     *  \param bCreate if true the root node is create if it does not exists.
     *  \returns the root node of the tree or NULL if it does not exists
     */
    PdfObject* GetRootNode( const PdfName & name, bool bCreate = false ) const;

    /** Recursively walk through the name tree and find the value for key.
     *  \param pObj the name tree 
     *  \param key the key to find a value for
     *  \return the value for the key or NULL if it was not found
     */
    PdfObject* GetKeyValue( PdfObject* pObj, const PdfString & key ) const;

    /** Inserts key into a names array if it fits into the limits.
     *  \param pObject try this object and all its kids.
     *  \param key write to this key
     *  \param rValue value of the key.
     *  \param pParent parent node of this value of NULL for root node
     *  \returns true if the key was inserted and false if not.
     */
    bool AddKeyValue( PdfObject* pObject, const PdfString & key, const PdfObject & rValue, PdfObject* pParent );

    /** Tests wether a key is in the range of a limits entry of a name tree node
     *  \returns true if the key is in the range of this nametree node and false otherwise
     */
    bool CheckLimitsInside( const PdfObject* pObj, const PdfString & key ) const;

    /** Tests wether a key is before a limits entry of a name tree node
     *  \returns true if the key is in the range of this nametree node and false otherwise
     */
    bool CheckLimitsBefore( const PdfObject* pObj, const PdfString & key ) const;

    /** Tests wether a key is after a limits entry of a name tree node
     *  \returns true if the key is in the range of this nametree node and false otherwise
     */
    bool CheckLimitsAfter( const PdfObject* pObj, const PdfString & key ) const;

    void Rebalance( PdfObject* pObj, PdfObject* pParent );
    void SetLimits( PdfObject* pObj ) ;

 private:
    PdfObject*	m_pCatalog;
};


};

#endif // _PDF_NAMES_TREE_H_

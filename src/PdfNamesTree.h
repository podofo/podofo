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

    /** Returns a PdfObject of type PdfArray consisting of the list of 
     *  named objects of the specified type (inWhichName).  If no such
     *  list exists, and bCreate is true, it will create an empty list
     *  otherwise it returns NULL.
     *  \param inWhichName which of the object types to retrieve
     *  \param bCreate create the necessary structures if missing
     *  \returns PdfObject of the list
     */
    PdfObject* GetOneArrayOfNames( const PdfName& inWhichName, bool bCreate = true );

    /** Get the object referenced by a string key in one of the dictionaries
     *  of the name tree.
     *  \param dictionary the name of the dictionary to search in.
     *  \param key the key to search for
     *  \returns the value of the key or NULL if the key was not found.
     *           if the value is a reference, the object referenced by 
     *           this reference is returned.
     */
    PdfObject* GetValue( const PdfName & dictionary, const PdfString & key ) const;

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

 private:
    PdfObject*	m_pCatalog;
};


};

#endif // _PDF_NAMES_TREE_H_

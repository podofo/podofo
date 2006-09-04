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
class PdfNamesTree : public PdfElement {
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

    /** Returns a PdfObject of type PdfArray consisting of the list of 
	 *  named objects of the specified type (inWhichName).  If no such
	 *  list exists, and bCreate is true, it will create an empty list
	 *  otherwise it returns NULL.
	 *  \param inWhichName which of the object types to retrieve
	 *  \param bCreate create the necessary structures if missing
	 *  \returns PdfObject of the list
     */
	PdfObject* GetOneArrayOfNames( PdfName& inWhichName, bool bCreate=true );

private:

	PdfObject*	m_pCatalog;
};


};

#endif // _PDF_NAMES_TREE_H_

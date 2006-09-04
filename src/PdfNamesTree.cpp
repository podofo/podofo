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

#include "PdfNamesTree.h"

#include "PdfArray.h"
#include "PdfDictionary.h"

namespace PoDoFo {

/*
	We use NULL for the PdfElement name, since the NamesTree dict
	does NOT have a /Type key!
*/
PdfNamesTree::PdfNamesTree( PdfVecObjects* pParent )
    : PdfElement( NULL, pParent ), m_pCatalog( NULL )
{
}

PdfNamesTree::PdfNamesTree( PdfObject* pObject, PdfObject* pCatalog )
    : PdfElement( NULL, pObject ), m_pCatalog( pCatalog )
{
}

PdfObject* PdfNamesTree::GetOneArrayOfNames( PdfName& inWhichName, bool bCreate )
{
	PdfObject*	nameArrObj = NULL;

	PdfObject*	nameDict = Object()->GetIndirectKey( inWhichName );
	if ( !nameDict ) {
		if ( bCreate && m_pCatalog ) {
			nameDict = Object()->GetParent()->CreateObject( PdfDictionary() );
			Object()->GetDictionary().AddKey( inWhichName, nameDict->Reference() );
		} else 
			return NULL;
	} else if ( nameDict->GetDataType() != ePdfDataType_Dictionary ) {
		RAISE_ERROR( ePdfError_InvalidDataType );
	}

	nameArrObj = nameDict->GetIndirectKey( PdfName( "Names" ) );
	if ( !nameArrObj && bCreate ) {
		// make new Array and add it
		nameArrObj = Object()->GetParent()->CreateObject( PdfArray() );
		nameDict->GetDictionary().AddKey( "Names", nameArrObj->Reference() );
	}

	return nameArrObj;
}

};


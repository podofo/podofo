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

PdfObject* PdfNamesTree::GetOneArrayOfNames( const PdfName& inWhichName, bool bCreate )
{
    PdfObject*	nameArrObj = NULL;

    PdfObject*	nameDict = GetObject()->GetIndirectKey( inWhichName );
    if ( !nameDict ) {
        if ( bCreate && m_pCatalog ) {
            nameDict = GetObject()->GetParent()->CreateObject( PdfDictionary() );
            GetObject()->GetDictionary().AddKey( inWhichName, nameDict->Reference() );
        } else 
            return NULL;
    } else if ( nameDict->GetDataType() != ePdfDataType_Dictionary ) {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }
    
    nameArrObj = nameDict->GetIndirectKey( PdfName( "Names" ) );
    if ( !nameArrObj && bCreate ) {
        // make new Array and add it
        nameArrObj = GetObject()->GetParent()->CreateObject( PdfArray() );
        nameDict->GetDictionary().AddKey( "Names", nameArrObj->Reference() );
    }
    
    return nameArrObj;
}

PdfObject* PdfNamesTree::GetValue( const PdfName & dictionary, const PdfString & key ) const 
{
    PdfObject* pObject = this->GetRootNode( dictionary );
    PdfObject* pResult;

    if( !pObject )
        return false;

    pResult = this->GetKeyValue( pObject, key );
    if( pResult && pResult->IsReference() )
        pResult = m_pObject->GetParent()->GetObject( pResult->GetReference() );

    return pResult;
}

PdfObject* PdfNamesTree::GetKeyValue( PdfObject* pObj, const PdfString & key ) const
{
    PdfObject* pResult = NULL;

    // check the limits first
    if( pObj->GetDictionary().HasKey("Limits") ) 
    {
        const PdfArray & limits = pObj->GetDictionary().GetKey("Limits")->GetArray();

        if( limits[0].GetString() > key )
            return NULL;

        if( limits[1].GetString() < key )
            return NULL;
    }

    if( pObj->GetDictionary().HasKey("Kids") )
    {
        const PdfArray & kids       = pObj->GetDictionary().GetKey("Kids")->GetArray();
        PdfArray::const_iterator it = kids.begin();
        PdfObject* pChild           = NULL;

        while( it != kids.end() )
        {
            pChild = m_pObject->GetParent()->GetObject( (*it).GetReference() );
            if( pChild ) 
            {
                pResult = GetKeyValue( pChild, key );
                if( pResult )
                    return pResult;
            }
            else
                PdfError::LogMessage( eLogSeverity_Debug, "Object %lu %lu is child of nametree but was not found!", 
                                      (*it).GetReference().ObjectNumber(), 
                                      (*it).GetReference().GenerationNumber() );

            ++it;
        }
    }
    else
    {
        const PdfArray & names      = pObj->GetDictionary().GetKey("Names")->GetArray();
        PdfArray::const_iterator it = names.begin();

        // a names array is a set of PdfString/PdfObject pairs
        // so we loop in sets of two - getting each pair
        while( it != names.end() ) 
        {
            if( (*it).GetString() == key ) 
            {
                ++it;
                return m_pObject->GetParent()->GetObject( (*it).GetReference() );
            }

            it += 2;
        }
        
    }

    return pResult;
}

PdfObject* PdfNamesTree::GetRootNode( const PdfName & name, bool bCreate ) const
{
    PdfObject* pObj = m_pObject->GetIndirectKey( name );
    if( !pObj && bCreate ) 
    {
        pObj = m_pObject->GetParent()->CreateObject();
        const_cast<PdfNamesTree*>(this)->m_pObject->GetDictionary().AddKey( name, pObj->Reference() );
    }

    return pObj;
}

};


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

bool PdfNamesTree::AddKeyValue( PdfObject* pObj, const PdfString & key, const PdfObject & rValue )
{
    // check the limits first
    if( pObj->GetDictionary().HasKey("Limits") ) 
    {
        const PdfArray & limits = pObj->GetDictionary().GetKey("Limits")->GetArray();

        if( limits[0].GetString() > key )
            return false;

        if( limits[1].GetString() < key )
            return false;
    }
    
    if( pObj->GetDictionary().HasKey("Kids") )
    {
        const PdfArray & kids       = pObj->GetDictionary().GetKey("Kids")->GetArray();
        PdfArray::const_iterator it = kids.begin();
        PdfObject* pChild           = NULL;
        
        while( it != kids.end() )
        {
            pChild = m_pObject->GetParent()->GetObject( (*it).GetReference() );
            if( pChild && AddKeyValue( pChild, key, rValue ) )
                return true;

            ++it;
        }
    }
    else
    {
        PdfArray limits;

        if( pObj->GetDictionary().HasKey( "Names" ) ) 
        {
            PdfArray& array = pObj->GetDictionary().GetKey("Names")->GetArray();
            PdfArray::iterator it = array.begin();
            
            while( it != array.end() )
            {
                if( (*it).GetString() == key ) 
                {
                    // no need to write the key as it is anyways the same
                    ++it;
                    // write the value
                    *it = rValue;
                    break;
                }
                else if( (*it).GetString() > key ) 
                {
                    // insert into the vector
                    array.insert( it, key );
                    array.insert( it, rValue );
                break;
                }
                
                it += 2;
            }

            if( it == array.end() )
            {
                array.push_back( key );
                array.push_back( rValue );
            }

            limits.push_back( (*array.begin()) );
            limits.push_back( (*(array.end()-2)) );
        }
        else
        {
            // we create a completely new node
            PdfArray array;
            array.push_back( key );
            array.push_back( rValue );

            limits.push_back( key );
            limits.push_back( key );

            // create a child object
            PdfObject* pChild = m_pObject->GetParent()->CreateObject();
            pChild->GetDictionary().AddKey( "Names", array );
            pChild->GetDictionary().AddKey( "Limits", limits );

            PdfArray kids( pChild->Reference() );
            pObj->GetDictionary().AddKey( "Kids", kids );
        }

        pObj->GetDictionary().AddKey( "Limits", limits );

        return true;
    }
    
    return false;
}

void PdfNamesTree::AddValue( const PdfName & dictionary, const PdfString & key, const PdfObject & rValue )
{
   PdfObject* pObject = this->GetRootNode( dictionary, true );
 
   if( !this->AddKeyValue( pObject, key, rValue ) )
   {
       RAISE_ERROR( ePdfError_InternalLogic );
   }
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

bool PdfNamesTree::HasValue( const PdfName & dictionary, const PdfString & key ) const
{
    return ( this->GetValue( dictionary, key) == NULL );
}

};


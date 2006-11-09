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
#define BALANCE_TREE_MAX 65
#define BALANCE_TREE_MIN 33
*/

#define BALANCE_TREE_MAX 9
#define BALANCE_TREE_MIN 5


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

bool PdfNamesTree::AddKeyValue( PdfObject* pObj, const PdfString & key, const PdfObject & rValue, PdfObject* pParent )
{
    if( pObj->GetDictionary().HasKey("Kids") )
    {
        const PdfArray & kids       = pObj->GetDictionary().GetKey("Kids")->GetArray();
        PdfArray::const_iterator it = kids.begin();
        PdfObject* pChild           = NULL;
        
        while( it != kids.end() )
        {
            pChild = m_pObject->GetParent()->GetObject( (*it).GetReference() );
            if( !pChild ) 
            {
                RAISE_ERROR( ePdfError_InvalidHandle );
            }

            if( (this->CheckLimitsBefore( pChild, key )  || 
                 this->CheckLimitsInside( pChild, key )) &&
                AddKeyValue( pChild, key, rValue, pObj ) )
                return true;

            ++it;
        }

        // not added, so add to last child
        pChild = m_pObject->GetParent()->GetObject( kids.back().GetReference() );
        if( !pChild ) 
        {
            RAISE_ERROR( ePdfError_InvalidHandle );
        }

        AddKeyValue( pChild, key, rValue, pObj );
        return true;
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
                    it = array.insert( it, rValue ); // array.insert invalidates the iterator
                    it = array.insert( it, key );
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

        // Rebalance node if necessary
        this->Rebalance( pObj, pParent );

        return true;
    }
    
    return false;
}

void PdfNamesTree::AddValue( const PdfName & dictionary, const PdfString & key, const PdfObject & rValue )
{
   PdfObject* pObject = this->GetRootNode( dictionary, true );
 
   //printf("adding key: %s\n", key.GetString() );
   if( !this->AddKeyValue( pObject, key, rValue, NULL ) )
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

    if( !this->CheckLimitsInside( pObj, key ) )
        return false;

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

bool PdfNamesTree::CheckLimitsBefore( const PdfObject* pObj, const PdfString & key ) const 
{
    if( pObj->GetDictionary().HasKey("Limits") ) 
    {
        const PdfArray & limits = pObj->GetDictionary().GetKey("Limits")->GetArray();

        if( limits[0].GetString() > key )
            return true;
    }
    return false;
}

bool PdfNamesTree::CheckLimitsAfter( const PdfObject* pObj, const PdfString & key ) const 
{
    if( pObj->GetDictionary().HasKey("Limits") ) 
    {
        const PdfArray & limits = pObj->GetDictionary().GetKey("Limits")->GetArray();

        if( limits[1].GetString() < key )
            return true;
    }
    return false;
}

bool PdfNamesTree::CheckLimitsInside( const PdfObject* pObj, const PdfString & key ) const
{
    if( pObj->GetDictionary().HasKey("Limits") ) 
    {
        const PdfArray & limits = pObj->GetDictionary().GetKey("Limits")->GetArray();

        if( limits[0].GetString() > key )
            return false;

        if( limits[1].GetString() < key )
            return false;
    }

    return true;
}

void PdfNamesTree::Rebalance( PdfObject* pObj, PdfObject* pParent ) 
{
    if( pObj->GetDictionary().HasKey( "Names" ) ) 
    {
        PdfArray& names = pObj->GetDictionary().GetKey("Names")->GetArray();        
        if( names.size() > BALANCE_TREE_MAX * 2 ) 
        {
            PdfArray first;
            PdfArray second;

            for( int i=0;i<names.size();i++ ) 
            {
                if( i <= BALANCE_TREE_MAX )
                    first.push_back( names[i] );
                else
                    second.push_back( names[i] );
            }

            if( !pParent ) 
            {
                pParent = pObj;
                pObj    = m_pObject->GetParent()->CreateObject();
            }

            printf("Created arrays of size %i and %i\n", first.size(), second.size() );

            PdfObject* pChild = m_pObject->GetParent()->CreateObject();

            pObj->GetDictionary().AddKey( "Names", first );
            pChild->GetDictionary().AddKey( "Names", second );
            
            PdfArray kids;
            if( pParent->GetDictionary().HasKey( "Kids" ) )
                kids = pParent->GetDictionary().GetKey("Kids")->GetArray();

            PdfArray::iterator it = kids.begin();
            while( it != kids.end() ) 
            {
                if( (*it).GetReference() == pObj->Reference() )
                {
                    ++it;
                    it = kids.insert( it, pChild->Reference() );
                    break;
                }
                
                ++it;
            }

            if( it == kids.end() )
            {
                kids.push_back( pObj->Reference() );
                kids.push_back( pChild->Reference() );
            }

            pParent->GetDictionary().AddKey( "Kids", kids );
            SetLimits( pParent );
        }
    }
}

void PdfNamesTree::SetLimits( PdfObject* pObj ) 
{
    PdfArray limits;

    if( pObj->GetDictionary().HasKey("Kids") )
    {
        PdfObject* pChild = m_pObject->GetParent()->GetObject( (*pObj->GetDictionary().GetKey("Kids")->GetArray().begin()).GetReference() );

        limits.push_back( (*pChild->GetDictionary().GetKey("Names")->GetArray().begin()) );

        pChild = m_pObject->GetParent()->GetObject( (*(pObj->GetDictionary().GetKey("Kids")->GetArray().end()-2)).GetReference() );
        limits.push_back( (*pChild->GetDictionary().GetKey("Names")->GetArray().begin()) );
    }
    else if( pObj->GetDictionary().HasKey("Names") ) 
    {
        limits.push_back( (*pObj->GetDictionary().GetKey("Names")->GetArray().begin()) );
        limits.push_back( (*(pObj->GetDictionary().GetKey("Names")->GetArray().end()-2)) );
    }

    pObj->GetDictionary().AddKey("Limits", limits );
}

};


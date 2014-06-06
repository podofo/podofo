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

#include "PdfNamesTree.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfOutputDevice.h"

#include <sstream>

namespace PoDoFo {

#define BALANCE_TREE_MAX 65

/*
#define BALANCE_TREE_MAX 9
*/

class PdfNameTreeNode {
public:
    PdfNameTreeNode( PdfNameTreeNode* pParent, PdfObject* pObject ) 
        : m_pParent( pParent ), m_pObject( pObject )
    {
        m_bHasKids = m_pObject->GetDictionary().HasKey("Kids");
    }

    bool AddValue( const PdfString & key, const PdfObject & value );

    void SetLimits();

    inline PdfObject* GetObject() { return m_pObject; }

private:
    bool Rebalance();

private:
    PdfNameTreeNode* m_pParent;
    PdfObject*       m_pObject;

    bool             m_bHasKids;

};

bool PdfNameTreeNode::AddValue( const PdfString & key, const PdfObject & rValue )
{
    if( m_bHasKids )
    {
        const PdfArray &         kids   = this->GetObject()->GetDictionary().GetKey("Kids")->GetArray();
        PdfArray::const_iterator it     = kids.begin();
        PdfObject*               pChild = NULL;
        EPdfNameLimits           eLimits = ePdfNameLimits_Before; // RG: TODO Compiler complains that this variable should be initialised

        while( it != kids.end() )
        {
            pChild = this->GetObject()->GetOwner()->GetObject( (*it).GetReference() );
            if( !pChild ) 
            {
                PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
            }

            eLimits = PdfNamesTree::CheckLimits( pChild, key );
            if( (eLimits == ePdfNameLimits_Before) || 
                (eLimits == ePdfNameLimits_Inside) )
            {
                break;
            }

            ++it;
        }

        if( it == kids.end() ) 
        {
            // not added, so add to last child
            pChild = this->GetObject()->GetOwner()->GetObject( kids.back().GetReference() );
            if( !pChild ) 
            {
                PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
            }

            eLimits = ePdfNameLimits_After;
        }
                
        PdfNameTreeNode child( this, pChild );
        if( child.AddValue( key, rValue ) ) 
        {
            // if a child insert the key in a way that the limits
            // are changed, we have to change our limits as well!
            // our parent has to change his parents too!
            if( eLimits != ePdfNameLimits_Inside )
                this->SetLimits();
            
            this->Rebalance();
            return true;
        }
        else
            return false;
    }
    else
    {
        bool bRebalance = false;
        PdfArray limits;

        if( this->GetObject()->GetDictionary().HasKey( "Names" ) ) 
        {
            PdfArray& array = this->GetObject()->GetDictionary().GetKey("Names")->GetArray();
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
            bRebalance = true;
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
            PdfObject* pChild = this->GetObject()->GetOwner()->CreateObject();
            pChild->GetDictionary().AddKey( "Names", array );
            pChild->GetDictionary().AddKey( "Limits", limits );

            PdfArray kids( pChild->Reference() );
            this->GetObject()->GetDictionary().AddKey( "Kids", kids );
            m_bHasKids = true;
        }

        if( m_pParent )
        {
            // Root node is not allowed to have a limits key!
            this->GetObject()->GetDictionary().AddKey( "Limits", limits );
        }
        
        if( bRebalance )
            this->Rebalance();

        return true;
    }
}

void PdfNameTreeNode::SetLimits() 
{
    PdfArray limits;

    if( m_bHasKids ) 
    {
        if( this->GetObject()->GetDictionary().HasKey( PdfName("Kids") ) &&
            this->GetObject()->GetDictionary().GetKey( PdfName("Kids") )->IsArray() )
        {
            const PdfReference & rRefFirst = (*this->GetObject()->GetDictionary().GetKey("Kids")->GetArray().begin()).GetReference();
            PdfObject* pChild = this->GetObject()->GetOwner()->GetObject( rRefFirst );
            if( pChild && pChild->GetDictionary().HasKey( PdfName("Limits") ) &&
                pChild->GetDictionary().GetKey( PdfName("Limits") )->IsArray() ) 
                limits.push_back( *(pChild->GetDictionary().GetKey("Limits")->GetArray().begin()) );
            
            const PdfReference & rRefLast = this->GetObject()->GetDictionary().GetKey("Kids")->GetArray().back().GetReference();
            pChild = this->GetObject()->GetOwner()->GetObject( rRefLast );
            if( pChild && pChild->GetDictionary().HasKey( PdfName("Limits") ) &&
                pChild->GetDictionary().GetKey( PdfName("Limits") )->IsArray() ) 
                limits.push_back( pChild->GetDictionary().GetKey("Limits")->GetArray().back() );
        }
        else
            PdfError::LogMessage( eLogSeverity_Error, 
                                  "Object %i %si does not have Kids array.", 
                                  this->GetObject()->Reference().ObjectNumber(), 
                                  this->GetObject()->Reference().GenerationNumber() );
    }
    else // has "Names"
    {
        if( this->GetObject()->GetDictionary().HasKey( PdfName("Names") ) &&
            this->GetObject()->GetDictionary().GetKey( PdfName("Names") )->IsArray() )
        {
            limits.push_back( (*this->GetObject()->GetDictionary().GetKey("Names")->GetArray().begin()) );
            limits.push_back( (*(this->GetObject()->GetDictionary().GetKey("Names")->GetArray().end()-2)) );
        }
        else
            PdfError::LogMessage( eLogSeverity_Error, 
                                  "Object %i %si does not have Names array.", 
                                  this->GetObject()->Reference().ObjectNumber(), 
                                  this->GetObject()->Reference().GenerationNumber() );
    }

    if( m_pParent )
    {
        // Root node is not allowed to have a limits key!
        this->GetObject()->GetDictionary().AddKey("Limits", limits );
    }
}

bool PdfNameTreeNode::Rebalance()
{
    PdfArray* pArray            = m_bHasKids ? &(this->GetObject()->GetDictionary().GetKey("Kids")->GetArray()) :
                                               &(this->GetObject()->GetDictionary().GetKey("Names")->GetArray());
    const PdfName& key          = m_bHasKids ? PdfName("Kids") : PdfName("Names");
    const unsigned int nLength  = m_bHasKids ? BALANCE_TREE_MAX : BALANCE_TREE_MAX * 2;

    if( !pArray ) 
        return false;

    if( pArray->size() > nLength )
    {
        PdfArray   first;
        PdfArray   second;
        PdfArray   kids;

        first.insert( first.end(), pArray->begin(), pArray->begin()+(nLength/2)+1 );
        second.insert( second.end(), pArray->begin()+(nLength/2)+1, pArray->end() );

        PdfObject* pChild1;
        PdfObject* pChild2 = this->GetObject()->GetOwner()->CreateObject();

        if( !m_pParent ) 
        {
            m_bHasKids = true;
            pChild1    = this->GetObject()->GetOwner()->CreateObject();
            this->GetObject()->GetDictionary().RemoveKey( "Names" );
        }
        else
        {
            pChild1 = this->GetObject();
            kids    = m_pParent->GetObject()->GetDictionary().GetKey("Kids")->GetArray();
        }

        pChild1->GetDictionary().AddKey( key, first );
        pChild2->GetDictionary().AddKey( key, second );
        
        PdfArray::iterator it = kids.begin();
        while( it != kids.end() ) 
        {
            if( (*it).GetReference() == pChild1->Reference() )
            {
                ++it;
                it = kids.insert( it, pChild2->Reference() );
                break;
            }
            
            ++it;
        }
        
        if( it == kids.end() )
        {
            kids.push_back( pChild1->Reference() );
            kids.push_back( pChild2->Reference() );
        }

        if( m_pParent ) 
            m_pParent->GetObject()->GetDictionary().AddKey( "Kids", kids );
        else
            this->GetObject()->GetDictionary().AddKey( "Kids", kids );

        // Important is to the the limits
        // of the children first,
        // because SetLimits( pParent )
        // depends on the /Limits key of all its children!
        PdfNameTreeNode( NULL, pChild1 ).SetLimits();
        PdfNameTreeNode( NULL, pChild2 ).SetLimits();

        // limits do only change if splitting name arrays
        if( m_bHasKids ) 
            this->SetLimits();
        else if( m_pParent )
            m_pParent->SetLimits();
        
        return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////

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

void PdfNamesTree::AddValue( const PdfName & tree, const PdfString & key, const PdfObject & rValue )
{
    PdfNameTreeNode root( NULL, this->GetRootNode( tree, true ) );    
    if( !root.AddValue( key, rValue ) )
    {
        PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
    }
}

PdfObject* PdfNamesTree::GetValue( const PdfName & tree, const PdfString & key ) const 
{
    PdfObject* pObject = this->GetRootNode( tree );
    PdfObject* pResult = NULL;

    if( pObject )
    {
        pResult = this->GetKeyValue( pObject, key );
        if( pResult && pResult->IsReference() )
            pResult = this->GetObject()->GetOwner()->GetObject( pResult->GetReference() );
    }

    return pResult;
}

PdfObject* PdfNamesTree::GetKeyValue( PdfObject* pObj, const PdfString & key ) const
{
    if( PdfNamesTree::CheckLimits( pObj, key ) != ePdfNameLimits_Inside )
        return NULL;

    if( pObj->GetDictionary().HasKey("Kids") )
    {
        const PdfArray & kids       = pObj->GetDictionary().GetKey("Kids")->GetArray();
        PdfArray::const_iterator it = kids.begin();

        while( it != kids.end() )
        {
            PdfObject* pChild = this->GetObject()->GetOwner()->GetObject( (*it).GetReference() );
            if( pChild ) 
            {
                PdfObject* pResult = GetKeyValue( pChild, key );
                if( pResult ) // If recursive call returns NULL, 
                              // continue with the next element
                              // in the kids array.
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
                return this->GetObject()->GetOwner()->GetObject( (*it).GetReference() );
            }

            it += 2;
        }
        
    }

    return NULL;
}

PdfObject* PdfNamesTree::GetRootNode( const PdfName & name, bool bCreate ) const
{
    PdfObject* pObj = this->GetObject()->GetIndirectKey( name );
    if( !pObj && bCreate ) 
    {
        pObj = this->GetObject()->GetOwner()->CreateObject();
        this->GetNonConstObject()->GetDictionary().AddKey( name, pObj->Reference() );
    }

    return pObj;
}

bool PdfNamesTree::HasValue( const PdfName & tree, const PdfString & key ) const
{
    return ( this->GetValue( tree, key ) != NULL );
}

EPdfNameLimits PdfNamesTree::CheckLimits( const PdfObject* pObj, const PdfString & key )
{
    if( pObj->GetDictionary().HasKey("Limits") ) 
    {
        const PdfArray & limits = pObj->GetDictionary().GetKey("Limits")->GetArray();

        if( limits[0].GetString() > key )
            return ePdfNameLimits_Before;

        if( limits[1].GetString() < key )
            return ePdfNameLimits_After;
    }
    else
    {
        PdfError::LogMessage( eLogSeverity_Debug, "Name tree object %lu %lu does not have a limits key!", 
                              pObj->Reference().ObjectNumber(), 
                              pObj->Reference().GenerationNumber() );
    }

    return ePdfNameLimits_Inside;
}

void PdfNamesTree::ToDictionary( const PdfName & tree, PdfDictionary& rDict )
{
    rDict.Clear();
    PdfObject* pObj = this->GetRootNode( tree );
    if( pObj )
        AddToDictionary( pObj, rDict );
}

void PdfNamesTree::AddToDictionary( PdfObject* pObj, PdfDictionary & rDict )
{
    if( pObj->GetDictionary().HasKey("Kids") )
    {
        const PdfArray & kids       = pObj->GetDictionary().GetKey("Kids")->GetArray();
        PdfArray::const_iterator it = kids.begin();

        while( it != kids.end() )
        {
            PdfObject* pChild = this->GetObject()->GetOwner()->GetObject( (*it).GetReference() );
            if( pChild ) 
                this->AddToDictionary( pChild, rDict );
            else
                PdfError::LogMessage( eLogSeverity_Debug, "Object %lu %lu is child of nametree but was not found!\n", 
                                      (*it).GetReference().ObjectNumber(), 
                                      (*it).GetReference().GenerationNumber() );

            ++it;
        }
    }
    else if( pObj->GetDictionary().HasKey("Names") )
    {
        const PdfArray & names      = pObj->GetDictionary().GetKey("Names")->GetArray();
        PdfArray::const_iterator it = names.begin();

        // a names array is a set of PdfString/PdfObject pairs
        // so we loop in sets of two - getting each pair
        while( it != names.end() ) 
        {
            // convert all strings into names 
            PdfName name( (*it).GetString().GetString() );
            ++it;
            rDict.AddKey( name, *(it) );
            ++it;
        }
        
    }

}

};


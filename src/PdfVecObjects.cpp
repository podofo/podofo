/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
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

#include "PdfVecObjects.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfObject.h"
#include "PdfReference.h"

#include <algorithm>

namespace PoDoFo {

bool ObjectLittle( PdfObject* p1, PdfObject* p2 )
{
    return *p1 < *p2;
}

class ObjectsComperator { 
public:
    ObjectsComperator( const PdfReference & ref )
        : m_ref( ref )
        {
        }
    
    bool operator()(const PdfObject* p1) const { 
        return p1 ? (p1->Reference() == m_ref ) : false;
    }

private:
    const PdfReference m_ref;
};

PdfVecObjects::PdfVecObjects()
    : m_bAutoDelete( false ), m_nObjectCount( 1 )
{
}

PdfVecObjects::PdfVecObjects( const PdfVecObjects & rhs )
{
    this->operator=( rhs );
}

PdfVecObjects::~PdfVecObjects()
{
    if( m_bAutoDelete ) 
    {
        TIVecObjects it = this->begin();
        while( it != this->end() )
        {
            delete *it;
            ++it;
        }
    }
}

const PdfVecObjects & PdfVecObjects::operator=( const PdfVecObjects & rhs )
{
    TIVecObjects it;
    std::vector<PdfObject*>::operator=( rhs );

    m_bAutoDelete    = rhs.m_bAutoDelete;
    m_nObjectCount   = rhs.m_nObjectCount;
    m_lstFreeObjects = rhs.m_lstFreeObjects;

    it = this->begin();
    while( it != this->end() )
    {
        (*it)->SetParent( this );
        ++it;
    }

    return *this;
}

PdfObject* PdfVecObjects::GetObject( const PdfReference & ref ) const
{
    TCIVecObjects it;

    it = std::find_if( this->begin(), this->end(), ObjectsComperator( ref ) );
    
    if( it != this->end() )
        return (*it);

    return NULL;
}

unsigned int PdfVecObjects::GetIndex( const PdfReference & ref ) const
{
    TCIVecObjects it;

    it = std::find_if( this->begin(), this->end(), ObjectsComperator( ref ) );
    
    if( it != this->end() )
        return (it - this->begin());

    return 0;
}


PdfObject* PdfVecObjects::RemoveObject( const PdfReference & ref )
{
    TIVecObjects it;
    PdfObject*   pObj;

    it = std::find_if( this->begin(), this->end(), ObjectsComperator( ref ) );
    
    if( it != this->end() )
    {
        pObj = *it;
        this->erase( it );
        this->AddFreeObject( pObj->Reference() );
        return pObj;
    }

    return NULL;
}

PdfReference PdfVecObjects::GetNextFreeObject()
{
    PdfReference ref( m_nObjectCount, 0 );

    if( m_lstFreeObjects.size() )
    {
        ref = m_lstFreeObjects.front();
        m_lstFreeObjects.pop_front();
    }

    return ref;
}

PdfObject* PdfVecObjects::CreateObject( const char* pszType )
{
    PdfReference ref = this->GetNextFreeObject();
    PdfObject*  pObj = new PdfObject( ref.ObjectNumber(), ref.GenerationNumber(), pszType );
    pObj->SetParent( this );

    this->push_back( pObj );

    return pObj;
}

PdfObject* PdfVecObjects::CreateObject( const PdfVariant & rVariant )
{
    PdfReference ref = this->GetNextFreeObject();
    PdfObject*  pObj = new PdfObject( ref.ObjectNumber(), ref.GenerationNumber(), rVariant );
    pObj->SetParent( this );    

    this->push_back( pObj );

    return pObj;
}

void PdfVecObjects::AddFreeObject( const PdfReference & rReference )
{
    m_lstFreeObjects.push_front( rReference );
    m_lstFreeObjects.sort();
}

void PdfVecObjects::push_back( PdfObject* pObj )
{
    if( pObj->ObjectNumber() >= m_nObjectCount )
        ++m_nObjectCount;

    pObj->SetParent( this );
    std::vector<PdfObject*>::push_back( pObj );
}

void PdfVecObjects::push_back_and_do_not_own( PdfObject* pObj )
{
    std::vector<PdfObject*>::push_back( pObj );
}

void PdfVecObjects::RenumberObjects( PdfObject* pTrailer, TPdfReferenceSet* pNotDelete )
{
    TVecReferencePointerList  list;
    TIVecReferencePointerList it;
    TIReferencePointerList    itList;
    TIVecObjects              itObjects;
    int                       i = 0;

    m_lstFreeObjects.clear();

    BuildReferenceCountVector( &list );
    InsertReferencesIntoVector( pTrailer, &list );

    GarbageCollection( &list, pTrailer, pNotDelete );

    // TODO: handle trailer correctly

    it = list.begin();
    while( it != list.end() )
    {
        PdfReference ref( i+1, 0 );
#ifdef _DEBUG
        if( (*this)[i] ) 
            PdfError::DebugMessage("RefCount of %i %i R = %i\n", (*this)[i]->Reference().ObjectNumber(), 
                                   (*this)[i]->Reference().GenerationNumber(), (*it).size() );
#endif // _DEBUG

        (*this)[i]->m_reference = ref;

        //if( true || (*it).size() )
        //{
            itList = (*it).begin();
            while( itList != (*it).end() )
            {
                *(*itList) = ref;
                
                ++itList;
            }
            ++i;
        //}
                
        ++it;
    }
}

void PdfVecObjects::InsertReferencesIntoVector( const PdfObject* pObj, TVecReferencePointerList* pList ) const
{
    PdfArray::const_iterator   itArray;
    TCIKeyMap                  itKeys;
    TCIVecObjects              it;
    int                        index;
  
    if( pObj->IsReference() )
    {
        it = std::find_if( this->begin(), this->end(), ObjectsComperator( pObj->GetReference() ) );

        if( it == this->end() )
        {
            RAISE_ERROR( ePdfError_NoObject );
        }

        index = (it - this->begin());
        
        (*pList)[index].push_back( const_cast<PdfReference*>(&(pObj->GetReference() )) );
    }
    else if( pObj->IsArray() )
    {
        itArray = pObj->GetArray().begin(); 
        while( itArray != pObj->GetArray().end() )
        {
            if( (*itArray).IsArray() ||
                (*itArray).IsDictionary() ||
                (*itArray).IsReference() )
                InsertReferencesIntoVector( &(*itArray), pList );

            ++itArray;
        }
    }
    else if( pObj->IsDictionary() )
    {
        itKeys = pObj->GetDictionary().GetKeys().begin();
        while( itKeys != pObj->GetDictionary().GetKeys().end() )
        {
            // optimization as this is really slow:
            // Call only for dictionaries, references and arrays
            if( (*itKeys).second->IsArray() ||
                (*itKeys).second->IsDictionary() ||
                (*itKeys).second->IsReference() )
                InsertReferencesIntoVector( (*itKeys).second, pList );
            
            ++itKeys;
        }
    }
}

void PdfVecObjects::GetObjectDependencies( const PdfObject* pObj, TPdfReferenceSet* pSet ) const
{
    PdfArray::const_iterator   itArray;
    TCIKeyMap                  itKeys;
  
    if( pObj->IsReference() )
    {
        pSet->insert( pObj->GetReference() );
    }
    else if( pObj->IsArray() )
    {
        itArray = pObj->GetArray().begin(); 
        while( itArray != pObj->GetArray().end() )
        {
            if( (*itArray).IsArray() ||
                (*itArray).IsDictionary() ||
                (*itArray).IsReference() )
                GetObjectDependencies( &(*itArray), pSet );

            ++itArray;
        }
    }
    else if( pObj->IsDictionary() )
    {
        itKeys = pObj->GetDictionary().GetKeys().begin();
        while( itKeys != pObj->GetDictionary().GetKeys().end() )
        {
            // optimization as this is really slow:
            // Call only for dictionaries, references and arrays
            if( (*itKeys).second->IsArray() ||
                (*itKeys).second->IsDictionary() ||
                (*itKeys).second->IsReference() )
                GetObjectDependencies( (*itKeys).second, pSet );
            
            ++itKeys;
        }
    }
}

void PdfVecObjects::BuildReferenceCountVector( TVecReferencePointerList* pList ) const
{
    TCIVecObjects      it      = this->begin();

    pList->clear();
    pList->resize( this->size() );

    while( it != this->end() )
    {
        // optimization as this is really slow:
        // Call only for dictionaries, references and arrays
        if( (*it)->IsArray() ||
            (*it)->IsDictionary() ||
            (*it)->IsReference() )
            InsertReferencesIntoVector( *it, pList );

        ++it;
    }
}

void PdfVecObjects::Sort()
{
    std::sort( this->begin(), this->end(), ObjectLittle );
}

void PdfVecObjects::GarbageCollection( TVecReferencePointerList* pList, PdfObject* pTrailer, TPdfReferenceSet* pNotDelete )
{
    TIVecReferencePointerList it        = pList->begin();
    int                       pos       = 0;
    bool                      bContains = false;
    while( it != pList->end() )
    {
        bContains = pNotDelete ? ( pNotDelete->find( (*this)[pos]->Reference() ) != pNotDelete->end() ) : false;
        if( !(*it).size() && !bContains && (*this)[pos] != pTrailer )
        {
            this->erase( this->begin() + pos );
        }
        
        ++pos;
        ++it;
    }

    m_nObjectCount = ++pos;
}

};


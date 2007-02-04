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
#include "PdfMemStream.h"
#include "PdfObject.h"
#include "PdfReference.h"
#include "PdfStream.h"

#include <algorithm>

namespace {

inline bool ObjectLittle( PoDoFo::PdfObject* p1, PoDoFo::PdfObject* p2 )
{
    return *p1 < *p2;
}

};

namespace PoDoFo {

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
    : m_bAutoDelete( false ), m_nObjectCount( 1 ), m_pDocument( NULL ), m_pStreamFactory( NULL )
{
}

PdfVecObjects::PdfVecObjects( const PdfVecObjects & rhs )
    : std::vector<PoDoFo::PdfObject*>()
{
    this->operator=( rhs );
}

PdfVecObjects::~PdfVecObjects()
{
    // always work on a copy of the vector
    // in case a child invalidates our iterators
    // with a call to attach or detach.
    
    TVecObservers copy( m_vecObservers );
    TIVecObservers itObservers = copy.begin();
    while( itObservers != copy.end() )
    {
        (*itObservers)->ParentDestructed();
        ++itObservers;
    }

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

    m_bAutoDelete         = rhs.m_bAutoDelete;
    m_nObjectCount        = rhs.m_nObjectCount;
    m_lstFreeObjects      = rhs.m_lstFreeObjects;
    m_pDocument           = rhs.m_pDocument;
    m_pStreamFactory      = rhs.m_pStreamFactory;

    it = this->begin();
    while( it != this->end() )
    {
        (*it)->SetOwner( this );
        ++it;
    }

    return *this;
}

PdfObject* PdfVecObjects::GetObject( const PdfReference & ref ) const
{
    const TCIVecObjects it ( std::find_if( this->begin(), this->end(), ObjectsComperator( ref ) ) );

    if( it != this->end() )
        return (*it);

    return NULL;
}

unsigned int PdfVecObjects::GetIndex( const PdfReference & ref ) const
{
    TCIVecObjects it;

    it = std::find_if( this->begin(), this->end(), ObjectsComperator( ref ) );
    
    if( it == this->end() )
    {
        RAISE_ERROR( ePdfError_NoObject );
    }

    return (it - this->begin());
}


PdfObject* PdfVecObjects::RemoveObject( const PdfReference & ref, bool bMarkAsFree )
{
    TIVecObjects it;
    PdfObject*   pObj;

    it = std::find_if( this->begin(), this->end(), ObjectsComperator( ref ) );
    
    if( it != this->end() )
    {
        pObj = *it;
        if( bMarkAsFree )
            this->AddFreeObject( pObj->Reference() );
        this->erase( it );
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
    PdfObject*  pObj = new PdfObject( ref, pszType );
    pObj->SetOwner( this );

    this->push_back( pObj );

    return pObj;
}

PdfObject* PdfVecObjects::CreateObject( const PdfVariant & rVariant )
{
    PdfReference ref = this->GetNextFreeObject();
    PdfObject*  pObj = new PdfObject( ref, rVariant );
    pObj->SetOwner( this );    

    this->push_back( pObj );

    return pObj;
}

void PdfVecObjects::AddFreeObject( const PdfReference & rReference )
{
    TIVecObjects it;

    it = std::find_if( this->begin(), this->end(), ObjectsComperator( rReference ) );
   
    // When append free objects from external doc we need plus one number objects
    if( it == this->end() )
        ++m_nObjectCount;
        
    m_lstFreeObjects.push_front( rReference );
    m_lstFreeObjects.sort();
}

void PdfVecObjects::push_back( PdfObject* pObj )
{
    if( pObj->Reference().ObjectNumber() >= m_nObjectCount )
        ++m_nObjectCount;

    pObj->SetOwner( this );
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

    // The following call slows everything down
    // optimization welcome
    BuildReferenceCountVector( &list );
    InsertReferencesIntoVector( pTrailer, &list );

    //GarbageCollection( &list, pTrailer, pNotDelete );

    it = list.begin();
    while( it != list.end() )
    {
        PdfReference ref( i+1, 0 );
        (*this)[i]->m_reference = ref;

        itList = (*it).begin();
        while( itList != (*it).end() )
        {
            *(*itList) = ref;
            
            ++itList;
        }

        ++i;
        ++it;
    }
}

void PdfVecObjects::InsertOneReferenceIntoVector( const PdfObject* pObj, TVecReferencePointerList* pList )  
{
    TCIVecObjects              it;
    int                        index;
    PdfReference               ref;

    // we asume that pObj is a reference - no checking here because of speed
    ref   = pObj->GetReference();
    it    = std::find_if( this->begin(), this->end(), ObjectsComperator( ref ) );
        
    if( it == this->end() )
    {
        // ignore this reference
        return;
        //RAISE_ERROR( ePdfError_NoObject );
    }
    
    index = (it - this->begin());
    (*pList)[index].push_back( const_cast<PdfReference*>(&(pObj->GetReference() )) );
}

void PdfVecObjects::InsertReferencesIntoVector( const PdfObject* pObj, TVecReferencePointerList* pList )
{
    PdfArray::const_iterator   itArray;
    TCIKeyMap                  itKeys;
  
    if( pObj->IsReference() )
    {
        InsertOneReferenceIntoVector( pObj, pList );
    }
    else if( pObj->IsArray() )
    {
        itArray = pObj->GetArray().begin(); 
        while( itArray != pObj->GetArray().end() )
        {
            if( (*itArray).IsReference() )
                InsertOneReferenceIntoVector( &(*itArray), pList );
            else if( (*itArray).IsArray() ||
                     (*itArray).IsDictionary() )
                InsertReferencesIntoVector( &(*itArray), pList );

            ++itArray;
        }
    }
    else if( pObj->IsDictionary() )
    {
        itKeys = pObj->GetDictionary().GetKeys().begin();
        while( itKeys != pObj->GetDictionary().GetKeys().end() )
        {
            if( (*itKeys).second->IsReference() )
                InsertOneReferenceIntoVector( (*itKeys).second, pList );
            // optimization as this is really slow:
            // Call only for dictionaries, references and arrays
            else if( (*itKeys).second->IsArray() ||
                (*itKeys).second->IsDictionary() )
                InsertReferencesIntoVector( (*itKeys).second, pList );
            
            ++itKeys;
        }
    }
}

void PdfVecObjects::GetObjectDependencies( const PdfObject* pObj, TPdfReferenceList* pList ) const
{
    PdfArray::const_iterator   itArray;
    TCIKeyMap                  itKeys;
  
    if( pObj->IsReference() )
    {
        if( std::find( pList->begin(), pList->end(), pObj->GetReference() ) == pList->end() )
            pList->push_back( pObj->GetReference() );
    }
    else if( pObj->IsArray() )
    {
        itArray = pObj->GetArray().begin(); 
        while( itArray != pObj->GetArray().end() )
        {
            if( (*itArray).IsArray() ||
                (*itArray).IsDictionary() ||
                (*itArray).IsReference() )
                GetObjectDependencies( &(*itArray), pList );

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
                GetObjectDependencies( (*itKeys).second, pList );
            
            ++itKeys;
        }
    }
}

void PdfVecObjects::BuildReferenceCountVector( TVecReferencePointerList* pList )
{
    TCIVecObjects      it      = this->begin();

    pList->clear();
    pList->resize( this->size() );

    while( it != this->end() )
    {
        if( (*it)->IsReference() )
            InsertOneReferenceIntoVector( *it, pList );
        // optimization as this is really slow:
        // Call only for dictionaries, references and arrays
        else if( (*it)->IsArray() ||
                 (*it)->IsDictionary() )
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
        if( !(*it).size() && !bContains )
        {
            this->erase( this->begin() + pos );
        }
        
        ++pos;
        ++it;
    }

    m_nObjectCount = ++pos;
}

void PdfVecObjects::Detach( Observer* pObserver )
{
    TIVecObservers it = m_vecObservers.begin();

    while( it != m_vecObservers.end() )
    {
        if( *it == pObserver ) 
        {
            m_vecObservers.erase( it );
            break;
        }
        else
            ++it;
    }
}

PdfStream* PdfVecObjects::CreateStream( PdfObject* pParent )
{
    PdfStream* pStream = m_pStreamFactory ?
        m_pStreamFactory->CreateStream( pParent ) :
        new PdfMemStream( pParent );

    // Tell any observers that there are new objects to write
    TIVecObservers itObservers = m_vecObservers.begin();
    while( itObservers != m_vecObservers.end() )
    {
        (*itObservers)->WriteObject( pParent );
        ++itObservers;
    }

    return pStream;
}

PdfStream* PdfVecObjects::CreateStream( const PdfStream & rhs )
{
    return NULL;
}

void PdfVecObjects::Finish()
{
    // always work on a copy of the vector
    // in case a child invalidates our iterators
    // with a call to attach or detach.
    
    TVecObservers copy( m_vecObservers );
    TIVecObservers itObservers = copy.begin();
    while( itObservers != copy.end() )
    {
        (*itObservers)->Finish();
        ++itObservers;
    }
}

};



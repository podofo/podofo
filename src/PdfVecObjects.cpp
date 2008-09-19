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

inline bool ObjectLittle( const PoDoFo::PdfObject* p1, const PoDoFo::PdfObject* p2 )
{
    return *p1 < *p2;
}

};

namespace PoDoFo {

struct ObjectComparatorPredicate {
public:
    inline bool operator()( const PdfObject* const & pObj, const PdfObject* const & pObj2 ) const { 
        return pObj->Reference() < pObj2->Reference();  
    }
    
    /*
    inline bool operator()( PdfObject* const & pObj, const PdfReference & ref ) const { return pObj->Reference() < ref;  }
    inline bool operator()( const PdfReference & ref, PdfObject* const & pObj ) const { return ref < pObj->Reference();  }
    */
};


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
    : m_bAutoDelete( false ), m_nObjectCount( 1 ), m_bSorted( true ), m_pDocument( NULL ), m_pStreamFactory( NULL )
{
}

PdfVecObjects::~PdfVecObjects()
{
    this->Clear();
}

void PdfVecObjects::Clear()
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

    m_vector.clear();

    m_bAutoDelete    = false;
    m_nObjectCount   = 1;
    m_bSorted        = true; // an emtpy vector is sorted
    m_pDocument      = NULL;
    m_pStreamFactory = NULL;
}

PdfObject* PdfVecObjects::GetObject( const PdfReference & ref ) const
{
    if( !m_bSorted )
        const_cast<PdfVecObjects*>(this)->Sort();

    PdfObject refObj( ref, NULL );
    std::pair<TCIVecObjects,TCIVecObjects> it = 
        std::equal_range( m_vector.begin(), m_vector.end(), &refObj, ObjectComparatorPredicate() );

    if( it.first != it.second )
        return *(it.first);

    /*
    const TCIVecObjects it ( std::find_if( this->begin(), this->end(), ObjectsComperator( ref ) ) );
    if( it != this->end() )
        return (*it);
    */

    return NULL;
}

unsigned int PdfVecObjects::GetIndex( const PdfReference & ref ) const
{
    if( !m_bSorted )
        const_cast<PdfVecObjects*>(this)->Sort();

    PdfObject refObj( ref, NULL );
    std::pair<TCIVecObjects,TCIVecObjects> it = 
        std::equal_range( m_vector.begin(), m_vector.end(), &refObj, ObjectComparatorPredicate() );

    if( it.first == it.second )
    {
        PODOFO_RAISE_ERROR( ePdfError_NoObject );
    }

    return (it.first - this->begin());
}

PdfObject* PdfVecObjects::RemoveObject( const PdfReference & ref, bool bMarkAsFree )
{
    if( !m_bSorted )
        this->Sort();


    PdfObject*         pObj;
    PdfObject refObj( ref, NULL );
    std::pair<TIVecObjects,TIVecObjects> it = 
        std::equal_range( m_vector.begin(), m_vector.end(), &refObj, ObjectComparatorPredicate() );

    if( it.first != it.second )
    {
        pObj = *(it.first);
        if( bMarkAsFree )
            this->AddFreeObject( pObj->Reference() );
        m_vector.erase( it.first );
        return pObj;
    }
    
    return NULL;
}

PdfObject* PdfVecObjects::RemoveObject( const TIVecObjects & it )
{
    PdfObject* pObj = *it;
    m_vector.erase( it );
    return pObj;
}

PdfReference PdfVecObjects::GetNextFreeObject()
{
    PdfReference ref( m_nObjectCount, 0 );

    if( !m_lstFreeObjects.empty() )
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

    if ( !m_lstFreeObjects.empty() && m_lstFreeObjects.back() < rReference )
    {
        // We can maintain sort order by just appending the new free object.
        // This is a whole lot faster than a push_front() and sort().
        m_lstFreeObjects.push_back( rReference );
    }
    else
    {
        m_lstFreeObjects.push_front( rReference );
        m_lstFreeObjects.sort();
    }
}

void PdfVecObjects::push_back( PdfObject* pObj )
{
    if( pObj->Reference().ObjectNumber() >= m_nObjectCount )
    // Peter Petrov 18 September 2008
    {
        // This was a bug.
        //++m_nObjectCount;

        // In fact "m_bObjectCount" is used for the next free object number.
        // We need to use the greatest object number + 1 for the next free object number.
        // Otherwise, object number overlap would have occurred.
        m_nObjectCount = pObj->Reference().ObjectNumber() + 1;
    }

    if( !m_vector.empty() && m_vector.back()->Reference() < pObj->Reference() )
        m_bSorted = false;

    pObj->SetOwner( this );
    m_vector.push_back( pObj );
}

void PdfVecObjects::RenumberObjects( PdfObject* pTrailer, TPdfReferenceSet* )
{
    TVecReferencePointerList  list;
    TIVecReferencePointerList it;
    TIReferencePointerList    itList;
    int                       i = 0;

    m_lstFreeObjects.clear();

    if( !m_bSorted )
        const_cast<PdfVecObjects*>(this)->Sort();

    // The following call slows everything down
    // optimization welcome
    BuildReferenceCountVector( &list );
    InsertReferencesIntoVector( pTrailer, &list );

    //GarbageCollection( &list, pTrailer, pNotDelete );

    it = list.begin();
    while( it != list.end() )
    {
        PdfReference ref( i+1, 0 );
        m_vector[i]->m_reference = ref;

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
    int                        index;

    PODOFO_RAISE_LOGIC_IF( !m_bSorted, 
                           "PdfVecObjects must be sorted before calling PdfVecObjects::InsertOneReferenceIntoVector!" );
    
    // we asume that pObj is a reference - no checking here because of speed
    std::pair<TCIVecObjects,TCIVecObjects> it = 
        std::equal_range( m_vector.begin(), m_vector.end(), pObj, ObjectComparatorPredicate() );

    if( it.first != it.second )
    {
        // ignore this reference
        return;
        //PODOFO_RAISE_ERROR( ePdfError_NoObject );
    }
    
    index = (it.first - this->begin());
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
    pList->resize( !m_vector.empty() );

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
    if( !m_bSorted )
    {
        std::sort( this->begin(), this->end(), ObjectLittle );
        m_bSorted = true;
    }
}

void PdfVecObjects::GarbageCollection( TVecReferencePointerList* pList, PdfObject*, TPdfReferenceSet* pNotDelete )
{
    TIVecReferencePointerList it        = pList->begin();
    int                       pos       = 0;
    bool                      bContains = false;

    while( it != pList->end() )
    {
        bContains = pNotDelete ? ( pNotDelete->find( m_vector[pos]->Reference() ) != pNotDelete->end() ) : false;
        if( !(*it).size() && !bContains )
        {
            m_vector.erase( this->begin() + pos );
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

    return pStream;
}

void PdfVecObjects::WriteObject( PdfObject* pObject )
{
    // Tell any observers that there are new objects to write
    TIVecObservers itObservers = m_vecObservers.begin();
    while( itObservers != m_vecObservers.end() )
    {
        (*itObservers)->WriteObject( pObject );
        ++itObservers;
    }
}

PdfStream* PdfVecObjects::CreateStream( const PdfStream & )
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

void PdfVecObjects::BeginAppendStream( const PdfStream* pStream )
{
    TIVecObservers itObservers = m_vecObservers.begin();
    while( itObservers != m_vecObservers.end() )
    {
        (*itObservers)->BeginAppendStream( pStream );
        ++itObservers;
    }
}
    
void PdfVecObjects::EndAppendStream( const PdfStream* pStream )
{
    TIVecObservers itObservers = m_vecObservers.begin();
    while( itObservers != m_vecObservers.end() )
    {
        (*itObservers)->EndAppendStream( pStream );
        ++itObservers;
    }
}

};



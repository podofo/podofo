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

#include "PdfVecObjects.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfMemStream.h"
#include "PdfObject.h"
#include "PdfReference.h"
#include "PdfStream.h"
#include "PdfDefinesPrivate.h"

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


struct ReferenceComparatorPredicate {
public:
    inline bool operator()( const PdfReference & pObj, const PdfReference & pObj2 ) const { 
        return pObj < pObj2;
    }
};

//RG: 1) Should this class not be moved to the header file
class ObjectsComparator { 
public:
    ObjectsComparator( const PdfReference & ref )
        : m_ref( ref )
        {
        }
    
    bool operator()(const PdfObject* p1) const { 
        return p1 ? (p1->Reference() == m_ref ) : false;
    }

private:
    /** default constructor, not implemented
     */
    ObjectsComparator(void);
    /** copy constructor, not implemented
     */
    ObjectsComparator(const ObjectsComparator& rhs);
    /** assignment operator, not implemented
     */
    ObjectsComparator& operator=(const ObjectsComparator& rhs);

    const PdfReference m_ref;
};

// This is static, IMHO (mabri) different values per-instance could cause confusion.
// It has to be defined here because of the one-definition rule.
size_t PdfVecObjects::m_nMaxReserveSize = static_cast<size_t>(8388607); // cf. Table C.1 in section C.2 of PDF32000_2008.pdf

PdfVecObjects::PdfVecObjects()
    : m_bAutoDelete( false ), m_bCanReuseObjectNumbers( true ), m_nObjectCount( 1 ), m_bSorted( true ), m_pDocument( NULL ), m_pStreamFactory( NULL )
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
    TCIVecObjects it = std::lower_bound( m_vector.begin(), m_vector.end(), &refObj, ObjectComparatorPredicate() );
    if( it != m_vector.end() && (refObj.Reference() == (*it)->Reference()) )
    {
        return *it;
    }

    return NULL;
}

PdfObject* PdfVecObjects::MustGetObject( const PdfReference & ref ) const
{
    PdfObject* obj = GetObject( ref );
    if (!obj)
        PODOFO_RAISE_ERROR( ePdfError_NoObject );
    return obj;
}

size_t PdfVecObjects::GetIndex( const PdfReference & ref ) const
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

void PdfVecObjects::CollectGarbage( PdfObject* pTrailer )
{
    // We do not have any objects that have
    // to be on the top, like in a linearized PDF.
    // So we just use an empty list.
    TPdfReferenceSet    setLinearizedGroup;

    this->RenumberObjects( pTrailer, &setLinearizedGroup, true );
}

PdfReference PdfVecObjects::GetNextFreeObject()
{
    PdfReference ref( static_cast<unsigned int>(m_nObjectCount), 0 );

    if( m_bCanReuseObjectNumbers && !m_lstFreeObjects.empty() )
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
    std::pair<TIPdfReferenceList,TIPdfReferenceList> it = 
        std::equal_range( m_lstFreeObjects.begin(), m_lstFreeObjects.end(), rReference, ReferenceComparatorPredicate() );

    if( it.first != it.second && !m_lstFreeObjects.empty() ) 
    {
        // Be sure that no reference is added twice to free list
        PdfError::DebugMessage( "Adding %d to free list, is already contained in it!", rReference.ObjectNumber() );
        return;
    }
    else
    {
        // When append free objects from external doc we need plus one number objects
        SetObjectCount( rReference );

        // Insert so that list stays sorted
        m_lstFreeObjects.insert( it.first, rReference );
    }
}

void PdfVecObjects::push_back( PdfObject* pObj )
{
    insert_sorted( pObj );
}

void PdfVecObjects::insert_sorted( PdfObject* pObj )
{
    SetObjectCount( pObj->Reference() );
    pObj->SetOwner( this );

    if( m_bSorted && !m_vector.empty() && pObj->Reference() < m_vector.back()->Reference() )
    {
        TVecObjects::iterator i_pos = 
            std::lower_bound(m_vector.begin(),m_vector.end(),pObj,ObjectLittle);
        m_vector.insert(i_pos, pObj );
    }
    else 
    {
        m_vector.push_back( pObj );
    }
}

void PdfVecObjects::RenumberObjects( PdfObject* pTrailer, TPdfReferenceSet* pNotDelete, bool bDoGarbageCollection )
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

    if( bDoGarbageCollection )
    {
        GarbageCollection( &list, pTrailer, pNotDelete );
    }

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
    size_t                        index;

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
        std::pair<TPdfReferenceList::iterator, TPdfReferenceList::iterator> itEqualRange
            = std::equal_range( pList->begin(), pList->end(), pObj->GetReference() );
        if( itEqualRange.first == itEqualRange.second )
        {
            pList->insert(itEqualRange.first, pObj->GetReference() );

            const PdfObject* referencedObject = this->GetObject(pObj->GetReference());
            if( referencedObject != NULL )
            {
                this->GetObjectDependencies( referencedObject, pList );
            }
        }
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

std::string PdfVecObjects::GetNextSubsetPrefix()
{
	if ( m_sSubsetPrefix == "" )
	{
		m_sSubsetPrefix = "AAAAAA+";
	}
	else
	{
		PODOFO_ASSERT( m_sSubsetPrefix.length() == 7 );
		PODOFO_ASSERT( m_sSubsetPrefix[6] == '+' );
	
		for ( int i = 5; i >= 0; i-- )
		{
			if ( m_sSubsetPrefix[i] < 'Z' )
			{
				m_sSubsetPrefix[i]++;
				break;
			}
			m_sSubsetPrefix[i] = 'A';
		}
	}

	return m_sSubsetPrefix;
}

void PdfVecObjects::SetCanReuseObjectNumbers( bool bCanReuseObjectNumbers )
{
    m_bCanReuseObjectNumbers = bCanReuseObjectNumbers;

    if( !m_bCanReuseObjectNumbers )
    {
        m_lstFreeObjects.clear();
    }
}

};



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

#include "PdfDictionary.h"

#include "PdfOutputDevice.h"
#include "PdfDefinesPrivate.h"

namespace PoDoFo {

PdfDictionary::PdfDictionary()
    : m_bDirty( false )
{
}

PdfDictionary::PdfDictionary( const PdfDictionary & rhs )
    : PdfOwnedDataType()
{
    this->operator=( rhs );
    m_bDirty = false;
}

PdfDictionary::~PdfDictionary()
{
    this->SetImmutable(false); // Destructor may change things, i.e. delete
    this->Clear();
}

const PdfDictionary & PdfDictionary::operator=( const PdfDictionary & rhs )
{
    TCIKeyMap it;

    this->Clear();

    it = rhs.m_mapKeys.begin();
    while( it != rhs.m_mapKeys.end() )
    {
        m_mapKeys[(*it).first] = new PdfObject( *(*it).second );
        ++it;
    }

    PdfOwnedDataType::operator=( rhs );
    m_bDirty = true;
    return *this;
}

bool PdfDictionary::operator==( const PdfDictionary& rhs ) const
{
    if (this == &rhs)
        return true;

    if ( m_mapKeys.size() != rhs.m_mapKeys.size() )
        return false;

    // It's not enough to test that our internal maps are equal, because
    // we store variants by pointer not value. However, since a dictionary's
    // keys are stored in a SORTED map, and there may be only one instance of
    // every key, we can do lockstep iteration and compare that way.

    const TCIKeyMap thisIt = m_mapKeys.begin();
    const TCIKeyMap thisEnd = m_mapKeys.end();
    const TCIKeyMap rhsIt = rhs.m_mapKeys.begin();
    const TCIKeyMap rhsEnd = rhs.m_mapKeys.end();
    while ( thisIt != thisEnd && rhsIt != rhsEnd )
    {
        if ( (*thisIt).first != (*rhsIt).first )
            // Name mismatch. Since the keys are sorted that means that there's a key present
            // in one dictionary but not the other.
            return false;
        if ( *(*thisIt).second != *(*rhsIt).second )
            // Value mismatch on same-named keys.
            return false;
    }
    // BOTH dictionaries must now be on their end iterators - since we checked that they were
    // the same size initially, we know they should run out of keys at the same time.
    PODOFO_RAISE_LOGIC_IF( thisIt != thisEnd || rhsIt != rhsEnd, "Dictionary compare error" );
    // We didn't find any mismatches
    return true;
}

void PdfDictionary::Clear()
{
    AssertMutable();

    if( !m_mapKeys.empty() )
    {
        TIKeyMap it;

        it = m_mapKeys.begin();
        while( it != m_mapKeys.end() )
        {
            delete (*it).second;
            ++it;
        }

        m_mapKeys.clear();
        m_bDirty = true;
    }
}

void PdfDictionary::AddKey( const PdfName & identifier, const PdfObject & rObject )
{
    AssertMutable();

    // Empty PdfNames are legal according to the PDF specification
    // weird but true. As a reason we cannot throw an error here
    /*
    if( !identifier.GetLength() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }
    */
    PdfObject *objToInsert = new PdfObject(rObject);
    std::pair<TKeyMap::iterator, bool> inserted = m_mapKeys.insert( std::make_pair( identifier, objToInsert ) );
    if ( !inserted.second )
    {
        delete inserted.first->second;
        inserted.first->second = objToInsert;
    }

    PdfVecObjects *pOwner = GetObjectOwner();
    if ( pOwner != NULL )
        inserted.first->second->SetOwner( pOwner );
    m_bDirty = true;
}

void PdfDictionary::AddKey( const PdfName & identifier, const PdfObject* pObject )
{
    this->AddKey( identifier, *pObject );
}

PdfObject * PdfDictionary::getKey( const PdfName & key ) const
{
    if( !key.GetLength() )
        return NULL;

    TCIKeyMap it;

    it = m_mapKeys.find( key );

    if( it == m_mapKeys.end() )
        return NULL;

    return (*it).second;
}

PdfObject * PdfDictionary::findKey( const PdfName &key ) const
{
    PdfObject *obj = getKey( key );
    if ( obj != NULL )
    {
        if ( obj->IsReference() )
            return GetIndirectObject( obj->GetReference() );
        else
            return obj;
    }

    return NULL;
}

PdfObject * PdfDictionary::findKeyParent( const PdfName & key ) const
{
    PdfObject *obj = findKey( key );
    if (obj == NULL)
    {
        PdfObject *parent = findKey( "Parent" );
        if ( parent == NULL )
        {
            return NULL;
        }
        else
        {
            if ( parent->IsDictionary() )
                return parent->GetDictionary().findKeyParent( key );
            else
                return NULL;
        }
    }
    else
    {
        return obj;
    }
}

pdf_int64 PdfDictionary::GetKeyAsLong( const PdfName & key, pdf_int64 lDefault ) const
{
    const PdfObject* pObject = GetKey( key );
    
    if( pObject && pObject->GetDataType() == ePdfDataType_Number ) 
    {
        return pObject->GetNumber();
    }

    return lDefault;
}

double PdfDictionary::GetKeyAsReal( const PdfName & key, double dDefault ) const
{
    const PdfObject* pObject = GetKey( key );
    
    if( pObject && (
        pObject->GetDataType() == ePdfDataType_Real ||
        pObject->GetDataType() == ePdfDataType_Number))
    {
        return pObject->GetReal();
    }

    return dDefault;
}

bool PdfDictionary::GetKeyAsBool( const PdfName & key, bool bDefault ) const
{
    const PdfObject* pObject = GetKey( key );

    if( pObject && pObject->GetDataType() == ePdfDataType_Bool ) 
    {
        return pObject->GetBool();
    }

    return bDefault;
}

PdfName PdfDictionary::GetKeyAsName( const PdfName & key ) const
{
    const PdfObject* pObject = GetKey( key );

    if( pObject && pObject->GetDataType() == ePdfDataType_Name ) 
    {
        return pObject->GetName();
    }
    
    return PdfName("");	// return an empty name
        
}

bool PdfDictionary::HasKey( const PdfName & key ) const
{
    if( !key.GetLength() )
        return false;
    
    return ( m_mapKeys.find( key ) != m_mapKeys.end() );
}

bool PdfDictionary::RemoveKey( const PdfName & identifier )
{
    TKeyMap::iterator found = m_mapKeys.find( identifier );
    if( found != m_mapKeys.end() )
    {
        AssertMutable();
        delete found->second;
        m_mapKeys.erase( found );
        m_bDirty = true;
        return true;
    }

    return false;
}

void PdfDictionary::Write( PdfOutputDevice* pDevice, EPdfWriteMode eWriteMode, const PdfEncrypt* pEncrypt, const PdfName & keyStop ) const
{
    TCIKeyMap     itKeys;

    if( (eWriteMode & ePdfWriteMode_Clean) == ePdfWriteMode_Clean ) 
    {
        pDevice->Print( "<<\n" );
    } 
    else
    {
        pDevice->Print( "<<" );
    }
    itKeys     = m_mapKeys.begin();

    if( keyStop != PdfName::KeyNull && keyStop.GetLength() && keyStop == PdfName::KeyType )
        return;

    if( this->HasKey( PdfName::KeyType ) ) 
    {
        // Type has to be the first key in any dictionary
        if( (eWriteMode & ePdfWriteMode_Clean) == ePdfWriteMode_Clean ) 
        {
            pDevice->Print( "/Type " );
        }
        else
        {
            pDevice->Print( "/Type" );
        }

        this->GetKey( PdfName::KeyType )->Write( pDevice, eWriteMode, pEncrypt );

        if( (eWriteMode & ePdfWriteMode_Clean) == ePdfWriteMode_Clean ) 
        {
            pDevice->Print( "\n" );
        }
    }

    while( itKeys != m_mapKeys.end() )
    {
        if( (*itKeys).first != PdfName::KeyType )
        {
            if( keyStop != PdfName::KeyNull && keyStop.GetLength() && (*itKeys).first == keyStop )
                return;

            (*itKeys).first.Write( pDevice, eWriteMode );
            if( (eWriteMode & ePdfWriteMode_Clean) == ePdfWriteMode_Clean ) 
            {
                pDevice->Write( " ", 1 ); // write a separator
            }
            (*itKeys).second->Write( pDevice, eWriteMode, pEncrypt );
            if( (eWriteMode & ePdfWriteMode_Clean) == ePdfWriteMode_Clean ) 
            {
                pDevice->Write( "\n", 1 );
            }
        }
        
        ++itKeys;
    }

    pDevice->Print( ">>" );
}

bool PdfDictionary::IsDirty() const
{
    // If the dictionary itself is dirty
    // return immediately
    // otherwise check all children.
    if( m_bDirty ) 
        return m_bDirty;

    TKeyMap::const_iterator it = this->GetKeys().begin();
    while( it != this->GetKeys().end() )
    {
        if( (*it).second->IsDirty() )
            return true;

        ++it;
    }

    return false;
}

void PdfDictionary::SetDirty( bool bDirty )
{
    m_bDirty = bDirty;

    if( !m_bDirty )
    {
        // Propagate state to all subclasses
        TKeyMap::iterator it = this->GetKeys().begin();
        while( it != this->GetKeys().end() )
        {
            (*it).second->SetDirty( m_bDirty );
            ++it;
        }
    }
}

TCIKeyMap PdfDictionary::begin() const
{
    return m_mapKeys.begin();
}

TCIKeyMap PdfDictionary::end() const
{
    return m_mapKeys.end();
}

void PdfDictionary::SetOwner( PdfObject *pOwner )
{
    PdfOwnedDataType::SetOwner( pOwner );
    PdfVecObjects *pVecOwner = pOwner->GetOwner();
    if ( pVecOwner != NULL )
    {
        // Set owmership for all children
        TCIKeyMap it = this->begin();
        TCIKeyMap end = this->end();
        for ( ; it != end; it++ )
            it->second->SetOwner( pVecOwner );
    }
}

};

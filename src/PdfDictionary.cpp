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

#include "PdfDictionary.h"

#include "PdfOutputDevice.h"

namespace PoDoFo {

PdfDictionary::PdfDictionary()
{
}

PdfDictionary::PdfDictionary( const PdfDictionary & rhs )
    : PdfDataType()
{
    this->operator=( rhs );
}

PdfDictionary::~PdfDictionary()
{
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
    TIKeyMap it;

    if( !m_mapKeys.empty() )
    {
        it = m_mapKeys.begin();
        while( it != m_mapKeys.end() )
        {
            delete (*it).second;
            ++it;
        }

        m_mapKeys.clear();
    }
}

void PdfDictionary::AddKey( const PdfName & identifier, const PdfObject & rObject )
{
    // Empty PdfNames are legal according to the PDF specification
    // weird but true. As a reason we cannot throw an error here
    /*
    if( !identifier.GetLength() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }
    */

    if( m_mapKeys.find( identifier ) != m_mapKeys.end() )
    {
        delete m_mapKeys[identifier];
        m_mapKeys.erase( identifier );
    }

	m_mapKeys[identifier] = new PdfObject( rObject );
}

void PdfDictionary::AddKey( const PdfName & identifier, const PdfObject* pObject )
{
    this->AddKey( identifier, *pObject );
}

const PdfObject* PdfDictionary::GetKey( const PdfName & key ) const
{
    TCIKeyMap it;

    if( HasKey( key ) )
    {
        it = m_mapKeys.find( key );
        return (*it).second;
    }
    
    return NULL;
}

PdfObject* PdfDictionary::GetKey( const PdfName & key )
{
    TIKeyMap it;

    if( HasKey( key ) )
    {
        it = m_mapKeys.find( key );
        return (*it).second;
    }
    
    return NULL;
}

long PdfDictionary::GetKeyAsLong( const PdfName & key, long lDefault ) const
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
    
    if( pObject && pObject->GetDataType() == ePdfDataType_Real ) 
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
    if( HasKey( identifier ) )
    {
        delete m_mapKeys[identifier];

        m_mapKeys.erase( identifier );
        return true;
    }

    return false;
}

void PdfDictionary::Write( PdfOutputDevice* pDevice, const PdfEncrypt* pEncrypt, const PdfName & keyStop ) const
{
    TCIKeyMap     itKeys;

    pDevice->Print( "<<\n" );

    itKeys     = m_mapKeys.begin();

    if( keyStop != PdfName::KeyNull && keyStop.GetLength() && keyStop == PdfName::KeyType )
        return;

    if( this->HasKey( PdfName::KeyType ) ) 
    {
        // Type has to be the first key in any dictionary
        pDevice->Print( "/Type " );
        this->GetKey( PdfName::KeyType )->Write( pDevice, pEncrypt );
        pDevice->Print( "\n" );
    }

    while( itKeys != m_mapKeys.end() )
    {
        if( (*itKeys).first != PdfName::KeyType )
        {
            if( keyStop != PdfName::KeyNull && keyStop.GetLength() && (*itKeys).first == keyStop )
                return;

            (*itKeys).first.Write( pDevice );
            pDevice->Write( " ", 1 ); // write a separator
            (*itKeys).second->Write( pDevice, pEncrypt );
            pDevice->Write( "\n", 1 );
        }
        
        ++itKeys;
    }

    pDevice->Print( ">>" );
}

};

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

#include "PdfArray.h"

#include "PdfOutputDevice.h"
#include "PdfDefinesPrivate.h"

#include <limits>

namespace PoDoFo {

PdfArray::PdfArray()
    : m_bDirty( false )
{
}

PdfArray::PdfArray( const PdfObject &var )
    : m_bDirty( false )
{
    this->push_back( var );
}

PdfArray::PdfArray(const PdfArray & rhs)
    : PdfOwnedDataType( rhs ), m_bDirty( rhs.m_bDirty ), m_objects( rhs.m_objects )
{
}

PdfArray::~PdfArray()
{
}

PdfObject * PdfArray::findAt( size_type idx ) const
{
    PdfObject *obj = &const_cast<PdfArray *>( this )->m_objects[idx];
    if ( obj->IsReference() )
        return GetIndirectObject( obj->GetReference() );
    else
        return obj;
}

void PdfArray::clear()
{
    AssertMutable();
    if ( m_objects.size() == 0 )
        return;

    m_objects.clear();
    m_bDirty = true;
}

PdfArray::iterator PdfArray::insert( const iterator &pos, const PdfObject &val )
{
    AssertMutable();

    m_bDirty = true;
    iterator ret = m_objects.insert( pos, val );
    PdfVecObjects *pOwner = GetObjectOwner();
    if ( pOwner != NULL )
        ret->SetOwner( pOwner );
    return ret;
}

void PdfArray::erase( const iterator &pos )
{
    AssertMutable();

    m_objects.erase( pos );
    m_bDirty = true;
}

void PdfArray::erase( const iterator &first, const iterator &last )
{
    AssertMutable();

    m_objects.erase( first, last );
    m_bDirty = true;
}
 
PdfArray& PdfArray::operator=( const PdfArray &rhs )
{
    if (this != &rhs)
    {
        m_bDirty = rhs.m_bDirty;
        m_objects = rhs.m_objects;
        this->PdfOwnedDataType::operator=( rhs );
    }
    else
    {
        //do nothing
    }
    
    return *this;
}

void PdfArray::resize( size_t count, value_type val )
{
    AssertMutable();

    size_t currentSize = size();
    m_objects.resize( count, val );
    PdfVecObjects *pOwner = GetObjectOwner();
    if ( pOwner != NULL )
    {
        for ( size_t i = currentSize; i < count; i++ )
            m_objects[i].SetOwner( pOwner );
    }

    m_bDirty = currentSize != count;
}

void PdfArray::Write( PdfOutputDevice* pDevice, EPdfWriteMode eWriteMode, 
                      const PdfEncrypt* pEncrypt ) const
{
    PdfArray::const_iterator it = this->begin();

    int count = 1;

    if( (eWriteMode & ePdfWriteMode_Clean) == ePdfWriteMode_Clean ) 
    {
        pDevice->Print( "[ " );
    }
    else
    {
        pDevice->Print( "[" );
    }

    while( it != this->end() )
    {
        (*it).Write( pDevice, eWriteMode, pEncrypt );
        if( (eWriteMode & ePdfWriteMode_Clean) == ePdfWriteMode_Clean ) 
        {
            pDevice->Print( (count % 10 == 0) ? "\n" : " " );
        }

        ++it;
        ++count;
    }

    pDevice->Print( "]" );
}

bool PdfArray::ContainsString( const std::string& cmpString ) const
{
    bool foundIt = false;

    TCIVariantList it(this->begin());
    while( it != this->end() )
    {
        if( (*it).GetDataType() == ePdfDataType_String )
        {
            if ( (*it).GetString().GetString() == cmpString ) {
                foundIt = true;
                break;
            }
        }
        
        ++it;
    }
    
    return foundIt;
}

size_t PdfArray::GetStringIndex( const std::string& cmpString ) const
{
    size_t foundIdx = std::numeric_limits<size_t>::max();
    
    for ( size_t i=0; i<this->size(); i++ ) {
        if( (*this)[i].GetDataType() == ePdfDataType_String )
        {
            if ( (*this)[i].GetString().GetString() == cmpString ) 
            {
                foundIdx = i;
                break;
            }
        }
    }
    
    return foundIdx;
}

bool PdfArray::IsDirty() const
{
    // If the array itself is dirty
    // return immediately
    // otherwise check all children.
    if( m_bDirty )
        return m_bDirty;

    PdfArray::const_iterator it(this->begin());
    while( it != this->end() )
    {
        if( (*it).IsDirty() )
            return true;

        ++it;
    }

    return false;
}

void PdfArray::SetDirty( bool bDirty )
{
    m_bDirty = bDirty;

    if( !m_bDirty )
    {
        // Propagate state to all subclasses
        PdfArray::iterator it(this->begin());
        while( it != this->end() )
        {
            (*it).SetDirty( m_bDirty );
            ++it;
        }
    }
}

void PdfArray::SetOwner( PdfObject *pOwner )
{
    PdfOwnedDataType::SetOwner( pOwner );
    PdfVecObjects *pVecOwner = pOwner->GetOwner();
    if ( pVecOwner != NULL )
    {
        // Set owmership for all children
        PdfArray::iterator it = this->begin();
        PdfArray::iterator end = this->end();
        for ( ; it != end; it++ )
            it->SetOwner( pVecOwner );
    }
}

};

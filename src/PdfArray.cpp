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

#include "PdfArray.h"

#include "PdfOutputDevice.h"
#include "PdfDefinesPrivate.h"

namespace PoDoFo {

PdfArray::PdfArray()
    : std::vector<PoDoFo::PdfObject>(), PdfDataType(), m_bDirty( false )
{
}

PdfArray::~PdfArray()
{
}

PdfArray::PdfArray( const PdfObject & var )
    : std::vector<PoDoFo::PdfObject>(), PdfDataType()
{
    this->push_back( var );
    m_bDirty = false;
}

PdfArray::PdfArray( const PdfArray & rhs )
    : std::vector<PoDoFo::PdfObject>(), PdfDataType()
{
    this->operator=( rhs );
    m_bDirty = false;
}

void PdfArray::Write( PdfOutputDevice* pDevice, const PdfEncrypt* pEncrypt ) const
{
    PdfArray::const_iterator it = this->begin();

    int count = 1;

    pDevice->Print( "[ " );
    while( it != this->end() )
    {
        (*it).Write( pDevice, pEncrypt );
        pDevice->Print( !(count % 10) ? "\n" : " " );

        ++it;
        ++count;
    }

    pDevice->Print( "]" );
}

bool PdfArray::ContainsString( const std::string& cmpString ) const
{
    bool foundIt = false;

    TCIVariantList it = this->begin();
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
    // FIXME (size_t)-1 is unsafe as size_t may be unsigned.
    size_t foundIdx = -1;
    
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

    PdfArray::const_iterator it = this->begin();
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
        PdfArray::iterator it = this->begin();
        while( it != this->end() )
        {
            (*it).SetDirty( m_bDirty );
            ++it;
        }
    }
}

};

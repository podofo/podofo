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
    : PdfArrayBaseClass(), PdfDataType(), m_bDirty( false )
{
}

PdfArray::~PdfArray()
{
}

PdfArray::PdfArray( const PdfObject & var )
    : PdfArrayBaseClass(), PdfDataType(), m_bDirty( false )
{
    this->push_back( var );
}

PdfArray::PdfArray( const PdfArray & rhs )
    : PdfArrayBaseClass(rhs), PdfDataType(rhs), m_bDirty(rhs.m_bDirty)
{
    this->operator=( rhs );
}

 
PdfArray& PdfArray::operator=(const PdfArray& rhs)
{
    if (this != &rhs)
    {
        m_bDirty = rhs.m_bDirty;
        PdfArrayBaseClass::operator=( rhs );
    }
    else
    {
        //do nothing
    }
    
    return *this;
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

};

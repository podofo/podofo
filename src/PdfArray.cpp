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

namespace PoDoFo {

PdfArray::PdfArray()
{
}

PdfArray::PdfArray( const PdfObject & var ) 
{
    this->push_back( var );
}

PdfArray::PdfArray( const PdfArray & rhs )
    : std::vector<PoDoFo::PdfObject>(), PdfDataType()
{
    this->operator=( rhs );
}

void PdfArray::Write( PdfOutputDevice* pDevice ) const
{
    PdfArray::const_iterator it = this->begin();

    pDevice->Print( "[ " );
    while( it != this->end() )
    {
        (*it).Write( pDevice );
        pDevice->Print( " " );

        ++it;
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

};

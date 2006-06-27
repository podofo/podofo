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
{
    this->operator=( rhs );
}

PdfDictionary::~PdfDictionary()
{
}

const PdfDictionary & PdfDictionary::operator=( const PdfDictionary & rhs )
{
    m_mapKeys = rhs.m_mapKeys;

    return *this;
}

void PdfDictionary::Clear()
{
    if( !m_mapKeys.empty() )
        m_mapKeys.clear();
}

PdfError PdfDictionary::AddKey( const PdfName & identifier, const PdfVariant & rVariant )
{
    PdfError eCode;

    if( !identifier.Length() )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    if( m_mapKeys.find( identifier ) != m_mapKeys.end() )
        m_mapKeys.erase( identifier );

    m_mapKeys[identifier] = rVariant;

    // TODO: sorted map!!!
    // -> faster find in other functions!

    return eCode;
}

const PdfVariant & PdfDictionary::GetKey( const PdfName & key ) const
{
    TCIKeyMap it;

    if( HasKey( key ) )
    {
        it = m_mapKeys.find( key );
        return (*it).second;
    }
    
    return PdfVariant::NullValue;
}

PdfVariant & PdfDictionary::GetKey( const PdfName & key )
{
    TIKeyMap it;

    if( HasKey( key ) )
    {
        it = m_mapKeys.find( key );
        return (*it).second;
    }
    
    return PdfVariant::NullValue;
}

long PdfDictionary::GetKeyAsLong( const PdfName & key, long lDefault ) const
{
    const PdfVariant & var = GetKey( key );
    
    if( var.GetDataType() == ePdfDataType_Number ) 
    {
        return var.GetNumber();
    }

    return lDefault;
}

bool PdfDictionary::GetKeyAsBool( const PdfName & key, bool bDefault ) const
{
    const PdfVariant & var = GetKey( key );
    
    if( var.GetDataType() == ePdfDataType_Bool ) 
    {
        return var.GetBool();
    }

    return bDefault;
}

bool PdfDictionary::HasKey( const PdfName & key ) const
{
    if( !key.Length() )
        return false;
    
    return ( m_mapKeys.find( key ) != m_mapKeys.end() );
}

bool PdfDictionary::RemoveKey( const PdfName & identifier )
{
    if( HasKey( identifier ) )
    {
        m_mapKeys.erase( identifier );
        return true;
    }

    return false;
}

PdfError PdfDictionary::Write( PdfOutputDevice* pDevice, const PdfName & keyStop )
{
    PdfError eCode;

    TCIKeyMap     itKeys;

    SAFE_OP( pDevice->Print( "<<\n" ) );

    itKeys     = m_mapKeys.begin();

    if( keyStop != PdfName::KeyNull && keyStop.Length() && keyStop == PdfName::KeyType )
        return eCode;

    if( this->HasKey( PdfName::KeyType ) ) 
    {
        // Type has to be the first key in any dictionary
        SAFE_OP( pDevice->Print( "/Type " ) );
        SAFE_OP( this->GetKey( PdfName::KeyType ).Write( pDevice ) );
        SAFE_OP( pDevice->Print( "\n" ) );
    }

    while( itKeys != m_mapKeys.end() )
    {
        if( (*itKeys).first != PdfName::KeyType )
        {
            if( keyStop != PdfName::KeyNull && keyStop.Length() && (*itKeys).first == keyStop )
                return eCode;

            SAFE_OP( pDevice->Print( "/%s ", (*itKeys).first.Name().c_str() ) );
            SAFE_OP( (*itKeys).second.Write( pDevice ) );
            SAFE_OP( pDevice->Print("\n") );
        }
        
        ++itKeys;
    }

    SAFE_OP( pDevice->Print( ">>\n" ) );

    return eCode;
}

};

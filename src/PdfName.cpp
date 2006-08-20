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

#include "PdfName.h"

#include "PdfOutputDevice.h"

namespace PoDoFo {

const PdfName PdfName::KeyContents  = PdfName( "Contents" );
const PdfName PdfName::KeyFlags     = PdfName( "Flags" );
const PdfName PdfName::KeyLength    = PdfName( "Length" );
const PdfName PdfName::KeyNull      = PdfName();
const PdfName PdfName::KeyRect      = PdfName( "Rect" );
const PdfName PdfName::KeySize      = PdfName( "Size" );
const PdfName PdfName::KeySubtype   = PdfName( "Subtype" );
const PdfName PdfName::KeyType      = PdfName( "Type" );
const PdfName PdfName::KeyFilter    = PdfName( "Filter" );

PdfName::PdfName() : m_Data( "" ) {}

PdfName::PdfName( const std::string& sName )
{
    m_Data = sName;
}

PdfName::PdfName( const char* pszName )
{
    if( pszName )
        m_Data.assign( pszName );
}

PdfName::PdfName( const char* pszName, long lLen )
{
    if( pszName )
        m_Data.assign( pszName, lLen );
}

PdfName::PdfName( const PdfName & rhs )
{
    this->operator=( rhs );
}

PdfName::~PdfName()
{
}

void PdfName::Write( PdfOutputDevice* pDevice ) const
{
    pDevice->Print( "/%s", m_Data.c_str() );
}

const PdfName& PdfName::operator=( const PdfName & rhs )
{
    m_Data = rhs.m_Data;
    return *this;
}

bool PdfName::operator==( const PdfName & rhs ) const
{
    return ( m_Data == rhs.m_Data );
}

bool PdfName::operator==( const std::string & rhs ) const
{
    return ( m_Data == rhs );
}

bool PdfName::operator==( const char* rhs ) const
{
    /*
      If the string is empty and you pass NULL - that's equivalent
      If the string is NOT empty and you pass NULL - that's not equal
      Otherwise, compare them
    */
    if( m_Data.empty() && !rhs )
        return true;
    else if( !m_Data.empty() && !rhs )
        return false;
    else
        return ( m_Data == std::string( rhs ) );
}

bool PdfName::operator<( const PdfName & rhs ) const
{
    return m_Data < rhs.m_Data;
}

};


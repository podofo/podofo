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

namespace PoDoFo {

const PdfName PdfName::KeyContents  = PdfName( "Contents" );
const PdfName PdfName::KeyFlags     = PdfName( "Flags" );
const PdfName PdfName::KeyLength    = PdfName( "Length" );
const PdfName PdfName::KeyNull      = PdfName();
const PdfName PdfName::KeyRect      = PdfName( "Rect" );
const PdfName PdfName::KeySize      = PdfName( "Size" );
const PdfName PdfName::KeySubtype   = PdfName( "Subtype" );
const PdfName PdfName::KeyType      = PdfName( "Type" );

PdfName::PdfName()
{
    m_pszData[0] = '\0';
    m_length     = 0;
}

PdfName::PdfName( const char* pszName )
{
    m_pszData[0] = '\0';

    if( pszName && (m_length = strlen( pszName )) < PDF_NAME_MAX_LENGTH )
        strncpy( m_pszData, pszName, PDF_NAME_MAX_LENGTH );
    else
    {
        PdfError::LogMessage( eLogSeverity_Warning, "Length of PDF Names has to be > 0 and < %i. Length of %i passed to PdfName.", PDF_NAME_MAX_LENGTH, m_length );
        m_length = 0;
    }
}

PdfName::PdfName( const char* pszName, long lLen )
{
    m_pszData[0] = '\0';
    m_length     = lLen;

    if( pszName && m_length < PDF_NAME_MAX_LENGTH )
    {
        strncpy( m_pszData, pszName, m_length );
        m_pszData[m_length] = '\0';
    }
    else
    {
        PdfError::LogMessage( eLogSeverity_Warning, "Length of PDF Names has to be > 0 and < %i. Length of %i passed to PdfName.", PDF_NAME_MAX_LENGTH, m_length );
        m_length = 0;
    }
}

PdfName::PdfName( const PdfName & rhs )
{
    this->operator=( rhs );
}

PdfName::~PdfName()
{
}

const PdfName& PdfName::operator=( const PdfName & rhs )
{
    m_length = rhs.m_length;
    if( rhs.m_pszData )
        strncpy( m_pszData, rhs.m_pszData, PDF_NAME_MAX_LENGTH );
    else
        m_pszData[0] = '\0';

    return *this;
}

bool PdfName::operator==( const PdfName & rhs ) const
{
    if( m_length != rhs.m_length )
        return false;
    else
    {
        return this->operator==( rhs.m_pszData );
    }

    return false;
}

bool PdfName::operator==( const char* rhs ) const
{
    if( !m_pszData && !rhs )
        return true;
    else if( !m_pszData || !rhs )
        return false;
    else
        return (strcmp( m_pszData, rhs ) == 0 );
}

bool PdfName::operator<( const PdfName & rhs ) const
{
    if( m_length == rhs.m_length )
    {
        if( !m_pszData && !rhs.m_pszData )
            return false;
        else if( !m_pszData && rhs.m_pszData )
            return false;
        else if( m_pszData && !rhs.m_pszData )
            return true;
        else
            return (strcmp( m_pszData, rhs.m_pszData ) < 0 );
    }
    else
        return m_length < rhs.m_length;
}

};


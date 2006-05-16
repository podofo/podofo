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

#include "PdfString.h"

#include "PdfAlgorithm.h"
#include "PdfOutputDevice.h"

namespace PoDoFo {

const PdfString PdfString::StringNull      = PdfString();

PdfString::PdfString()
    : m_pszData( NULL ), m_lLen( 0 ), m_bHex( false )
{
}

PdfString::PdfString( const char* pszString )
    : m_pszData( NULL ), m_lLen( 0 ), m_bHex( false )
{
    init( pszString );
}

PdfString::PdfString( const char* pszString, bool bHex )
    : m_pszData( NULL ), m_lLen( 0 ), m_bHex( bHex )
{
    init( pszString );
}

PdfString::PdfString( const char* pszData, long lLen, bool bHex, long lPadding, bool bHexEncode )
    : m_pszData( NULL ), m_lLen( 0 ), m_bHex( bHex )
{
    PdfError eCode;
    char*    pHex;
    long     lHex;
    long     lSpace;

    if( pszData ) 
    {
        if( bHex && bHexEncode ) 
        {
            eCode = PdfAlgorithm::HexEncodeBuffer( pszData, lLen, &pHex, &lHex );
            if( eCode.IsError() )
            {
                eCode.PrintErrorMsg();
                return;
            }
        }
        else
        {
            lHex = lLen;
            pHex = const_cast<char*>(pszData);
        }

        m_lLen = lHex + 1;
        if( m_lLen < lPadding )
            m_lLen = lPadding + 1;

        if( !allocate() )
            return;

        memcpy( m_pszData, pHex, lHex );
        
        lSpace = m_lLen - (lHex);
        if( lSpace )
            memset( (m_pszData+lHex), '0', lSpace );

        m_pszData[m_lLen-1] = '\0';
    }
}

PdfString::PdfString( const PdfString & rhs )
    : m_pszData( NULL ), m_lLen( 0 ), m_bHex( false )
{
    this->operator=( rhs );
}

PdfString::~PdfString()
{
    free( m_pszData );
}

PdfError PdfString::Write ( PdfOutputDevice* pDevice )
{
    PdfError eCode;
    
    SAFE_OP( pDevice->Print( m_bHex ? "<%s>" : "(%s)", m_pszData ) );

    return eCode;
}

const PdfString & PdfString::operator=( const PdfString & rhs )
{
    if( m_pszData )
        free( m_pszData );

    this->m_lLen    = rhs.m_lLen;
    this->m_bHex    = rhs.m_bHex;
    
    if( allocate() )
        memcpy( m_pszData, rhs.m_pszData, m_lLen );

    return *this;
}

bool PdfString::allocate()
{
    m_pszData = (char*)malloc( sizeof(char) * m_lLen );

    if( !m_pszData )
    {
        PdfError::LogMessage( eLogSeverity_Error, "PdfString: Cannot allocate memory for string of size %i\n", m_lLen );
        m_lLen = 0;
        return false;
    }

    return true;
}

void PdfString::init( const char* pszString ) 
{
    long len;

    // TODO: escape characters inside of strings!
    
    if( pszString ) 
    {
        len    = strlen( pszString );
        m_lLen = len + 1;

        if( !allocate() )
            return;

        strncpy( m_pszData, pszString, len );
        m_pszData[m_lLen-1] = '\0';
    }
}

};



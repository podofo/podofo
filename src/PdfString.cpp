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

#include "PdfFilter.h"
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
    if( pszString )
        Init( pszString, strlen( pszString ) );
}

PdfString::PdfString( const char* pszString, long lLen, bool bHex )
    : m_pszData( NULL ), m_lLen( 0 ), m_bHex( bHex )
{
    Init( pszString, lLen );
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

void PdfString::SetHexData( const char* pszHex, long lLen )
{

    if( !pszHex ) 
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( m_pszData )
    {
        free( m_pszData );
        m_pszData = NULL;
    }

    if( lLen == -1 )
        lLen = strlen( pszHex );

    m_lLen    = lLen + 1;
    m_bHex    = true;
    
    if( Allocate() )
        memcpy( m_pszData, pszHex, lLen );

    m_pszData[m_lLen-1] = '\0';
}

void PdfString::Write ( PdfOutputDevice* pDevice ) const
{
    // Strings in PDF documents may contain \0 especially if they are encrypted
    // this case has to be handled!

    pDevice->Print( m_bHex ? "<" : "(" );
    pDevice->Write( m_pszData, m_lLen-1 );
    pDevice->Print( m_bHex ? ">" : ")" );
}

const PdfString & PdfString::operator=( const PdfString & rhs )
{
    if( m_pszData )
        free( m_pszData );

    this->m_lLen    = rhs.m_lLen;
    this->m_bHex    = rhs.m_bHex;
    
    if( Allocate() )
        memcpy( m_pszData, rhs.m_pszData, m_lLen );

    return *this;
}

bool PdfString::Allocate()
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

void PdfString::Init( const char* pszString, long lLen )
{
    PdfError         eCode;
    const PdfFilter* pFilter;

    // TODO: escape characters inside of strings!
    if( m_pszData )
    {
        free( m_pszData );
        m_pszData = NULL;
    }

    if( pszString ) 
    {
        if( m_bHex ) 
        {
            pFilter = PdfFilterFactory::Create( ePdfFilter_ASCIIHexDecode );
            if( pFilter ) 
                pFilter->Encode( pszString, lLen, &m_pszData, &m_lLen );
            else
            {
                RAISE_ERROR( ePdfError_UnsupportedFilter );
            }
        }
        else
        {
            m_lLen = lLen + 1;
            if( !Allocate() )
                return;
            
            strncpy( m_pszData, pszString, lLen );
            m_pszData[m_lLen-1] = '\0';
        }
    }
}

};



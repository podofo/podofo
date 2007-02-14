/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
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

#include "PdfFileStream.h"

#include "PdfOutputDevice.h"

namespace PoDoFo {

PdfFileStream::PdfFileStream( PdfObject* pParent, PdfOutputDevice* pDevice )
    : PdfStream( pParent ), m_pDevice( pDevice ), m_lLength( 0 ), m_lOffset( -1 )
{
    m_pLength = pParent->GetOwner()->CreateObject( PdfVariant(0L) );
    m_pParent->GetDictionary().AddKey( PdfName::KeyLength, m_pLength->Reference() );
}

PdfFileStream::~PdfFileStream() 
{
}

void PdfFileStream::Write( PdfOutputDevice* )
{
}

void PdfFileStream::Set( char* szBuffer, long lLen, bool )
{
    if( static_cast<long>(m_lOffset) == -1L )
        m_lOffset = m_pDevice->GetLength();
    else
        m_pDevice->Seek( m_lOffset );

    m_pDevice->Write( szBuffer, lLen );
    m_lLength = lLen;

    m_pLength->SetNumber( m_lLength );
}

void PdfFileStream::Append( const char* pszString, size_t lLen )
{
    if( static_cast<long>(m_lOffset) == -1L )
        m_lOffset = m_pDevice->GetLength();

    m_pDevice->Write( pszString, lLen );
    m_lLength += lLen;

    m_pLength->SetNumber( m_lLength );
}

void PdfFileStream::GetCopy( char**, long* ) const
{
    PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
}

};


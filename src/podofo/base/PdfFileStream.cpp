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

#include "PdfFileStream.h"

#include "PdfEncrypt.h"
#include "PdfFilter.h"
#include "PdfOutputDevice.h"
#include "PdfOutputStream.h"
#include "PdfDefinesPrivate.h"

namespace PoDoFo {

PdfFileStream::PdfFileStream( PdfObject* pParent, PdfOutputDevice* pDevice )
    : PdfStream( pParent ), m_pDevice( pDevice ), m_pStream( NULL ), m_pDeviceStream( NULL ),
      m_pEncryptStream( NULL ), m_lLenInitial( 0 ), m_lLength( 0 ), m_pCurEncrypt( NULL )
{
    m_pLength = pParent->GetOwner()->CreateObject( PdfVariant(static_cast<pdf_int64>(PODOFO_LL_LITERAL(0))) );
    m_pParent->GetDictionary().AddKey( PdfName::KeyLength, m_pLength->Reference() );
}

PdfFileStream::~PdfFileStream() 
{
}

void PdfFileStream::Write( PdfOutputDevice*, PdfEncrypt* )
{
}

void PdfFileStream::BeginAppendImpl( const TVecFilters & vecFilters )
{
    m_pParent->GetOwner()->WriteObject( m_pParent );

    m_lLenInitial = m_pDevice->GetLength();

    if( vecFilters.size() )
    {
        m_pDeviceStream = new PdfDeviceOutputStream( m_pDevice );
        if( m_pCurEncrypt ) 
        {
            m_pEncryptStream = m_pCurEncrypt->CreateEncryptionOutputStream( m_pDeviceStream );
            m_pStream        = PdfFilterFactory::CreateEncodeStream( vecFilters, m_pEncryptStream );
        }
        else
            m_pStream        = PdfFilterFactory::CreateEncodeStream( vecFilters, m_pDeviceStream );
    }
    else 
    {
        if( m_pCurEncrypt ) 
        {
            m_pDeviceStream = new PdfDeviceOutputStream( m_pDevice );
            m_pStream       = m_pCurEncrypt->CreateEncryptionOutputStream( m_pDeviceStream );
        }
        else
            m_pStream = new PdfDeviceOutputStream( m_pDevice );
    }
}

void PdfFileStream::AppendImpl( const char* pszString, size_t lLen )
{
    m_pStream->Write( pszString, static_cast<long>(lLen) );
}

void PdfFileStream::EndAppendImpl()
{
    if( m_pStream ) 
    {
        m_pStream->Close();
        delete m_pStream;
        m_pStream = NULL;
    }

    if( m_pEncryptStream ) 
    {
        m_pEncryptStream->Close();
        delete m_pEncryptStream;
        m_pEncryptStream = NULL;
    }

    if( m_pDeviceStream ) 
    {
        m_pDeviceStream->Close();
        delete m_pDeviceStream;
        m_pDeviceStream = NULL;
    }

    m_lLength = m_pDevice->GetLength() - m_lLenInitial;
    if( m_pCurEncrypt ) 
    {
        m_lLength = m_pCurEncrypt->CalculateStreamLength(m_lLength);
    }
    m_pLength->SetNumber( static_cast<long>(m_lLength) );
}

void PdfFileStream::GetCopy( char**, pdf_long* ) const
{
    PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
}

void PdfFileStream::GetCopy(PdfOutputStream*) const
{
	PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
}

void PdfFileStream::SetEncrypted( PdfEncrypt* pEncrypt ) 
{
    m_pCurEncrypt = pEncrypt;
    if( m_pCurEncrypt )
        m_pCurEncrypt->SetCurrentReference( m_pParent->Reference() );
}

};


/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
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

#include "PdfStream.h"

#include "PdfArray.h"
#include "PdfFilter.h" 
#include "PdfInputStream.h"
#include "PdfOutputStream.h"

#include "PdfOutputDevice.h"
#include <iostream>
using namespace std;

namespace PoDoFo {

PdfStream::PdfStream( PdfObject* pParent )
    : m_pParent( pParent ), m_bAppend( false )
{
}

PdfStream::~PdfStream()
{
}

void PdfStream::GetFilteredCopy( PdfOutputStream* pStream ) const
{
    TVecFilters      vecFilters    = PdfFilterFactory::CreateFilterList( m_pParent );
    if( vecFilters.size() )
    {
        PdfOutputStream* pDecodeStream = PdfFilterFactory::CreateDecodeStream( vecFilters, pStream, 
                                                                               m_pParent ? 
                                                                               &(m_pParent->GetDictionary()) : NULL  );
        
        pDecodeStream->Write( const_cast<char*>(this->GetInternalBuffer()), this->GetInternalBufferSize() );
        pDecodeStream->Close();

        delete pDecodeStream;
    }
    else
        // Also work on unencoded streams
        pStream->Write( const_cast<char*>(this->GetInternalBuffer()), this->GetInternalBufferSize() );
}

void PdfStream::GetFilteredCopy( char** ppBuffer, long* lLen ) const
{
    TVecFilters            vecFilters    = PdfFilterFactory::CreateFilterList( m_pParent );
    PdfMemoryOutputStream  stream;
    if( vecFilters.size() )
    {
        PdfOutputStream*       pDecodeStream = PdfFilterFactory::CreateDecodeStream( vecFilters, &stream, 
                                                                           m_pParent ? 
                                                                           &(m_pParent->GetDictionary()) : NULL  );

        pDecodeStream->Write( this->GetInternalBuffer(), this->GetInternalBufferSize() );
        pDecodeStream->Close();

        delete pDecodeStream;
    }
    else
    {
        // Also work on unencoded streams
        stream.Write( const_cast<char*>(this->GetInternalBuffer()), this->GetInternalBufferSize() );
        stream.Close();
    }

    *lLen     = stream.GetLength();
    *ppBuffer = stream.TakeBuffer();
}

const PdfStream & PdfStream::operator=( const PdfStream & rhs )
{
    this->Set( const_cast<char*>(rhs.GetInternalBuffer()), rhs.GetInternalBufferSize() );

    if( m_pParent ) 
        m_pParent->GetDictionary().AddKey( PdfName::KeyLength, 
                                           PdfVariant( static_cast<long>(rhs.GetInternalBufferSize()) ) );

    return (*this);
}

void PdfStream::Set( char* szBuffer, long lLen, const TVecFilters & vecFilters )
{
    this->BeginAppend( vecFilters );
    this->Append( szBuffer, lLen );
    this->EndAppend();
}

void PdfStream::Set( char* szBuffer, long lLen )
{
    this->BeginAppend();
    this->Append( szBuffer, lLen );
    this->EndAppend();
}

void PdfStream::Set( PdfInputStream* pStream )
{
    TVecFilters vecFilters;
    vecFilters.push_back( ePdfFilter_FlateDecode );

    this->Set( pStream, vecFilters );
}

void PdfStream::Set( PdfInputStream* pStream, const TVecFilters & vecFilters )
{
    const int BUFFER_SIZE = 4096;
    long      lLen        = 0;
    char      buffer[BUFFER_SIZE];

    this->BeginAppend( vecFilters );

    do {
        lLen = pStream->Read( buffer, BUFFER_SIZE );
        this->Append( buffer, lLen );
    } while( lLen == BUFFER_SIZE );

    this->EndAppend();
}

void PdfStream::SetRawData( PdfInputStream* pStream, long lLen )
{
    const int   BUFFER_SIZE = 4096;
    char        buffer[BUFFER_SIZE];
    long        lRead;
    TVecFilters vecEmpty;

    // TODO: DS, give begin append a size hint so that it knows
    //       how many data has to be allocated
    this->BeginAppend( vecEmpty, true, false );
    if( lLen == -1 ) 
    {
        do {
            lRead = pStream->Read( buffer, BUFFER_SIZE );
            this->Append( buffer, lRead );
        } while( lRead > 0 );
    }
    else
    {
        do {
            lRead = pStream->Read( buffer, PDF_MIN( BUFFER_SIZE, lLen ) );
            lLen -= lRead;
            this->Append( buffer, lRead );
        } while( lLen && lRead > 0 );
    }

    this->EndAppend();
}

void PdfStream::BeginAppend( bool bClearExisting )
{
    TVecFilters vecFilters;
    vecFilters.push_back( ePdfFilter_FlateDecode );

    this->BeginAppend( vecFilters, bClearExisting );
}

void PdfStream::BeginAppend( const TVecFilters & vecFilters, bool bClearExisting, bool bDeleteFilters )
{
    char* pBuffer = NULL;
    long  lLen;

    PODOFO_RAISE_LOGIC_IF( m_bAppend, "BeginAppend() failed because EndAppend() was not yet called!" );

    m_pParent->GetOwner()->BeginAppendStream( this );

    if( !bClearExisting && this->GetLength() ) 
        this->GetFilteredCopy( &pBuffer, &lLen );

    if( !vecFilters.size() && bDeleteFilters )
    {
            m_pParent->GetDictionary().RemoveKey( PdfName::KeyFilter );
    }
    if( vecFilters.size() == 1 )
    {
        m_pParent->GetDictionary().AddKey( PdfName::KeyFilter, 
                                           PdfName( PdfFilterFactory::FilterTypeToName( vecFilters.front() ) ) );
    }
    else if( vecFilters.size() > 1 )
    {
        PdfArray filters;
        TCIVecFilters it = vecFilters.begin();
        while( it != vecFilters.end() )
        {
            filters.push_back( PdfName( PdfFilterFactory::FilterTypeToName( *it ) ) );
            ++it;
        }
        
        m_pParent->GetDictionary().AddKey( PdfName::KeyFilter, filters );
    }

    this->BeginAppendImpl( vecFilters );
    m_bAppend = true;
    if( pBuffer ) 
    {
        this->Append( pBuffer, lLen );
        free( pBuffer );
    }
}

void PdfStream::EndAppend()
{
    PODOFO_RAISE_LOGIC_IF( !m_bAppend, "BeginAppend() failed because EndAppend() was not yet called!" );

    m_bAppend = false;
    this->EndAppendImpl();

    m_pParent->GetOwner()->EndAppendStream( this );
}

};

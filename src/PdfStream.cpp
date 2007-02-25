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
    PdfOutputStream* pDecodeStream = PdfFilterFactory::CreateDecodeStream( vecFilters, pStream );

    pDecodeStream->Write( const_cast<char*>(this->GetInternalBuffer()), this->GetInternalBufferSize() );

    delete pDecodeStream;
}

void PdfStream::GetFilteredCopy( char** ppBuffer, long* lLen ) const
{
    TVecFilters            vecFilters    = PdfFilterFactory::CreateFilterList( m_pParent );
    PdfMemoryOutputStream  stream;
    PdfOutputStream*       pDecodeStream = PdfFilterFactory::CreateDecodeStream( vecFilters, &stream );

    pDecodeStream->Write( this->GetInternalBuffer(), this->GetInternalBufferSize() );
    delete pDecodeStream;

    *lLen     = stream.GetLength();
    *ppBuffer = stream.TakeBuffer();
}

/*
void PdfStream::GetFilteredCopy( char** ppBuffer, long* lLen ) const
{
    TVecFilters            vecFilters;
    TIVecFilters           it;

    TVecDictionaries       tDecodeParams;
    TCIVecDictionaries     itDecodeParams;

    char*           pInBuf;
    long            lInLen;

    if( !ppBuffer || !lLen )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    *ppBuffer = NULL;
    *lLen     = 0;

    if( !this->GetInternalBufferSize() )
        return;

    pInBuf = const_cast<char*>(this->GetInternalBuffer());
    lInLen = this->GetInternalBufferSize();

    FillFilterList( vecFilters );
    GetDecodeParms( &tDecodeParams );

    if( tDecodeParams.size() > 0 && tDecodeParams.size() != vecFilters.size() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidPredictor );
    }

    if( !vecFilters.empty() )
    {
        it = vecFilters.begin();
        if( tDecodeParams.size() )
            itDecodeParams = tDecodeParams.begin();

        while( it != vecFilters.end() ) 
        {
            std::auto_ptr<PdfFilter> pFilter = PdfFilterFactory::Create( *it );
            if( !pFilter.get() ) 
            {
                FreeDecodeParms( &tDecodeParams );

                PdfError::LogMessage( eLogSeverity_Error, "Error: Found an unsupported filter: %i\n", *it );
                PODOFO_RAISE_ERROR( ePdfError_UnsupportedFilter );
                break;
            }

            try {
                pFilter->Decode( pInBuf, lInLen, ppBuffer, lLen, tDecodeParams.size() ? *itDecodeParams : NULL );
            } catch( PdfError & e ) {
                e.AddToCallstack( __FILE__, __LINE__ );

                if( pInBuf != this->GetInternalBuffer() )
                {
                    // the input buffer was malloc'ed by another filter before
                    // so free it and let it point to the output buffer
                    free( pInBuf );
                }

                FreeDecodeParms( &tDecodeParams );
                free( *ppBuffer );

                throw e;
            }

            if( pInBuf != this->GetInternalBuffer() )
            {
                // the input buffer was malloc'ed by another filter before
                // so free it and let it point to the output buffer
                free( pInBuf );
            }
            pInBuf = *ppBuffer;
            lInLen = *lLen;

            ++it;

            if( tDecodeParams.size() )
                ++itDecodeParams;
        }
    }
    else
    {
        GetCopy( ppBuffer, lLen );
    }

    FreeDecodeParms( &tDecodeParams );
}
*/

void PdfStream::GetDecodeParms( TVecDictionaries* pParams ) const
{
    PdfObject*               pObj = NULL;
    PdfArray                 array;
    PdfArray::const_iterator it;

    if( !pParams )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // Make sure it is empty
    FreeDecodeParms( pParams );

    if( m_pParent->GetDictionary().HasKey( "DecodeParms" ) )
        pObj = m_pParent->GetIndirectKey( "DecodeParms" );
    else if( m_pParent->GetDictionary().HasKey( "DP" ) )
        // See Implementation Note 3.2.7:
        // Adobe Viewers support DP as abbreviation for DecodeParms
        pObj = m_pParent->GetIndirectKey( "DP" );

    if( !pObj )
        // No Decode Params dictionary
        return;
    else if( pObj->IsDictionary() )
    {
        pParams->push_back( new PdfDictionary( pObj->GetDictionary() ) );
        // nothin else to do;
        return;
    }
    else if( pObj->IsArray() )
        array = pObj->GetArray();
    else
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    // we are sure to have an array now!
    it = array.begin();

    while( it != array.end() ) 
    {
        if( (*it).IsNull() )
            pParams->push_back( NULL );
        else if( (*it).IsDictionary() )
            pParams->push_back( new PdfDictionary( (*it).GetDictionary() ) );
        else
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
        }

        ++it;
    }
}

void PdfStream::SetDecodeParms( TVecDictionaries* pParams )
{
    PdfArray             array;    
    TCIVecDictionaries   it;

    if( !pParams )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // remove any existing old keys first
    if( m_pParent->GetDictionary().HasKey( "DecodeParms" ) )
    {
        m_pParent->GetDictionary().RemoveKey( "DecodeParms" );
    }
    else if( m_pParent->GetDictionary().HasKey( "DP" ) )
    {
        m_pParent->GetDictionary().RemoveKey( "DP" );
    }

    // add the new DecodeParms 
    if( pParams->size() > 1 ) 
    {
        it = pParams->begin();
        while( it != pParams->end() )
        {
            if( *it ) 
            {
                array.push_back( PdfVariant( *(*it) ) );
            }
            else
            {
                array.push_back( PdfVariant() );
            }
            
            ++it;
        }

        m_pParent->GetDictionary().AddKey( "DecodeParms", PdfVariant( array ) );
    }
    else if( pParams->size() == 1 ) 
    {
        if( (*pParams)[0] )
            m_pParent->GetDictionary().AddKey( "DecodeParms", *((*pParams)[0] ) );
    }
}

void PdfStream::FreeDecodeParms( TVecDictionaries* pParams ) const
{
    TVecDictionaries::iterator it = pParams->begin();

    while( it != pParams->end() ) 
    {
        delete *it;

        ++it;
    }

    pParams->clear();
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

void PdfStream::BeginAppend( bool bClearExisting )
{
    TVecFilters vecFilters;
    vecFilters.push_back( ePdfFilter_FlateDecode );

    this->BeginAppend( vecFilters, bClearExisting );
}

void PdfStream::BeginAppend( const TVecFilters & vecFilters, bool bClearExisting )
{
    char* pBuffer = NULL;
    long  lLen;

    PODOFO_RAISE_LOGIC_IF( m_bAppend, "BeginAppend() failed because EndAppend() was not yet called!" );

    if( !bClearExisting && this->GetLength() ) 
        this->GetFilteredCopy( &pBuffer, &lLen );

    if( vecFilters.size() == 1 )
    {
        m_pParent->GetDictionary().AddKey( PdfName::KeyFilter, 
                                           PdfName( PdfFilterFactory::FilterTypeToName( vecFilters.front() ) ) );
    }
    else if( vecFilters.size() >= 1 ) 
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
    else 
        m_pParent->GetDictionary().RemoveKey( PdfName::KeyFilter );

    this->BeginAppendImpl( vecFilters );
    m_bAppend = true;
    if( pBuffer ) 
        this->Append( pBuffer, lLen );
}

void PdfStream::EndAppend()
{
    PODOFO_RAISE_LOGIC_IF( !m_bAppend, "BeginAppend() failed because EndAppend() was not yet called!" );

    m_bAppend = false;
    this->EndAppendImpl();
}

};

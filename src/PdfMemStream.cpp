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

#include "PdfMemStream.h"

#include "PdfArray.h"
#include "PdfFilter.h"
#include "PdfObject.h"
#include "PdfOutputDevice.h"
#include "PdfVariant.h"

namespace PoDoFo {

#define STREAM_SIZE_INCREASE 1024

PdfMemStream::PdfMemStream( PdfObject* pParent )
    : PdfStream( pParent )
{
}

PdfMemStream::PdfMemStream( const PdfMemStream & rhs )
    : PdfStream( NULL )
{
    operator=(rhs);
}

PdfMemStream::~PdfMemStream()
{
}

void PdfMemStream::Set( char* szBuffer, long lLen, bool takePossession )
{
    PdfRefCountedBuffer tmp( szBuffer, lLen );

    if( !szBuffer )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_buffer = tmp;
    m_buffer.SetTakePossesion( takePossession );

    if( m_pParent )
        m_pParent->GetDictionary().AddKey( PdfName::KeyLength, PdfVariant( static_cast<long>(m_buffer.GetSize()) ) );
}

void PdfMemStream::Append( const char* pszString, size_t lLen )
{
    if( !pszString )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // only append to uncommpressed streams
    this->Uncompress();

    m_buffer.Append( pszString, lLen );

    if( m_pParent )
        m_pParent->GetDictionary().AddKey( PdfName::KeyLength, PdfVariant( static_cast<long>(m_buffer.GetSize()) ) );
}

void PdfMemStream::GetCopy( char** pBuffer, long* lLen ) const
{
    if( !pBuffer || !lLen )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    *pBuffer = static_cast<char*>(malloc( sizeof( char ) * m_buffer.GetSize() ));
    *lLen = m_buffer.GetSize();
    
    if( !*pBuffer )
    {
        PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
    }
    
    memcpy( *pBuffer, m_buffer.GetBuffer(), m_buffer.GetSize() );
}

void PdfMemStream::FlateCompress()
{
    PdfObject*        pObj;
    PdfVariant        vFilter( PdfName("FlateDecode" ) );
    PdfVariant        vFilterList;
    PdfArray          tFilters;
    TVecDictionaries  tDecodeParams;

    PdfArray::const_iterator tciFilters;
    
    if( !m_buffer.GetSize() )
        return; // ePdfError_ErrOk

    if( m_pParent->GetDictionary().HasKey( "Filter" ) )
    {
        // DecodeParms dictionaries of already used filters
        // have to be handled.
        GetDecodeParms( &tDecodeParams );

        pObj = m_pParent->GetIndirectKey( "Filter" );

        if( pObj->IsName() )
        {
            if( pObj->GetName() != "DCTDecode" && pObj->GetName() != "FlateDecode" )
            {
                tFilters.push_back( vFilter );
                tFilters.push_back( *pObj );
            }
            else
            {
                FreeDecodeParms( &tDecodeParams );
                // do not compress DCTDecoded are already FlateDecoded streams again
                return;
            }
        }
        else if( pObj->IsArray() )
        {
            tciFilters = pObj->GetArray().begin();

            while( tciFilters != pObj->GetArray().end() )
            {
                if( (*tciFilters).IsName() )
                {
                    // do not compress DCTDecoded are already FlateDecoded streams again
                    if( (*tciFilters).GetName() == "DCTDecode" || (*tciFilters).GetName() == "FlateDecode" )
                    {
                        FreeDecodeParms( &tDecodeParams );
                        return;
                    }
                }

                ++tciFilters;
            }

            tFilters.push_back( vFilter );

            tciFilters = pObj->GetArray().begin();

            while( tciFilters != pObj->GetArray().end() )
            {
                tFilters.push_back( (*tciFilters) );
                
                ++tciFilters;
            }
        }
        else
        {
            FreeDecodeParms( &tDecodeParams );
            // TODO: handle other cases
            return;
        }

        vFilterList = PdfVariant( tFilters );
        m_pParent->GetDictionary().AddKey( "Filter", vFilterList );

        try {
            // add an empty DecodeParams dictionary to the list of DecodeParams,
            // but only if another DecodeParams dictionary does exist.
            if( tDecodeParams.size() ) 
            {
                tDecodeParams.insert( tDecodeParams.begin(), NULL );
                // Write the decode params back to the file
                SetDecodeParms( &tDecodeParams ); // throws an exception on error
            }
            
            FlateCompressStreamData(); // throws an exception on error
        } catch( PdfError & e ) {
            e.AddToCallstack( __FILE__, __LINE__ );

            FreeDecodeParms( &tDecodeParams );

            throw e;
        }
    }
    else
    {
        m_pParent->GetDictionary().AddKey( "Filter", PdfName( "FlateDecode" ) );
        FlateCompressStreamData();
    }
}

void PdfMemStream::Uncompress()
{
    long         lLen;
    char*        pBuffer;
    
    if( m_pParent && m_pParent->IsDictionary() && m_pParent->GetDictionary().HasKey( "Filter" ) && m_buffer.GetSize() )
    {
        this->GetFilteredCopy( &pBuffer, &lLen );
        this->Set( pBuffer, lLen );
        m_pParent->GetDictionary().RemoveKey( "Filter" ); 
        
        // TODO: DS:
        // If there is a decode params dictionary it might
        // be necessary to remove it
    }
}

void PdfMemStream::FlateCompressStreamData()
{
    char*            pBuffer;
    long             lLen;

    if( !m_buffer.GetSize() ) 
        return;

    std::auto_ptr<PdfFilter> pFilter = PdfFilterFactory::Create( ePdfFilter_FlateDecode );
    if( pFilter.get() )
    {
        pFilter->Encode( m_buffer.GetBuffer(), m_buffer.GetSize(), &pBuffer, &lLen );
        this->Set( pBuffer, lLen );
    }
    else
    {
        PODOFO_RAISE_ERROR( ePdfError_UnsupportedFilter );
    }
}

const PdfStream & PdfMemStream::operator=( const PdfStream & rhs )
{
    const PdfMemStream* pStream = dynamic_cast<const PdfMemStream*>(&rhs);
    if( pStream )
        m_buffer = pStream->m_buffer;
    else
        return PdfStream::operator=( rhs );

    if( m_pParent ) 
        m_pParent->GetDictionary().AddKey( PdfName::KeyLength, PdfVariant( static_cast<long>(m_buffer.GetSize()) ) );

    return *this;
}

void PdfMemStream::Write( PdfOutputDevice* pDevice ) 
{
    pDevice->Print( "stream\n" );
    pDevice->Write( this->Get(), this->GetLength() );
    pDevice->Print( "\nendstream\n" );
}

};

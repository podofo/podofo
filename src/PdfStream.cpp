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
#include "PdfObject.h"
#include "PdfVariant.h"

namespace PoDoFo {

#define STREAM_SIZE_INCREASE 1024

PdfStream::PdfStream( PdfObject* pParent )
    : m_pParent( pParent )
{
}

PdfStream::PdfStream( const PdfStream & rhs )
    : m_pParent( NULL )
{
    operator=(rhs);
}

PdfStream::~PdfStream()
{
}

void PdfStream::Empty()
{
    m_buffer = PdfRefCountedBuffer();

    if( m_pParent )
        m_pParent->GetDictionary().AddKey( PdfName::KeyLength, PdfVariant( static_cast<long>(m_buffer.GetSize()) ) );
}

void PdfStream::Set( char* szBuffer, long lLen, bool takePossession )
{
    PdfRefCountedBuffer tmp( szBuffer, lLen );

    if( !szBuffer )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_buffer = tmp;
    m_buffer.SetTakePossesion( takePossession );

    if( m_pParent )
        m_pParent->GetDictionary().AddKey( PdfName::KeyLength, PdfVariant( static_cast<long>(m_buffer.GetSize()) ) );
}

void PdfStream::Set( const char* pszString )
{
    if( !pszString )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // clear the buffer
    this->Empty();

    this->Append( pszString );
}

void PdfStream::Append( const char* pszString )
{
    if( !pszString )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    this->Append( pszString, strlen( pszString ) );
}

void PdfStream::Append( const char* pszString, size_t lLen )
{
    if( !pszString )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // only append to uncommpressed streams
    this->Uncompress();

    m_buffer.Append( pszString, lLen );

    if( m_pParent )
        m_pParent->GetDictionary().AddKey( PdfName::KeyLength, PdfVariant( static_cast<long>(m_buffer.GetSize()) ) );
}

void PdfStream::GetCopy( char** pBuffer, long* lLen ) const
{
    if( !pBuffer || !lLen )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    *pBuffer = static_cast<char*>(malloc( sizeof( char ) * m_buffer.GetSize() ));
    *lLen = m_buffer.GetSize();
    
    if( !*pBuffer )
    {
        RAISE_ERROR( ePdfError_OutOfMemory );
    }
    
    memcpy( *pBuffer, m_buffer.GetBuffer(), m_buffer.GetSize() );
}

void PdfStream::GetFilteredCopy( char** ppBuffer, long* lLen ) const
{
    TVecFilters            vecFilters;
    TIVecFilters           it;
    const PdfFilter*       pFilter;

    TVecDictionaries       tDecodeParams;
    TCIVecDictionaries     itDecodeParams;

    char*           pInBuf;
    long            lInLen;

    if( !ppBuffer || !lLen )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    *ppBuffer = NULL;
    *lLen     = 0;

    if( !m_buffer.GetSize() )
        return;

    pInBuf = m_buffer.GetBuffer();
    lInLen = m_buffer.GetSize();

    FillFilterList( vecFilters );
    GetDecodeParms( &tDecodeParams );

    if( tDecodeParams.size() > 0 && tDecodeParams.size() != vecFilters.size() )
    {
        RAISE_ERROR( ePdfError_InvalidPredictor );
    }

    if( !vecFilters.empty() )
    {
        it = vecFilters.begin();
        if( tDecodeParams.size() )
            itDecodeParams = tDecodeParams.begin();

        while( it != vecFilters.end() ) 
        {
            pFilter = PdfFilterFactory::Create( *it );
            if( !pFilter ) 
            {
                FreeDecodeParms( &tDecodeParams );

                PdfError::LogMessage( eLogSeverity_Error, "Error: Found an unsupported filter: %i\n", *it );
                RAISE_ERROR( ePdfError_UnsupportedFilter );
                break;
            }

            try {            
                pFilter->Decode( pInBuf, lInLen, ppBuffer, lLen, tDecodeParams.size() ? *itDecodeParams : NULL );
            } catch( PdfError & e ) {
                e.AddToCallstack( __FILE__, __LINE__ );

                if( pInBuf != m_buffer.GetBuffer() )
                {
                    // the input buffer was malloc'ed by another filter before
                    // so free it and let it point to the output buffer
                    free( pInBuf );
                }

                FreeDecodeParms( &tDecodeParams );
                free( *ppBuffer );

                throw e;
            }
  
            if( pInBuf != m_buffer.GetBuffer() )
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

void PdfStream::FlateCompress()
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

void PdfStream::Uncompress()
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

void PdfStream::FlateCompressStreamData()
{
    const PdfFilter* pFilter;
    char*            pBuffer;
    long             lLen;

    if( !m_buffer.GetSize() ) 
        return;

    pFilter = PdfFilterFactory::Create( ePdfFilter_FlateDecode );
    if( pFilter ) 
    {
        pFilter->Encode( m_buffer.GetBuffer(), m_buffer.GetSize(), &pBuffer, &lLen );
        this->Set( pBuffer, lLen );
    }
    else
    {
        RAISE_ERROR( ePdfError_UnsupportedFilter );
    }
}

void PdfStream::FillFilterList( TVecFilters & vecFilters ) const
{
    PdfObject* pObj;
    EPdfFilter eFilter = ePdfFilter_Unknown;

    vecFilters.clear();

    if( m_pParent->GetDictionary().HasKey( "Filter" ) )
    {
        pObj = m_pParent->GetDictionary().GetKey( "Filter" );
        if( pObj->IsName() )
        {
            eFilter = FilterNameToType( pObj->GetName() );
            vecFilters.push_back( eFilter );
        }
        else if( pObj->IsArray() )
        {
            TVariantList vecVariants = pObj->GetArray();
            TIVariantList it = vecVariants.begin();

            while( it != vecVariants.end() )
            {
                if( (*it).GetDataType() != ePdfDataType_Name )
                {
                    RAISE_ERROR( ePdfError_InvalidDataType );
                }

                eFilter = PdfStream::FilterNameToType( (*it).GetName() );
                vecFilters.push_back( eFilter );
                
                ++it;
            }
        }
        else
        {
            RAISE_ERROR( ePdfError_InvalidDataType );
        }
    }
}

EPdfFilter PdfStream::FilterNameToType( const PdfName & name )
{
    static const char* aszFilters[] = {
        "ASCIIHexDecode",
        "ASCII85Decode",
        "LZWDecode",
        "FlateDecode",
        "RunLengthDecode",
        "CCITTFaxDecode", 
        "JBIG2Decode",
        "DCTDecode",
        "JPXDecode",
        "Crypt",
        NULL
    };

    int i = 0;

    if( !name.GetLength() )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    while( aszFilters[i] )
    {
        if( name == aszFilters[i] )
            return static_cast<EPdfFilter>(i);
        
        ++i;
    }

    RAISE_ERROR( ePdfError_UnsupportedFilter );
    return ePdfFilter_Unknown;
}

const PdfStream & PdfStream::operator=( const PdfStream & rhs )
{
    m_buffer = rhs.m_buffer;

    if( m_pParent ) 
        m_pParent->GetDictionary().AddKey( PdfName::KeyLength, PdfVariant( static_cast<long>(m_buffer.GetSize()) ) );

    return *this;
}

void PdfStream::GetDecodeParms( TVecDictionaries* pParams ) const
{
    PdfObject*               pObj = NULL;
    PdfArray                 array;
    PdfArray::const_iterator it;

    if( !pParams )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
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
        RAISE_ERROR( ePdfError_InvalidDataType );
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
            RAISE_ERROR( ePdfError_InvalidDataType );
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
        RAISE_ERROR( ePdfError_InvalidHandle );
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

};

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

#include "PdfFilter.h"
#include "PdfObject.h"
#include "PdfVariant.h"

namespace PoDoFo {

#define STREAM_SIZE_INCREASE 1024

PdfStream::PdfStream( PdfObject* pParent )
    :m_pParent( pParent ) 
{
    m_szStream = NULL;
    m_lLength  = 0;
    m_lSize    = 0;
	m_bOwnedBuffer = false;
}

PdfStream::PdfStream( const PdfStream & rhs )
{
    m_szStream = NULL;
    m_lLength  = 0;
    m_lSize    = 0;
	m_bOwnedBuffer = false;

    operator=(rhs);
}

PdfStream::~PdfStream()
{
    if( m_szStream && m_lLength && m_bOwnedBuffer )
        free( m_szStream );
}

PdfError PdfStream::Set( char* szBuffer, long lLen, bool takePossession )
{
    PdfError eCode;

    if( m_szStream && m_lLength && m_bOwnedBuffer )
        free( m_szStream );

    if( !szBuffer )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_szStream = szBuffer;
    m_lLength  = lLen;
    m_lSize    = lLen;

    return eCode;
}

PdfError PdfStream::Set( const char* pszString )
{
    PdfError eCode;

    if( !pszString )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( m_szStream && m_lLength )
    {
		if ( m_bOwnedBuffer )
			free( m_szStream );

        m_szStream = NULL;
        m_lLength  = 0;
        m_lSize    = 0;
    }

    return Append( pszString );
}

PdfError PdfStream::Append( const char* pszString )
{
    PdfError eCode;

    if( !pszString )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    return this->Append( pszString, strlen( pszString ) );
}

PdfError PdfStream::Append( const char* pszString, long lLen )
{
    PdfError eCode;
    long     lSize;
    char*    pBuf;

    if( !pszString )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( m_lSize < m_lLength + lLen )
    {
        // TODO: increase the size as in a vector
        lSize = (((m_lLength + lLen) / STREAM_SIZE_INCREASE) + 1) * STREAM_SIZE_INCREASE;
        pBuf = (char*)malloc( sizeof( char ) * lSize );
        if( !pBuf )
        {
            RAISE_ERROR( ePdfError_OutOfMemory );
        }

        memcpy( pBuf, m_szStream, m_lLength );
        memcpy( pBuf+m_lLength, pszString, lLen );
        
        free( m_szStream );
        m_szStream = pBuf;

        m_lLength += lLen;
        m_lSize    = lSize;
    }
    else
    {
        memcpy( m_szStream+m_lLength, pszString, lLen );
        m_lLength += lLen;
    }

    return eCode;
}

PdfError PdfStream::GetCopy( char** pBuffer, long* lLen ) const
{
    PdfError eCode;
    
    if( !pBuffer || !lLen )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    *pBuffer = (char*)malloc( sizeof( char ) * m_lLength );
    *lLen = m_lLength;
    
    if( !*pBuffer )
    {
        RAISE_ERROR( ePdfError_OutOfMemory );
    }
    
    memcpy( *pBuffer, m_szStream, m_lLength );
    
    return eCode;
}

PdfError PdfStream::GetFilteredCopy( char** ppBuffer, long* lLen ) const
{
    PdfError               eCode;
    TVecFilters            vecFilters;
    TIVecFilters           it;
    const PdfFilter*       pFilter;

    TVecObjects     tDecodeParams;
    TCIVecObjects   itDecodeParams;

    char*           pInBuf = m_szStream;
    long            lInLen = m_lLength;

    if( !ppBuffer || !lLen )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    SAFE_OP( FillFilterList( vecFilters ) );
    SAFE_OP( GetDecodeParms( &tDecodeParams ) );

    tDecodeParams.SetAutoDelete( true );

    if( tDecodeParams.size() > 0 && tDecodeParams.size() != vecFilters.size() )
    {
        RAISE_ERROR( ePdfError_InvalidPredictor );
    }

    if( !vecFilters.empty() )
    {
        it = vecFilters.begin();
        if( tDecodeParams.size() )
            itDecodeParams = tDecodeParams.begin();

        while( it != vecFilters.end() && !eCode.IsError() ) 
        {
            pFilter = PdfFilterFactory::Create( *it );
            if( !pFilter ) 
            {
                PdfError::LogMessage( eLogSeverity_Error, "Error: Found an unsupported filter: %i\n", *it );
                eCode.SetError( ePdfError_UnsupportedFilter );
                break;
            }

            eCode = pFilter->Decode( pInBuf, lInLen, ppBuffer, lLen, tDecodeParams.size() ? *itDecodeParams : NULL );
            
            if( pInBuf != m_szStream ) 
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
        *ppBuffer = (char*)malloc( sizeof( char ) * m_lLength );
        *lLen = m_lLength;

        if( !*ppBuffer )
        {
            RAISE_ERROR( ePdfError_OutOfMemory );
        }
                
        memcpy( *ppBuffer, m_szStream, m_lLength );
    }

    if( eCode.IsError() )
        free( *ppBuffer );

    return eCode;
}

PdfError PdfStream::FlateDecode()
{
    PdfError       eCode;
    PdfVariant     vVar;
    PdfVariant     vFilter;
    PdfVariant     vFilterList;
    TVariantList   tFilters;
    TVecObjects    tDecodeParams;
    TCIVariantList tciFilters;
    
    if( !m_szStream )
        return eCode; // ePdfError_ErrOk

    // add another compression filter to this stream
    vFilter.Init( "FlateDecode", ePdfDataType_Name );

    if( m_pParent->HasKey( "Filter" ) )
    {
        // DecodeParms dictionaries of already used filters
        // have to be handled.
        SAFE_OP( GetDecodeParms( &tDecodeParams ) );
        tDecodeParams.SetAutoDelete( true );

        SAFE_OP( m_pParent->GetKeyValueVariant( "Filter", vVar ) );
        if( vVar.GetDataType() == ePdfDataType_Name )
        {
            if( vVar.GetName() != "DCTDecode" && vVar.GetName() != "FlateDecode" )
            {
                tFilters.push_back( vFilter );
                tFilters.push_back( vVar );
            }
            else
                // do not compress DCTDecoded are already FlateDecoded streams again
                return ePdfError_ErrOk;
        }
        else if( vVar.GetDataType() == ePdfDataType_Array )
        {
            tciFilters = vVar.GetArray().begin();

            while( tciFilters != vVar.GetArray().end() )
            {
                if( (*tciFilters).GetDataType() == ePdfDataType_Name )
                {
                    // do not compress DCTDecoded are already FlateDecoded streams again
                    if( (*tciFilters).GetName() == "DCTDecode" || (*tciFilters).GetName() == "FlateDecode" )
                        return ePdfError_ErrOk;
                }

                ++tciFilters;
            }

            tFilters.push_back( vFilter );

            tciFilters = vVar.GetArray().begin();

            while( tciFilters != vVar.GetArray().end() )
            {
                tFilters.push_back( (*tciFilters) );
                
                ++tciFilters;
            }
        }
        else if( vVar.GetDataType() == ePdfDataType_Reference )
        {
            // TODO: handle references
            return ePdfError_ErrOk;
        }
        else
            // TODO: handle other cases
            return ePdfError_ErrOk;

        vFilterList.Init( tFilters );
        m_pParent->AddKey( "Filter", vFilterList );

        // add an empty DecodeParams dictionary to the list of DecodeParams,
        // but only if another DecodeParams dictionary does exist.
        if( tDecodeParams.size() ) 
        {
            tDecodeParams.insert( tDecodeParams.begin(), NULL );
            // Write the decode params back to the file
            SAFE_OP( SetDecodeParms( &tDecodeParams ) );
        }

        SAFE_OP( FlateDecodeStreamData() );
    }
    else
    {
        m_pParent->AddKey( "Filter", PdfName( "FlateDecode" ) );
        SAFE_OP( FlateDecodeStreamData() );
    }
    
    return eCode;
}

PdfError PdfStream::FlateDecodeStreamData()
{
    PdfError         eCode;
    const PdfFilter* pFilter;
    char*            pBuffer;
    long             lLen;

    pFilter = PdfFilterFactory::Create( ePdfFilter_FlateDecode );
    if( pFilter ) 
    {
        SAFE_OP( pFilter->Encode( m_szStream, m_lLength, &pBuffer, &lLen ) );
        free( m_szStream );
    
        m_szStream  = pBuffer;
        m_lSize     = lLen;
        m_lLength   = lLen;
    }
    else
    {
        RAISE_ERROR( ePdfError_UnsupportedFilter );
    }

    return eCode;
}

PdfError PdfStream::FillFilterList( TVecFilters & vecFilters ) const
{
    PdfError   eCode;
    PdfVariant variant;
    EPdfFilter eFilter = ePdfFilter_Unknown;

    vecFilters.clear();

    if( m_pParent->HasKey( "Filter" ) )
    {
        SAFE_OP( m_pParent->GetKeyValueVariant( "Filter", variant ) );
        std::string str;
        variant.ToString( str );
        if( variant.GetDataType() == ePdfDataType_Name )
        {
            SAFE_OP( FilterNameToType( variant.GetName(), &eFilter ) );
            vecFilters.push_back( eFilter );
        }
        else if( variant.GetDataType() == ePdfDataType_Array )
        {
            TVariantList vecVariants = variant.GetArray();
            TIVariantList it = vecVariants.begin();

            while( it != vecVariants.end() )
            {
                if( (*it).GetDataType() != ePdfDataType_Name )
                {
                    RAISE_ERROR( ePdfError_InvalidDataType );
                }

                SAFE_OP( PdfStream::FilterNameToType( (*it).GetName(), &eFilter ) );
                vecFilters.push_back( eFilter );
                
                ++it;
            }
        }
        else
        {
            RAISE_ERROR( ePdfError_InvalidDataType );
        }
    }

    return eCode;
}

PdfError PdfStream::FilterNameToType( const PdfName & name, EPdfFilter* peFilter )
{
    PdfError eCode;

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

    if( !name.Length() || !peFilter )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    while( aszFilters[i] )
    {
        if( name == aszFilters[i] )
        {
            *peFilter = (EPdfFilter)i;
            return eCode;
        }
        
        ++i;
    }

    eCode.SetError( ePdfError_UnsupportedFilter );
    return eCode;
}

const PdfStream & PdfStream::operator=( const PdfStream & rhs )
{
    // Clear any existing old data
    if( m_szStream && m_lLength )
        free( m_szStream );

    m_pParent  = rhs.m_pParent;
    m_lSize    = rhs.m_lLength;
    m_lLength  = rhs.m_lLength;
	m_bOwnedBuffer = true;	// since we're making a copy
    
    m_szStream = (char*)malloc( sizeof(char) * m_lLength );
    memcpy( m_szStream, rhs.m_szStream, m_lLength );

    return *this;
}

PdfError PdfStream::GetDecodeParms( TVecObjects* pParams ) const
{
    PdfError       eCode;
    PdfObject*     pDecodeParams;
    PdfVariant     array;
    TVariantList   list;
    TCIVariantList it;

    if( !pParams )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( m_pParent->HasObjectKey( "DecodeParms" ) || m_pParent->HasObjectKey( "DP" ) )
    {
        pDecodeParams = m_pParent->GetKeyValueObject( "DecodeParms" );
        
        // See Implementation Note 3.2.7:
        // Adobe Viewers support DP as abbreviation for DecodeParms
        if( !pDecodeParams )
            pDecodeParams = m_pParent->GetKeyValueObject( "DP" );

        pParams->push_back( new PdfObject( *pDecodeParams ) );
        return eCode;
    }
    else if( m_pParent->HasKey( "DecodeParms" ) )
    {
        SAFE_OP( m_pParent->GetKeyValueVariant( "DecodeParms", array ) );
    }
    else if( m_pParent->HasKey( "DP" ) )
    {
        SAFE_OP( m_pParent->GetKeyValueVariant( "DP", array ) );
    }

    // No decode params dictionary!
    if( array.IsEmpty() )
        return eCode;

    if( array.GetDataType() != ePdfDataType_Array ) 
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    list = array.GetArray();
    it = list.begin();

    while( it != list.end() ) 
    {
        if( (*it).GetDataType() == ePdfDataType_Null )
            pParams->push_back( NULL );
        else if( (*it).GetDataType() == ePdfDataType_Dictionary )
            pParams->push_back( new PdfObject( (*it).GetDictionary()) );
        else
        {
            RAISE_ERROR( ePdfError_InvalidDataType );
        }

        ++it;
    }
    
    return eCode;
}

PdfError PdfStream::SetDecodeParms( TVecObjects* pParams )
{
    PdfError       eCode;
    PdfVariant     var;
    TVariantList   array;    
    TCIVecObjects   it;

    if( !pParams )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // remove any existing old keys first
    if( m_pParent->HasKey( "DecodeParms" ) )
    {
        m_pParent->RemoveKey( "DecodeParms" );
    }
    else if( m_pParent->HasKey( "DP" ) )
    {
        m_pParent->RemoveKey( "DP" );
    }
    else if( m_pParent->HasObjectKey( "DecodeParms" ) ) 
    {
        m_pParent->RemoveObjectKey( "DecodeParms" );
    }
    else if( m_pParent->HasObjectKey( "DP" ) ) 
    {
        m_pParent->RemoveObjectKey( "DP" );
    }


    // add the new DecodeParms 
    if( pParams->size() > 1 ) 
    {
        it = pParams->begin();
        while( it != pParams->end() )
        {
            if( *it ) 
            {
                var.SetDataType( ePdfDataType_Dictionary );
                var.SetDictionary( *(*it) );
                array.push_back( var );
            }
            else
            {
                var.SetDataType( ePdfDataType_Null );
                array.push_back( var );
            }
            
            ++it;
        }

        var.SetDataType( ePdfDataType_Array );
        var.SetArray( array );

        m_pParent->AddKey( "DecodeParms", var );
    }
    else if( pParams->size() == 1 ) 
    {
        if( (*pParams)[0] )
        {
            m_pParent->AddKey( "DecodeParms", (*pParams)[0] );
            pParams->SetAutoDelete( false );
        }
    }

    return eCode;
}

};

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

namespace PoDoFo {

PdfStream::PdfStream( PdfObject* pParent )
    : m_pParent( pParent )
{
}

PdfStream::~PdfStream()
{
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

    if( !this->GetInternalBufferSize() )
        return;

    pInBuf = const_cast<char*>(this->GetInternalBuffer());
    lInLen = this->GetInternalBufferSize();

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


};

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
#include "PdfOutputStream.h"
#include "PdfVariant.h"

namespace PoDoFo {

#define STREAM_SIZE_INCREASE 1024

/** An output stream that writes to a PdfRefCountedBuffer
 */
class PODOFO_API PdfBufferOutputStream : public PdfOutputStream {
 public:
    
    /** 
     *  Write to an already opened input device
     * 
     *  \param pBuffer data is written to this buffer
     */
    PdfBufferOutputStream( PdfRefCountedBuffer* pBuffer )
        : m_pBuffer( pBuffer )
    {
    }
    
    /** Write data to the output stream
     *  
     *  \param pBuffer the data is read from this buffer
     *  \param lLen    the size of the buffer 
     *
     *  \returns the number of bytes written, -1 if an error ocurred
     */
    virtual long Write( const char* pBuffer, long lLen )
    {
        m_pBuffer->Append( pBuffer, lLen );
        return lLen;
    }

    virtual void Close() 
    {
    }

 private:
    PdfRefCountedBuffer* m_pBuffer;
};

PdfMemStream::PdfMemStream( PdfObject* pParent )
    : PdfStream( pParent ), m_pStream( NULL ), m_pBufferStream( NULL )
{
}

PdfMemStream::PdfMemStream( const PdfMemStream & rhs )
    : PdfStream( NULL ), m_pStream( NULL ), m_pBufferStream( NULL )
{
    operator=(rhs);
}

PdfMemStream::~PdfMemStream()
{
}

void PdfMemStream::BeginAppendImpl( const TVecFilters & vecFilters )
{
    m_buffer = PdfRefCountedBuffer();

    if( vecFilters.size() )
    {
        m_pBufferStream = new PdfBufferOutputStream( &m_buffer );
        m_pStream       = PdfFilterFactory::CreateEncodeStream( vecFilters, m_pBufferStream );
    }
    else 
        m_pStream = new PdfBufferOutputStream( &m_buffer );

}

void PdfMemStream::AppendImpl( const char* pszString, size_t lLen )
{
    m_pStream->Write( pszString, lLen );
}

void PdfMemStream::EndAppendImpl()
{
    if( m_pStream ) 
    {
        m_pStream->Close();
        delete m_pStream;
        m_pStream = NULL;
    }

    if( m_pBufferStream ) 
    {
        m_pBufferStream->Close();
        delete m_pBufferStream;
        m_pBufferStream = NULL;
    }

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

    PdfArray::const_iterator tciFilters;
    
    if( !m_buffer.GetSize() )
        return; // ePdfError_ErrOk

    // TODO: Handle DecodeParms
    if( m_pParent->GetDictionary().HasKey( "Filter" ) )
    {
        pObj = m_pParent->GetIndirectKey( "Filter" );

        if( pObj->IsName() )
        {
            if( pObj->GetName() != "DCTDecode" && pObj->GetName() != "FlateDecode" )
            {
                tFilters.push_back( vFilter );
                tFilters.push_back( *pObj );
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
            return;

        vFilterList = PdfVariant( tFilters );
        m_pParent->GetDictionary().AddKey( "Filter", vFilterList );

        FlateCompressStreamData(); // throws an exception on error
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
    
    TVecFilters  vecEmpty;

    if( m_pParent && m_pParent->IsDictionary() && m_pParent->GetDictionary().HasKey( "Filter" ) && m_buffer.GetSize() )
    {
        this->GetFilteredCopy( &pBuffer, &lLen );
        this->Set( pBuffer, lLen, vecEmpty );
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

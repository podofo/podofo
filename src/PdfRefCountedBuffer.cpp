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

#include "PdfRefCountedBuffer.h"

namespace PoDoFo {

#define STREAM_SIZE_INCREASE 1024

PdfRefCountedBuffer::PdfRefCountedBuffer()
    : m_pBuffer( NULL )
{

}

PdfRefCountedBuffer::PdfRefCountedBuffer( char* pBuffer, long lSize )
    : m_pBuffer( NULL )
{
    if( pBuffer && lSize ) 
    {
        m_pBuffer = new TRefCountedBuffer();
        m_pBuffer->m_lRefCount     = 1;
        m_pBuffer->m_pBuffer       = pBuffer;
        m_pBuffer->m_lSize         = 0;
        m_pBuffer->m_lInternalSize = lSize;
        m_pBuffer->m_bPossesion    = true;
    }
}

PdfRefCountedBuffer::PdfRefCountedBuffer( long lSize )
    : m_pBuffer( NULL )
{
    this->Resize( lSize );
}

PdfRefCountedBuffer::PdfRefCountedBuffer( const PdfRefCountedBuffer & rhs )
    : m_pBuffer( NULL )
{
    this->operator=( rhs );
}

PdfRefCountedBuffer::~PdfRefCountedBuffer()
{
    FreeBuffer();
}

void PdfRefCountedBuffer::FreeBuffer()
{
    if( m_pBuffer && !--m_pBuffer->m_lRefCount ) 
    {
        // last owner of the file!
        if( m_pBuffer->m_bPossesion )
            free( m_pBuffer->m_pBuffer );
        delete m_pBuffer;
        m_pBuffer = NULL;
    }
}

void PdfRefCountedBuffer::Detach( long lExtraLen )
{
    if( m_pBuffer )
    {
        if( m_pBuffer->m_lRefCount == 1 ) 
        {
            // it is our buffer, so no operation has to be performed
            return;
        }
        else
        {
            long lSize                 = m_pBuffer->m_lInternalSize + lExtraLen; 
            TRefCountedBuffer* pBuffer = new TRefCountedBuffer();
            pBuffer->m_lRefCount       = 1;
            pBuffer->m_pBuffer         = static_cast<char*>(malloc( sizeof(char)*lSize ));
            pBuffer->m_lSize           = m_pBuffer->m_lSize;
            pBuffer->m_lInternalSize   = lSize;
            pBuffer->m_bPossesion      = true;
        
            if( !pBuffer->m_pBuffer ) 
            {
                delete pBuffer;
                pBuffer = NULL;

                PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
            }

            memcpy( pBuffer->m_pBuffer, this->GetBuffer(), this->GetSize() );
            --m_pBuffer->m_lRefCount;

            m_pBuffer = pBuffer;
        }
    }
}

void PdfRefCountedBuffer::Resize( size_t lSize ) 
{
    if( m_pBuffer ) 
    {
        this->Detach( m_pBuffer->m_lInternalSize < lSize ? lSize - m_pBuffer->m_lInternalSize : 0 );
        if( m_pBuffer->m_lInternalSize < lSize )
        {
            m_pBuffer->m_lInternalSize = PDF_MAX(lSize,m_pBuffer->m_lInternalSize << 1 );

            char* pBuffer = static_cast<char*>(malloc( sizeof(char) * m_pBuffer->m_lInternalSize ));
            if( !pBuffer ) 
            {
                PODOFO_RAISE_ERROR_INFO( ePdfError_OutOfMemory, "PdfRefCountedBuffer::Resize failed!" );
            }
            memcpy( pBuffer, m_pBuffer->m_pBuffer, m_pBuffer->m_lSize );
            
            if( m_pBuffer->m_bPossesion )
                free( m_pBuffer->m_pBuffer );
            m_pBuffer->m_pBuffer = pBuffer;
        }
        else
        {
            // buffer is large enough, do nothing
            return;
        }
    }
    else
    {
        m_pBuffer = new TRefCountedBuffer();
        m_pBuffer->m_lRefCount     = 1;
        m_pBuffer->m_pBuffer       = static_cast<char*>(malloc( sizeof(char)*lSize ));
        m_pBuffer->m_lSize         = 0;
        m_pBuffer->m_lInternalSize = lSize;
        m_pBuffer->m_bPossesion    = true;
        
        if( !m_pBuffer->m_pBuffer ) 
        {
            delete m_pBuffer;
            m_pBuffer = NULL;

            PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
        }
    }
}

void PdfRefCountedBuffer::Append( const char* pszString, long lLen )
{
    if( m_pBuffer )
    {
        // will do a detach if necessary
        this->Resize( m_pBuffer->m_lSize + lLen );

        memcpy( m_pBuffer->m_pBuffer + m_pBuffer->m_lSize, pszString, lLen );
        m_pBuffer->m_lSize += lLen;
    }
    else
    {
        // Allocate a completely new buffer
        PdfRefCountedBuffer buffer( lLen );
        buffer.Append( pszString, lLen );
        *this = buffer;
    }
}

const PdfRefCountedBuffer & PdfRefCountedBuffer::operator=( const PdfRefCountedBuffer & rhs )
{
    FreeBuffer();

    m_pBuffer = rhs.m_pBuffer;
    if( m_pBuffer )
        m_pBuffer->m_lRefCount++;

    return *this;
}

bool PdfRefCountedBuffer::operator==( const PdfRefCountedBuffer & rhs ) const
{
    if( m_pBuffer != rhs.m_pBuffer )
    {
        if( m_pBuffer && rhs.m_pBuffer ) 
        {
            /*
              char* m_pBuffer;
              long  m_lSize;
              long  m_lInternalSize;
              long  m_lRefCount;
              bool  m_bPossesion;
            */
            
            return (memcmp( m_pBuffer->m_pBuffer, rhs.m_pBuffer->m_pBuffer, PDF_MIN( m_pBuffer->m_lSize, rhs.m_pBuffer->m_lSize ) ) == 0 );
        }
        else 
            return false;
    }

    return true;
}

bool PdfRefCountedBuffer::operator<( const PdfRefCountedBuffer & rhs ) const
{
    // equal buffers are neither smaller nor greater
    if( m_pBuffer == rhs.m_pBuffer )
        return false;

    if( !m_pBuffer && rhs.m_pBuffer ) 
        return true;
    else if( m_pBuffer && !rhs.m_pBuffer ) 
        return false;
    else
        return (memcmp( m_pBuffer->m_pBuffer, rhs.m_pBuffer->m_pBuffer, PDF_MIN( m_pBuffer->m_lSize, rhs.m_pBuffer->m_lSize ) ) < 0 );

    return false;
}

bool PdfRefCountedBuffer::operator>( const PdfRefCountedBuffer & rhs ) const
{
    // equal buffers are neither smaller nor greater
    if( m_pBuffer == rhs.m_pBuffer )
        return false;

    if( !m_pBuffer && rhs.m_pBuffer ) 
        return false;
    else if( m_pBuffer && !rhs.m_pBuffer ) 
        return true;
    else
        return (memcmp( m_pBuffer->m_pBuffer, rhs.m_pBuffer->m_pBuffer, PDF_MIN( m_pBuffer->m_lSize, rhs.m_pBuffer->m_lSize ) ) > 0 );

    return false;
}


};

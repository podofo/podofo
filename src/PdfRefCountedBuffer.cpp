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

PdfRefCountedBuffer::PdfRefCountedBuffer( char* pBuffer, long lSize )
    : m_pBuffer( NULL )
{
    if( pBuffer && lSize ) 
    {
        m_pBuffer = new TRefCountedBuffer();
        m_pBuffer->m_lRefCount     = 1;
        m_pBuffer->m_pBuffer       = pBuffer;
        m_pBuffer->m_lBufferSize   = lSize;
        m_pBuffer->m_lVisibleSize  = lSize;
        m_pBuffer->m_bPossesion    = true;
    }
}

void PdfRefCountedBuffer::FreeBuffer()
{
    if( m_pBuffer && !--m_pBuffer->m_lRefCount ) 
    {
        if( m_pBuffer->m_bPossesion )
        {
            free( m_pBuffer->m_pBuffer );
            m_pBuffer->m_pBuffer = 0;
        }
        delete m_pBuffer;
        // last owner of the file!
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
            long lSize                 = m_pBuffer->m_lBufferSize + lExtraLen; 
            TRefCountedBuffer* pBuffer = new TRefCountedBuffer();
            pBuffer->m_lRefCount       = 1;
            pBuffer->m_pBuffer         = static_cast<char*>(malloc( sizeof(char)*lSize ));
            pBuffer->m_lBufferSize     = lSize;
            pBuffer->m_bPossesion      = true;
        
            if( !pBuffer->m_pBuffer ) 
            {
                delete pBuffer;
                pBuffer = NULL;

                PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
            }

            memcpy( pBuffer->m_pBuffer, this->GetBuffer(), this->GetSize() );
            --m_pBuffer->m_lRefCount;
            // Detaching the buffer should have NO visible effect to clients.
            pBuffer->m_lVisibleSize    = m_pBuffer->m_lVisibleSize;

            m_pBuffer = pBuffer;
        }
    }
}

void PdfRefCountedBuffer::Resize( const size_t lSize ) 
{
    if( m_pBuffer ) 
    {
        // Resizing the buffer counts as altering it, so detach as per copy on write behaviour. If the detach
        // actually has to do anything it'll reallocate the buffer at the new desired size.
        this->Detach( static_cast<size_t>(m_pBuffer->m_lBufferSize) < lSize ? lSize - static_cast<size_t>(m_pBuffer->m_lBufferSize) : 0 );
        // We might have pre-allocated enough to service the request already
        if( static_cast<size_t>(m_pBuffer->m_lBufferSize) < lSize )
        {
            // Allocate more space, since we need it. We over-allocate so that clients can efficiently
            // request lots of small resizes if they want, but these over allocations are not visible
            // to clients.
            //
            // TODO: Why don't we use realloc(...) here  since we're using C-style memory management?
            //
            const size_t lAllocSize = PDF_MAX(lSize,static_cast<size_t>(m_pBuffer->m_lBufferSize) << 1 );
            char* pBuffer = static_cast<char*>(malloc( sizeof(char) * lAllocSize ));
            if( !pBuffer ) 
            {
                PODOFO_RAISE_ERROR_INFO( ePdfError_OutOfMemory, "PdfRefCountedBuffer::Resize failed!" );
            }
            // Only bother copying the visible portion of the buffer. It's completely incorrect
            // to rely on anything more than that, and not copying it will help catch those errors.
            memcpy( pBuffer, m_pBuffer->m_pBuffer, m_pBuffer->m_lVisibleSize );
            // Record the newly allocated size. The visible size gets updated later.
            m_pBuffer->m_lBufferSize = lAllocSize;

            if( m_pBuffer->m_bPossesion )
                free( m_pBuffer->m_pBuffer );
            m_pBuffer->m_pBuffer = pBuffer;
        }
        else
        {
            // Allocated buffer is large enough, do nothing
        }
    }
    else
    {
        // No buffer was allocated at all, so we need to make one.
        m_pBuffer = new TRefCountedBuffer();
        m_pBuffer->m_lRefCount     = 1;
        m_pBuffer->m_pBuffer       = static_cast<char*>(malloc( sizeof(char)*lSize ));
        m_pBuffer->m_lBufferSize   = lSize;
        m_pBuffer->m_bPossesion    = true;

        if( !m_pBuffer->m_pBuffer ) 
        {
            delete m_pBuffer;
            m_pBuffer = NULL;

            PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
        }
    }
    m_pBuffer->m_lVisibleSize = lSize;

    PODOFO_RAISE_LOGIC_IF ( m_pBuffer->m_lVisibleSize > m_pBuffer->m_lBufferSize, "Buffer improperly allocated/resized");
}

/*
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
*/
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
            if ( m_pBuffer->m_lVisibleSize != rhs.m_pBuffer->m_lVisibleSize )
                // Unequal buffer sizes cannot be equal buffers
                return false;
            // Test for byte-for-byte equality since lengths match
            return (memcmp( m_pBuffer->m_pBuffer, rhs.m_pBuffer->m_pBuffer, m_pBuffer->m_lVisibleSize ) == 0 );
        }
        else
            // Cannot be equal if only one object has a real data buffer
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
    {
        int cmp = memcmp( m_pBuffer->m_pBuffer, rhs.m_pBuffer->m_pBuffer, PDF_MIN( m_pBuffer->m_lVisibleSize, rhs.m_pBuffer->m_lVisibleSize ) );
        if (cmp == 0)
            // If one is a prefix of the other, ie they compare equal for the length of the shortest but one is longer,
            // the longer buffer is the greater one.
            return m_pBuffer->m_lVisibleSize < rhs.m_pBuffer->m_lVisibleSize;
        else
            return cmp < 0;
    }

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
    {
        int cmp = memcmp( m_pBuffer->m_pBuffer, rhs.m_pBuffer->m_pBuffer, PDF_MIN( m_pBuffer->m_lVisibleSize, rhs.m_pBuffer->m_lVisibleSize ) );
        if (cmp == 0)
            // If one is a prefix of the other, ie they compare equal for the length of the shortest but one is longer,
            // the longer buffer is the greater one.
            return m_pBuffer->m_lVisibleSize > rhs.m_pBuffer->m_lVisibleSize;
        else
            return cmp > 0;
    }

    return false;
}


};

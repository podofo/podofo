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

#include "PdfRefCountedBuffer.h"
#include "PdfDefinesPrivate.h"

#include <stdlib.h>
#include <string.h>

namespace PoDoFo {

PdfRefCountedBuffer::PdfRefCountedBuffer( char* pBuffer, size_t lSize )
    : m_pBuffer( NULL )
{
    if( pBuffer && lSize ) 
    {
        m_pBuffer = new TRefCountedBuffer();
        m_pBuffer->m_lRefCount     = 1;
        m_pBuffer->m_pHeapBuffer   = pBuffer;
        m_pBuffer->m_bOnHeap       = true;
        m_pBuffer->m_lBufferSize   = lSize;
        m_pBuffer->m_lVisibleSize  = lSize;
        m_pBuffer->m_bPossesion    = true;
    }
}

void PdfRefCountedBuffer::FreeBuffer()
{
    PODOFO_RAISE_LOGIC_IF( !m_pBuffer || m_pBuffer->m_lRefCount, "Tried to free in-use buffer" );

    // last owner of the file!
    if( m_pBuffer->m_bOnHeap && m_pBuffer->m_bPossesion )
        podofo_free( m_pBuffer->m_pHeapBuffer );
    delete m_pBuffer;
}

void PdfRefCountedBuffer::ReallyDetach( size_t lExtraLen )
{
    PODOFO_RAISE_LOGIC_IF( m_pBuffer && m_pBuffer->m_lRefCount == 1, "Use Detach() rather than calling ReallyDetach() directly." )

    if( !m_pBuffer )
    {
        // throw error rather than de-referencing NULL
        PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
    }
    
    size_t lSize                 = m_pBuffer->m_lBufferSize + lExtraLen;
    TRefCountedBuffer* pBuffer = new TRefCountedBuffer();
    pBuffer->m_lRefCount       = 1;

    pBuffer->m_bOnHeap = (lSize > TRefCountedBuffer::INTERNAL_BUFSIZE);
    if ( pBuffer->m_bOnHeap )
        pBuffer->m_pHeapBuffer  = static_cast<char*>(podofo_calloc( lSize, sizeof(char) ));
    else
        pBuffer->m_pHeapBuffer = 0;
    pBuffer->m_lBufferSize     = PDF_MAX( lSize, static_cast<size_t>(+TRefCountedBuffer::INTERNAL_BUFSIZE) );
    pBuffer->m_bPossesion      = true;

    if( pBuffer->m_bOnHeap && !pBuffer->m_pHeapBuffer ) 
    {
        delete pBuffer;
        pBuffer = NULL;

        PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
    }

    memcpy( pBuffer->GetRealBuffer(), this->GetBuffer(), this->GetSize() );
    // Detaching the buffer should have NO visible effect to clients, so the
    // visible size must not change.
    pBuffer->m_lVisibleSize    = m_pBuffer->m_lVisibleSize;

    // Now that we've copied the data, release our claim on the old buffer,
    // deleting it if needed, and link up the new one.
    DerefBuffer();
    m_pBuffer = pBuffer;
}

void PdfRefCountedBuffer::ReallyResize( const size_t lSize ) 
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
            const size_t lAllocSize = PDF_MAX(lSize, m_pBuffer->m_lBufferSize) << 1;
            if ( m_pBuffer->m_bPossesion && m_pBuffer->m_bOnHeap )
            {
                // We have an existing on-heap buffer that we own. Realloc()
                // it, potentially saving us a memcpy and free().
                void* temp = podofo_realloc( m_pBuffer->m_pHeapBuffer, lAllocSize );
                if (!temp)
                {
                    PODOFO_RAISE_ERROR_INFO( ePdfError_OutOfMemory, "PdfRefCountedBuffer::Resize failed!" );
                }
                m_pBuffer->m_pHeapBuffer = static_cast<char*>(temp);
                m_pBuffer->m_lBufferSize = lAllocSize;
            }
            else
            {
                // Either we don't own the buffer or it's a local static buffer that's no longer big enough.
                // Either way, it's time to move to a heap-allocated buffer we own.
                char* pBuffer = static_cast<char*>(podofo_calloc( lAllocSize, sizeof(char) ));
                if( !pBuffer ) 
                {
                    PODOFO_RAISE_ERROR_INFO( ePdfError_OutOfMemory, "PdfRefCountedBuffer::Resize failed!" );
                }
                // Only bother copying the visible portion of the buffer. It's completely incorrect
                // to rely on anything more than that, and not copying it will help catch those errors.
                memcpy( pBuffer, m_pBuffer->GetRealBuffer(), m_pBuffer->m_lVisibleSize );
                // Record the newly allocated buffer's details. The visible size gets updated later.
                m_pBuffer->m_lBufferSize = lAllocSize;
                m_pBuffer->m_pHeapBuffer = pBuffer;
                m_pBuffer->m_bOnHeap = true;
            }
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
        m_pBuffer->m_lRefCount       = 1;
        m_pBuffer->m_bOnHeap   = (lSize > TRefCountedBuffer::INTERNAL_BUFSIZE);
        if ( m_pBuffer->m_bOnHeap )
        {
            m_pBuffer->m_pHeapBuffer = static_cast<char*>(podofo_calloc( lSize, sizeof(char) ));
        }
        else
        {
            m_pBuffer->m_pHeapBuffer = 0;
        }

        m_pBuffer->m_lBufferSize     = PDF_MAX( lSize, static_cast<size_t>(+TRefCountedBuffer::INTERNAL_BUFSIZE) );
        m_pBuffer->m_bPossesion      = true;

        if( m_pBuffer->m_bOnHeap && !m_pBuffer->m_pHeapBuffer ) 
        {
            delete m_pBuffer;
            m_pBuffer = NULL;

            PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
        }
    }
    m_pBuffer->m_lVisibleSize = lSize;

    PODOFO_RAISE_LOGIC_IF ( m_pBuffer->m_lVisibleSize > m_pBuffer->m_lBufferSize, "Buffer improperly allocated/resized");
}

const PdfRefCountedBuffer & PdfRefCountedBuffer::operator=( const PdfRefCountedBuffer & rhs )
{
    // Self assignment is a no-op
    if (this == &rhs)
        return rhs;

    DerefBuffer();

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
            return (memcmp( m_pBuffer->GetRealBuffer(), rhs.m_pBuffer->GetRealBuffer(), m_pBuffer->m_lVisibleSize ) == 0 );
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
        int cmp = memcmp( m_pBuffer->GetRealBuffer(), rhs.m_pBuffer->GetRealBuffer(), PDF_MIN( m_pBuffer->m_lVisibleSize, rhs.m_pBuffer->m_lVisibleSize ) );
        if (cmp == 0)
            // If one is a prefix of the other, ie they compare equal for the length of the shortest but one is longer,
            // the longer buffer is the greater one.
            return m_pBuffer->m_lVisibleSize < rhs.m_pBuffer->m_lVisibleSize;
        else
            return cmp < 0;
    }
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
        int cmp = memcmp( m_pBuffer->GetRealBuffer(), rhs.m_pBuffer->GetRealBuffer(), PDF_MIN( m_pBuffer->m_lVisibleSize, rhs.m_pBuffer->m_lVisibleSize ) );
        if (cmp == 0)
            // If one is a prefix of the other, ie they compare equal for the length of the shortest but one is longer,
            // the longer buffer is the greater one.
            return m_pBuffer->m_lVisibleSize > rhs.m_pBuffer->m_lVisibleSize;
        else
            return cmp > 0;
    }
}


};

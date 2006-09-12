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
        m_pBuffer->m_lSize         = lSize;
        m_pBuffer->m_lInternalSize = lSize;
        m_pBuffer->m_bPossesion    = true;
    }
}

PdfRefCountedBuffer::PdfRefCountedBuffer( long lSize )
    : m_pBuffer( NULL )
{
    if( lSize ) 
    {
        m_pBuffer = new TRefCountedBuffer();
        m_pBuffer->m_lRefCount     = 1;
        m_pBuffer->m_pBuffer       = (char*)malloc( sizeof(char)*lSize );
        m_pBuffer->m_lSize         = lSize;
        m_pBuffer->m_lInternalSize = lSize;
        m_pBuffer->m_bPossesion    = true;
        
        if( !m_pBuffer->m_pBuffer ) 
        {
            delete m_pBuffer;
            m_pBuffer = NULL;

            RAISE_ERROR( ePdfError_OutOfMemory );
        }
    }
}

PdfRefCountedBuffer::PdfRefCountedBuffer( const PdfRefCountedBuffer & rhs )
    : m_pBuffer( NULL )
{
    this->operator=( rhs );
}

PdfRefCountedBuffer::~PdfRefCountedBuffer()
{
    Detach();
}

void PdfRefCountedBuffer::Detach()
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

void PdfRefCountedBuffer::Append( const char* pszString, long lLen )
{
    char* pBuffer;

    if( m_pBuffer )
    {
        if( m_pBuffer->m_lRefCount == 1 ) 
        {
            // it is our buffer, 
            // we can simply resize it and use it
            if( m_pBuffer->m_lSize + lLen < m_pBuffer->m_lInternalSize )
            {
                memcpy( m_pBuffer->m_pBuffer + m_pBuffer->m_lSize, pszString, lLen );
                m_pBuffer->m_lSize += lLen;
            }
            else
            {
                // TODO: increase the size as in a vector
                m_pBuffer->m_lInternalSize = (((m_pBuffer->m_lSize + lLen) / STREAM_SIZE_INCREASE) + 1) * STREAM_SIZE_INCREASE;
                pBuffer = (char*)malloc( sizeof(char) * m_pBuffer->m_lInternalSize );
                memcpy( pBuffer, m_pBuffer->m_pBuffer, m_pBuffer->m_lSize );
                memcpy( pBuffer + m_pBuffer->m_lSize, pszString, lLen );

                m_pBuffer->m_lSize += lLen;
                if( m_pBuffer->m_bPossesion )
                    free( m_pBuffer->m_pBuffer );
                m_pBuffer->m_pBuffer = pBuffer;
            }
        }
        else
        {
            // allocate a new buffer
            PdfRefCountedBuffer buffer( this->Size() + lLen );
            memcpy( buffer.Buffer(), this->Buffer(), this->Size() );
            memcpy( buffer.Buffer() + this->Size(), pszString, lLen );
        }
    }
    else
    {
        // Allocate a completely new buffer
        PdfRefCountedBuffer buffer( lLen );
        memcpy( buffer.Buffer(), pszString, lLen );
        *this = buffer;
    }
}

const PdfRefCountedBuffer & PdfRefCountedBuffer::operator=( const PdfRefCountedBuffer & rhs )
{
    Detach();

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

};

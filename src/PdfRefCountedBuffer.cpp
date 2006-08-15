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

PdfRefCountedBuffer::PdfRefCountedBuffer()
    : m_pBuffer( NULL )
{

}

PdfRefCountedBuffer::PdfRefCountedBuffer( long lSize )
    : m_pBuffer( NULL )
{
    m_pBuffer = new TRefCountedBuffer();
    m_pBuffer->m_lRefCount = 1;
    m_pBuffer->m_pBuffer   = (char*)malloc( sizeof(char)*lSize );
    m_pBuffer->m_lSize     = lSize;

    if( !m_pBuffer->m_pBuffer ) 
    {
        delete m_pBuffer;
        m_pBuffer = NULL;

        RAISE_ERROR( ePdfError_OutOfMemory );
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
        free( m_pBuffer->m_pBuffer );
        delete m_pBuffer;
        m_pBuffer = NULL;
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


};

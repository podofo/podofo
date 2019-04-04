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

#include "PdfOutputStream.h"

#include "PdfOutputDevice.h"
#include "PdfRefCountedBuffer.h"
#include "PdfDefinesPrivate.h"

#include <stdlib.h>
#include <string.h>

namespace PoDoFo {

PdfFileOutputStream::PdfFileOutputStream( const char* pszFilename )
{
    m_hFile = fopen( pszFilename, "wb" );
    if( !m_hFile ) 
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_FileNotFound, pszFilename );
    }
}

PdfFileOutputStream::~PdfFileOutputStream()
{
    Close();
}

pdf_long PdfFileOutputStream::Write( const char* pBuffer, pdf_long lLen )
{
    return fwrite( pBuffer, sizeof(char), lLen, m_hFile );
}

void PdfFileOutputStream::Close() 
{
    if( m_hFile )
    {
        fclose( m_hFile );
        m_hFile = NULL;
    }
}

PdfMemoryOutputStream::PdfMemoryOutputStream( pdf_long lInitial )
    : m_lLen( 0 ), m_bOwnBuffer( true )
{
    m_lSize   = lInitial;
    m_pBuffer = static_cast<char*>(podofo_calloc( m_lSize, sizeof(char) ));
    
    if( !m_pBuffer ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
    }
}

PdfMemoryOutputStream::PdfMemoryOutputStream( char* pBuffer, pdf_long lLen )
    : m_lLen( 0 ), m_bOwnBuffer( false )
{
    if( !pBuffer ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_lSize   = lLen;
    m_pBuffer = pBuffer;
}


PdfMemoryOutputStream::~PdfMemoryOutputStream()
{
    if( m_bOwnBuffer )
        podofo_free( m_pBuffer );
}

pdf_long PdfMemoryOutputStream::Write( const char* pBuffer, pdf_long lLen )
{
    if( !pBuffer ) 
    {
        return 0;
    }

    if( m_lLen + lLen > m_lSize ) 
    {
        if( m_bOwnBuffer )
        {
            // a reallocation is required
            m_lSize = PDF_MAX( (m_lLen + lLen), (m_lSize << 1 ) );
            m_pBuffer = static_cast<char*>(podofo_realloc( m_pBuffer, m_lSize ));
            if( !m_pBuffer ) 
            {
                PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
            }
        }
        else
        {
            PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
        }
    }

    memcpy( m_pBuffer + m_lLen, pBuffer, lLen );
    m_lLen += lLen;

    return lLen;
}

PdfDeviceOutputStream::PdfDeviceOutputStream( PdfOutputDevice* pDevice )
    : m_pDevice( pDevice )
{
}

pdf_long PdfDeviceOutputStream::Write( const char* pBuffer, pdf_long lLen )
{
    pdf_long lTell = m_pDevice->Tell();
    m_pDevice->Write( pBuffer, lLen );
    return m_pDevice->Tell() - lTell;
}


pdf_long PdfBufferOutputStream::Write( const char* pBuffer, pdf_long lLen )
{
    if( m_lLength + lLen >= static_cast<pdf_long>(m_pBuffer->GetSize()) ) 
        m_pBuffer->Resize( m_lLength + lLen );

    memcpy( m_pBuffer->GetBuffer() + m_lLength, pBuffer, lLen );
    m_lLength += lLen;
    
    return lLen;
}

};

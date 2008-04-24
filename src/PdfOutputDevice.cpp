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

#include "PdfOutputDevice.h"

#include <cstdio>
#include <fstream>
#include <sstream>

#include "PdfRefCountedBuffer.h"

namespace PoDoFo {


PdfOutputDevice::PdfOutputDevice()
{
    this->Init();
}

PdfOutputDevice::PdfOutputDevice( const char* pszFilename )
{
    this->Init();

    if( !pszFilename ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_hFile = fopen( pszFilename, "wb" );
    if( !m_hFile )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_FileNotFound, pszFilename );
    }
}

PdfOutputDevice::PdfOutputDevice( char* pBuffer, long lLen )
{
    this->Init();

    if( !pBuffer )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_lBufferLen = lLen;
    m_pBuffer    = pBuffer;
}

PdfOutputDevice::PdfOutputDevice( const std::ostream* pOutStream )
{
    this->Init();

    m_pStream = const_cast< std::ostream* >( pOutStream );
    PdfLocaleImbue(*m_pStream);
}

PdfOutputDevice::PdfOutputDevice( PdfRefCountedBuffer* pOutBuffer )
{
    this->Init();
    m_pRefCountedBuffer = pOutBuffer;
}

PdfOutputDevice::~PdfOutputDevice()
{
    if( m_hFile )
        fclose( m_hFile );
}

void PdfOutputDevice::Init()
{
    m_ulLength          = 0;

    m_hFile             = NULL;
    m_pBuffer           = NULL;
    m_pStream           = NULL;
    m_pRefCountedBuffer = NULL;
    m_lBufferLen        = 0;
    m_ulPosition        = 0;
}

void PdfOutputDevice::Print( const char* pszFormat, ... )
{
    va_list args;
    long    lBytes;

    if( !pszFormat )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( m_hFile )
    {
        va_start( args, pszFormat );
        if( (lBytes = vfprintf( m_hFile, pszFormat, args )) < 0 )
        {
            perror( NULL );
            PODOFO_RAISE_ERROR( ePdfError_UnexpectedEOF );
        }
        va_end( args );
    }
    else
    {
        va_start( args, pszFormat );
#ifdef _MSC_VER	// vsnprintf without buffer does not work with MS-VC
        int len = 1024;
        do
        {
            char * temp = new char[len];
            lBytes = vsnprintf( temp, len+1, pszFormat, args );
            delete[] temp;
            len *= 2;
        } while (lBytes < 0 );
#else
        lBytes = vsnprintf( NULL, 0, pszFormat, args );
#endif
        va_end( args );
    }

    va_start( args, pszFormat );

    if( m_pBuffer )
    {
        if( m_ulPosition + lBytes <= m_lBufferLen )
        {
            vsnprintf( m_pBuffer + m_ulPosition, m_lBufferLen - m_ulPosition, pszFormat, args );
        }
        else
        {
            PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
        }
    }
    else if( m_pStream || m_pRefCountedBuffer )
    {
        ++lBytes;
        char* data = static_cast<char*>(malloc( lBytes * sizeof(char) ));
        if( !data )
        {
            PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
        }
        
        vsnprintf( data, lBytes, pszFormat, args );
        if( lBytes )
            --lBytes;

        if( m_pStream ) 
        {
            std::string str;
            str.assign( data, lBytes );
            *m_pStream << str;
        }
        else // if( m_pRefCountedBuffer ) 
        {
            if( m_ulPosition + lBytes > static_cast<unsigned long>(m_pRefCountedBuffer->GetSize()) )
                m_pRefCountedBuffer->Resize( m_ulPosition + lBytes );

            memcpy( m_pRefCountedBuffer->GetBuffer() + m_ulPosition, data, lBytes );
        }

        free( data );
    }
    va_end( args );

    m_ulPosition += lBytes;
    m_ulLength += lBytes;
}

void PdfOutputDevice::Write( const char* pBuffer, long lLen )
{
    if( m_hFile )
    {
        if( fwrite( pBuffer, sizeof(char), lLen, m_hFile ) != static_cast<size_t>(lLen) )
        {
            PODOFO_RAISE_ERROR( ePdfError_UnexpectedEOF );
        }
    }
    else if( m_pBuffer )
    {
        if( m_ulPosition + lLen <= m_lBufferLen )
        {
            memcpy( m_pBuffer + m_ulPosition, pBuffer, lLen );
        }
        else
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_OutOfMemory, "Allocated buffer to small for PdfOutputDevice. Cannot write!"  );
        }
    }
    else if( m_pStream )
    {
        m_pStream->write( pBuffer, lLen );
    }
    else if( m_pRefCountedBuffer ) 
    {
        if( m_ulPosition + lLen > static_cast<unsigned long>(m_pRefCountedBuffer->GetSize()) )
            m_pRefCountedBuffer->Resize( m_ulPosition + lLen );

        memcpy( m_pRefCountedBuffer->GetBuffer() + m_ulPosition, pBuffer, lLen );
    }

    m_ulLength   += lLen;
    m_ulPosition += lLen;
}

void PdfOutputDevice::Seek( size_t offset )
{
    if( m_hFile )
    {
        if( fseek( m_hFile, offset, SEEK_SET ) == -1 )
        {
            PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
        }
    }
    else if( m_pBuffer )
    {
        if( offset >= m_lBufferLen )
        {
            PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
        }
    }
    else if( m_pStream )
    {
        m_pStream->seekp( offset, std::ios_base::beg );
    }
    else if( m_pRefCountedBuffer ) 
    {
        m_ulPosition = offset;
    }

    m_ulPosition = offset;
    // Seek should not change the length of the device
    // m_ulLength = offset;
}

void PdfOutputDevice::Flush()
{
    if( m_hFile )
    {
        if( fflush( m_hFile ) )
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
        }
    }
    else if( m_pStream )
    {
        m_pStream->flush();
    }
}

};

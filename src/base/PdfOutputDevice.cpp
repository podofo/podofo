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
#include "PdfRefCountedBuffer.h"
#include "PdfDefinesPrivate.h"

#include <fstream>
#include <sstream>


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

    m_pStream = new std::ofstream(pszFilename, std::ofstream::binary);
    PdfLocaleImbue(*m_pStream);

    /*
    m_hFile = fopen( pszFilename, "wb" );
    if( !m_hFile )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_FileNotFound, pszFilename );
    }
    */
}


#ifdef _WIN32
PdfOutputDevice::PdfOutputDevice( const wchar_t* pszFilename )
{
    this->Init();

    if( !pszFilename ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_hFile = _wfopen( pszFilename, L"wb" );
    if( !m_hFile )
    {
        PdfError e( ePdfError_FileNotFound, __FILE__, __LINE__ );
        e.SetErrorInformation( pszFilename );
        throw e;
    }
}
#endif // _WIN32

PdfOutputDevice::PdfOutputDevice( char* pBuffer, size_t lLen )
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
    m_pStreamOwned = false;
    m_pStreamSavedLocale = m_pStream->getloc();
    PdfLocaleImbue(*m_pStream);
}

PdfOutputDevice::PdfOutputDevice( PdfRefCountedBuffer* pOutBuffer )
{
    this->Init();
    m_pRefCountedBuffer = pOutBuffer;
}

PdfOutputDevice::~PdfOutputDevice()
{
    if( m_pStreamOwned ) 
        // remember, deleting a null pointer is safe
        delete m_pStream; // will call close
    else
        m_pStream->imbue(m_pStreamSavedLocale);

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
    m_pStreamOwned      = true;
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
        // OC 17.08.2010: Use new function _vscprintf to get the number of characters:
        // visual c++  8.0 == 1400 (Visual Studio 2005)
        // i am not shure if 1300 is ok here, but who cares this cruel compiler version
#if (defined _MSC_VER && _MSC_VER >= 1400 )
        lBytes = _vscprintf( pszFormat, args );
#elif (defined _MSC_VER || defined __hpux)  // vsnprintf without buffer does not work with MS-VC or HPUX
        int len = 1024;
        do
        {
            char * temp = new char[len+1]; // OC 17.08.2010 BugFix: +1 avoids corrupted heap
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
        // TODO: keep the buffer between subsequent calls!
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

    m_ulPosition += static_cast<size_t>(lBytes);
    m_ulLength += static_cast<size_t>(lBytes);
}

void PdfOutputDevice::Write( const char* pBuffer, size_t lLen )
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
        if( m_ulPosition + lLen > m_pRefCountedBuffer->GetSize() )
            m_pRefCountedBuffer->Resize( m_ulPosition + lLen );

        memcpy( m_pRefCountedBuffer->GetBuffer() + m_ulPosition, pBuffer, lLen );
    }

    m_ulLength   += static_cast<size_t>(lLen);
    m_ulPosition += static_cast<size_t>(lLen);
}

void PdfOutputDevice::Seek( size_t offset )
{
    if( m_hFile )
    {
        if( fseeko( m_hFile, offset, SEEK_SET ) == -1 )
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

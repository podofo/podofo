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

	std::fstream *pStream = new std::fstream(pszFilename, std::fstream::binary|std::ios_base::in | std::ios_base::out | std::ios_base::trunc);
	if(pStream->fail()) {
        delete pStream;
		PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
	}
	m_pStream = pStream;
	m_pReadStream = pStream;
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

    m_hFile = _wfopen( pszFilename, L"w+b" );
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

#if USE_CXX_LOCALE
    m_pStreamSavedLocale = m_pStream->getloc();
    PdfLocaleImbue(*m_pStream);
#endif

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

#if USE_CXX_LOCALE
    if( !m_pStreamOwned )	
        m_pStream->imbue(m_pStreamSavedLocale);
#endif
	
    if( m_hFile )
        fclose( m_hFile );
}

void PdfOutputDevice::Init()
{
    m_ulLength          = 0;

    m_hFile             = NULL;
    m_pBuffer           = NULL;
    m_pStream           = NULL;
	m_pReadStream       = NULL;
    m_pRefCountedBuffer = NULL;
    m_lBufferLen        = 0;
    m_ulPosition        = 0;
    m_pStreamOwned      = true;
}

void PdfOutputDevice::Print( const char* pszFormat, ... )
{
    va_list args;
    long lBytes;

	va_start( args, pszFormat );
	lBytes = PrintVLen(pszFormat, args);
	va_end( args );

	va_start( args, pszFormat );
	PrintV(pszFormat, lBytes, args);
	va_end( args );
}

long PdfOutputDevice::PrintVLen( const char* pszFormat, va_list args )
{
    long    lBytes;

    if( !pszFormat )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( m_hFile )
    {
        if( (lBytes = vfprintf( m_hFile, pszFormat, args )) < 0 )
        {
            perror( NULL );
            PODOFO_RAISE_ERROR( ePdfError_UnexpectedEOF );
        }
    }
    else
    {
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
    }

    return lBytes;
}

void PdfOutputDevice::PrintV( const char* pszFormat, long lBytes, va_list args )
{
    if( !pszFormat )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

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
        m_printBuffer.Resize( lBytes );
        char* data = m_printBuffer.GetBuffer();
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
            {
                m_pRefCountedBuffer->Resize( m_ulPosition + lBytes );
            }

            memcpy( m_pRefCountedBuffer->GetBuffer() + m_ulPosition, data, lBytes );
        }

    }

    m_ulPosition += static_cast<size_t>(lBytes);
    if(m_ulPosition>m_ulLength) 
    {
        m_ulLength = m_ulPosition;
    }
}

size_t PdfOutputDevice::Read( char* pBuffer, size_t lLen )
{
	size_t numRead = 0;
    if( m_hFile )
    {
		numRead = fread( pBuffer, sizeof(char), lLen, m_hFile );
		if(ferror(m_hFile)!=0)
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidDeviceOperation );
        }		
    }
    else if( m_pBuffer )
    {		
        if( m_ulPosition <= m_ulLength )
        {
			numRead = PODOFO_MIN(lLen, m_ulLength-m_ulPosition);
            memcpy( pBuffer, m_pBuffer + m_ulPosition, numRead);
        }
    }
    else if( m_pReadStream )
    {
		size_t iPos = m_pReadStream->tellg();
		m_pReadStream->read( pBuffer, lLen );
		if(m_pReadStream->fail()&&!m_pReadStream->eof()) {
			PODOFO_RAISE_ERROR( ePdfError_InvalidDeviceOperation );
		}
		numRead = m_pReadStream->tellg();
		numRead -= iPos;
    }
    else if( m_pRefCountedBuffer ) 
    {
        if( m_ulPosition <= m_ulLength )
		{
			numRead = PODOFO_MIN(lLen, m_ulLength-m_ulPosition);
            memcpy( pBuffer, m_pRefCountedBuffer->GetBuffer() + m_ulPosition, numRead );
		}
    }

    m_ulPosition += static_cast<size_t>(numRead);
	return numRead;
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

    m_ulPosition += static_cast<size_t>(lLen);
	if(m_ulPosition>m_ulLength) m_ulLength = m_ulPosition;
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

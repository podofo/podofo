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

#include "PdfInputStream.h"

#include "PdfInputDevice.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

namespace PoDoFo {

PdfFileInputStream::PdfFileInputStream( const char* pszFilename )
{
    m_hFile = fopen( pszFilename, "rb" );
    if( !m_hFile ) 
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_FileNotFound, pszFilename );
    }
}

PdfFileInputStream::PdfFileInputStream( const wchar_t* pszFilename )
{
#ifdef _WIN32
	m_hFile = _wfopen( pszFilename, L"rb" );
#else
    m_hFile = wfopen( pszFilename, L"rb" );
#endif // _WIN32
    if( !m_hFile ) 
    {
		PdfError e( ePdfError_FileNotFound, __FILE__, __LINE__ );
		e.SetErrorInformation( pszFilename );
	    throw e;
	}
}

PdfFileInputStream::~PdfFileInputStream()
{
    if( m_hFile )
        fclose( m_hFile );
}

long PdfFileInputStream::Read( char* pBuffer, long lLen )
{
    if( !pBuffer ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // return zero if EOF is reached
    if( feof( m_hFile ) )
        return 0;

    // return the number of bytes read and read the data
    // into pBuffer
    return fread( pBuffer, sizeof(char), lLen, m_hFile );
}

long PdfFileInputStream::GetFileLength()
{
    long lOffset = ftell( m_hFile );
    long lLen;

    fseek( m_hFile, 0L, SEEK_END );
    lLen = ftell( m_hFile );
    fseek( m_hFile, lOffset, SEEK_SET );

    return lLen;
}

PdfMemoryInputStream::PdfMemoryInputStream( const char* pBuffer, long lBufferLen )
    : m_pBuffer( pBuffer ), m_pCur( pBuffer ), m_lBufferLen( lBufferLen )
{

}

PdfMemoryInputStream::~PdfMemoryInputStream()
{
}

long PdfMemoryInputStream::Read( char* pBuffer, long lLen )
{
    if( !pBuffer ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    long lRead = m_pCur - m_pBuffer;

    // return zero if EOF is reached
    if( lRead == m_lBufferLen ) 
        return 0;

    lLen = ( lRead + lLen <= m_lBufferLen ? lLen : m_lBufferLen - lRead );
    memcpy( pBuffer, m_pCur, lLen );
    m_pCur += lLen;
    
    return lLen;
}

PdfDeviceInputStream::PdfDeviceInputStream( PdfInputDevice* pDevice )
    : m_pDevice( pDevice )
{
}

PdfDeviceInputStream::~PdfDeviceInputStream()
{
}

long PdfDeviceInputStream::Read( char* pBuffer, long lLen )
{
    return m_pDevice->Read( pBuffer, lLen );
}

};

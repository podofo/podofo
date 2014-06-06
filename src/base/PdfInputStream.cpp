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

#include "PdfInputStream.h"

#include "PdfInputDevice.h"
#include "PdfDefinesPrivate.h"

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

#ifdef _WIN32
PdfFileInputStream::PdfFileInputStream( const wchar_t* pszFilename )
{
    m_hFile = _wfopen( pszFilename, L"rb" );
    if( !m_hFile ) 
    {
        PdfError e( ePdfError_FileNotFound, __FILE__, __LINE__ );
        e.SetErrorInformation( pszFilename );
        throw e;
    }
}
#endif // _WIN32

PdfFileInputStream::~PdfFileInputStream()
{
    if( m_hFile )
        fclose( m_hFile );
}

pdf_long PdfFileInputStream::Read( char* pBuffer, pdf_long lLen, pdf_long* )
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

pdf_long PdfFileInputStream::GetFileLength()
{
    pdf_long lOffset = ftello( m_hFile );
    pdf_long lLen;

    fseeko( m_hFile, 0L, SEEK_END );
    lLen = ftello( m_hFile );
    fseeko( m_hFile, lOffset, SEEK_SET );

    return lLen;
}

FILE*
PdfFileInputStream::GetHandle()
{
    return m_hFile;
}


PdfMemoryInputStream::PdfMemoryInputStream( const char* pBuffer, pdf_long lBufferLen )
    : m_pBuffer( pBuffer ), m_pCur( pBuffer ), m_lBufferLen( lBufferLen )
{

}

PdfMemoryInputStream::~PdfMemoryInputStream()
{
}

pdf_long PdfMemoryInputStream::Read( char* pBuffer, pdf_long lLen, pdf_long* )
{
    if( !pBuffer ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    pdf_long lRead = m_pCur - m_pBuffer;

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

pdf_long PdfDeviceInputStream::Read( char* pBuffer, pdf_long lLen, pdf_long* )
{
    return m_pDevice->Read( pBuffer, lLen );
}

};

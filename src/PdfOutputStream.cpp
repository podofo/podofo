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

#include "PdfOutputStream.h"

#include "PdfOutputDevice.h"

namespace PoDoFo {

PdfFileOutputStream::PdfFileOutputStream( const char* pszFilename )
{
    m_hFile = fopen( pszFilename, "wb" );
    if( !m_hFile ) 
    {
        RAISE_ERROR( ePdfError_FileNotFound );
    }
}

PdfFileOutputStream::~PdfFileOutputStream()
{
    if( m_hFile ) 
        fclose( m_hFile );
}

long PdfFileOutputStream::Write( const char* pBuffer, long lLen )
{
    return fwrite( pBuffer, sizeof(char), lLen, m_hFile );
}

PdfMemoryOutputStream::PdfMemoryOutputStream( char* pBuffer, long lBufferLen )
    : m_pBuffer( pBuffer ), m_pCur( pBuffer ), m_lBufferLen( lBufferLen )
{

}

PdfMemoryOutputStream::~PdfMemoryOutputStream()
{
}

long PdfMemoryOutputStream::Write( const char* pBuffer, long lLen )
{
    long lWrite = m_pCur - m_pBuffer;

    lLen = ( lWrite + lLen <= m_lBufferLen ? lLen : m_lBufferLen - lWrite );
    memcpy( m_pCur, pBuffer, lLen );
    m_pCur += lLen;
    
    return lLen;
}

PdfDeviceOutputStream::PdfDeviceOutputStream( PdfOutputDevice* pDevice )
    : m_pDevice( pDevice )
{
}

PdfDeviceOutputStream::~PdfDeviceOutputStream()
{
}

long PdfDeviceOutputStream::Write( const char* pBuffer, long lLen )
{
    long lTell = m_pDevice->GetLength();
    m_pDevice->Write( pBuffer, lLen );
    return m_pDevice->GetLength() - lTell;
}

};

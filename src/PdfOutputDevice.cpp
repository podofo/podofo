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
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_hFile = fopen( pszFilename, "wb" );
    if( !m_hFile )
    {
        RAISE_ERROR( ePdfError_FileNotFound );
    }
}

PdfOutputDevice::PdfOutputDevice( char* pBuffer, long lLen )
{
    this->Init();

    if( !pBuffer )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_lBufferLen = lLen;
    m_pBuffer    = pBuffer;
}

PdfOutputDevice::PdfOutputDevice( const std::ostream* pOutStream )
{
    this->Init();

    m_pStream = const_cast< std::ostream* >( pOutStream );
}

PdfOutputDevice::~PdfOutputDevice()
{
    if( m_hFile )
        fclose( m_hFile );
}

void PdfOutputDevice::Init()
{
    m_ulLength   = 0;

    m_hFile      = NULL;
    m_pBuffer    = NULL;
    m_pStream    = NULL;
    m_lBufferLen = 0;
}

void PdfOutputDevice::Print( const char* pszFormat, ... )
{
    va_list  args;
    long     lBytes;

    if( !pszFormat )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( m_hFile )
    {
        va_start( args, pszFormat );
        if( (lBytes = vfprintf( m_hFile, pszFormat, args )) < 0 )
        {
            printf("lBytes=%i %i\n", lBytes, ferror( m_hFile ) );
            perror( NULL );
            RAISE_ERROR( ePdfError_UnexpectedEOF );
        }
        va_end( args );
    }
    else
    {
        va_start( args, pszFormat );
        lBytes = vsnprintf( NULL, 0, pszFormat, args );
        va_end( args );
    }

    va_start( args, pszFormat );

    if( m_pBuffer )
    {
        if( m_ulLength + lBytes <= m_lBufferLen )
        {
            vsnprintf( m_pBuffer + m_ulLength, m_lBufferLen - m_ulLength, pszFormat, args );
        }
        else
        {
            RAISE_ERROR( ePdfError_OutOfMemory );
        }
    }
    else if( m_pStream )
    {
        ++lBytes;
        std::string str;
        char* data = static_cast<char*>(malloc( lBytes * sizeof(char) ));
        if( !data )
        {
            RAISE_ERROR( ePdfError_OutOfMemory );
        }
        
        vsnprintf( data, lBytes, pszFormat, args );
        if( lBytes )
            --lBytes;

        str.assign( data, lBytes );
        *m_pStream << str;
        free( data );
    }

    va_end( args );

    m_ulLength += lBytes;
}

void PdfOutputDevice::Write( const char* pBuffer, long lLen )
{
    if( m_hFile )
    {
        if( fwrite( pBuffer, sizeof(char), lLen, m_hFile ) != static_cast<size_t>(lLen) )
        {
            RAISE_ERROR( ePdfError_UnexpectedEOF );
        }
    }
    else if( m_pBuffer )
    {
        if( m_ulLength + lLen <= m_lBufferLen )
        {
            memcpy( m_pBuffer + m_ulLength, pBuffer, lLen );
        }
        else
        {
            RAISE_ERROR( ePdfError_OutOfMemory );
        }
    }
    else if( m_pStream )
    {
        m_pStream->write( pBuffer, lLen );
    }

    m_ulLength += lLen;
}

void PdfOutputDevice::Seek( size_t offset )
{
    if( m_hFile )
    {
        if( fseek( m_hFile, offset, SEEK_SET ) == -1 )
        {
            RAISE_ERROR( ePdfError_ValueOutOfRange );
        }
    }
    else if( m_pBuffer )
    {
        if( offset >= m_lBufferLen )
        {
            RAISE_ERROR( ePdfError_ValueOutOfRange );
        }
    }
    else if( m_pStream )
    {
        m_pStream->seekp( offset, std::ios_base::beg );
    }

    m_ulLength = offset;
}

void PdfOutputDevice::Flush()
{
    if( m_hFile )
    {
        if( fflush( m_hFile ) )
        {
            RAISE_ERROR( ePdfError_InvalidHandle );
        }
    }
    else if( m_pStream )
    {
        m_pStream->flush();
    }
}


};

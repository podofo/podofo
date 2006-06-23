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
#include <cstdarg>
#include <fstream>
#include <sstream>

namespace PoDoFo {


PdfOutputDevice::PdfOutputDevice()
{
    Init();
}

PdfOutputDevice::~PdfOutputDevice()
{
    if( m_hFile )
        fclose( m_hFile );
}

PdfError PdfOutputDevice::Init( const char* pszFilename )
{
    PdfError eCode;

    SAFE_OP( this->Init() );

    m_hFile = fopen( pszFilename, "wb" );
    if( !m_hFile )
    {
        RAISE_ERROR( ePdfError_FileNotFound );
    }

    return eCode;
}

PdfError PdfOutputDevice::Init( char* pBuffer, long lLen )
{
    PdfError eCode;

    if( !pBuffer )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    SAFE_OP( this->Init() );

    m_lBufferLen = lLen;
    m_pBuffer    = pBuffer;

    return eCode;
}

PdfError PdfOutputDevice::Init( const std::ostream* pOutStream )
{
    PdfError eCode;

    SAFE_OP( this->Init() );

    m_pStream = const_cast< std::ostream* >( pOutStream );

    return eCode;
}

PdfError PdfOutputDevice::Init()
{
    m_ulLength   = 0;

    m_hFile      = NULL;
    m_pBuffer    = NULL;
    m_pStream    = NULL;
    m_lBufferLen = 0;

    return ePdfError_ErrOk;
}

PdfError PdfOutputDevice::Print( const char* pszFormat, ... )
{
    PdfError eCode;
    va_list  args;
    long     lBytes;

    if( !pszFormat )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    va_start( args, pszFormat );
    lBytes = vsnprintf( NULL, 0, pszFormat, args );
    va_end( args );

    va_start( args, pszFormat );
    if( m_hFile )
    {
        if( vfprintf( m_hFile, pszFormat, args ) != lBytes )
            eCode.SetError( ePdfError_UnexpectedEOF, __FILE__, __LINE__ );
    }
    else if( m_pBuffer )
    {
        if( m_ulLength + lBytes <= m_lBufferLen )
        {
            vsnprintf( m_pBuffer + m_ulLength, m_lBufferLen - m_ulLength, pszFormat, args );
        }
        else
            eCode.SetError( ePdfError_OutOfMemory, __FILE__, __LINE__ );
    }
    else if( m_pStream )
    {
        ++lBytes;
        std::string str;
        char* data = (char*)malloc( lBytes * sizeof(char) );
        if( !data )
        {
            RAISE_ERROR( ePdfError_OutOfMemory );
        }
        
        vsnprintf( data, lBytes, pszFormat, args );
        str.assign( data, lBytes );
        *m_pStream << str;
        free( data );
    }

    va_end( args );

    m_ulLength += lBytes;

    return eCode;
}

PdfError PdfOutputDevice::Write( const char* pBuffer, long lLen )
{
    PdfError eCode;

    if( m_hFile )
    {
        if( fwrite( pBuffer, sizeof(char), lLen, m_hFile ) != lLen )
            eCode.SetError( ePdfError_UnexpectedEOF, __FILE__, __LINE__ );
    }
    else if( m_pBuffer )
    {
        if( m_ulLength + lLen <= m_lBufferLen )
        {
            memcpy( m_pBuffer + m_ulLength, pBuffer, lLen );
        }
        else
            eCode.SetError( ePdfError_OutOfMemory, __FILE__, __LINE__ );
    }
    else if( m_pStream )
    {
        m_pStream->write( pBuffer, lLen );
    }

    if( !eCode.IsError() )
        m_ulLength += lLen;

    return eCode;
}

};

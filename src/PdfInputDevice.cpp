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

#include "PdfInputDevice.h"

#include <cstdio>
#include <cstdarg>
#include <fstream>
#include <sstream>

namespace PoDoFo {

PdfInputDevice::PdfInputDevice()
{
    this->Init();
}

PdfInputDevice::PdfInputDevice( const char* pszFilename )
{
    this->Init();

    if( !pszFilename ) 
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    try {
        m_pStream = static_cast< std::istream* >( new std::ifstream( pszFilename, std::ios::binary ) );
        if( !m_pStream )
        {
            RAISE_ERROR( ePdfError_FileNotFound );
        }
        m_StreamOwned = true;
    }
    catch(...) {
        // should probably check the exact error, but for now it's a good error
        RAISE_ERROR( ePdfError_FileNotFound );
    }
}

PdfInputDevice::PdfInputDevice( const char* pBuffer, long lLen )
{
    this->Init();
    
    if( !pBuffer || !lLen ) 
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
    try {
        m_pStream = static_cast< std::istream* >( new std::istringstream( std::string( pBuffer, lLen), std::ios::binary ) );
        if( !m_pStream )
        {
            RAISE_ERROR( ePdfError_FileNotFound );
        }
        m_StreamOwned = true;
    }
    catch(...) {
        // should probably check the exact error, but for now it's a good error
        RAISE_ERROR( ePdfError_FileNotFound );
    }
}

PdfInputDevice::PdfInputDevice( const std::istream* pInStream )
{
    this->Init();

    m_pStream = const_cast< std::istream* >( pInStream );
}

PdfInputDevice::~PdfInputDevice()
{
    this->Close();

    if ( m_StreamOwned ) 
    {
        delete m_pStream;
    }
}

void PdfInputDevice::Init()
{
    m_pStream     = NULL;
    m_StreamOwned = false;
}

void PdfInputDevice::Close()
{
    // nothing to do here, but maybe necessary for inheriting classes
}

int PdfInputDevice::GetChar() const
{
    return m_pStream->get();
}

int PdfInputDevice::Look() const 
{
    return m_pStream->peek();
}

std::streamoff PdfInputDevice::Tell() const
{
    return m_pStream->tellg();
}

void PdfInputDevice::Seek( std::streamoff off, std::ios_base::seekdir dir )
{
    m_pStream->seekg( off, dir );
}

std::streamoff PdfInputDevice::Read( char* pBuffer, std::streamsize lLen )
{
    std::streamoff lPos = this->Tell();

    m_pStream->read( pBuffer, lLen );
    return (this->Tell() - lPos);
}

};

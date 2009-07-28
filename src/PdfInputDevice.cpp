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

#include <cstdarg>
#include <fstream>
#include <sstream>
#include "PdfDefinesPrivate.h"

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
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    try {
        m_pFile = fopen(pszFilename, "rb");
        //m_pStream = new std::ifstream( pszFilename, std::ios::binary );
        if( !m_pFile)
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_FileNotFound, pszFilename );
        }
        m_StreamOwned = true;
    }
    catch(...) {
        // should probably check the exact error, but for now it's a good error
        PODOFO_RAISE_ERROR_INFO( ePdfError_FileNotFound, pszFilename );
    }
    //PdfLocaleImbue(*m_pStream);
}

#ifdef _WIN32
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200			// nicht für Visualstudio 6
#else
PdfInputDevice::PdfInputDevice( const wchar_t* pszFilename )
{
    this->Init();

    if( !pszFilename ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    try {
        //m_pStream = new std::ifstream( pszFilename, std::ios::binary );
        size_t strLen = wcslen(pszFilename);
        char * pStr = new char[strLen+1];
        wcstombs(pStr, pszFilename, strLen);
        m_pFile = fopen(pStr, "rb");
        delete pStr;
        if( !m_pFile)
        //if( !m_pStream || !m_pStream->good() )
        {
            PdfError e( ePdfError_FileNotFound, __FILE__, __LINE__ );
            e.SetErrorInformation( pszFilename );
            throw e;
        }
        m_StreamOwned = true;
    }
    catch(...) {
        // should probably check the exact error, but for now it's a good error
        PdfError e( ePdfError_FileNotFound, __FILE__, __LINE__ );
        e.SetErrorInformation( pszFilename );
        throw e;
    }
    //PdfLocaleImbue(*m_pStream);
}
#endif
#endif // _WIN32

PdfInputDevice::PdfInputDevice( const char* pBuffer, size_t lLen )
{
    this->Init();

    if( !pBuffer ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    try {
        m_pStream = static_cast< std::istream* >( 
            new std::istringstream( std::string( pBuffer, lLen ), std::ios::binary ) );
        if( !m_pStream || !m_pStream->good() )
        {
            PODOFO_RAISE_ERROR( ePdfError_FileNotFound );
        }
        m_StreamOwned = true;
    }
    catch(...) {
        // should probably check the exact error, but for now it's a good error
        PODOFO_RAISE_ERROR( ePdfError_FileNotFound );
    }
    PdfLocaleImbue(*m_pStream);
}

PdfInputDevice::PdfInputDevice( const std::istream* pInStream )
{
    this->Init();

    m_pStream = const_cast< std::istream* >( pInStream );
    if( !m_pStream->good() )
    {
        PODOFO_RAISE_ERROR( ePdfError_FileNotFound );
    }
    PdfLocaleImbue(*m_pStream);
}

PdfInputDevice::~PdfInputDevice()
{
    this->Close();

    if ( m_StreamOwned ) 
    {
			if (m_pStream)
        delete m_pStream;
			if (m_pFile)
				fclose(m_pFile);
    }
}

void PdfInputDevice::Init()
{
    m_pStream     = NULL;
		m_pFile = 0;
    m_StreamOwned = false;
    m_bIsSeekable = true;
}

void PdfInputDevice::Close()
{
    // nothing to do here, but maybe necessary for inheriting classes
}

int PdfInputDevice::GetChar() const
{
	if (m_pStream)
    return m_pStream->get();
	if (m_pFile)
		return fgetc(m_pFile);
	return 0;
}

int PdfInputDevice::Look() const 
{
	if (m_pStream)
    return m_pStream->peek();
	if (m_pFile) {
		pdf_long lOffset = ftello( m_pFile );
		int ch = GetChar();
		fseeko( m_pFile, lOffset, SEEK_SET );
		return ch;
	}

	return 0;
}

std::streamoff PdfInputDevice::Tell() const
{
	if (m_pStream)
    return m_pStream->tellg();
	if (m_pFile)
		return ftello(m_pFile);
	return 0;
}
/*
void PdfInputDevice::Seek( std::streamoff off, std::ios_base::seekdir dir )
{
    if (m_bIsSeekable)
        m_pStream->seekg( off, dir );
    else
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDeviceOperation, "Tried to seek an unseekable input device." );
}
*/

void PdfInputDevice::Seek( std::streamoff off, std::ios_base::seekdir dir )
{
    if (m_bIsSeekable)
    {
        if (m_pStream)
        {
            m_pStream->seekg( off, dir );
        }

        if (m_pFile)
        {
            fseeko( m_pFile, off, dir );
        }
    }
    else
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDeviceOperation, "Tried to seek an unseekable input device." );
    }
}


std::streamoff PdfInputDevice::Read( char* pBuffer, std::streamsize lLen )
{
	if (m_pStream) {
    m_pStream->read( pBuffer, lLen );
    return m_pStream->gcount();
	}
	else 
	{
		return fread(pBuffer, 1, lLen, m_pFile);
	}
}

}; // namespace PoDoFo

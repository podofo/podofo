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
PdfInputDevice::PdfInputDevice( const wchar_t* pszFilename )
{
    this->Init();

    if( !pszFilename ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    try {
        // James McGill 16.02.2011 Fix wide character filename loading in windows
        m_pFile = _wfopen(pszFilename, L"rb");
        if( !m_pFile)
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
}
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
        {
            delete m_pStream;
        }
        
        if (m_pFile)
        {
            fclose(m_pFile);
        }
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
    {
        return m_pStream->get();
    }
    
	if (m_pFile)
    {
		return fgetc(m_pFile);
    }
    
	return 0;
}

int PdfInputDevice::Look() const 
{
    if (m_pStream)
    {
        return m_pStream->peek();
    }
    
    if (m_pFile)
    {
        pdf_long lOffset = ftello( m_pFile );
        
        if( lOffset == -1 )
        {    
            PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDeviceOperation, "Failed to read the current file position" );
        }

        int ch = GetChar();

        if( fseeko( m_pFile, lOffset, SEEK_SET ) == -1 )
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDeviceOperation, "Failed to seek back to the previous position" );
        }
        
        return ch;
    }

    return 0;
}

std::streamoff PdfInputDevice::Tell() const
{
	if (m_pStream)
    {
        return m_pStream->tellg();
    }
    
	if (m_pFile)
    {
		return ftello(m_pFile);
    }
    
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
            int whence;

            if( dir == std::ios_base::beg )
                whence = SEEK_SET;
            else if( dir == std::ios_base::cur )
                whence = SEEK_CUR;
            else // if( dir == std::ios_base::end )
                whence = SEEK_END;
            
            if( fseeko( m_pFile, off, whence ) == -1)
            {
                PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDeviceOperation, "Failed to seek to given position in the file" );
            }
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

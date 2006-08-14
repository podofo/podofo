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

#include "PdfRefCountedFile.h"

namespace PoDoFo {

PdfRefCountedFile::PdfRefCountedFile()
    : m_pFile( NULL )
{

}

PdfRefCountedFile::PdfRefCountedFile( const char* pszFilename, const char* pszMode )
    : m_pFile( NULL )
{
    m_pFile = new TRefCountedFile();
    m_pFile->m_lRefCount = 1;
    m_pFile->m_hFile     = fopen( pszFilename, pszMode );

    if( !m_pFile->m_hFile ) 
    {
        delete m_pFile;
        m_pFile = NULL;

        RAISE_ERROR( ePdfError_FileNotFound );
    }
}

PdfRefCountedFile::PdfRefCountedFile( const PdfRefCountedFile & rhs )
    : m_pFile( NULL )
{
    this->operator=( rhs );
}

PdfRefCountedFile::~PdfRefCountedFile()
{
    Detach();
}

void PdfRefCountedFile::Detach()
{
    if( m_pFile && !--m_pFile->m_lRefCount ) 
    {
        // last owner of the file!
        fclose( m_pFile->m_hFile );
        delete m_pFile;
        m_pFile = NULL;
    }

}

const PdfRefCountedFile & PdfRefCountedFile::operator=( const PdfRefCountedFile & rhs )
{
    Detach();

    m_pFile = rhs.m_pFile;
    if( m_pFile )
        m_pFile->m_lRefCount++;

	return *this;
}


};

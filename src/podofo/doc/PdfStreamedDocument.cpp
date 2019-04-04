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

#include "PdfStreamedDocument.h"

#include "base/PdfDefinesPrivate.h"

namespace PoDoFo {

PdfStreamedDocument::PdfStreamedDocument( PdfOutputDevice* pDevice, EPdfVersion eVersion, PdfEncrypt* pEncrypt, EPdfWriteMode eWriteMode )
    : m_pWriter( NULL ), m_pDevice( NULL ), m_pEncrypt( pEncrypt ), m_bOwnDevice( false )
{
    Init( pDevice, eVersion, pEncrypt, eWriteMode );
}

PdfStreamedDocument::PdfStreamedDocument( const char* pszFilename, EPdfVersion eVersion, PdfEncrypt* pEncrypt, EPdfWriteMode eWriteMode )
    : m_pWriter( NULL ), m_pEncrypt( pEncrypt ), m_bOwnDevice( true )
{
    m_pDevice = new PdfOutputDevice( pszFilename );
    Init( m_pDevice, eVersion, pEncrypt, eWriteMode );
}

#ifdef _WIN32
PdfStreamedDocument::PdfStreamedDocument( const wchar_t* pszFilename, EPdfVersion eVersion, PdfEncrypt* pEncrypt, EPdfWriteMode eWriteMode )
    : m_pWriter( NULL ), m_pEncrypt( pEncrypt ), m_bOwnDevice( true )
{
    m_pDevice = new PdfOutputDevice( pszFilename );
    Init( m_pDevice, eVersion, pEncrypt, eWriteMode );
}
#endif // _WIN32

PdfStreamedDocument::~PdfStreamedDocument()
{
    delete m_pWriter;
    if( m_bOwnDevice )
        delete m_pDevice;
}

void PdfStreamedDocument::Init( PdfOutputDevice* pDevice, EPdfVersion eVersion, 
                                PdfEncrypt* pEncrypt, EPdfWriteMode eWriteMode )
{
    m_pWriter = new PdfImmediateWriter( pDevice, this->GetObjects(), this->GetTrailer(), eVersion, pEncrypt, eWriteMode );
}

void PdfStreamedDocument::Close()
{
    // TODO: Check if this works correctly
	// makes sure pending subset-fonts are embedded
	m_fontCache.EmbedSubsetFonts();
    
    this->GetObjects()->Finish();
}



};

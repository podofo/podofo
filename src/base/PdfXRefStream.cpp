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

#include "PdfXRefStream.h"

#include "PdfObject.h"
#include "PdfStream.h"
#include "PdfWriter.h"
#include "PdfDefinesPrivate.h"

#if defined(_AIX) || defined(__sun)
#include <alloca.h>
#elif defined(__APPLE__) || defined(__linux)
#include <cstdlib>
#elif defined(_WIN32)
#include <malloc.h>
#endif

namespace PoDoFo {

PdfXRefStream::PdfXRefStream( PdfVecObjects* pParent, PdfWriter* pWriter )
    : m_pParent( pParent ), m_pWriter( pWriter ), m_pObject( NULL )
{
    m_bufferLen = 2 + sizeof( pdf_uint32 );

    m_pObject    = pParent->CreateObject( "XRef" );
    m_offset    = 0;
}

PdfXRefStream::~PdfXRefStream()
{
}

void PdfXRefStream::BeginWrite( PdfOutputDevice* )
{
    m_pObject->GetStream()->BeginAppend();
}

void PdfXRefStream::WriteSubSection( PdfOutputDevice*, pdf_objnum first, pdf_uint32 count )
{
    PdfError::DebugMessage("Writing XRef section: %u %u\n", first, count );

    m_indeces.push_back( static_cast<pdf_int64>(first) );
    m_indeces.push_back( static_cast<pdf_int64>(count) );
}

void PdfXRefStream::WriteXRefEntry( PdfOutputDevice*, pdf_uint64 offset, pdf_gennum generation, 
                                    char cMode, pdf_objnum objectNumber ) 
{
    std::vector<char>	bytes(m_bufferLen);
#if (defined(_MSC_VER)  &&  _MSC_VER < 1700) || (defined(__BORLANDC__))	// MSC before VC11 has no data member, same as BorlandC
    char * buffer = &bytes[0];
#else
    char * buffer = bytes.data();
#endif

    if( cMode == 'n' && objectNumber == m_pObject->Reference().ObjectNumber() )
        m_offset = offset;
    
    buffer[0]             = static_cast<char>( cMode == 'n' ? 1 : 0 );
    buffer[m_bufferLen-1] = static_cast<char>( cMode == 'n' ? 0 : generation );

    const pdf_uint32 offset_be = ::PoDoFo::compat::podofo_htonl(static_cast<pdf_uint32>(offset));
    memcpy( &buffer[1], reinterpret_cast<const char*>(&offset_be), sizeof(pdf_uint32) );

    m_pObject->GetStream()->Append( buffer, m_bufferLen );
}

void PdfXRefStream::EndWrite( PdfOutputDevice* pDevice )
{
    PdfArray w;

    w.push_back( static_cast<pdf_int64>(1) );
    w.push_back( static_cast<pdf_int64>(sizeof(pdf_uint32)) );
    w.push_back( static_cast<pdf_int64>(1) );

    // Add our self to the XRef table
    this->WriteXRefEntry( pDevice, pDevice->Tell(), 0, 'n' );

    m_pObject->GetStream()->EndAppend();
    m_pWriter->FillTrailerObject( m_pObject, this->GetSize(), false, false );

    m_pObject->GetDictionary().AddKey( "Index", m_indeces );
    m_pObject->GetDictionary().AddKey( "W", w );

    pDevice->Seek( static_cast<size_t>(m_offset) );
    m_pObject->WriteObject( pDevice, m_pWriter->GetWriteMode(), NULL ); // DominikS: Requires encryption info??
    m_indeces.Clear();
}

};


/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
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

#include "PdfObjectStreamParserObject.h"

#include "PdfDictionary.h"
#include "PdfEncrypt.h"
#include "PdfInputDevice.h"
#include "PdfParserObject.h"
#include "PdfStream.h"
#include "PdfVecObjects.h"

#include <algorithm>

#if defined(PODOFO_VERBOSE_DEBUG)
#include <iostream>
#endif

namespace PoDoFo {

PdfObjectStreamParserObject::PdfObjectStreamParserObject(PdfParserObject* pParser, PdfVecObjects* pVecObjects, const PdfRefCountedBuffer & rBuffer, PdfEncrypt* pEncrypt )
    : m_pParser( pParser ), m_vecObjects( pVecObjects ), m_buffer( rBuffer ), m_pEncrypt( pEncrypt )
{

}

PdfObjectStreamParserObject::~PdfObjectStreamParserObject()
{

}

void PdfObjectStreamParserObject::Parse(ObjectIdList const & list)
{
    long long lNum   = m_pParser->GetDictionary().GetKeyAsLong( "N", 0 );
    long long lFirst = m_pParser->GetDictionary().GetKeyAsLong( "First", 0 );
    
    char* pBuffer;
    pdf_long lBufferLen;
    m_pParser->GetStream()->GetFilteredCopy( &pBuffer, &lBufferLen );

    try {
        this->ReadObjectsFromStream( pBuffer, lBufferLen, lNum, lFirst, list );
        free( pBuffer );

        // the object stream is not needed anymore in the final PDF
        delete m_vecObjects->RemoveObject( m_pParser->Reference() );
        m_pParser = NULL;

    } catch( const PdfError & rError ) {
        free( pBuffer );
        throw rError;
    }
}

void PdfObjectStreamParserObject::ReadObjectsFromStream( char* pBuffer, pdf_long lBufferLen, long long lNum, long long lFirst, ObjectIdList const & list)
{
    PdfRefCountedInputDevice device( pBuffer, lBufferLen );
    PdfTokenizer             tokenizer( device, m_buffer );
    PdfVariant               var;
    int                      i = 0;

    while( static_cast<long long>(i) < lNum )
    {
        const long long lObj     = tokenizer.GetNextNumber();
        const long long lOff     = tokenizer.GetNextNumber();
        const std::streamoff pos = device.Device()->Tell();

        // move to the position of the object in the stream
        device.Device()->Seek( static_cast<std::streamoff>(lFirst + lOff) );

		// use a second tokenizer here so that anything that gets dequeued isn't left in the tokenizer that reads the offsets and lengths
	    PdfTokenizer variantTokenizer( device, m_buffer );
        variantTokenizer.GetNextVariant( var, m_pEncrypt );
		bool should_read = std::find(list.begin(), list.end(), lObj) != list.end();
#if defined(PODOFO_VERBOSE_DEBUG)
        std::cerr << "ReadObjectsFromStream STREAM=" << m_pParser->Reference().ToString() <<
			", OBJ=" << lObj <<
			", " << (should_read ? "read" : "skipped") << std::endl;
#endif
		if (should_read)
        {
			if(m_vecObjects->GetObject(PdfReference( static_cast<int>(lObj), 0LL ))) 
            {
                PdfError::LogMessage( eLogSeverity_Warning, "Object: %li 0 R will be deleted and loaded again.\n", lObj );
                delete m_vecObjects->RemoveObject(PdfReference( static_cast<int>(lObj), 0LL ),false);
            }
            m_vecObjects->insert_sorted( new PdfObject( PdfReference( static_cast<int>(lObj), 0LL ), var ) );
		}

        // move back to the position inside of the table of contents
        device.Device()->Seek( pos );

        ++i;
    }
}

}; 

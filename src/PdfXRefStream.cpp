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

#include "PdfXRefStream.h"

#include "PdfArray.h"
#include "PdfObject.h"
#include "PdfStream.h"
#include "PdfWriter.h"

// for htonl
#ifdef _WIN32
#include <winsock2.h>
#undef GetObject
#else 
#include <arpa/inet.h>
#endif // _WIN32

namespace PoDoFo {

#ifdef WIN32
typedef signed char 	int8_t;
typedef unsigned char 	uint8_t;
typedef signed short 	int16_t;
typedef unsigned short 	uint16_t;
typedef signed int 	int32_t;
typedef unsigned int 	uint32_t;
#endif

#define STREAM_OFFSET_TYPE uint32_t

bool podofo_is_little_endian()
{ 
    int _p = 1;
    return ((reinterpret_cast<char*>(&_p))[0] == 1);
}

PdfXRefStream::PdfXRefStream( PdfVecObjects* pParent, PdfWriter* pWriter )
    : m_pParent( pParent ), m_pWriter( pWriter )
{

}

PdfXRefStream::~PdfXRefStream()
{

}

void PdfXRefStream::Write( PdfOutputDevice* pDevice )
{
    const size_t        bufferLen = 2 + sizeof( STREAM_OFFSET_TYPE );
    char                buffer[bufferLen];
    bool                bLittle   = podofo_is_little_endian();

    STREAM_OFFSET_TYPE* pValue    = reinterpret_cast<STREAM_OFFSET_TYPE*>(buffer+1);

    // Make sure to not crash even for empty documents.
    PdfReference        reference( m_vecXRef.size() ? m_vecXRef.back().reference.ObjectNumber() + 1 : 0, 0 );
    PdfObject           object( reference, "XRef" );
    PdfArray            indeces;
    PdfArray            w;

    PdfXRef::TCIVecXRefItems  it     = m_vecXRef.begin();
    PdfXRef::TCIVecReferences itFree = m_vecFreeObjects.begin();

    int nFirst = 0;
    int nCount = 0;

    object.SetOwner( m_pParent );

    w.push_back( 1l );
    w.push_back( static_cast<long>(sizeof(STREAM_OFFSET_TYPE)) );
    w.push_back( 1l );

    while( it != m_vecXRef.end() ) 
    {
        nCount = GetItemCount( it, itFree );
        nFirst = PDF_MIN( it != m_vecXRef.end() ? (*it).reference.ObjectNumber() : EMPTY_OBJECT_OFFSET,
                          itFree != m_vecFreeObjects.end() ? (*itFree).ObjectNumber() : EMPTY_OBJECT_OFFSET );

        if( nFirst == 1 )
            --nFirst;

        if( !nFirst ) 
        {
            // write free object
            buffer[0]           = static_cast<char>(0);
            buffer[bufferLen-1] = static_cast<char>(1);
            // TODO: This might cause bus errors on HP-UX machines 
            //       which require integers to be alligned on byte boundaries.
            //       -> Better use memcpy here!
            *pValue           = static_cast<STREAM_OFFSET_TYPE>( m_vecFreeObjects.size() ? 
                                                                 m_vecFreeObjects.front().ObjectNumber() : 0 );
            if( bLittle )
                *pValue = static_cast<STREAM_OFFSET_TYPE>(htonl( *pValue ));

 
            object.GetStream()->Append( buffer, bufferLen );
        }

        indeces.push_back( static_cast<long>(nFirst) );
        indeces.push_back( static_cast<long>(nCount) );

        while( --nCount && it != m_vecXRef.end() ) 
        {
            while( itFree != m_vecFreeObjects.end() &&
                   *itFree < (*it).reference && nCount )
            {

                ++itFree;

                // write free object
                buffer[0]           = static_cast<char>(0);
                buffer[bufferLen-1] = static_cast<char>(1);
                *pValue             = static_cast<STREAM_OFFSET_TYPE>(itFree != m_vecFreeObjects.end() ? 
                                                                    (*itFree).ObjectNumber() : 0);
                if( bLittle )
                    *pValue = static_cast<STREAM_OFFSET_TYPE>(htonl( *pValue ));

                object.GetStream()->Append( buffer, bufferLen );
                --nCount;
            }

            buffer[0]           = static_cast<char>(1);
            buffer[bufferLen-1] = static_cast<char>(0);
            *pValue             = static_cast<STREAM_OFFSET_TYPE>((*it).lOffset);
            if( bLittle )
                *pValue = static_cast<STREAM_OFFSET_TYPE>(htonl( *pValue ));

            object.GetStream()->Append( buffer, bufferLen );
            ++it;
        }
    }    

    m_pWriter->FillTrailerObject( &object, this->GetSize(), false, false );

    object.GetDictionary().AddKey( "Index", indeces );
    object.GetDictionary().AddKey( "W", w );
    //object.FlateCompressStream();
    object.WriteObject( pDevice );
}

};


/***************************************************************************
 *   Copyright (C) 2009 by Dominik Seichter                                *
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

#include "PdfXRefStreamParserObject.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfStream.h"
#include "PdfVariant.h"

#include <stdio.h>

namespace PoDoFo {

PdfXRefStreamParserObject::PdfXRefStreamParserObject(PdfVecObjects* pCreator, const PdfRefCountedInputDevice & rDevice, 
                                                     const PdfRefCountedBuffer & rBuffer, PdfParser::TVecOffsets* pOffsets )
    : PdfParserObject( pCreator, rDevice, rBuffer ), m_lNextOffset(-1L), m_pOffsets( pOffsets )
{

}

PdfXRefStreamParserObject::~PdfXRefStreamParserObject() 
{

}

void PdfXRefStreamParserObject::Parse()
{
    // Ignore the encryption in the XREF as the XREF stream must no be encrypted (see PDF Reference 3.4.7)
    this->ParseFile( NULL );

    // Do some very basic error checking
    if( !this->GetDictionary().HasKey( PdfName::KeyType ) )
    {
        PODOFO_RAISE_ERROR( ePdfError_NoXRef );
    } 

    PdfObject* pObj = this->GetDictionary().GetKey( PdfName::KeyType );
    if( !pObj->IsName() || ( pObj->GetName() != "XRef" ) )
    {
        PODOFO_RAISE_ERROR( ePdfError_NoXRef );
    }

    if( !this->GetDictionary().HasKey( PdfName::KeySize ) 
        || !this->GetDictionary().HasKey( "W" ) )
    {
        PODOFO_RAISE_ERROR( ePdfError_NoXRef );
    } 

    if( !this->HasStreamToParse() )
    {
        PODOFO_RAISE_ERROR( ePdfError_NoXRef );
    }

    if( this->GetDictionary().HasKey("Prev") )
    {
        m_lNextOffset = static_cast<pdf_long>(this->GetDictionary().GetKeyAsLong( "Prev", 0 ));
    }
}

void PdfXRefStreamParserObject::ReadXRefTable() 
{
    pdf_int64  lSize   = this->GetDictionary().GetKeyAsLong( PdfName::KeySize, 0 );
    PdfVariant vWArray = *(this->GetDictionary().GetKey( "W" ));

    // The pdf reference states that W is always an array with 3 entries
    // all of them have to be integers
    if( !vWArray.IsArray() || vWArray.GetArray().size() != 3 )
    {
        PODOFO_RAISE_ERROR( ePdfError_NoXRef );
    }


    pdf_int64 nW[W_ARRAY_SIZE] = { 0, 0, 0 };
    for( int i=0;i<W_ARRAY_SIZE;i++ )
    {
        if( !vWArray.GetArray()[i].IsNumber() )
        {
            PODOFO_RAISE_ERROR( ePdfError_NoXRef );
        }

        nW[i] = static_cast<pdf_int64>(vWArray.GetArray()[i].GetNumber());
    }

    std::vector<pdf_int64> vecIndeces;
    GetIndeces( vecIndeces, static_cast<pdf_int64>(lSize) );

    ParseStream( nW, vecIndeces );
}

void PdfXRefStreamParserObject::ParseStream( const pdf_int64 nW[W_ARRAY_SIZE], const std::vector<pdf_int64> & rvecIndeces )
{
    char*        pBuffer;
    pdf_long     lBufferLen;
    const size_t entryLen  = static_cast<size_t>(nW[0] + nW[1] + nW[2]);

    this->GetStream()->GetFilteredCopy( &pBuffer, &lBufferLen );

    
    std::vector<pdf_int64>::const_iterator it = rvecIndeces.begin();
    char* const pStart = pBuffer;
    while( it != rvecIndeces.end() )
    {
        pdf_int64 nFirstObj = *it; ++it;
        pdf_int64 nCount    = *it; ++it;

        //pdf_int64 nFirstObjOrg = nFirstObj;
        //pdf_int64 nCountOrg = nCount;
        
        //printf("\n");
        //printf("nFirstObj=%i\n", static_cast<int>(nFirstObj));
        //printf("nCount=%i\n", static_cast<int>(nCount));
        while( nCount > 0 )
        {
            if( (pBuffer - pStart) >= lBufferLen ) 
            {
                PODOFO_RAISE_ERROR_INFO( ePdfError_NoXRef, "Invalid count in XRef stream" );
            }

            //printf("nCount=%i ", static_cast<int>(nCount));
            //printf("pBuffer=%li ", (long)(pBuffer - pStart));
            //printf("pEnd=%li ", lBufferLen);
            if ( nFirstObj >= 0 && nFirstObj < static_cast<pdf_int64>(m_pOffsets->size()) 
                 && ! (*m_pOffsets)[static_cast<int>(nFirstObj)].bParsed)
            {
	        ReadXRefStreamEntry( pBuffer, lBufferLen, nW, static_cast<int>(nFirstObj) );
            }

			nFirstObj++ ;
            pBuffer += entryLen;
            --nCount;
        }
        //printf("Exp: nFirstObj=%i nFirstObjOrg + nCount=%i\n", nFirstObj - 1, nFirstObjOrg + nCountOrg - 1 );
        //printf("===\n");
    }
    podofo_free( pStart );

}

void PdfXRefStreamParserObject::GetIndeces( std::vector<pdf_int64> & rvecIndeces, pdf_int64 size ) 
{
    // get the first object number in this crossref stream.
    // it is not required to have an index key though.
    if( this->GetDictionary().HasKey( "Index" ) )
    {
        PdfVariant array = *(this->GetDictionary().GetKey( "Index" ));
        if( !array.IsArray() )
        {
            PODOFO_RAISE_ERROR( ePdfError_NoXRef );
        }

        TCIVariantList it = array.GetArray().begin();
        while ( it != array.GetArray().end() )
        {
            rvecIndeces.push_back( (*it).GetNumber() );
            ++it;
        }
    }
    else
    {
        // Default
        rvecIndeces.push_back( static_cast<pdf_int64>(0) );
        rvecIndeces.push_back( size );
    }

    // vecIndeces must be a multiple of 2
    if( rvecIndeces.size() % 2 != 0)
    {
        PODOFO_RAISE_ERROR( ePdfError_NoXRef );
    }
}

void PdfXRefStreamParserObject::ReadXRefStreamEntry( char* pBuffer, pdf_long, const pdf_int64 lW[W_ARRAY_SIZE], int nObjNo )
{
    int              i;
    pdf_int64        z;
    unsigned long    nData[W_ARRAY_SIZE];

    for( i=0;i<W_ARRAY_SIZE;i++ )
    {
        if( lW[i] > W_MAX_BYTES )
        {
            PdfError::LogMessage( eLogSeverity_Error, 
                                  "The XRef stream dictionary has an entry in /W of size %i.\nThe maximum supported value is %i.\n", 
                                  lW[i], W_MAX_BYTES );

            PODOFO_RAISE_ERROR( ePdfError_InvalidXRefStream );
        }
        
        nData[i] = 0;
        for( z=W_MAX_BYTES-lW[i];z<W_MAX_BYTES;z++ )
        {
            nData[i] = (nData[i] << 8) + static_cast<unsigned char>(*pBuffer);
            ++pBuffer;
        }
    }


    //printf("OBJ=%i nData = [ %i %i %i ]\n", nObjNo, static_cast<int>(nData[0]), static_cast<int>(nData[1]), static_cast<int>(nData[2]) );
    (*m_pOffsets)[nObjNo].bParsed = true;
    switch( lW[0] == 0 ? 1 : nData[0] ) // nData[0] contains the type information of this entry
    {
        case 0:
            // a free object
            (*m_pOffsets)[nObjNo].lOffset     = nData[1];
            (*m_pOffsets)[nObjNo].lGeneration = nData[2];
            (*m_pOffsets)[nObjNo].cUsed       = 'f';
            break;
        case 1:
            // normal uncompressed object
            (*m_pOffsets)[nObjNo].lOffset     = nData[1];
            (*m_pOffsets)[nObjNo].lGeneration = nData[2];
            (*m_pOffsets)[nObjNo].cUsed       = 'n';
            break;
        case 2:
            // object that is part of an object stream
            (*m_pOffsets)[nObjNo].lOffset     = nData[2]; // index in the object stream
            (*m_pOffsets)[nObjNo].lGeneration = nData[1]; // object number of the stream
            (*m_pOffsets)[nObjNo].cUsed       = 's';      // mark as stream
            break;
        default:
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidXRefType );
        }
    }
    //printf("m_offsets = [ %i %i %c ]\n", (*m_pOffsets)[nObjNo].lOffset, (*m_pOffsets)[nObjNo].lGeneration, (*m_pOffsets)[nObjNo].cUsed );
}

};

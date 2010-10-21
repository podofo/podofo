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

#include "PdfFileSpec.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfDictionary.h"
#include "base/PdfInputStream.h"
#include "base/PdfObject.h"
#include "base/PdfStream.h"

#include <sstream>

namespace PoDoFo {

PdfFileSpec::PdfFileSpec( const char* pszFilename, bool bEmbedd, PdfDocument* pParent )
    : PdfElement( "Filespec", pParent )
{
    PdfObject* pEmbeddedStream;

    this->GetObject()->GetDictionary().AddKey( "F", this->CreateFileSpecification( pszFilename ) );

    if( bEmbedd ) 
    {
        PdfDictionary ef;

        pEmbeddedStream = this->CreateObject( "EmbeddedFile" );
        this->EmbeddFile( pEmbeddedStream, pszFilename );

        ef.AddKey( "F",  pEmbeddedStream->Reference() );

        this->GetObject()->GetDictionary().AddKey( "EF", ef );
    }
}

PdfFileSpec::PdfFileSpec( const char* pszFilename, bool bEmbedd, PdfVecObjects* pParent )
    : PdfElement( "Filespec", pParent )
{
    PdfObject* pEmbeddedStream;

    this->GetObject()->GetDictionary().AddKey( "F", this->CreateFileSpecification( pszFilename ) );

    if( bEmbedd ) 
    {
        PdfDictionary ef;

        pEmbeddedStream = this->CreateObject( "EmbeddedFile" );
        this->EmbeddFile( pEmbeddedStream, pszFilename );

        ef.AddKey( "F",  pEmbeddedStream->Reference() );

        this->GetObject()->GetDictionary().AddKey( "EF", ef );
    }
}

PdfFileSpec::PdfFileSpec( const char* pszFilename, const unsigned char* data, ptrdiff_t size, PdfVecObjects* pParent)
    : PdfElement( "Filespec", pParent )
{
    PdfObject* pEmbeddedStream;

    this->GetObject()->GetDictionary().AddKey( "F", this->CreateFileSpecification( pszFilename ) );

    PdfDictionary ef;

    pEmbeddedStream = this->CreateObject( "EmbeddedFile" );
    this->EmbeddFileFromMem( pEmbeddedStream, data, size );

    ef.AddKey( "F",  pEmbeddedStream->Reference() );

    this->GetObject()->GetDictionary().AddKey( "EF", ef );
}

PdfFileSpec::PdfFileSpec( PdfObject* pObject )
    : PdfElement( "Filespec", pObject )
{

}

PdfString PdfFileSpec::CreateFileSpecification( const char* pszFilename ) const
{
    std::ostringstream str;
    size_t                nLen = strlen( pszFilename );

    // Not sure if we really get a platform independent file specifier here.
    
    for( size_t i=0;i<nLen;i++ ) 
    {
        if( pszFilename[i] == ':' || pszFilename[i] == '\\' ) 
            str.put( '/' );
        else 
            str.put( pszFilename[i] );
    }

    return PdfString( str.str() );
}

void PdfFileSpec::EmbeddFile( PdfObject* pStream, const char* pszFilename ) const
{
    PdfFileInputStream stream( pszFilename );
    pStream->GetStream()->Set( &stream );

    // Add additional information about the embedded file to the stream
    PdfDictionary params;
    params.AddKey( "Size", static_cast<pdf_int64>(stream.GetFileLength()) );
    // TODO: CreationDate and ModDate
    pStream->GetDictionary().AddKey("Params", params );
}

void PdfFileSpec::EmbeddFileFromMem( PdfObject* pStream, const unsigned char* data, ptrdiff_t size ) const
{
    PdfMemoryInputStream memstream(reinterpret_cast<const char*>(data),size);
    pStream->GetStream()->Set( &memstream );

    // Add additional information about the embedded file to the stream
    PdfDictionary params;
    params.AddKey( "Size", static_cast<pdf_int64>(size) );
    pStream->GetDictionary().AddKey("Params", params );
}

const PdfString & PdfFileSpec::GetFilename() const
{
    if( this->GetObject()->GetDictionary().HasKey( "F" ) )
    {
        return this->GetObject()->GetDictionary().GetKey( "F" )->GetString();
    }

    PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
}


};

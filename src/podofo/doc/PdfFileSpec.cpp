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

#include "PdfFileSpec.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfDictionary.h"
#include "base/PdfInputStream.h"
#include "base/PdfObject.h"
#include "base/PdfStream.h"

#include <sstream>

namespace PoDoFo {

PdfFileSpec::PdfFileSpec( const char* pszFilename, bool bEmbedd, PdfDocument* pParent, bool bStripPath)
    : PdfElement( "Filespec", pParent )
{
    Init( pszFilename, bEmbedd, bStripPath );
}

PdfFileSpec::PdfFileSpec( const char* pszFilename, bool bEmbedd, PdfVecObjects* pParent, bool bStripPath)
    : PdfElement( "Filespec", pParent )
{
    Init( pszFilename, bEmbedd, bStripPath );
}

PdfFileSpec::PdfFileSpec( const char* pszFilename, const unsigned char* data, ptrdiff_t size, PdfVecObjects* pParent, bool bStripPath)
    : PdfElement( "Filespec", pParent )
{
    Init( pszFilename, data, size, bStripPath );
}


PdfFileSpec::PdfFileSpec( const char* pszFilename, const unsigned char* data, ptrdiff_t size, PdfDocument* pParent, bool bStripPath)
    : PdfElement( "Filespec", pParent )
{
    Init( pszFilename, data, size, bStripPath );
}

PdfFileSpec::PdfFileSpec( PdfObject* pObject )
    : PdfElement( "Filespec", pObject )
{
}

#ifdef _WIN32

PdfFileSpec::PdfFileSpec( const wchar_t* pszFilename, bool bEmbedd, PdfDocument* pParent, bool bStripPath)
    : PdfElement( "Filespec", pParent )
{
    Init( pszFilename, bEmbedd, bStripPath );
}

PdfFileSpec::PdfFileSpec( const wchar_t* pszFilename, bool bEmbedd, PdfVecObjects* pParent, bool bStripPath)
    : PdfElement( "Filespec", pParent )
{
    Init( pszFilename, bEmbedd, bStripPath );
}

PdfFileSpec::PdfFileSpec( const wchar_t* pszFilename, const unsigned char* data, ptrdiff_t size, PdfVecObjects* pParent, bool bStripPath)
    : PdfElement( "Filespec", pParent )
{
    Init( pszFilename, data, size, bStripPath );
}

PdfFileSpec::PdfFileSpec( const wchar_t* pszFilename, const unsigned char* data, ptrdiff_t size, PdfDocument* pParent, bool bStripPath)
    : PdfElement( "Filespec", pParent )
{
    Init( pszFilename, data, size, bStripPath );
}

void PdfFileSpec::Init( const wchar_t* pszFilename, bool bEmbedd, bool bStripPath) 
{
    PdfObject* pEmbeddedStream;
    PdfString filename;

    filename.setFromWchar_t( MaybeStripPath( pszFilename, true) );

    this->GetObject()->GetDictionary().AddKey( "F", this->CreateFileSpecification( MaybeStripPath( pszFilename, bStripPath ) ) );
    this->GetObject()->GetDictionary().AddKey( "UF", filename.ToUnicode () );

    if( bEmbedd ) 
    {
        PdfDictionary ef;

        pEmbeddedStream = this->CreateObject( "EmbeddedFile" );
        this->EmbeddFile( pEmbeddedStream, pszFilename );

        ef.AddKey( "F",  pEmbeddedStream->Reference() );

        this->GetObject()->GetDictionary().AddKey( "EF", ef );
    }
}

void PdfFileSpec::Init( const wchar_t* pszFilename, const unsigned char* data, ptrdiff_t size, bool bStripPath)
{
    PdfObject* pEmbeddedStream;
    PdfString filename;

    filename.setFromWchar_t( MaybeStripPath( pszFilename, true) );

    this->GetObject()->GetDictionary().AddKey( "F", this->CreateFileSpecification( MaybeStripPath( pszFilename, bStripPath ) ) );
    this->GetObject()->GetDictionary().AddKey( "UF", filename.ToUnicode() );

    PdfDictionary ef;

    pEmbeddedStream = this->CreateObject( "EmbeddedFile" );
    this->EmbeddFileFromMem( pEmbeddedStream, data, size );

    ef.AddKey( "F",  pEmbeddedStream->Reference() );

    this->GetObject()->GetDictionary().AddKey( "EF", ef );
}

PdfString PdfFileSpec::CreateFileSpecification( const wchar_t* pszFilename ) const
{
    std::ostringstream str;
    size_t                nLen = wcslen( pszFilename );
    char buff[5];

    // Construct a platform independent file specifier
    
    for( size_t i=0;i<nLen;i++ ) 
    {
        wchar_t ch = pszFilename[i];
        if (ch == L':' || ch == L'\\')
            ch = L'/';
        if ((ch >= L'a' && ch <= L'z') ||
            (ch >= L'A' && ch <= L'Z') ||
            (ch >= L'0' && ch <= L'9') ||
             ch == L'_') {
            str.put( ch & 0xFF );
        } else if (ch == L'/') {
            str.put( '\\' );
            str.put( '\\' );
            str.put( '/' );
        } else {
            sprintf(buff, "%04X", ch & 0xFFFF);
            str << buff;
        }
    }

    return PdfString( str.str() );
}

void PdfFileSpec::EmbeddFile( PdfObject* pStream, const wchar_t* pszFilename ) const
{
    PdfFileInputStream stream( pszFilename );
    pStream->GetStream()->Set( &stream );

    // Add additional information about the embedded file to the stream
    PdfDictionary params;
    params.AddKey( "Size", static_cast<pdf_int64>(stream.GetFileLength()) );
    // TODO: CreationDate and ModDate
    pStream->GetDictionary().AddKey("Params", params );
}

const wchar_t *PdfFileSpec::MaybeStripPath( const wchar_t* pszFilename, bool bStripPath ) const
{
    if (!bStripPath)
    {
        return pszFilename;
    }

    const wchar_t *lastFrom = pszFilename;
    while (pszFilename && *pszFilename)
    {
        if (
            #ifdef _WIN32
            *pszFilename == L':' || *pszFilename == L'\\' ||
            #endif // _WIN32
            *pszFilename == L'/')
        {
            lastFrom = pszFilename + 1;
        }

        pszFilename++;
    }

    return lastFrom;
}

#endif // _WIN32

void PdfFileSpec::Init( const char* pszFilename, bool bEmbedd, bool bStripPath ) 
{
    PdfObject* pEmbeddedStream;
    PdfString filename( MaybeStripPath( pszFilename, true) );

    this->GetObject()->GetDictionary().AddKey( "F", this->CreateFileSpecification( MaybeStripPath( pszFilename, bStripPath ) ) );
    this->GetObject()->GetDictionary().AddKey( "UF", filename.ToUnicode () );

    if( bEmbedd ) 
    {
        PdfDictionary ef;

        pEmbeddedStream = this->CreateObject( "EmbeddedFile" );
        this->EmbeddFile( pEmbeddedStream, pszFilename );

        ef.AddKey( "F",  pEmbeddedStream->Reference() );

        this->GetObject()->GetDictionary().AddKey( "EF", ef );
    }
}

void PdfFileSpec::Init( const char* pszFilename, const unsigned char* data, ptrdiff_t size, bool bStripPath ) 
{
    PdfObject* pEmbeddedStream;
    PdfString filename( MaybeStripPath( pszFilename, true) );

    this->GetObject()->GetDictionary().AddKey( "F", this->CreateFileSpecification( MaybeStripPath( pszFilename, bStripPath) ) );
    this->GetObject()->GetDictionary().AddKey( "UF", filename.ToUnicode () );

    PdfDictionary ef;

    pEmbeddedStream = this->CreateObject( "EmbeddedFile" );
    this->EmbeddFileFromMem( pEmbeddedStream, data, size );

    ef.AddKey( "F",  pEmbeddedStream->Reference() );

    this->GetObject()->GetDictionary().AddKey( "EF", ef );
}

PdfString PdfFileSpec::CreateFileSpecification( const char* pszFilename ) const
{
    std::ostringstream str;
    size_t                nLen = strlen( pszFilename );
    char buff[5];

    // Construct a platform independent file specifier
    
    for( size_t i=0;i<nLen;i++ ) 
    {
        char ch = pszFilename[i];
        if (ch == ':' || ch == '\\')
            ch = '/';
        if ((ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') ||
             ch == '_') {
            str.put( ch & 0xFF );
        } else if (ch == '/') {
            str.put( '\\' );
            str.put( '\\' );
            str.put( '/' );
        } else {
            sprintf(buff, "%02X", ch & 0xFF);
            str << buff;
        }
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

const char *PdfFileSpec::MaybeStripPath( const char* pszFilename, bool bStripPath ) const
{
    if (!bStripPath)
    {
        return pszFilename;
    }

    const char *lastFrom = pszFilename;
    while (pszFilename && *pszFilename)
    {
        if (
            #ifdef _WIN32
            *pszFilename == ':' || *pszFilename == '\\' ||
            #endif // _WIN32
            *pszFilename == '/')
        {
            lastFrom = pszFilename + 1;
        }

        pszFilename++;
    }

    return lastFrom;
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

const PdfString & PdfFileSpec::GetFilename(bool canUnicode) const
{
    if( canUnicode && this->GetObject()->GetDictionary().HasKey( "UF" ) )
{
        return this->GetObject()->GetDictionary().GetKey( "UF" )->GetString();
    }

    if( this->GetObject()->GetDictionary().HasKey( "F" ) )
    {
        return this->GetObject()->GetDictionary().GetKey( "F" )->GetString();
    }

    PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
}


};

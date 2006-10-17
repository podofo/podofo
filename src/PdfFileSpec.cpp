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

#include "PdfDictionary.h"
#include "PdfObject.h"
#include "PdfStream.h"

#include <sstream>

namespace PoDoFo {

PdfFileSpec::PdfFileSpec( const char* pszFilename, bool bEmbedd, PdfVecObjects* pParent )
    : PdfElement( "Filespec", pParent )
{
    PdfObject* pEmbeddedStream;

    m_pObject->GetDictionary().AddKey( "F", this->CreateFileSpecification( pszFilename ) );

    if( bEmbedd ) 
    {
        PdfDictionary ef;

        pEmbeddedStream = pParent->CreateObject( "EmbeddedFile" );
        this->EmbeddFile( pEmbeddedStream->GetStream(), pszFilename );

        ef.AddKey( "DOS",  pEmbeddedStream->Reference() );
        ef.AddKey( "Mac",  pEmbeddedStream->Reference() );
        ef.AddKey( "Unix", pEmbeddedStream->Reference() );
            
        m_pObject->GetDictionary().AddKey( "EF", ef );
    }
}

PdfFileSpec::PdfFileSpec( PdfObject* pObject )
    : PdfElement( "Filespec", pObject )
{

}

PdfString PdfFileSpec::CreateFileSpecification( const char* pszFilename ) const
{
    std::ostringstream str;
    int                nLen = strlen( pszFilename );

    // Not sure if we really get a platform independent file specifier here.
    
    for( int i=0;i<nLen;i++ ) 
    {
        if( pszFilename[i] == ':' || pszFilename[i] == '\\' ) 
            str.put( '/' );
        else 
            str.put( pszFilename[i] );
    }

    return PdfString( str.str() );
}

void PdfFileSpec::EmbeddFile( PdfStream* pStream, const char* pszFilename ) const
{
    long  lLen;
    char* pBuf;
    FILE* hFile = fopen( pszFilename, "rb" );

    if( !hFile ) 
    {
        RAISE_ERROR( ePdfError_FileNotFound );
    }

    fseek( hFile, 0L, SEEK_END );
    lLen = ftell( hFile );
    fseek( hFile, 0L, SEEK_SET );

    pBuf = static_cast<char*>(malloc( sizeof(char) * lLen ));
    if( !pBuf ) 
    {
        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    pStream->Set( pBuf, lLen, true );

    fclose( hFile );

}

};

/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
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

#include "PdfFontType1.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfStream.h"

namespace PoDoFo {

PdfFontType1::PdfFontType1( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, bool bEmbed, PdfVecObjects* pParent )
    : PdfFontSimple( pMetrics, pEncoding, pParent )
{
    this->Init( bEmbed, PdfName("Type1") );
}

void PdfFontType1::EmbedFont( PdfObject* pDescriptor )
{
    long        lSize    = 0;
    long        lLength1 = 0L;
    long        lLength2 = 0L;
    long        lLength3 = 0L;
    PdfObject*  pContents;
    const char* pBuffer;
    char*       pAllocated = NULL;

    pContents = m_pObject->GetOwner()->CreateObject();
    if( !pContents )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
        
    pDescriptor->GetDictionary().AddKey( "FontFile", pContents->Reference() );

    // if the data was loaded from memory - use it from there
    // otherwise, load from disk
    if ( m_pMetrics->GetFontDataLen() && m_pMetrics->GetFontData() ) 
    {
        pBuffer = m_pMetrics->GetFontData();
        lSize   = m_pMetrics->GetFontDataLen();
    }
    else
    {
        FILE* hFile = fopen( m_pMetrics->GetFilename(), "rb" );
        if( !hFile )
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_FileNotFound, m_pMetrics->GetFilename() );
        }

        fseek( hFile, 0L, SEEK_END );
        lSize = ftell( hFile );
        fseek( hFile, 0L, SEEK_SET );

        pAllocated = static_cast<char*>(malloc( sizeof(char) * lSize ));
        if( !pAllocated )
        {
            fclose( hFile );
            PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
        }

        fread( pAllocated, sizeof(char), lSize, hFile );
        fclose( hFile );

        pBuffer = pAllocated;
    }

    // Parse the font data buffer to get the values for length1, length2 and length3
    lLength1 = FindInBuffer( "eexec", pBuffer, lSize );
    if( lLength1 > 0 )
        lLength1 += 6; // 6 == eexec + lf
    else
        lLength1 = 0;
    
    if( lLength1 )
    {
        lLength2 = FindInBuffer( "cleartomark", pBuffer, lSize );
        if( lLength2 > 0 )
            lLength2 = lSize - lLength1 - 520; // 520 == 512 + strlen(cleartomark)
        else
            lLength1 = 0;
    }
    
    lLength3 = lSize - lLength2 - lLength1;
    
    // TODO: Pdf Supports only Type1 fonts with binary encrypted sections and not the hex format
    pContents->GetStream()->Set( pBuffer, lSize );
    if( pAllocated )
        free( pAllocated );

    pContents->GetDictionary().AddKey( "Length1", PdfVariant( lLength1 ) );
    pContents->GetDictionary().AddKey( "Length2", PdfVariant( lLength2 ) );
    pContents->GetDictionary().AddKey( "Length3", PdfVariant( lLength3 ) );
}

long PdfFontType1::FindInBuffer( const char* pszNeedle, const char* pszHaystack, long lLen ) const
{
    // if lNeedleLen is 0 the while loop will not be executed and we return -1
    long lNeedleLen      = pszNeedle ? strlen( pszNeedle ) : 0; 
    const char* pszEnd   = pszHaystack + lLen - lNeedleLen; 
    const char* pszStart = pszHaystack;

    while( pszHaystack < pszEnd ) 
    {
        if( strncmp( pszHaystack, pszNeedle, lNeedleLen ) == 0 )
            return pszHaystack - pszStart;

        ++pszHaystack;
    }

    return -1;
}

};


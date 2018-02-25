/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "ImageExtractor.h"

#include <sys/stat.h>
#include <stdlib.h>
#include <cstdio>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

ImageExtractor::ImageExtractor()
    : m_pszOutputDirectory( NULL ), m_nSuccess( 0 ), m_nCount( 0 )
{

}

ImageExtractor::~ImageExtractor()
{
}

void ImageExtractor::Init( const char* pszInput, const char* pszOutput, int* pnNum )
{
    PdfObject*  pObj  = NULL;

    if( !pszInput || !pszOutput )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }


    PdfMemDocument document( pszInput );

    m_pszOutputDirectory = const_cast<char*>(pszOutput);

    TCIVecObjects it = document.GetObjects().begin();

    if( pnNum )
        *pnNum = 0;

    while( it != document.GetObjects().end() )
    {
        if( (*it)->IsDictionary() )
        {            
            PdfObject* pObjType = (*it)->GetDictionary().GetKey( PdfName::KeyType );
            PdfObject* pObjSubType = (*it)->GetDictionary().GetKey( PdfName::KeySubtype );
            if( ( pObjType && pObjType->IsName() && ( pObjType->GetName().GetName() == "XObject" ) ) ||
                ( pObjSubType && pObjSubType->IsName() && ( pObjSubType->GetName().GetName() == "Image" ) ) )
            {
                pObj = (*it)->GetDictionary().GetKey( PdfName::KeyFilter );
                if( pObj && pObj->IsArray() && pObj->GetArray().GetSize() == 1 && 
                    pObj->GetArray()[0].IsName() && (pObj->GetArray()[0].GetName().GetName() == "DCTDecode") )
                    pObj = &pObj->GetArray()[0];

                if( pObj && pObj->IsName() && ( pObj->GetName().GetName() == "DCTDecode" ) )
                {
                    // The only filter is JPEG -> create a JPEG file
                    ExtractImage( *it, true );
                }
                else
                {
                    ExtractImage( *it, false );
                }
                
                document.FreeObjectMemory( *it );
            }
        }

        ++it;
    }
}

void ImageExtractor::ExtractImage( PdfObject* pObject, bool bJpeg )
{
    FILE*       hFile        = NULL;
    const char* pszExtension = bJpeg ? "jpg" : "ppm"; 

    // Do not overwrite existing files:
    do {
        snprintf( m_szBuffer, MAX_PATH, "%s/pdfimage_%04i.%s", m_pszOutputDirectory, m_nCount++, pszExtension );
    } while( FileExists( m_szBuffer ) );

    hFile = fopen( m_szBuffer, "wb" );
    if( !hFile )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    printf("-> Writing image object %s to the file: %s\n", pObject->Reference().ToString().c_str(), m_szBuffer);

    if( bJpeg ) 
    {
        PdfMemStream* pStream = dynamic_cast<PdfMemStream*>(pObject->GetStream());
        fwrite( pStream->Get(), pStream->GetLength(), sizeof(char), hFile );
    }
    else
    {
        //long lBitsPerComponent = pObject->GetDictionary().GetKey( PdfName("BitsPerComponent" ) )->GetNumber();
        // TODO: Handle colorspaces

        // Create a ppm image
        const char* pszPpmHeader = "P6\n# Image extracted by PoDoFo\n%" PDF_FORMAT_INT64 " %" PDF_FORMAT_INT64 "\n%li\n";
        
        
        fprintf( hFile, pszPpmHeader, 
                 pObject->GetDictionary().GetKey( PdfName("Width" ) )->GetNumber(),
                 pObject->GetDictionary().GetKey( PdfName("Height" ) )->GetNumber(),
                 255 );
                 
        char*    pBuffer;
        pdf_long lLen;
        pObject->GetStream()->GetFilteredCopy( &pBuffer, &lLen );
        fwrite( pBuffer, lLen, sizeof(char), hFile );
        free( pBuffer );
    }

    fclose( hFile );

    ++m_nSuccess;
}

bool ImageExtractor::FileExists( const char* pszFilename )
{
    bool result = true;
    
    // if there is an error, it's probably because the file doesn't yet exist
    struct stat	stBuf;
    if ( stat( pszFilename, &stBuf ) == -1 )	result = false;

    return result;
}

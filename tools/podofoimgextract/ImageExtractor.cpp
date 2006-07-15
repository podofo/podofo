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

ImageExtractor::ImageExtractor()
    : m_pszOutputDirectory( NULL ), m_nCount( 0 )
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
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pszOutputDirectory = (char*)pszOutput;

    m_parser.Init( pszInput );
    
    const TVecObjects&  vecObjects = m_parser.GetObjects();
    TCIVecObjects it = vecObjects.begin();

    if( pnNum )
        *pnNum = 0;

    while( it != vecObjects.end() )
    {
        if( (*it)->GetDictionary().HasKey( PdfName::KeyType ) )
        {
            pObj = (*it)->GetDictionary().GetKey( PdfName::KeyType );
            if( pObj->IsName() && ( pObj->GetName().Name() == "XObject" ) )
            {
                if( (*it)->GetDictionary().HasKey( PdfName::KeySubtype ) )
                {
                    pObj = (*it)->GetDictionary().GetKey( PdfName::KeySubtype );
                    if( pObj->IsName() && ( pObj->GetName().Name() == "Image" ) )
                    {
                        pObj = (*it)->GetDictionary().GetKey( PdfName::KeyFilter );
                        if( pObj->IsName() && ( pObj->GetName().Name() == "DCTDecode" ) )
                        {	
                           // ONLY images with filter of DCTDecode can be extracted out as JPEG this way!
                            
                            ExtractImage( *it );

                            if( pnNum )
                                ++(*pnNum);
                        }
                    }
                }
            }
                    
        }

        ++it;
    }
}

void ImageExtractor::ExtractImage( PdfObject* pObject )
{
    FILE*    hFile = NULL;
//    long     lLen;

    // Do not overwrite existing files:
    do {
        snprintf( m_szBuffer, MAX_PATH, "%s/pdfimage_%i.jpg", m_pszOutputDirectory, m_nCount++ );
    } while( FileExists( m_szBuffer ) );

    hFile = fopen( m_szBuffer, "wb" );
    if( !hFile )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    printf("-> Writing image object %s to the file: %s\n", pObject->Reference().ToString().c_str(), m_szBuffer);

    fwrite( pObject->Stream()->Get(), pObject->Stream()->Length(), sizeof(char), hFile );
    fclose( hFile );
}

bool ImageExtractor::FileExists( const char* pszFilename )
{
    bool result = true;
    
	// if there is an error, it's probably because the file doesn't yet exist
	struct	stat	stBuf;
	if ( stat( pszFilename, &stBuf ) == -1 )	result = false;

    return result;
}

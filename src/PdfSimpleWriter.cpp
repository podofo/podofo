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

#include "PdfSimpleWriter.h"

#include "PdfDate.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfObject.h"
#include "PdfPage.h"
#include "PdfImage.h"

#include <fontconfig.h>

#include <sstream>

#define PRODUCER_NAME "(PdfSignature Library)"

namespace PoDoFo {

using namespace std;

class FontComperator { 
public:
    FontComperator( const string & sPath )
        {
            m_sPath = sPath;
        }
    
    bool operator()(const PdfFont* pFont) 
        { 
            return (m_sPath == pFont->FontMetrics()->Filename());
        }
private:
    string m_sPath;
};


PdfSimpleWriter::PdfSimpleWriter()
{
    m_pPageTree     = NULL;
    m_nPageTreeSize = 0;
    m_bInitDone     = false;

    m_pFcConfig     = (void*)FcInitLoadConfigAndFonts();
}    

PdfSimpleWriter::~PdfSimpleWriter()
{
    FcConfigDestroy( (FcConfig*)m_pFcConfig );

    if( m_bInitDone )    
        FT_Done_FreeType( m_ftLibrary );
}

PdfError PdfSimpleWriter::Init( const char* pszFilename )
{
    PdfError  eCode;
    PdfDate   cDate;
    PdfString sDate;

    m_bInitDone = true;
    if( FT_Init_FreeType( &m_ftLibrary ) )
    {
        RAISE_ERROR( ePdfError_FreeType );
    }

    SAFE_OP( PdfWriter::Init( pszFilename ) );

    m_pPageTree = CreateObject( "Pages", true );
    m_pPageTree->AddKey( "Kids", "[ ]" );

    this->GetCatalog()->AddKey( "Pages", m_pPageTree->Reference().c_str() );

    cDate.ToString( sDate );
    this->GetInfo()->AddKey( "Producer", PRODUCER_NAME );
    this->GetInfo()->AddKey( "CreationDate", sDate );

    return eCode;
}

PdfPage* PdfSimpleWriter::CreatePage( const TSize & tSize )
{
    ostringstream oStream;
    ostringstream oStreamKids;

    TCIStringList it;
    PdfPage*      pPage    = new PdfPage( tSize, this, m_nObjectCount++, 0 );

    m_vecObjects       .push_back( pPage );
    m_vecPageReferences.push_back( pPage->Reference() );

    oStream << ++m_nPageTreeSize;

    oStreamKids << "[ ";
    it = m_vecPageReferences.begin();
    while( it != m_vecPageReferences.end()  )
    {
        oStreamKids << (*it) << " ";
        ++it;
    }
    oStreamKids << "]";

    m_pPageTree->AddKey( "Count", oStream.str().c_str() );
    m_pPageTree->AddKey( "Kids",  oStreamKids.str().c_str() );

    pPage->AddKey( "Parent", m_pPageTree->Reference().c_str() );
    if( pPage->Init() != ePdfError_ErrOk )
        return NULL;

    return pPage;
}

PdfFont* PdfSimpleWriter::CreateFont( const char* pszFontName, bool bEmbedd )
{
    PdfError          eCode;
    std::string       sPath = PdfFontMetrics::GetFilenameForFont( (FcConfig*)m_pFcConfig, pszFontName );
    PdfFont*          pFont;
    PdfFontMetrics*   pMetrics;
    TCISortedFontList it;

    if( sPath.empty() )
    {
        PdfError::LogMessage( eLogSeverity_Critical, "No path was found for the specified fontname: %s\n", pszFontName );
        return NULL;
    }

    it = std::find_if( m_vecFonts.begin(), m_vecFonts.end(), FontComperator( sPath ) );

    if( it == m_vecFonts.end() )
    {
        pMetrics = new PdfFontMetrics( &m_ftLibrary, sPath.c_str() );
        pFont    = new PdfFont( pMetrics, this, m_nObjectCount++, 0 );

        m_vecObjects.push_back( pFont );
        m_vecFonts  .push_back( pFont );

        // Now sort the font list
        std::sort( m_vecFonts.begin(), m_vecFonts.end() );

        eCode = pFont->Init( bEmbedd );
        if( eCode.IsError() )
        {
            eCode.PrintErrorMsg();
            PdfError::LogMessage( eLogSeverity_Error, "Cannot initialize font: %s\n", pszFontName );
            return NULL;
        }
    }
    else
        pFont = *it;

    return pFont;
}

PdfImage* PdfSimpleWriter::CreateImage()
{
    PdfImage* pImage = new PdfImage( m_nObjectCount++, 0 );

    m_vecObjects.push_back( pImage );

    return pImage;
}

void PdfSimpleWriter::SetDocumentAuthor( const PdfString & sAuthor )
{
    this->GetInfo()->AddKey( "Author", sAuthor );
}

void PdfSimpleWriter::SetDocumentCreator( const PdfString & sCreator )
{
    this->GetInfo()->AddKey( "Creator", sCreator );
}

void PdfSimpleWriter::SetDocumentKeywords( const PdfString & sKeywords )
{
    this->GetInfo()->AddKey( "Keywords", sKeywords );
}

void PdfSimpleWriter::SetDocumentSubject( const PdfString & sSubject )
{
    this->GetInfo()->AddKey( "Subject", sSubject );
}

void PdfSimpleWriter::SetDocumentTitle( const PdfString & sTitle )
{
    this->GetInfo()->AddKey( "Title", sTitle );
}

}; 

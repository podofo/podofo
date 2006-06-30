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

#include "PdfDictionary.h"
#include "PdfDate.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfObject.h"
#include "PdfPage.h"
#include "PdfImage.h"

#ifndef _WIN32
#include <fontconfig.h>
#endif

#include <algorithm>
#include <sstream>

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

#ifndef _WIN32
    m_pFcConfig     = (void*)FcInitLoadConfigAndFonts();
#endif
}    

PdfSimpleWriter::~PdfSimpleWriter()
{
#ifndef _WIN32
    FcConfigDestroy( (FcConfig*)m_pFcConfig );
#endif

    if( m_bInitDone )    
        FT_Done_FreeType( m_ftLibrary );
}

PdfError PdfSimpleWriter::Init()
{
    PdfError  eCode;
    PdfDate   cDate;
    PdfString sDate;

    m_bInitDone = true;
    if( FT_Init_FreeType( &m_ftLibrary ) )
    {
        RAISE_ERROR( ePdfError_FreeType );
    }

    SAFE_OP( PdfWriter::Init() );

    m_pPageTree = m_vecObjects.CreateObject( "Pages" );
    m_pPageTree->GetDictionary().AddKey( "Kids", PdfArray() );

    this->GetCatalog()->GetDictionary().AddKey( "Pages", m_pPageTree->Reference() );

    cDate.ToString( sDate );
    this->GetInfo()->GetDictionary().AddKey( "Producer", PdfString("PoDoFo") );
    this->GetInfo()->GetDictionary().AddKey( "CreationDate", sDate );

    return eCode;
}

PdfPage* PdfSimpleWriter::CreatePage( const TSize & tSize )
{
#if 1	// until this gets revamped to use a PdfDocument
    PdfPage*         pPage = new PdfPage( NULL, m_vecObjects.GetObjectCount(), 0 );
	PdfObject* pObject   = dynamic_cast<PdfObject*>(pPage);
	if( !pObject )
	{
		delete pPage;
		return NULL;
	}

	m_vecObjects.push_back( pObject );
#else
	PdfPage*         pPage    = m_pDocument.CreateObject<PdfPage>();
#endif

    m_vecPageReferences.push_back( pPage->GetObject()->Reference() );

    m_pPageTree->GetDictionary().AddKey( "Count", PdfVariant( (long)++m_nPageTreeSize ) );
    m_pPageTree->GetDictionary().AddKey( "Kids",  PdfVariant( m_vecPageReferences ) );

    pPage->GetObject()->GetDictionary().AddKey( "Parent", m_pPageTree->Reference() );
    if( pPage->Init( tSize, &m_vecObjects ).IsError() )
    {
        delete pPage;
        return NULL;
    }

    return pPage;
}

PdfFont* PdfSimpleWriter::CreateFont( const char* pszFontName, bool bEmbedd )
{
    PdfError          eCode;
#ifdef _WIN32
    std::string       sPath = PdfFontMetrics::GetFilenameForFont( pszFontName );
#else
    std::string       sPath = PdfFontMetrics::GetFilenameForFont( (FcConfig*)m_pFcConfig, pszFontName );
#endif
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
        pFont    = m_vecObjects.CreateObject<PdfFont>();

        m_vecFonts  .push_back( pFont );

        // Now sort the font list
        std::sort( m_vecFonts.begin(), m_vecFonts.end() );

        eCode = pFont->Init( pMetrics, &m_vecObjects, bEmbedd );
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
    return m_vecObjects.CreateObject<PdfImage>();
}

void PdfSimpleWriter::SetDocumentAuthor( const PdfString & sAuthor )
{
    this->GetInfo()->GetDictionary().AddKey( "Author", sAuthor );
}

void PdfSimpleWriter::SetDocumentCreator( const PdfString & sCreator )
{
    this->GetInfo()->GetDictionary().AddKey( "Creator", sCreator );
}

void PdfSimpleWriter::SetDocumentKeywords( const PdfString & sKeywords )
{
    this->GetInfo()->GetDictionary().AddKey( "Keywords", sKeywords );
}

void PdfSimpleWriter::SetDocumentSubject( const PdfString & sSubject )
{
    this->GetInfo()->GetDictionary().AddKey( "Subject", sSubject );
}

void PdfSimpleWriter::SetDocumentTitle( const PdfString & sTitle )
{
    this->GetInfo()->GetDictionary().AddKey( "Title", sTitle );
}

}; 

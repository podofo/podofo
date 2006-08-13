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

#include <algorithm>
#include <deque>

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfDocument.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfObject.h"
#include "PdfPage.h"
#include "PdfPagesTree.h"
#include "PdfVecObjects.h"

#ifndef _WIN32
#include <fontconfig.h>
#endif

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


PdfDocument::PdfDocument()
    : m_pPagesTree( NULL ), m_pTrailer( NULL ), m_ftLibrary( NULL )
{
    m_eVersion    = ePdfVersion_1_3;
    m_bLinearized = false;

    m_pTrailer = new PdfObject();
    m_pTrailer->SetParent( &m_vecObjects );
    m_pCatalog = m_vecObjects.CreateObject( "Catalog" );

    m_pTrailer->GetDictionary().AddKey( "Root", m_pCatalog->Reference() );

    InitPagesTree();
    InitFonts();
}

PdfDocument::PdfDocument( const char* pszFilename )
    : m_pPagesTree( NULL ), m_pTrailer( NULL ), m_ftLibrary( NULL )
{
    PdfParser parser( pszFilename, true );

    InitFromParser( &parser );
    InitPagesTree();
    InitFonts();
}

PdfDocument::PdfDocument( PdfParser* pParser )
    : m_pPagesTree( NULL ), m_pTrailer( NULL )
{
    InitFromParser( pParser );
    InitPagesTree();
    InitFonts();
}

PdfDocument::~PdfDocument()
{
    TIVecObjects     it     = m_vecObjects.begin();
    TISortedFontList itFont = m_vecFonts.begin();

    while( it != m_vecObjects.end() )
    {
        delete (*it);
        ++it;
    }

    while( itFont != m_vecFonts.end() )
    {
        delete (*itFont);
        ++itFont;
    }

    m_vecObjects.clear();
    m_vecFonts.clear();

    if ( m_pPagesTree ) 
    {
        delete m_pPagesTree;
        m_pPagesTree = NULL;
    }

    if ( m_pTrailer ) 
    {
        delete m_pTrailer;
        m_pTrailer = NULL;
    }

#ifndef _WIN32
    FcConfigDestroy( (FcConfig*)m_pFcConfig );
#endif

    if( m_ftLibrary ) 
    {
        FT_Done_FreeType( m_ftLibrary );
        m_ftLibrary = NULL;
    }
}

void PdfDocument::InitFonts()
{
#ifndef _WIN32
    m_pFcConfig     = (void*)FcInitLoadConfigAndFonts();
#endif

    if( FT_Init_FreeType( &m_ftLibrary ) )
    {
        RAISE_ERROR( ePdfError_FreeType );
    }
}

void PdfDocument::InitFromParser( PdfParser* pParser )
{
    PdfObject* pRoot;

    m_vecObjects   = pParser->GetObjects();
    m_eVersion     = pParser->GetPdfVersion();
    m_bLinearized  = pParser->IsLinearized();

    // clear the parsers object value
    // other PdfWriter and PdfParser
    // would delete the same objects
    pParser->m_vecObjects.clear();

    m_pTrailer = new PdfObject( *(pParser->GetTrailer()) );
    m_pTrailer->SetParent( &m_vecObjects );

    pRoot      = m_pTrailer->GetDictionary().GetKey( "Root" );
    if( pRoot && pRoot->IsReference() )
        m_pCatalog = m_vecObjects.GetObject( pRoot->GetReference() );
    else
    {
        RAISE_ERROR( ePdfError_NoObject );
    }
}

void PdfDocument::InitPagesTree()
{
    PdfObject* pagesRootObj = m_pCatalog->GetIndirectKey( PdfName( "Pages" ) );
    if ( pagesRootObj ) 
    {
        m_pPagesTree = new PdfPagesTree( pagesRootObj );
    }
    else
    {
        m_pPagesTree = new PdfPagesTree( &m_vecObjects );
        m_pCatalog->GetDictionary().AddKey( "Pages", m_pPagesTree->Object()->Reference() );
    }
}

void PdfDocument::Write( const char* pszFilename )
{
    PdfOutputDevice device( pszFilename );
    PdfWriter       writer;
    
    writer.Init( this );
    writer.Write( pszFilename );    
}

PdfObject* PdfDocument::GetNamedObjectFromCatalog( const char* pszName ) const 
{
    return m_pCatalog->GetIndirectKey( PdfName( pszName ) );
}

PdfObject* PdfDocument::GetInfo( bool bCreate )
{ 
    PdfObject* pObj = m_pTrailer->GetIndirectKey( PdfName( "Info" ) );

    if( !pObj && bCreate ) 
    {
        pObj = m_vecObjects.CreateObject( "Info" );
        m_pTrailer->GetDictionary().AddKey( PdfName( "Info" ), pObj->Reference() );
    }

    return pObj; 
}

int PdfDocument::GetPageCount() const
{
    return m_pPagesTree->GetTotalNumberOfPages();
}

PdfPage PdfDocument::GetPage( int nIndex ) const
{
    if( nIndex < 0 || nIndex > m_pPagesTree->GetTotalNumberOfPages() )
    {
        RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    PdfObject*	pgObj = m_pPagesTree->GetPage( nIndex );
    if ( !pgObj ) 
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    return PdfPage( pgObj );
}

PdfFont* PdfDocument::CreateFont( const char* pszFontName, bool bEmbedd )
{
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

        try {
            pFont    = new PdfFont( pMetrics, bEmbedd, &m_vecObjects );

            m_vecFonts  .push_back( pFont );

            // Now sort the font list
            std::sort( m_vecFonts.begin(), m_vecFonts.end() );
        } catch( PdfError & e ) {
            e.AddToCallstack( __FILE__, __LINE__ );
            e.PrintErrorMsg();
            PdfError::LogMessage( eLogSeverity_Error, "Cannot initialize font: %s\n", pszFontName );
            return NULL;
        }
    }
    else
        pFont = *it;

    return pFont;
}

PdfPage PdfDocument::CreatePage( const PdfRect & rSize )
{
    PdfPage page( rSize, &m_vecObjects );

    m_pPagesTree->InsertPage( page.Object() );

    return page;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfDocument::SetAuthor( const PdfString & sAuthor )
{
    this->GetInfo( true )->GetDictionary().AddKey( "Author", sAuthor );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfString & PdfDocument::Author() const
{
    PdfObject* pObj = this->GetInfo()->GetDictionary().GetKey( "Author" );
    return pObj && pObj->IsString() ? pObj->GetString() : PdfString::StringNull;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfDocument::SetCreator( const PdfString & sCreator )
{
    this->GetInfo( true )->GetDictionary().AddKey( "Creator", sCreator );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfString & PdfDocument::Creator() const
{
    PdfObject* pObj = this->GetInfo()->GetDictionary().GetKey( "Creator" );
    return pObj && pObj->IsString() ? pObj->GetString() : PdfString::StringNull;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfDocument::SetKeywords( const PdfString & sKeywords )
{
    this->GetInfo( true )->GetDictionary().AddKey( "Keywords", sKeywords );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfString & PdfDocument::Keywords() const
{
    PdfObject* pObj = this->GetInfo()->GetDictionary().GetKey( "Keywords" );
    return pObj && pObj->IsString() ? pObj->GetString() : PdfString::StringNull;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfDocument::SetSubject( const PdfString & sSubject )
{
    this->GetInfo( true )->GetDictionary().AddKey( "Subject", sSubject );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfString & PdfDocument::Subject() const
{
    PdfObject* pObj = this->GetInfo()->GetDictionary().GetKey( "Subject" );
    return pObj && pObj->IsString() ? pObj->GetString() : PdfString::StringNull;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfDocument::SetTitle( const PdfString & sTitle )
{
    this->GetInfo( true )->GetDictionary().AddKey( "Title", sTitle );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfString & PdfDocument::Title() const
{
    PdfObject* pObj = this->GetInfo()->GetDictionary().GetKey( "Title" );
    return pObj && pObj->IsString() ? pObj->GetString() : PdfString::StringNull;
}

};


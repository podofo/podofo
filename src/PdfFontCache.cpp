/***************************************************************************
*   Copyright (C) 2007 by Dominik Seichter                                *
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

#include "PdfFontCache.h" 

#include "PdfFont.h"
#include "PdfFontMetrics.h"

using namespace std;

namespace PoDoFo {

class FontComperator { 
public:
    FontComperator( const string & sPath )
    {
        m_sPath = sPath;
    }
    
    bool operator()(const PdfFont* pFont) 
    { 
        return (m_sPath == pFont->GetFontMetrics()->GetFilename());
    }

private:
    string m_sPath;
};

PdfFontCache::PdfFontCache( PdfVecObjects* pParent )
    : m_pParent( pParent )
{
    // Initialize all the fonts stuff

#if !defined(_WIN32) && !defined(__APPLE_CC__)
    m_pFcConfig     = FcInitLoadConfigAndFonts();
#endif

    if( FT_Init_FreeType( &m_ftLibrary ) )
    {
        RAISE_ERROR( ePdfError_FreeType );
    }
}

PdfFontCache::~PdfFontCache()
{
    this->EmptyCache();

#if !defined(_WIN32) && !defined(__APPLE_CC__)
    FcConfigDestroy( static_cast<FcConfig*>(m_pFcConfig) );
#endif

    if( m_ftLibrary ) 
    {
        FT_Done_FreeType( m_ftLibrary );
        m_ftLibrary = NULL;
    }
}

void PdfFontCache::EmptyCache() 
{
    TISortedFontList itFont = m_vecFonts.begin();

    while( itFont != m_vecFonts.end() )
    {
        delete (*itFont);
        ++itFont;
    }

    m_vecFonts.clear();
}

PdfFont* PdfFontCache::GetFont( const char* pszFontName, bool bEmbedd )
{
    PdfFont*          pFont;
    PdfFontMetrics*   pMetrics;
    TCISortedFontList it;

    std::string sPath = this->GetFontPath( pszFontName );
    if( sPath.empty() )
    {
        PdfError::LogMessage( eLogSeverity_Critical, "No path was found for the specified fontname: %s\n", pszFontName );
        return NULL;
    }

    it = std::find_if( m_vecFonts.begin(), m_vecFonts.end(), FontComperator( sPath ) );

    if( it == m_vecFonts.end() )
    {
        pMetrics = new PdfFontMetrics( &m_ftLibrary, sPath.c_str() );
        pFont    = this->CreateFont( pMetrics, bEmbedd, pszFontName );
    }
    else
        pFont = *it;

    return pFont;
}

PdfFont* PdfFontCache::GetFont( FT_Face face, bool bEmbedd )
{
    PdfFont*          pFont;
    PdfFontMetrics*   pMetrics;
    TCISortedFontList it;

    std::string sPath = FT_Get_Postscript_Name( face );
    if( sPath.empty() )
    {
        PdfError::LogMessage( eLogSeverity_Critical, "Could not retrieve fontname for font!\n" );
        return NULL;
    }

    it = std::find_if( m_vecFonts.begin(), m_vecFonts.end(), FontComperator( sPath ) );

    if( it == m_vecFonts.end() )
    {
        pMetrics = new PdfFontMetrics( &m_ftLibrary, face );
        pFont    = this->CreateFont( pMetrics, bEmbedd );
    }
    else
        pFont = *it;

    return pFont;
}

std::string PdfFontCache::GetFontPath( const char* pszFontName )
{
#if defined(_WIN32) || defined(__APPLE_CC__)
    std::string sPath = PdfFontMetrics::GetFilenameForFont( pszFontName );
#else
    std::string sPath = PdfFontMetrics::GetFilenameForFont( static_cast<FcConfig*>(m_pFcConfig), pszFontName );
#endif

    return sPath;
}

PdfFont* PdfFontCache::CreateFont( PdfFontMetrics* pMetrics, bool bEmbedd, const char* pszFontName ) 
{
    PdfFont* pFont;

    try {
        pFont    = new PdfFont( pMetrics, bEmbedd, m_pParent );
        
        m_vecFonts  .push_back( pFont );
        
        // Now sort the font list
        std::sort( m_vecFonts.begin(), m_vecFonts.end() );
    } catch( PdfError & e ) {
        e.AddToCallstack( __FILE__, __LINE__ );
        e.PrintErrorMsg();
        PdfError::LogMessage( eLogSeverity_Error, "Cannot initialize font: %s\n", pszFontName ? pszFontName : "" );
        return NULL;
    }

    return pFont;
}

};

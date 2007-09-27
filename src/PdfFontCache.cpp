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

#ifdef _WIN32
#include <windows.h>
#endif

#include "PdfFontCache.h" 

#include "PdfFont.h"
#include "PdfFontMetrics.h"

#include <algorithm>

#include <ft2build.h>
#include FT_FREETYPE_H

#if defined(HAVE_FONTCONFIG)
#include <fontconfig/fontconfig.h>
#endif

using namespace std;

namespace PoDoFo {

class FontComperator { 
public:
    FontComperator( const char* pszFontName, bool bBold, bool bItalic )
		: m_pszFontName( pszFontName ),
		  m_bBold( bBold ), m_bItalic( bItalic )
    {
    }
    
    bool operator()(const TFontCacheElement & rhs ) 
    { 
		return ( rhs.m_sFontName == m_pszFontName && 
				((m_bBold && rhs.m_bBold) || (!m_bBold && !rhs.m_bBold)) &&
				((m_bItalic && rhs.m_bItalic) || (!m_bItalic && !rhs.m_bItalic)) );
    }

private:
	const char* m_pszFontName;
	bool        m_bBold;
	bool        m_bItalic;
};

#ifdef _WIN32
static bool GetDataFromLPFONT( const LOGFONTA* inFont, char** outFontBuffer, unsigned int& outFontBufferLen )
{
	HFONT 	hf;
	HDC		hdc;

	if ( ( hf = ::CreateFontIndirect( inFont ) ) == NULL )
		return false;

	if ( ( hdc = GetDC(0) ) == NULL ) {
		DeleteObject(hf);
		return false;
	}

	SelectObject(hdc, hf);

	outFontBufferLen = GetFontData(hdc, 0, 0, 0, 0);

	if (outFontBufferLen == GDI_ERROR) {
		ReleaseDC(0, hdc);
		DeleteObject(hf);
		return false;
	}

	*outFontBuffer = (char *) malloc( outFontBufferLen );

	if ( GetFontData( hdc, 0, 0, *outFontBuffer, (DWORD) outFontBufferLen ) == GDI_ERROR ) {
		free( *outFontBuffer );
		*outFontBuffer = NULL;
		outFontBufferLen = 0;
		ReleaseDC(0, hdc);
		DeleteObject(hf);
		return false;
	}

	ReleaseDC( 0, hdc );
	DeleteObject( hf );

	return true;
}
#endif // _WIN32

PdfFontCache::PdfFontCache( PdfVecObjects* pParent )
    : m_pParent( pParent )
{
    // Initialize all the fonts stuff

#if defined(HAVE_FONTCONFIG)
    m_pFcConfig     = FcInitLoadConfigAndFonts();
#endif

    if( FT_Init_FreeType( &m_ftLibrary ) )
    {
        PODOFO_RAISE_ERROR( ePdfError_FreeType );
    }
}

PdfFontCache::~PdfFontCache()
{
    this->EmptyCache();

#if defined(HAVE_FONTCONFIG)
    FcConfigDestroy( m_pFcConfig );
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
		delete (*itFont).m_pFont;
        ++itFont;
    }

    m_vecFonts.clear();
}

PdfFont* PdfFontCache::GetFont( const char* pszFontName, bool bBold, bool bItalic, bool bEmbedd )
{
    PdfFont*          pFont;
    PdfFontMetrics*   pMetrics;
    TCISortedFontList it;

    it = std::find_if( m_vecFonts.begin(), m_vecFonts.end(), FontComperator( pszFontName, bBold, bItalic ) );
    if( it == m_vecFonts.end() )
    {
	    std::string sPath = this->GetFontPath( pszFontName, bBold, bItalic );
	    if( sPath.empty() )
		{
#if _WIN32
			return GetWin32Font( pszFontName, bBold, bItalic, bEmbedd );
#else
		    PdfError::LogMessage( eLogSeverity_Critical, "No path was found for the specified fontname: %s\n", pszFontName );
			return NULL;
#endif // _WIN32
	    }

		pMetrics = new PdfFontMetrics( &m_ftLibrary, sPath.c_str() );
        pFont    = this->CreateFont( pMetrics, bEmbedd, bBold, bItalic, pszFontName );
    }
    else
		pFont = (*it).m_pFont;

    return pFont;
}

PdfFont* PdfFontCache::GetFont( FT_Face face, bool bEmbedd )
{
    PdfFont*          pFont;
    PdfFontMetrics*   pMetrics;
    TCISortedFontList it;
	PODOFO_RAISE_ERROR( ePdfError_Unknown );

    std::string sName = FT_Get_Postscript_Name( face );
    if( sName.empty() )
    {
        PdfError::LogMessage( eLogSeverity_Critical, "Could not retrieve fontname for font!\n" );
        return NULL;
    }

	bool bBold   = ((face->style_flags & FT_STYLE_FLAG_BOLD)   != 0);
	bool bItalic = ((face->style_flags & FT_STYLE_FLAG_ITALIC) != 0);

    it = std::find_if( m_vecFonts.begin(), m_vecFonts.end(), FontComperator( sName.c_str(), bBold, bItalic ) );
    if( it == m_vecFonts.end() )
    {
        pMetrics = new PdfFontMetrics( &m_ftLibrary, face );
        pFont    = this->CreateFont( pMetrics, bEmbedd, bBold, bItalic, sName.c_str() );
    }
    else
		pFont = (*it).m_pFont;

    return pFont;
}

#ifdef _WIN32
PdfFont* PdfFontCache::GetWin32Font( const char* pszFontName, bool bBold, bool bItalic, bool bEmbedd )
{
	LOGFONT	lf;

	lf.lfHeight			= 0;
	lf.lfWidth			= 0;
	lf.lfEscapement		= 0;
	lf.lfOrientation	= 0;
	lf.lfWeight			= bBold ? FW_BOLD : 0;
	lf.lfItalic			= bItalic;
	lf.lfUnderline		= 0;
	lf.lfStrikeOut		= 0;
	lf.lfCharSet		= DEFAULT_CHARSET;
	lf.lfOutPrecision	= OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision	= CLIP_DEFAULT_PRECIS;
	lf.lfQuality		= DEFAULT_QUALITY;
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

	if (strlen(pszFontName) >= LF_FACESIZE)
		return NULL;

	memset(&(lf.lfFaceName), 0, LF_FACESIZE);
	strcpy( (char *)lf.lfFaceName, pszFontName );

	char*        pBuffer;
	unsigned int nLen;
	if( !GetDataFromLPFONT( &lf, &pBuffer, nLen ) )
		return NULL;

	PdfFontMetrics* pMetrics;
	PdfFont*        pFont = NULL;
	try {
		pMetrics = new PdfFontMetrics( &m_ftLibrary, pBuffer, nLen );
		pFont    = this->CreateFont( pMetrics, bEmbedd, bBold, bItalic, pszFontName );
	} catch( PdfError & error ) {
		//free( pBuffer );
		throw error;
	}

	// TODO: DS: Enable free as soons as PdfFontMetrics is using PdfRefCountedBuffer!
	//free( pBuffer );
	return pFont;
}
#endif // _WIN32

std::string PdfFontCache::GetFontPath( const char* pszFontName, bool bBold, bool bItalic )
{
#if defined(HAVE_FONTCONFIG)
    std::string sPath = PdfFontMetrics::GetFilenameForFont( m_pFcConfig, pszFontName );
#else
	std::string sPath = "";
#endif
    return sPath;
}

PdfFont* PdfFontCache::CreateFont( PdfFontMetrics* pMetrics, bool bEmbedd, bool bBold, bool bItalic, const char* pszFontName ) 
{
    PdfFont* pFont;

    try {
        pFont    = new PdfFont( pMetrics, bEmbedd, bBold, bItalic, m_pParent );

		TFontCacheElement element;
		element.m_pFont     = pFont;
		element.m_bBold     = pFont->IsBold();
		element.m_bItalic   = pFont->IsItalic();
		element.m_sFontName = pszFontName;

        m_vecFonts  .push_back( element );
        
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

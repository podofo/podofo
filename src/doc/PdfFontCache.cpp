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

#include "base/PdfDefinesPrivate.h"

#include "base/PdfDictionary.h"
#include "base/PdfInputDevice.h"
#include "base/PdfOutputDevice.h"

#include "PdfDifferenceEncoding.h"
#include "PdfFont.h"
#include "PdfFontFactory.h"
#include "PdfFontMetricsFreetype.h"
#include "PdfFontMetricsBase14.h"
#include "PdfFontTTFSubset.h"
#include "PdfFontType1.h"

#include <algorithm>

#ifdef _WIN32

//#include <windows.h>
// Undefined stuff which windows does
// define that breaks are build
// e.g. GetObject is defined to either GetObjectA or GetObjectW
#ifdef GetObject
#undef GetObject
#endif // GetObject

#ifdef CreateFont
#undef CreateFont
#endif // CreateFont

#endif // _WIN32

#include <ft2build.h>
#include FT_FREETYPE_H

#if defined(PODOFO_HAVE_FONTCONFIG)
#include <fontconfig/fontconfig.h>
#include "base/util/PdfMutexWrapper.h"
#endif

using namespace std;

namespace PoDoFo {

#ifdef _WIN32
static bool GetDataFromHFONT( HFONT hf, char** outFontBuffer, unsigned int& outFontBufferLen )
{
    HDC		hdc;

    if ( ( hdc = GetDC(0) ) == NULL ) {
        DeleteObject(hf);
        return false;
    }
    
    // Petr Petrov (22 December 2009)
    HGDIOBJ oldFont = SelectObject(hdc, hf);

    outFontBufferLen = GetFontData(hdc, 0, 0, 0, 0);
    
    if (outFontBufferLen == GDI_ERROR) {
        SelectObject(hdc,oldFont);
        ReleaseDC(0, hdc);
        DeleteObject(hf);
        return false;
    }
    
    *outFontBuffer = (char *) malloc( outFontBufferLen );
    
    if ( GetFontData( hdc, 0, 0, *outFontBuffer, (DWORD) outFontBufferLen ) == GDI_ERROR ) {
        free( *outFontBuffer );
        *outFontBuffer = NULL;
        outFontBufferLen = 0;
        SelectObject(hdc,oldFont);
        ReleaseDC(0, hdc);
        DeleteObject(hf);
        return false;
    }
    
    SelectObject(hdc,oldFont);
    ReleaseDC( 0, hdc );
    DeleteObject( hf );
    
    return true;
}
    
static bool GetDataFromLPFONT( const LOGFONTA* inFont, char** outFontBuffer, unsigned int& outFontBufferLen )
{
    HFONT 	hf;

    if ( ( hf = ::CreateFontIndirectA( inFont ) ) == NULL )
        return false;

    return GetDataFromHFONT( hf, outFontBuffer, outFontBufferLen );
}

static bool GetDataFromLPFONT( const LOGFONTW* inFont, char** outFontBuffer, unsigned int& outFontBufferLen )
{
    HFONT 	hf;

    if ( ( hf = ::CreateFontIndirectW( inFont ) ) == NULL )
        return false;

    return GetDataFromHFONT( hf, outFontBuffer, outFontBufferLen );
}
#endif // _WIN32

#if defined(PODOFO_HAVE_FONTCONFIG)
Util::PdfMutex PdfFontCache::m_FcMutex;
#endif

PdfFontCache::PdfFontCache( PdfVecObjects* pParent )
    : m_pParent( pParent )
{
    // Initialize all the fonts stuff

#if defined(PODOFO_HAVE_FONTCONFIG)
    {
        Util::PdfMutexWrapper mutex(m_FcMutex);
        m_pFcConfig     = static_cast<void*>(FcInitLoadConfigAndFonts());
    }
#endif

    if( FT_Init_FreeType( &m_ftLibrary ) )
    {
        PODOFO_RAISE_ERROR( ePdfError_FreeType );
    }
}

PdfFontCache::~PdfFontCache()
{
    this->EmptyCache();

#if defined(PODOFO_HAVE_FONTCONFIG)
    {
        Util::PdfMutexWrapper mutex(m_FcMutex);
        FcConfigDestroy( static_cast<FcConfig*>(m_pFcConfig) );
    }
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

    itFont = m_vecFontSubsets.begin();
    while( itFont != m_vecFontSubsets.end() )
    {
        delete (*itFont).m_pFont;
        ++itFont;
    }

    m_vecFonts.clear();
    m_vecFontSubsets.clear();
}

PdfFont* PdfFontCache::GetFont( PdfObject* pObject )
{
    TCISortedFontList it = m_vecFonts.begin();
    const PdfReference & ref = pObject->Reference(); 

    // Search if the object is a cached normal font
    while( it != m_vecFonts.end() )
    {
        if( (*it).m_pFont->GetObject()->Reference() == ref ) 
            return (*it).m_pFont;

        ++it;
    }

    // Search if the object is a cached font subset
    it = m_vecFontSubsets.begin();
    while( it != m_vecFontSubsets.end() )
    {
        if( (*it).m_pFont->GetObject()->Reference() == ref ) 
            return (*it).m_pFont;

        ++it;
    }

    // Create a new font
    PdfFont* pFont = PdfFontFactory::CreateFont( &m_ftLibrary, pObject );
    if( pFont ) 
    {
        TFontCacheElement element;
        element.m_pFont     = pFont;
        element.m_bBold     = pFont->IsBold();
        element.m_bItalic   = pFont->IsItalic();
        element.m_sFontName = pFont->GetFontMetrics()->GetFontname();
        element.m_pEncoding = NULL;
        m_vecFonts  .push_back( element );
        
        // Now sort the font list
        std::sort( m_vecFonts.begin(), m_vecFonts.end() );
    }
    
    return pFont;
}

PdfFont* PdfFontCache::GetFont( const char* pszFontName, bool bBold, bool bItalic, 
                                bool bEmbedd, EFontCreationFlags eFontCreationFlags,
                                const PdfEncoding * const pEncoding, 
                                const char* pszFileName)
{
    PODOFO_ASSERT( pEncoding );

    PdfFont*          pFont = NULL;
    PdfFontMetrics*   pMetrics = NULL;
    std::pair<TISortedFontList,TCISortedFontList> it;

    it = std::equal_range( m_vecFonts.begin(), m_vecFonts.end(), 
			   TFontCacheElement( pszFontName, bBold, bItalic, pEncoding ) );

		
    if( it.first == it.second )
    {
		if ( (eFontCreationFlags & eFontCreationFlags_AutoSelectBase14) 
             && PODOFO_Base14FontDef_FindBuiltinData(pszFontName))
		{  
			pFont = CreateBase14Font(pszFontName,pEncoding,m_pParent);

			if( pFont ) 
			{
				TFontCacheElement element;
				element.m_pFont     = pFont;
				element.m_bBold     = pFont->IsBold();
				element.m_bItalic   = pFont->IsItalic();
				element.m_sFontName = pszFontName;
				element.m_pEncoding = pEncoding;

				// Do a sorted insert, so no need to sort again
				//rvecContainer.insert( itSorted, element ); 
				m_vecFonts.insert( it.first, element );
				
			 }

		}

		if (!pFont)
		{
			std::string sPath;
			if ( pszFileName == NULL )
				sPath = this->GetFontPath( pszFontName, bBold, bItalic );
			else
				sPath = pszFileName;
	        
			if( sPath.empty() )
			{
#ifdef _WIN32
				pFont = GetWin32Font( it.first, m_vecFonts, pszFontName, bBold, bItalic, bEmbedd, pEncoding );
#endif // _WIN32
			}
			else
			{
				bool bSubsetting = (eFontCreationFlags & eFontCreationFlags_Type1Subsetting) != 0;
				pMetrics = new PdfFontMetricsFreetype( &m_ftLibrary, sPath.c_str() );
				pFont    = this->CreateFontObject( it.first, m_vecFonts, pMetrics, 
						   bEmbedd, bBold, bItalic, pszFontName, pEncoding, bSubsetting );
			}

		}
    }
    else
        pFont = (*it.first).m_pFont;

#ifndef WIN32
		if (!pFont)
	        PdfError::LogMessage( eLogSeverity_Critical, "No path was found for the specified fontname: %s\n", pszFontName );
#endif             

    return pFont;
}

#ifdef _WIN32
PdfFont* PdfFontCache::GetFont( const wchar_t* pszFontName, bool bBold, bool bItalic, 
                                bool bEmbedd, const PdfEncoding * const pEncoding )
{
    PODOFO_ASSERT( pEncoding );

    PdfFont*          pFont;
    std::pair<TISortedFontList,TCISortedFontList> it;

    it = std::equal_range( m_vecFonts.begin(), m_vecFonts.end(), 
			   TFontCacheElement( pszFontName, bBold, bItalic, pEncoding ) );
    if( it.first == it.second )
        return GetWin32Font( it.first, m_vecFonts, pszFontName, bBold, bItalic, bEmbedd, pEncoding );
    else
        pFont = (*it.first).m_pFont;
    
    return pFont;
}
#endif // _WIN32

PdfFont* PdfFontCache::GetFont( FT_Face face, bool bEmbedd, const PdfEncoding * const pEncoding )
{
    PdfFont*          pFont;
    PdfFontMetrics*   pMetrics;
    std::pair<TISortedFontList,TCISortedFontList> it;

    std::string sName = FT_Get_Postscript_Name( face );
    if( sName.empty() )
    {
        PdfError::LogMessage( eLogSeverity_Critical, "Could not retrieve fontname for font!\n" );
        return NULL;
    }

    bool bBold   = ((face->style_flags & FT_STYLE_FLAG_BOLD)   != 0);
    bool bItalic = ((face->style_flags & FT_STYLE_FLAG_ITALIC) != 0);

    it = std::equal_range( m_vecFonts.begin(), m_vecFonts.end(), 
			   TFontCacheElement( sName.c_str(), bBold, bItalic, pEncoding ) );
    if( it.first == it.second )
    {
        pMetrics = new PdfFontMetricsFreetype( &m_ftLibrary, face );
        pFont    = this->CreateFontObject( it.first, m_vecFonts, pMetrics, 
					   bEmbedd, bBold, bItalic, sName.c_str(), pEncoding );
    }
    else
        pFont = (*it.first).m_pFont;

    return pFont;
}

PdfFont* PdfFontCache::GetDuplicateFontType1( PdfFont * pFont, const char* pszSuffix )
{
    TCISortedFontList it = m_vecFonts.begin();

	std::string id = pFont->GetIdentifier().GetName();
	id += pszSuffix;

    // Search if the object is a cached normal font
    while( it != m_vecFonts.end() )
    {
		if( (*it).m_pFont->GetIdentifier() == id ) 
            return (*it).m_pFont;

        ++it;
    }

    // Search if the object is a cached font subset
    it = m_vecFontSubsets.begin();
    while( it != m_vecFontSubsets.end() )
    {
        if( (*it).m_pFont->GetIdentifier() == id ) 
            return (*it).m_pFont;

        ++it;
    }

    // Create a copy of the font
	PODOFO_ASSERT( pFont->GetFontMetrics()->GetFontType() == ePdfFontType_Type1Pfb );
	PdfFontMetrics* pMetrics = new PdfFontMetricsFreetype( &m_ftLibrary, pFont->GetFontMetrics()->GetFilename() );
	PdfFont* newFont = new PdfFontType1( static_cast<PdfFontType1 *>(pFont), pMetrics, pszSuffix, m_pParent );
    if( newFont ) 
    {
		std::string name = newFont->GetFontMetrics()->GetFontname();
		name += pszSuffix;
        TFontCacheElement element;
        element.m_pFont     = newFont;
        element.m_bBold     = newFont->IsBold();
        element.m_bItalic   = newFont->IsItalic();
        element.m_sFontName = name;
        element.m_pEncoding = newFont->GetEncoding();
        m_vecFonts  .push_back( element );
        
        // Now sort the font list
        std::sort( m_vecFonts.begin(), m_vecFonts.end() );
    }

    return newFont;
}

PdfFont* PdfFontCache::GetFontSubset( const char* pszFontName, bool bBold, bool bItalic, 
				      const PdfEncoding * const pEncoding,
				      const char* pszFileName )
{
    PdfFont*        pFont;
    PdfFontMetrics* pMetrics;
    std::pair<TISortedFontList,TCISortedFontList> it;

    // WARNING: The characters are completely ignored right now!

    it = std::equal_range( m_vecFontSubsets.begin(), m_vecFontSubsets.end(), 
			   TFontCacheElement( pszFontName, bBold, bItalic, pEncoding ) );
    if( it.first == it.second )
    {
        std::string sPath; 
        if( pszFileName == NULL ) 
        {
            sPath = this->GetFontPath( pszFontName, bBold, bItalic );
            if( sPath.empty() )
            {
#ifdef _WIN32
                // TODO: GetWin32Font
                PODOFO_ASSERT( 0 );
#else	    
                PdfError::LogMessage( eLogSeverity_Critical, "No path was found for the specified fontname: %s\n", pszFontName );
                return NULL;
#endif // _WIN32
            }
        }
        else
            sPath = pszFileName;
        
        pMetrics = new PdfFontMetricsFreetype( &m_ftLibrary, sPath.c_str() );
        if( !(pMetrics && pMetrics->GetFontType() == ePdfFontType_TrueType ) )
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidFontFile, "Subsetting is only supported for TrueType fonts." );
        }
        
        PdfInputDevice          input( sPath.c_str() );
        PdfRefCountedBuffer     buffer;
        PdfOutputDevice         output( &buffer );
        
        PdfFontTTFSubset        subset( &input, pMetrics, PdfFontTTFSubset::eFontFileType_TTF );
        PdfEncoding::const_iterator itChar
            = pEncoding->begin();
        while( itChar != pEncoding->end() )
        {
            subset.AddCharacter( *itChar );
            ++itChar;
        }
        subset.BuildFont( &output );
        
        // Delete metrics object, as it was only used so that PdfFontTTFSubset could
        // match unicode character points to glyph indeces
        delete pMetrics;
        // TODO: Do not hardcode unique basenames...
        pMetrics = new PdfFontMetricsFreetype( &m_ftLibrary, buffer, "ABCDEF+" );
        pFont = this->CreateFontObject( it.first, m_vecFontSubsets, pMetrics, 
                                        true, bBold, bItalic, pszFontName, pEncoding );
    }
    else
        pFont = (*it.first).m_pFont;
    
    
    return pFont;
}

void PdfFontCache::EmbedSubsetFonts()
{
    TCISortedFontList it = m_vecFonts.begin();

    while( it != m_vecFonts.end() )
    {
        if( (*it).m_pFont->IsSubsetting() )
        {
            (*it).m_pFont->EmbedSubsetFont();
        }

        ++it;
    }
}

#ifdef _WIN32
PdfFont* PdfFontCache::GetWin32Font( TISortedFontList itSorted, TSortedFontList & vecContainer, 
                                     const char* pszFontName, bool bBold, bool bItalic, 
                                     bool bEmbedd, const PdfEncoding * const pEncoding )
{
    LOGFONTA lf;
    
    lf.lfHeight			= 0;
    lf.lfWidth			= 0;
    lf.lfEscapement		= 0;
    lf.lfOrientation    = 0;
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
    strcpy( lf.lfFaceName, pszFontName );

    char* pBuffer = NULL;
    unsigned int nLen;
    if( !GetDataFromLPFONT( &lf, &pBuffer, nLen ) )
        return NULL;
    
    PdfFontMetrics* pMetrics;
    PdfFont*        pFont = NULL;
    try {
        pMetrics = new PdfFontMetricsFreetype( &m_ftLibrary, pBuffer, nLen );
        pFont    = this->CreateFontObject( itSorted, vecContainer, pMetrics, 
					   bEmbedd, bBold, bItalic, pszFontName, pEncoding );
    } catch( PdfError & error ) {
        free( pBuffer );
        throw error;
    }
    
    free( pBuffer );
    return pFont;
}

PdfFont* PdfFontCache::GetWin32Font( TISortedFontList itSorted, TSortedFontList & vecContainer, 
                                     const wchar_t* pszFontName, bool bBold, bool bItalic, 
                                     bool bEmbedd, const PdfEncoding * const pEncoding )
{
    LOGFONTW	lf;
    
    lf.lfHeight			= 0;
    lf.lfWidth			= 0;
    lf.lfEscapement		= 0;
    lf.lfOrientation    = 0;
    lf.lfWeight			= bBold ? FW_BOLD : 0;
    lf.lfItalic			= bItalic;
    lf.lfUnderline		= 0;
    lf.lfStrikeOut		= 0;
    lf.lfCharSet		= DEFAULT_CHARSET;
    lf.lfOutPrecision	= OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision	= CLIP_DEFAULT_PRECIS;
    lf.lfQuality		= DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    
    pdf_long lFontNameLen = wcslen(pszFontName);
    if (lFontNameLen >= LF_FACESIZE)
        return NULL;
    
    memset(&(lf.lfFaceName), 0, LF_FACESIZE);
    wcscpy( static_cast<wchar_t*>(lf.lfFaceName), pszFontName );
    
    char*        pBuffer = NULL;
    unsigned int nLen;
    if( !GetDataFromLPFONT( &lf, &pBuffer, nLen ) )
        return NULL;
    
    pdf_long lMaxLen = lFontNameLen * 5;
    char* pmbFontName = static_cast<char*>(malloc(lMaxLen));
    if( !pmbFontName )
    {
        free( pBuffer );
        PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
    }

    if( wcstombs( pmbFontName, pszFontName, lMaxLen ) <= 0 )
    {
        free( pBuffer );
        free( pmbFontName );
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Conversion to multibyte char failed" );
    }

    PdfFontMetrics* pMetrics;
    PdfFont*        pFont = NULL;
    try {
        pMetrics = new PdfFontMetricsFreetype( &m_ftLibrary, pBuffer, nLen );
        pFont    = this->CreateFontObject( itSorted, vecContainer, pMetrics, 
					   bEmbedd, bBold, bItalic, pmbFontName, pEncoding );
        free( pmbFontName );
        pmbFontName = NULL;
    } catch( PdfError & error ) {
        free( pmbFontName );
        pmbFontName = NULL;
        free( pBuffer );
        throw error;
    }
    
    free( pBuffer );
    return pFont;
}
#endif // _WIN32

#if defined(PODOFO_HAVE_FONTCONFIG)
std::string PdfFontCache::GetFontConfigFontPath( FcConfig* pConfig, const char* pszFontName, bool bBold, bool bItalic )
{
    FcPattern*  pattern;
    FcPattern*  matched;
    FcResult    result = FcResultMatch;
    FcValue     v;
    std::string sPath;
    Util::PdfMutexWrapper mutex(m_FcMutex);

    // Build a pattern to search using fontname, bold and italic
    pattern = FcPatternBuild (0, FC_FAMILY, FcTypeString, pszFontName, 
                              FC_WEIGHT, FcTypeInteger, (bBold ? FC_WEIGHT_BOLD : FC_WEIGHT_MEDIUM),
                              FC_SLANT, FcTypeInteger, (bItalic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN),  
                              static_cast<char*>(0));

    FcDefaultSubstitute( pattern );

    if( !FcConfigSubstitute( pConfig, pattern, FcMatchFont ) )
    {
        FcPatternDestroy( pattern );
        return sPath;
    }

    matched = FcFontMatch( pConfig, pattern, &result );
    if( result != FcResultNoMatch )
    {
        result = FcPatternGet( matched, FC_FILE, 0, &v );
        sPath = reinterpret_cast<const char*>(v.u.s);
#ifdef PODOFO_VERBOSE_DEBUG
        PdfError::LogMessage( eLogSeverity_Debug,
                              "Got Font %s for for %s\n", sPath.c_str(), pszFontName );
#endif // PODOFO_DEBUG
    }

    FcPatternDestroy( pattern );
    FcPatternDestroy( matched );
    return sPath;
}

#endif // PODOFO_HAVE_FONTCONFIG

std::string PdfFontCache::GetFontPath( const char* pszFontName, bool bBold, bool bItalic )
{
#if defined(PODOFO_HAVE_FONTCONFIG)
    Util::PdfMutexWrapper mutex(m_FcMutex);
    FcConfig*   pConfig = FcInitLoadConfigAndFonts();
    std::string sPath   = this->GetFontConfigFontPath( pConfig, pszFontName, bBold, bItalic );
    FcConfigDestroy( pConfig );    
#else
    std::string sPath = "";
#endif
    return sPath;
}

PdfFont* PdfFontCache::CreateFontObject( TISortedFontList itSorted, TSortedFontList & rvecContainer, 
					 PdfFontMetrics* pMetrics, bool bEmbedd, bool bBold, bool bItalic, 
					 const char* pszFontName, const PdfEncoding * const pEncoding, bool bSubsetting ) 
{
    PdfFont* pFont;

    try {
        int nFlags = ePdfFont_Normal;

		if ( bSubsetting )
			nFlags |= ePdfFont_Subsetting;

		if( bEmbedd )
            nFlags |= ePdfFont_Embedded;

        if( bBold ) 
            nFlags |= ePdfFont_Bold;

        if( bItalic )
            nFlags |= ePdfFont_Italic;

        pFont    = PdfFontFactory::CreateFontObject( pMetrics, nFlags, pEncoding, m_pParent );

        if( pFont ) 
        {
            TFontCacheElement element;
            element.m_pFont     = pFont;
            element.m_bBold     = pFont->IsBold();
            element.m_bItalic   = pFont->IsItalic();
            element.m_sFontName = pszFontName;
            element.m_pEncoding = pEncoding;

            // Do a sorted insert, so no need to sort again
            rvecContainer.insert( itSorted, element );
        }
    } catch( PdfError & e ) {
        e.AddToCallstack( __FILE__, __LINE__ );
        e.PrintErrorMsg();
        PdfError::LogMessage( eLogSeverity_Error, "Cannot initialize font: %s\n", pszFontName ? pszFontName : "" );
        return NULL;
    }

    return pFont;
}

};

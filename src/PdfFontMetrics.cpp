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

#ifdef _WIN32
#include <windows.h>
#endif

#include "PdfFontMetrics.h"

#include "PdfVariant.h"
#include <sstream>


#define FIRST_READABLE 31

namespace PoDoFo {

#ifdef _WIN32
	static bool GetWin32HostFont( const std::string& inFontName, char** outFontBuffer, unsigned int& outFontBufferLen );
#endif


PdfFontMetrics::PdfFontMetrics( FT_Library* pLibrary, const char* pszFilename )
    : m_pLibrary( pLibrary ), m_sFilename( pszFilename ), m_pFontData( NULL	), m_nFontDataLen( 0 )
{
    m_face                = NULL;

    // TODO: Handle errors here
    FT_Error err = FT_New_Face( *pLibrary, pszFilename, 0, &m_face );
#ifdef _WIN32
    if ( err )
    {	// try to load from the OS by name
        char*	     fontBuf = NULL;
        unsigned int fontBufLen = 0;
        if ( GetWin32HostFont( m_sFilename, &fontBuf, fontBufLen ) ) 
        {
            err = FT_New_Memory_Face( *pLibrary, (unsigned char*)fontBuf, fontBufLen, 0, &m_face );
            m_pFontData = fontBuf;
            m_nFontDataLen = fontBufLen;
        }
    }
#endif

    InitFromFace();
}

PdfFontMetrics::PdfFontMetrics( FT_Library* pLibrary, const char* pBuffer, unsigned int nBufLen )
	: m_pLibrary( pLibrary ), m_sFilename( "" ), m_pFontData( const_cast<char*>(pBuffer) ), m_nFontDataLen( nBufLen )
{
    m_face                = NULL;

    // TODO: handle errors here
    FT_Error	error = FT_New_Memory_Face( *pLibrary, (unsigned char*)pBuffer, nBufLen, 0, &m_face );
    
    InitFromFace();
}

/*
PdfFontMetrics::PdfFontMetrics( FT_Library* pLibrary, const char* pBuffer, unsigned int nBufLen )
	: m_pLibrary( pLibrary ), m_sFilename( "" ), m_pFontData( const_cast<char*>(pBuffer) ), m_nFontDataLen( nBufLen )
{
	m_face                = NULL;

	// TODO: handle errors here
	FT_Error	error = FT_New_Memory_Face( *pLibrary, (unsigned char*)pBuffer, nBufLen, 0, &m_face );

	InitFromFace();
}
*/

PdfFontMetrics::~PdfFontMetrics()
{
    if ( m_face )
        FT_Done_Face( m_face );
    
    if ( m_pFontData && m_nFontDataLen ) 
    {
        free( m_pFontData );
        m_nFontDataLen = 0;
    }
}

void PdfFontMetrics::InitFromFace()
{
    m_nWeight             = 500;
    m_nItalicAngle        = 0;
    m_lLineSpacing        = 0;
    m_lUnderlineThickness = 0;
    m_lUnderlinePosition  = 0;
    
    if ( m_face )
    {	// better be, but just in case...
        m_dAscent  = m_face->ascender  * 1000.0 / m_face->units_per_EM;
        m_dDescent = m_face->descender * 1000.0 / m_face->units_per_EM;
    }
}

const char* PdfFontMetrics::Fontname() const
{
    const char*	s = FT_Get_Postscript_Name( m_face );
    return s ? s : "";
}

/*
const std::string PdfFontMetrics::Fontname() const
{
	const char*	s = FT_Get_Postscript_Name( m_face );
	return  s ? std::string( s ) : std::string( "" );
//    return FT_Get_Postscript_Name( m_face );
}
*/

PdfError PdfFontMetrics::GetWidthArray( PdfVariant & var, unsigned int nFirst, unsigned int nLast ) const
{
    PdfError      eCode;
    unsigned int  i;
 //   FT_UInt       uiCharCode;
    TVariantList  list;

    if( !m_face ) 
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    var.SetDataType( ePdfDataType_Real );

    for( i=nFirst;i<=nLast;i++ )
    {
        if( i < FIRST_READABLE )
        {
            var.SetNumber( 0.0 );
            list.push_back( var );
        }
        else
        {
            if( !FT_Load_Char( m_face, i, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP ) )  // | FT_LOAD_NO_RENDER
            {
                //RAISE_ERROR( ePdfError_FreeType );
            }

            var.SetNumber( m_face->glyph->metrics.horiAdvance * 1000.0 / m_face->units_per_EM );
            list.push_back( var );
        }
    }

    var.SetDataType( ePdfDataType_Array );
    var.SetArray   ( list );

    return eCode;
}

PdfError PdfFontMetrics::GetBoundingBox( std::string & rsBounds ) const
{
    PdfError      eCode;
    std::ostringstream out;

    if( !m_face ) 
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    out << "[ " << m_face->bbox.xMin * 1000.0 / m_face->units_per_EM << " " << m_face->bbox.yMin  * 1000.0 / m_face->units_per_EM << " "
        << m_face->bbox.xMax  * 1000.0 / m_face->units_per_EM << " " << m_face->bbox.yMax  * 1000.0 / m_face->units_per_EM << " ]";

    rsBounds = out.str();

    return eCode;
}

std::string PdfFontMetrics::GetFilenameForFont( const char* pszFontname )
{
#ifdef _WIN32
	return std::string( pszFontname );	// return the name...
#else
    FcConfig* pConfig = FcInitLoadConfigAndFonts();
    std::string sPath = PdfFontMetrics::GetFilenameForFont( pConfig, pszFontname );

    FcConfigDestroy( pConfig );    
// TODO: Supported only by newer fontconfig versions 
//       but fixes a memory leak
//    FcFini(void);

    return sPath;
#endif
}

#ifdef _WIN32
static bool GetDataFromLPFONT( LOGFONT* inFont, char** outFontBuffer, unsigned int& outFontBufferLen )
{
	HFONT 	hf;
	HDC		hdc;

	if ( ( hf = CreateFontIndirect( inFont ) ) == NULL )
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

static bool GetWin32HostFont( const std::string& inFontName, char** outFontBuffer, unsigned int& outFontBufferLen )
{
	LOGFONT	lf;

	std::string	localFName( inFontName );
	bool	isBold = false,
		isItalic = false;

	// deal with BOLD and ITALIC versions of TimesNewRomanPS
	if ( inFontName.find( "TimesNewRomanPS" ) != std::string::npos )	 {
		isBold = ( inFontName.find( "Bold" ) != std::string::npos );
		isItalic = ( inFontName.find( "Italic" ) != std::string::npos );
		localFName = "Times New Roman";
	}

	lf.lfHeight			= 0;
	lf.lfWidth			= 0;
	lf.lfEscapement		= 0;
	lf.lfOrientation	= 0;
	lf.lfWeight			= isBold ? FW_BOLD : 0;
	lf.lfItalic			= isItalic;
	lf.lfUnderline		= 0;
	lf.lfStrikeOut		= 0;
	lf.lfCharSet		= DEFAULT_CHARSET;
	lf.lfOutPrecision	= OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision	= CLIP_DEFAULT_PRECIS;
	lf.lfQuality		= DEFAULT_QUALITY;
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

	if ( localFName.length() >= LF_FACESIZE) {
		return false;
	}

	memset(&(lf.lfFaceName), 0, LF_FACESIZE);
	strcpy( (char *)lf.lfFaceName, localFName.c_str() );

	return GetDataFromLPFONT( &lf, outFontBuffer, outFontBufferLen );
}

#else

std::string PdfFontMetrics::GetFilenameForFont( FcConfig* pConfig, const char* pszFontname )
{
    FcPattern* pattern;
    FcPattern* matched;
    FcResult result = FcResultMatch;
    FcValue v;
    std::string sPath;

    pattern = FcPatternBuild (0, FC_FAMILY, FcTypeString, pszFontname, (char *) 0);
    FcDefaultSubstitute( pattern );

    if( !FcConfigSubstitute( pConfig, pattern, FcMatchFont ) )
    {
        FcPatternDestroy( pattern );
        return NULL;
    }

    matched = FcFontMatch( pConfig, pattern, &result );
    if( result != FcResultNoMatch )
    {
        result = FcPatternGet( matched, FC_FILE, 0, &v );
        sPath = (char*)(v.u.s);
    }
    else
    {
        FcPatternDestroy( pattern );
        FcPatternDestroy( matched );
        return NULL;
    }

    FcPatternDestroy( pattern );
    FcPatternDestroy( matched );
    return sPath;
}
#endif

unsigned long PdfFontMetrics::CharWidth( char c ) const
{
    FT_Error       ftErr;
    unsigned long lWidth = 0;

    ftErr = FT_Load_Char( m_face, (FT_UInt)c, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING );
    if( ftErr )
        return lWidth;

    lWidth = m_face->glyph->advance.x;

    return (unsigned long)((double)lWidth/64.0/CONVERSION_CONSTANT);
}

unsigned long PdfFontMetrics::StringWidth( const char* pszText, unsigned int nLength ) const
{
    unsigned long lWidth = 0;

    if( !pszText )
        return lWidth;

    if( !nLength )
        nLength = (unsigned int)strlen( pszText );

    while( nLength-- && *pszText )
    {
        lWidth += CharWidth( *pszText );

        ++pszText;
    }
    
    return lWidth;
}

void PdfFontMetrics::SetFontSize( float fSize )
{
    FT_Error ftErr;

    ftErr = FT_Set_Char_Size( m_face, (int)(fSize*64.0), 0, 72, 72 );

    // calculate the line spacing now, as it changes only with the font size
    m_lLineSpacing        = (unsigned long)((double)(m_face->ascender + abs(m_face->descender)) * fSize / m_face->units_per_EM / CONVERSION_CONSTANT);

    m_lUnderlineThickness = (unsigned long)((double)m_face->underline_thickness * fSize  / m_face->units_per_EM / CONVERSION_CONSTANT);
    m_lUnderlinePosition  = (long)((double)m_face->underline_position * fSize  / m_face->units_per_EM / CONVERSION_CONSTANT);
}

};

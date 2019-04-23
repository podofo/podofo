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
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#include "PdfFontMetricsFreetype.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfVariant.h"

#include "PdfFontFactory.h"

#include <iostream>
#include <sstream>

#include <wchar.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H

#define PODOFO_FIRST_READABLE 31
#define PODOFO_WIDTH_CACHE_SIZE 256

namespace PoDoFo {

#if defined(__APPLE_CC__) && !defined(PODOFO_HAVE_FONTCONFIG)
#include <Carbon/Carbon.h>
#endif

struct Scoped_FT_Face {
    Scoped_FT_Face() : ftFace(0) {
    }
    ~Scoped_FT_Face() {
        if (ftFace) {
            FT_Done_Face(ftFace);
        }
    }
    FT_Face ftFace;
};

PdfFontMetricsFreetype* PdfFontMetricsFreetype::CreateForSubsetting(FT_Library* pLibrary, const char* pszFilename, bool pIsSymbol, const char* pszSubsetPrefix ) 
{
    Scoped_FT_Face scoped_face;

    FT_Error err = FT_New_Face( *pLibrary, pszFilename, 0, &scoped_face.ftFace );
    if (!err) {
        FT_ULong  length = 0;
        err = FT_Load_Sfnt_Table( scoped_face.ftFace, 0, 0, NULL, &length );
        if (!err) {
            PdfRefCountedBuffer buffer(length);
            err = FT_Load_Sfnt_Table( scoped_face.ftFace, 0, 0, reinterpret_cast<FT_Byte*>(buffer.GetBuffer()), &length );
            if (!err) {
                return new PdfFontMetricsFreetype( pLibrary, buffer, pIsSymbol, pszSubsetPrefix );
            }
        }
        // throw an exception
        PdfError::LogMessage( eLogSeverity_Critical, "FreeType returned the error %i when calling FT_Load_Sfnt_Table for font %s.", 
                              err, pszFilename );
        PODOFO_RAISE_ERROR( ePdfError_FreeType );
    }
    else {
        // throw an exception
        PdfError::LogMessage( eLogSeverity_Critical, "FreeType returned the error %i when calling FT_New_Face for font %s.", 
                              err, pszFilename );
        PODOFO_RAISE_ERROR( ePdfError_FreeType );
    }
    return 0;
}

PdfFontMetricsFreetype::PdfFontMetricsFreetype( FT_Library* pLibrary, const char* pszFilename, 
                                                bool pIsSymbol, const char* pszSubsetPrefix )
    : PdfFontMetrics( PdfFontMetrics::FontTypeFromFilename( pszFilename ),
                      pszFilename, pszSubsetPrefix ),
      m_pLibrary( pLibrary ),
      m_pFace( NULL ),
      m_bSymbol( pIsSymbol )
{
    FT_Error err = FT_New_Face( *pLibrary, pszFilename, 0, &m_pFace );
    if ( err )
    {	
        // throw an exception
        PdfError::LogMessage( eLogSeverity_Critical, "FreeType returned the error %i when calling FT_New_Face for font %s.", 
                              err, pszFilename );
        PODOFO_RAISE_ERROR( ePdfError_FreeType );
    }
    
    InitFromFace(pIsSymbol);
}

PdfFontMetricsFreetype::PdfFontMetricsFreetype( FT_Library* pLibrary, 
                                                const char* pBuffer, unsigned int nBufLen,
                                                                bool pIsSymbol,
                                                const char* pszSubsetPrefix )
    : PdfFontMetrics( ePdfFontType_Unknown, "", pszSubsetPrefix ),
      m_pLibrary( pLibrary ),
      m_pFace( NULL ),
      m_bSymbol( pIsSymbol )
{
    m_bufFontData = PdfRefCountedBuffer( nBufLen ); // const_cast is ok, because we SetTakePossension to false!
    memcpy( m_bufFontData.GetBuffer(), pBuffer, nBufLen );

    InitFromBuffer(pIsSymbol);
}

PdfFontMetricsFreetype::PdfFontMetricsFreetype( FT_Library* pLibrary, 
                                                const PdfRefCountedBuffer & rBuffer,
                                                                bool pIsSymbol,
                                                const char* pszSubsetPrefix ) 
    : PdfFontMetrics( ePdfFontType_Unknown, "", pszSubsetPrefix ),
      m_pLibrary( pLibrary ),
      m_pFace( NULL ),
      m_bSymbol( pIsSymbol ),
      m_bufFontData( rBuffer )
{
    InitFromBuffer(pIsSymbol);
}

PdfFontMetricsFreetype::PdfFontMetricsFreetype( FT_Library* pLibrary, 
                                                FT_Face face, 
                                                                bool pIsSymbol,
                                                const char* pszSubsetPrefix  )
    : PdfFontMetrics( ePdfFontType_TrueType, 
                      // Try to initialize the pathname from m_face
                      // so that font embedding will work
                      (face->stream ? 
                       reinterpret_cast<char*>(face->stream->pathname.pointer) : ""),
                      pszSubsetPrefix ),
      m_pLibrary( pLibrary ),
      m_pFace( face ),
      m_bSymbol( pIsSymbol )
{
    // asume true type
    // m_eFontType = ePdfFontType_TrueType;

    InitFromFace(pIsSymbol);
}

PdfFontMetricsFreetype::~PdfFontMetricsFreetype()
{
    if ( m_pFace )
    {
        FT_Done_Face( m_pFace );
    }
}

void PdfFontMetricsFreetype::InitFromBuffer(bool pIsSymbol)
{
    FT_Open_Args openArgs;
    memset(&openArgs, 0, sizeof(openArgs));
    openArgs.flags = FT_OPEN_MEMORY;
    openArgs.memory_base = reinterpret_cast<FT_Byte*>(m_bufFontData.GetBuffer()), 
    openArgs.memory_size = static_cast<FT_Long>(m_bufFontData.GetSize());
    FT_Error error = FT_Open_Face( *m_pLibrary, &openArgs, 0, &m_pFace ); 
    if( error ) 
    {
        PdfError::LogMessage( eLogSeverity_Critical, "FreeType returned the error %i when calling FT_New_Face for a buffered font.", error );
        PODOFO_RAISE_ERROR( ePdfError_FreeType );
    }
    else
    {
        // asume true type
        this->SetFontType( ePdfFontType_TrueType );
    }

    InitFromFace(pIsSymbol);
}

void PdfFontMetricsFreetype::InitFromFace(bool pIsSymbol)
{
    if ( m_eFontType == ePdfFontType_Unknown ) {
        // We need to have identified the font type by this point
        // Unsupported font.
        PODOFO_RAISE_ERROR_INFO( ePdfError_UnsupportedFontFormat, m_sFilename.c_str() );
    }

    m_nWeight             = 500;
    m_nItalicAngle        = 0;
    m_dLineSpacing        = 0.0;
    m_dUnderlineThickness = 0.0;
    m_dUnderlinePosition  = 0.0;
    m_dStrikeOutPosition  = 0.0;
    m_dStrikeOutThickness = 0.0;
    m_fFontSize           = 0.0f;
     m_bSymbol = pIsSymbol;
    m_bIsBold = false;
    m_bIsItalic = false;

    if ( m_pFace )
    {	// better be, but just in case...
        m_dPdfAscent  = m_pFace->ascender  * 1000.0 / m_pFace->units_per_EM;
        m_dPdfDescent = m_pFace->descender * 1000.0 / m_pFace->units_per_EM;
        m_bIsBold = (m_pFace->style_flags & FT_STYLE_FLAG_BOLD) != 0;
        m_bIsItalic = (m_pFace->style_flags & FT_STYLE_FLAG_ITALIC) != 0;

        // Try to get a unicode charmap
        FT_Select_Charmap( m_pFace, pIsSymbol ? FT_ENCODING_MS_SYMBOL : FT_ENCODING_UNICODE );

        // Try to determine if it is a symbol font
        for( int c=0; c<m_pFace->num_charmaps; c++ ) 
        {  
            FT_CharMap charmap = m_pFace->charmaps[c]; 

            if( charmap->encoding == FT_ENCODING_MS_SYMBOL ) 
            {
                m_bSymbol = true;
                FT_Set_Charmap( m_pFace, charmap );
                break;
            }
            // TODO: Also check for FT_ENCODING_ADOBE_CUSTOM and set it?
        }
    
        // we cache the 256 first width entries as they
        // are most likely needed quite often
        m_vecWidth.clear();
        m_vecWidth.reserve( PODOFO_WIDTH_CACHE_SIZE );
        for( unsigned int i=0; i < PODOFO_WIDTH_CACHE_SIZE; i++ )
        {
            if( i < PODOFO_FIRST_READABLE || !m_pFace )
                m_vecWidth.push_back( 0.0  );
            else
            {
                int index = i;
                // Handle symbol fonts
                if( m_bSymbol ) 
                {
                    index = index | 0xf000;
                }

                if( FT_Load_Char( m_pFace, index, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP) == 0 )  // | FT_LOAD_NO_RENDER
                {
                    m_vecWidth.push_back( static_cast<double>(m_pFace->glyph->metrics.horiAdvance) * 1000.0 / m_pFace->units_per_EM );
                    continue;
                }
                
                m_vecWidth.push_back( 0.0  );
            }
        }
    }

    InitFontSizes();
}

void PdfFontMetricsFreetype::InitFontSizes()
{
    if( !m_pFace )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle, "Cannot set font size on invalid font!" );
    }
 
    float fSize = 1.0f;
    // TODO: Maybe we have to set this for charwidth!!!
    FT_Set_Char_Size( m_pFace, static_cast<int>(fSize*64.0), 0, 72, 72 );

    // calculate the line spacing now, as it changes only with the font size
    m_dLineSpacing        = (static_cast<double>(m_pFace->height) / m_pFace->units_per_EM);
    m_dUnderlineThickness = (static_cast<double>(m_pFace->underline_thickness) / m_pFace->units_per_EM);
    m_dUnderlinePosition  = (static_cast<double>(m_pFace->underline_position)  / m_pFace->units_per_EM);
    m_dAscent  = static_cast<double>(m_pFace->ascender) / m_pFace->units_per_EM;
    m_dDescent = static_cast<double>(m_pFace->descender) / m_pFace->units_per_EM;
    // Set default values for strikeout, in case the font has no direct values
    m_dStrikeOutPosition  = m_dAscent / 2.0; 
    m_dStrikeOutThickness = m_dUnderlineThickness;

    TT_OS2* pOs2Table = static_cast<TT_OS2*>(FT_Get_Sfnt_Table( m_pFace, ft_sfnt_os2 ));
    if( pOs2Table ) 
    {
        m_dStrikeOutPosition  = static_cast<double>(pOs2Table->yStrikeoutPosition) / m_pFace->units_per_EM;
        m_dStrikeOutThickness = static_cast<double>(pOs2Table->yStrikeoutSize) / m_pFace->units_per_EM;
    }
}

const char* PdfFontMetricsFreetype::GetFontname() const
{
    const char*	s = FT_Get_Postscript_Name( m_pFace );
    return s ? s : "";
}

void PdfFontMetricsFreetype::GetWidthArray( PdfVariant & var, unsigned int nFirst, unsigned int nLast, const PdfEncoding* pEncoding ) const
{
    unsigned int  i;
    PdfArray  list;

    if( !m_pFace ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    for( i=nFirst;i<=nLast;i++ )
    {
        if( i < PODOFO_WIDTH_CACHE_SIZE && pEncoding == NULL )
            list.push_back( PdfVariant( m_vecWidth[i] ) );
        else
        {
            if (pEncoding != NULL)
            {
                unsigned short shCode = pEncoding->GetCharCode(i);
#ifdef PODOFO_IS_LITTLE_ENDIAN
                shCode = ((shCode & 0x00FF) << 8) | ((shCode & 0xFF00) >> 8);
#endif
                list.push_back( PdfVariant( (pdf_int64)this->GetGlyphWidth(this->GetGlyphId(shCode)) ) );
                continue;
            }
            else if( FT_Load_Char( m_pFace, i, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP ) == 0 )  // | FT_LOAD_NO_RENDER
            {
                // zero return code is success!
                list.push_back( PdfVariant( m_pFace->glyph->metrics.horiAdvance * 1000.0 / m_pFace->units_per_EM ) );
                continue;
            }
            //PODOFO_RAISE_ERROR( ePdfError_FreeType );
            list.push_back( PdfVariant( 0.0 ) );
        }
    }

    var = PdfVariant( list );
}

double PdfFontMetricsFreetype::GetGlyphWidth( int nGlyphId ) const
{
    if( !m_pFace ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( !FT_Load_Glyph( m_pFace, nGlyphId, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP ) )  // | FT_LOAD_NO_RENDER
    {
        // zero return code is success!
        return m_pFace->glyph->metrics.horiAdvance * 1000.0 / m_pFace->units_per_EM;
    }

    return 0.0;
}

double PdfFontMetricsFreetype::GetGlyphWidth( const char* pszGlyphname ) const
{
    return GetGlyphWidth( FT_Get_Name_Index( m_pFace, const_cast<char *>(pszGlyphname) ) );
}

void PdfFontMetricsFreetype::GetBoundingBox( PdfArray & array ) const
{
    if( !m_pFace ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    array.Clear();
    array.push_back( PdfVariant( m_pFace->bbox.xMin * 1000.0 / m_pFace->units_per_EM ) );
    array.push_back( PdfVariant( m_pFace->bbox.yMin  * 1000.0 / m_pFace->units_per_EM ) );
    array.push_back( PdfVariant( m_pFace->bbox.xMax  * 1000.0 / m_pFace->units_per_EM ) );
    array.push_back( PdfVariant( m_pFace->bbox.yMax  * 1000.0 / m_pFace->units_per_EM ) );
}

double PdfFontMetricsFreetype::CharWidth( unsigned char c ) const
{
    double dWidth = m_vecWidth[static_cast<unsigned int>(c)];

    return dWidth * static_cast<double>(this->GetFontSize() * this->GetFontScale() / 100.0) / 1000.0 +
        static_cast<double>( this->GetFontSize() * this->GetFontScale() / 100.0 * this->GetFontCharSpace() / 100.0);
}

double PdfFontMetricsFreetype::UnicodeCharWidth( unsigned short c ) const
{
    FT_Error ftErr;
    double   dWidth = 0.0;

    if( static_cast<int>(c) < PODOFO_WIDTH_CACHE_SIZE ) 
    {
        dWidth = m_vecWidth[static_cast<unsigned int>(c)];
    }
    else
    {
        ftErr = FT_Load_Char( m_pFace, static_cast<FT_UInt>(c), FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP );
        if( ftErr )
            return dWidth;

        dWidth = m_pFace->glyph->metrics.horiAdvance * 1000.0 / m_pFace->units_per_EM;
    }

    return dWidth * static_cast<double>(this->GetFontSize() * this->GetFontScale() / 100.0) / 1000.0 +
        static_cast<double>( this->GetFontSize() * this->GetFontScale() / 100.0 * this->GetFontCharSpace() / 100.0);
}

long PdfFontMetricsFreetype::GetGlyphId( long lUnicode ) const
{
    long lGlyph = 0L;

    // Handle symbol fonts!
    if( m_bSymbol ) 
    {
        lUnicode = lUnicode | 0xf000;
    }
    lGlyph = FT_Get_Char_Index( m_pFace, lUnicode );

    return lGlyph;
}

bool PdfFontMetricsFreetype::IsBold(void) const
{
    return m_bIsBold;
}

bool PdfFontMetricsFreetype::IsItalic(void) const
{
    return m_bIsItalic;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsFreetype::GetLineSpacing() const
{
    return m_dLineSpacing * this->GetFontSize();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsFreetype::GetUnderlinePosition() const
{
    return m_dUnderlinePosition * this->GetFontSize();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsFreetype::GetStrikeOutPosition() const
{
    return m_dStrikeOutPosition * this->GetFontSize();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsFreetype::GetUnderlineThickness() const
{
    return m_dUnderlineThickness * this->GetFontSize();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsFreetype::GetStrikeoutThickness() const
{
    return m_dStrikeOutThickness * this->GetFontSize();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const char* PdfFontMetricsFreetype::GetFontData() const
{
    return m_bufFontData.GetBuffer();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
pdf_long PdfFontMetricsFreetype::GetFontDataLen() const
{
    return m_bufFontData.GetSize();
}  

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned int PdfFontMetricsFreetype::GetWeight() const
{
    return m_nWeight;
}  

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsFreetype::GetAscent() const
{
    return m_dAscent * this->GetFontSize();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsFreetype::GetPdfAscent() const
{
    return m_dPdfAscent;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsFreetype::GetDescent() const
{
    return m_dDescent * this->GetFontSize();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsFreetype::GetPdfDescent() const
{
    return m_dPdfDescent;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
int PdfFontMetricsFreetype::GetItalicAngle() const
{
    return m_nItalicAngle;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfFontMetricsFreetype::IsSymbol() const
{
    return m_bSymbol;
}

};

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

#include "PdfFontMetrics.h"

#include "PdfVariant.h"
#include <sstream>

#define FIRST_READABLE 31

namespace PoDoFo {

PdfFontMetrics::PdfFontMetrics( FT_Library* pLibrary, const char* pszFilename )
    : m_pLibrary( pLibrary ), m_sFilename( pszFilename )
{
    m_nWeight             = 500;
    m_nItalicAngle        = 0;
    m_lLineSpacing        = 0;
    m_lUnderlineThickness = 0;
    m_lUnderlinePosition  = 0;
    m_face                = NULL;

    // TODO: Handle errors here
    FT_New_Face( *pLibrary, pszFilename, 0, &m_face );
    
    m_dAscent  = m_face->ascender  * 1000.0 / m_face->units_per_EM;
    m_dDescent = m_face->descender * 1000.0 / m_face->units_per_EM;
}

PdfFontMetrics::~PdfFontMetrics()
{
    FT_Done_Face( m_face );
}

const char* PdfFontMetrics::Fontname() const
{
    return FT_Get_Postscript_Name( m_face );
}

PdfError PdfFontMetrics::GetWidthArray( PdfVariant & var, unsigned int nFirst, unsigned int nLast ) const
{
    PdfError      eCode;
    unsigned int  i;
    FT_UInt       uiCharCode;
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
    FcConfig* pConfig = FcInitLoadConfigAndFonts();
    std::string sPath = PdfFontMetrics::GetFilenameForFont( pConfig, pszFontname );

    FcConfigDestroy( pConfig );    
// TODO: Supported only by newer fontconfig versions 
//       but fixes a memory leak
//    FcFini(void);

    return sPath;
}

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
        nLength = strlen( pszText );

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

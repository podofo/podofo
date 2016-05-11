/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
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

#include "PdfFontMetricsBase14.h"

#include "base/PdfDefinesPrivate.h"
#include "base/PdfArray.h"

#include "PdfFontFactoryBase14Data.h"

namespace PoDoFo {


PdfFontMetricsBase14::PdfFontMetricsBase14(const char      *mfont_name,
                                           const PODOFO_CharData  *mwidths_table,
                                           bool             mis_font_specific,
                                           pdf_int16            mascent,
                                           pdf_int16            mdescent,
                                           pdf_uint16           mx_height,
                                           pdf_uint16           mcap_height,
                                           const PdfRect &      mbbox)
    : PdfFontMetrics( ePdfFontType_Type1Base14, "", NULL),
      font_name(mfont_name),
      widths_table(mwidths_table),
      is_font_specific(mis_font_specific),
      ascent(mascent), 
      descent(mdescent),
      x_height(mx_height), 
      cap_height(mcap_height), 
      bbox(mbbox), 
      m_bSymbol(is_font_specific)
{			
    m_nWeight             = 500;
    m_nItalicAngle        = 0;
    m_dLineSpacing        = 0.0;
    m_dUnderlineThickness = 0.0;
    m_dUnderlinePosition  = 0.0;
    m_dStrikeOutPosition  = 0.0;
    m_dStrikeOutThickness = 0.0;
    units_per_EM          = 1000;
    m_dPdfAscent          = ascent * 1000 / units_per_EM;
    m_dPdfDescent         = descent * 1000 / units_per_EM;
    
    m_dAscent             = ascent;
    m_dDescent            = descent;	     

    // calculate the line spacing now, as it changes only with the font size
    m_dLineSpacing        = (static_cast<double>(ascent + abs(descent)) / units_per_EM);
    m_dAscent             = static_cast<double>(ascent) /  units_per_EM;
    m_dDescent            = static_cast<double>(descent) /  units_per_EM;
    
    // Set default values for strikeout, in case the font has no direct values
    m_dStrikeOutPosition  = m_dAscent / 2.0; 
	//	m_dStrikeOutThickness = m_dUnderlineThickness;
}

PdfFontMetricsBase14::~PdfFontMetricsBase14()
{
}

double PdfFontMetricsBase14::GetGlyphWidth( int nGlyphId ) const 
{
    return widths_table[static_cast<unsigned int>(nGlyphId)].width; 
}

double PdfFontMetricsBase14::GetGlyphWidth( const char* ) const 
{
    return 0.0;
}

double PdfFontMetricsBase14::CharWidth( unsigned char c ) const 
{
    double dWidth = widths_table[static_cast<unsigned int>(GetGlyphId(c) )].width;
    
    return dWidth * static_cast<double>(this->GetFontSize() * this->GetFontScale() / 100.0) / 1000.0 +
        static_cast<double>( this->GetFontSize() * this->GetFontScale() / 100.0 * this->GetFontCharSpace() / 100.0);
}

double PdfFontMetricsBase14::UnicodeCharWidth( unsigned short c ) const 
{
    double   dWidth = 0.0;
    
    dWidth = widths_table[static_cast<unsigned int>(GetGlyphIdUnicode(c) )].width;
	
    return dWidth * static_cast<double>(this->GetFontSize() * this->GetFontScale() / 100.0) / 1000.0 +
        static_cast<double>( this->GetFontSize() * this->GetFontScale() / 100.0 * this->GetFontCharSpace() / 100.0);
}

inline double PdfFontMetricsBase14::GetLineSpacing() const 
{
    return m_dLineSpacing * this->GetFontSize();
}

inline double PdfFontMetricsBase14::GetUnderlineThickness() const 
{
    return m_dUnderlineThickness * this->GetFontSize();
}

inline double PdfFontMetricsBase14::GetUnderlinePosition() const 
{
    return m_dUnderlinePosition * this->GetFontSize();
}

inline double PdfFontMetricsBase14::GetStrikeOutPosition() const 
{
    return m_dStrikeOutPosition * this->GetFontSize();
}

inline double PdfFontMetricsBase14::GetStrikeoutThickness() const 
{
    return m_dStrikeOutThickness * this->GetFontSize();
}

const char* PdfFontMetricsBase14::GetFontname() const 
{
#ifdef MYASSERT
    assert(font_name != NULL);
#endif
    return font_name;
}

unsigned int PdfFontMetricsBase14::GetWeight() const 
{
    return m_nWeight;
}

double PdfFontMetricsBase14::GetAscent() const 
{
    return m_dAscent * this->GetFontSize();
}

double PdfFontMetricsBase14::GetPdfAscent() const 
{
    return m_dPdfAscent;
}

double PdfFontMetricsBase14::GetDescent() const 
{
    return m_dDescent * this->GetFontSize();
}

double PdfFontMetricsBase14::GetPdfDescent() const 
{
    return m_dPdfDescent;
}

int PdfFontMetricsBase14::GetItalicAngle() const 
{
    return m_nItalicAngle;
}

long PdfFontMetricsBase14::GetGlyphIdUnicode( long lUnicode ) const
{
    long lGlyph = 0;
    
    // Handle symbol fonts!
    /*
      if( m_bSymbol ) 
      {
      lUnicode = lUnicode | 0xf000;
      }
    */
    
    for(int i = 0; widths_table[i].unicode != 0xFFFF ; ++i)
    {
        if (widths_table[i].unicode == lUnicode) 
        {
            lGlyph = i; //widths_table[i].char_cd ;
            break;
        }
    }
    
    //FT_Get_Char_Index( m_face, lUnicode );
	
    return lGlyph;
}

long PdfFontMetricsBase14::GetGlyphId( long charId ) const
{
    long lGlyph = 0;
    
    // Handle symbol fonts!
    /*
      if( m_bSymbol ) 
      {
      charId = charId | 0xf000;
      }
    */
    
    for(int i = 0; widths_table[i].unicode != 0xFFFF  ; ++i)
    {
        if (widths_table[i].char_cd == charId) 
        {
            lGlyph = i; //widths_table[i].char_cd ;
            break;
        }
    }
    
    //FT_Get_Char_Index( m_face, lUnicode );

    return lGlyph;
}

inline bool PdfFontMetricsBase14::IsSymbol() const
{
    
    return m_bSymbol;
}

void PdfFontMetricsBase14::GetBoundingBox( PdfArray & array ) const
{
    array.Clear();
    array.push_back( PdfVariant( bbox.GetLeft() * 1000.0 / units_per_EM ) );
    array.push_back( PdfVariant( bbox.GetBottom() * 1000.0 / units_per_EM ) );
    array.push_back( PdfVariant( bbox.GetWidth() * 1000.0 / units_per_EM ) );
    array.push_back( PdfVariant( bbox.GetHeight() * 1000.0 / units_per_EM ) );
    
    return;
}

void PdfFontMetricsBase14::GetWidthArray( PdfVariant & var, unsigned int nFirst, unsigned int nLast, const PdfEncoding* pEncoding ) const
{
    unsigned int i;
    PdfArray     list;
    
    for( i=nFirst;i<=nLast;i++ )
    {
        if (pEncoding != NULL)
        {
            unsigned short shCode = pEncoding->GetCharCode(i);
#ifdef PODOFO_IS_LITTLE_ENDIAN
            shCode = ((shCode & 0x00FF) << 8) | ((shCode & 0xFF00) >> 8);
#endif
            list.push_back(PdfObject( (pdf_int64)this->GetGlyphWidth(this->GetGlyphIdUnicode(shCode) )));
        }
        else
        {
            list.push_back( PdfVariant(  double(widths_table[i].width)  ) );
        }
    }
    
    var = PdfVariant( list );
}

const char* PdfFontMetricsBase14::GetFontData() const
{
    return NULL;
}

pdf_long PdfFontMetricsBase14::GetFontDataLen() const
{
    return 0;
}

};

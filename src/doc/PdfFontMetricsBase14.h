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

#ifndef _PDF_FONT_METRICS_BASE14_H_
#define _PDF_FONT_METRICS_BASE14_H_

#include "podofo/base/PdfDefines.h"
#include "podofo/base/PdfRect.h"
#include "podofo/base/PdfVariant.h"

#include "PdfFontMetrics.h"

#include <string.h>

/*
  The following are the Base 14 fonts data copied from libharu.
  - kaushik April 12 2010
*/

namespace PoDoFo {

struct PODOFO_CharData;

class PdfArray;

 /*
   This is the main class to handle the base14 metric data.
   The member functions are accessed only through PDFFontmetrics.
   For eg. pdffontmetrics->GetFontSize would check if it is a base14 font,
   and call PdfFontMetricsBase14->GetFontSize.
   
   This is done to ensure all existing paths work as is.
   The changes to Base 14 get added without affecting the existing workflow and fit in exactly.
   
   Ideally PdfFontMetrics should be abstract or the metric related interface should be seperated out
   from the implementation details - such as whether the font metric data is read from a file/buffer/hard coded.
   
   Kaushik : April 12th 2010
   
 */
class PODOFO_DOC_API PdfFontMetricsBase14 : public PdfFontMetrics {
public:
	PdfFontMetricsBase14(const char   *mfont_name,
                         const PODOFO_CharData  *mwidths_table,
                         bool             mis_font_specific,
                         pdf_int16         mascent,
                         pdf_int16         mdescent,
                         pdf_uint16        mx_height,
                         pdf_uint16        mcap_height,
                         pdf_int16         mstrikeout_pos,
                         pdf_int16         munderline_pos,
                         const PdfRect &  mbbox);

    ~PdfFontMetricsBase14();

	friend  const PdfFontMetricsBase14*
		PODOFO_Base14FontDef_FindBuiltinData  (const char  *font_name);

    /** Create a width array for this font which is a required part
     *  of every font dictionary.
     *  \param var the final width array is written to this PdfVariant
     *  \param nFirst first character to be in the array
     *  \param nLast last character code to be in the array
     *  \param pEncoding encoding for correct character widths. If not passed default (latin1) encoding is used
     */
    virtual void GetWidthArray( PdfVariant & var, unsigned int nFirst, unsigned int nLast, const PdfEncoding* pEncoding = NULL ) const;

    /** Get the width of a single glyph id
     *
     *  \returns the width of a single glyph id
     */
    virtual double GetGlyphWidth( int nGlyphId ) const;

    /** Get the width of a single named glyph
     *
     *  \param pszGlyphname name of the glyph
     *  \returns the width of a single named glyph
     */
	virtual double GetGlyphWidth( const char* pszGlyphname ) const;

    /** Create the bounding box array as required by the PDF reference
     *  so that it can be written directly to a PDF file.
     * 
     *  \param array write the bounding box to this array.
     */
    virtual void GetBoundingBox( PdfArray & array ) const;

    /** Retrieve the width of the given character in PDF units in the current font
     *  \param c character
     *  \returns the width in PDF units
     */
    virtual double CharWidth( unsigned char c ) const;

    // Peter Petrov 20 March 2009
    /** Retrieve the width of the given character in PDF units in the current font
     *  \param c character
     *  \returns the width in PDF units
     */
    virtual double UnicodeCharWidth( unsigned short c ) const;

    /** Retrieve the line spacing for this font
     *  \returns the linespacing in PDF units
     */
    virtual double GetLineSpacing() const;

    /** Get the width of the underline for the current 
     *  font size in PDF units
     *  \returns the thickness of the underline in PDF units
     */
    virtual double GetUnderlineThickness() const;

    /** Return the position of the underline for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
    virtual double GetUnderlinePosition() const;

    /** Return the position of the strikeout for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
    virtual double GetStrikeOutPosition() const;

    /** Get the width of the strikeout for the current 
     *  font size in PDF units
     *  \returns the thickness of the strikeout in PDF units
     */
    virtual double GetStrikeoutThickness() const;

    /** Get a string with the postscript name of the font.
     *  \returns the postscript name of the font or NULL string if no postscript name is available.
     */
    virtual const char* GetFontname() const;

    /** Get the weight of this font.
     *  Used to build the font dictionay
     *  \returns the weight of this font (500 is normal).
     */
    virtual  unsigned int GetWeight() const;

    /** Get the ascent of this font in PDF
     *  units for the current font size.
     *
     *  \returns the ascender for this font
     *  
     *  \see GetPdfAscent
     */
    virtual double GetAscent() const;

    /** Get the ascent of this font
     *  Used to build the font dictionay
     *  \returns the ascender for this font
     *  
     *  \see GetAscent
     */
    virtual double GetPdfAscent() const;

    /** Get the descent of this font in PDF 
     *  units for the current font size.
     *  This value is usually negative!
     *
     *  \returns the descender for this font
     *
     *  \see GetPdfDescent
     */
    virtual double GetDescent() const;

    /** Get the descent of this font
     *  Used to build the font dictionay
     *  \returns the descender for this font
     *
     *  \see GetDescent
     */
    virtual double GetPdfDescent() const;

    /** Get the italic angle of this font.
     *  Used to build the font dictionay
     *  \returns the italic angle of this font.
     */
    virtual int GetItalicAngle() const;
 
    /** Get the glyph id for a unicode character
     *  in the current font.
     *
     *  \param lUnicode the unicode character value
     *  \returns the glyhph id for the character or 0 if the glyph was not found.
     */
    virtual long GetGlyphId( long lUnicode ) const;

    /** Symbol fonts do need special treatment in a few cases.
     *  Use this method to check if the current font is a symbol
     *  font. Symbold fonts are detected by checking 
     *  if they use FT_ENCODING_MS_SYMBOL as internal encoding.
     * 
     * \returns true if this is a symbol font
     */
    virtual bool IsSymbol() const;

    /** Get a pointer to the actual font data - if it was loaded from memory.
     *  \returns a binary buffer of data containing the font data
     */
    virtual const char* GetFontData() const;

    /** Get the length of the actual font data - if it was loaded from memory.
     *  \returns a the length of the font data
     */
    virtual pdf_long GetFontDataLen() const;

    inline double GetCapHeight() const;

    /** Get a glyph ID by Unicode value. This is needed for generation
    *  array of widths to /Font element.
    *
    *  \param lUnicode the unicode character value
    *  \returns a glyph ID
    *  \see PdfFontType1Base14
    */
    long GetGlyphIdUnicode( long lUnicode ) const;

private :
//	const PODOFO_Base14FontDefDataRec& base14font_data;
	const char      *font_name;
    const PODOFO_CharData  *widths_table;
    bool             is_font_specific;
    pdf_int16            ascent;
    pdf_int16            descent;
    pdf_uint16           x_height;
    pdf_uint16           cap_height;
    PdfRect              bbox;

	bool          m_bSymbol;  ///< Internal member to singnal a symbol font

    unsigned int  m_nWeight;
    int           m_nItalicAngle;

	
    double        m_dAscent;
    double        m_dPdfAscent;
    double        m_dDescent;
    double        m_dPdfDescent;

    double        m_dLineSpacing;
    double        m_dUnderlineThickness;
    double        m_dUnderlinePosition;
    double        m_dStrikeOutThickness;
    double        m_dStrikeOutPosition;

	int units_per_EM;

};


const PdfFontMetricsBase14*
PODOFO_Base14FontDef_FindBuiltinData  (const char  *font_name);

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline double PdfFontMetricsBase14::GetCapHeight() const 
{
    return cap_height;
}

};

#endif // _PDF_FONT_METRICS_BASE14_H_

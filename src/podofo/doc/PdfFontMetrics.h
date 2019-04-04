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

#ifndef _PDF_FONT_METRICS_H_
#define _PDF_FONT_METRICS_H_

#include "podofo/base/PdfDefines.h"
#include "podofo/base/Pdf3rdPtyForwardDecl.h"
#include "podofo/base/PdfString.h"
#include "podofo/base/PdfEncoding.h"

namespace PoDoFo {

class PdfArray;
class PdfObject;
class PdfVariant;

/**
 * This abstract class provides access
 * to fontmetrics informations.
 */
class PODOFO_DOC_API PdfFontMetrics {
 public:
    PdfFontMetrics( EPdfFontType eFontType, const char* pszFilename, const char* pszSubsetPrefix );


    virtual ~PdfFontMetrics();

    /** Create a width array for this font which is a required part
     *  of every font dictionary.
     *  \param var the final width array is written to this PdfVariant
     *  \param nFirst first character to be in the array
     *  \param nLast last character code to be in the array
     *  \param pEncoding encoding for correct character widths. If not passed default (latin1) encoding is used
     */
    virtual void GetWidthArray( PdfVariant & var, unsigned int nFirst, unsigned int nLast, const PdfEncoding* pEncoding = NULL ) const = 0;

    /** Get the width of a single glyph id
     *
     *  \param nGlyphId id of the glyph
     *  \returns the width of a single glyph id
     */
    virtual double GetGlyphWidth( int nGlyphId ) const = 0;

    /** Get the width of a single named glyph
     *
     *  \param pszGlyphname name of the glyph
     *  \returns the width of a single named glyph
     */
    virtual double GetGlyphWidth( const char* pszGlyphname ) const = 0;

    /** Create the bounding box array as required by the PDF reference
     *  so that it can be written directly to a PDF file.
     *
     *  \param array write the bounding box to this array.
     */
    virtual void GetBoundingBox( PdfArray & array ) const = 0;

    /** Retrieve the width of a given text string in PDF units when
     *  drawn with the current font
     *  \param rsString a PdfString from which the width shall be calculated
     *  \returns the width in PDF units
     *
     *  This is an overloaded method for your convinience!
     */
    inline double StringWidth( const PdfString & rsString ) const;

    /** Retrieve the width of a given text string in PDF units when
     *  drawn with the current font
     *  \param pszText a text string of which the width should be calculated
     *  \param nLength if != 0 only the width of the nLength first characters is calculated
     *  \returns the width in PDF units
     */
    double StringWidth( const char* pszText, pdf_long nLength = 0 ) const;

    /** Retrieve the width of a given text string in PDF units when
     *  drawn with the current font
     *  \param pszText a text string of which the width should be calculated
     *  \param nLength if != 0 only the width of the nLength first characters is calculated
     *  \returns the width in PDF units
     */
    double StringWidth( const pdf_utf16be* pszText, unsigned int nLength = 0 ) const;

#ifndef _WCHAR_T_DEFINED
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200    // not for MS Visual Studio 6
#else
    /** Retrieve the width of a given text string in PDF units when
     *  drawn with the current font
     *  \param pszText a text string of which the width should be calculated
     *  \param nLength if != 0 only the width of the nLength first characters is calculated
     *  \returns the width in PDF units
     */
    double StringWidth( const wchar_t* pszText, unsigned int nLength = 0 ) const;
#endif
#endif

    /** Retrieve the width of a given text string in 1/1000th mm when
     *  drawn with the current font
     *  \param pszText a text string of which the width should be calculated
     *  \param nLength if != 0 only the width of the nLength first characters is calculated
     *  \returns the width in 1/1000th mm
     */
    inline unsigned long StringWidthMM( const char* pszText, unsigned int nLength = 0 ) const;

    /** Retrieve the width of a given text string in 1/1000th mm when
     *  drawn with the current font
     *  \param pszText a text string of which the width should be calculated
     *  \param nLength if != 0 only the width of the nLength first characters is calculated
     *  \returns the width in 1/1000th mm
     */
    inline unsigned long StringWidthMM( const pdf_utf16be* pszText, unsigned int nLength = 0 ) const;

#ifndef _WCHAR_T_DEFINED
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200    // not for MS Visual Studio 6
#else
    /** Retrieve the width of a given text string in 1/1000th mm when
     *  drawn with the current font
     *  \param pszText a text string of which the width should be calculated
     *  \param nLength if != 0 only the width of the nLength first characters is calculated
     *  \returns the width in 1/1000th mm
     */
    inline unsigned long StringWidthMM( const wchar_t* pszText, unsigned int nLength = 0 ) const;
#endif
#endif

    /** Retrieve the width of the given character in PDF units in the current font
     *  \param c character
     *  \returns the width in PDF units
     */
    virtual double CharWidth( unsigned char c ) const = 0;

    // Peter Petrov 20 March 2009
    /** Retrieve the width of the given character in PDF units in the current font
     *  \param c character
     *  \returns the width in PDF units
     */
    virtual double UnicodeCharWidth( unsigned short c ) const = 0;

    /** Retrieve the width of the given character in 1/1000th mm in the current font
     *  \param c character
     *  \returns the width in 1/1000th mm
     */
    inline unsigned long CharWidthMM( unsigned char c ) const;

    /** Retrieve the line spacing for this font
     *  \returns the linespacing in PDF units
     */
    virtual double GetLineSpacing() const = 0;

    /** Retrieve the line spacing for this font
     *  \returns the linespacing in 1/1000th mm
     */
    inline unsigned long GetLineSpacingMM() const;

    /** Get the width of the underline for the current
     *  font size in PDF units
     *  \returns the thickness of the underline in PDF units
     */
    virtual double GetUnderlineThickness() const = 0;

    /** Get the width of the underline for the current
     *  font size in 1/1000th mm
     *  \returns the thickness of the underline in 1/1000th mm
     */
    inline unsigned long GetUnderlineThicknessMM() const;

    /** Return the position of the underline for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
    virtual double GetUnderlinePosition() const = 0;

    /** Return the position of the underline for the current font
     *  size in 1/1000th mm
     *  \returns the underline position in 1/1000th mm
     */
    inline long GetUnderlinePositionMM() const;

    /** Return the position of the strikeout for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
    virtual double GetStrikeOutPosition() const = 0;

    /** Return the position of the strikeout for the current font
     *  size in 1/1000th mm
     *  \returns the underline position in 1/1000th mm
     */
    inline unsigned long GetStrikeOutPositionMM() const;

    /** Get the width of the strikeout for the current
     *  font size in PDF units
     *  \returns the thickness of the strikeout in PDF units
     */
    virtual double GetStrikeoutThickness() const = 0;

    /** Get the width of the strikeout for the current
     *  font size in 1/1000th mm
     *  \returns the thickness of the strikeout in 1/1000th mm
     */
    inline unsigned long GetStrikeoutThicknessMM() const;

    /** Get a pointer to the path of the font file.
     *  \returns a zero terminated string containing the filename of the font file
     */
    inline const char* GetFilename() const;

    /** Get a pointer to the actual font data - if it was loaded from memory.
     *  \returns a binary buffer of data containing the font data
     */
    virtual const char* GetFontData() const = 0;

    /** Get the length of the actual font data - if it was loaded from memory.
     *  \returns a the length of the font data
     */
    virtual pdf_long GetFontDataLen() const = 0;

    /** Get a string with the postscript name of the font.
     *  \returns the postscript name of the font or NULL string if no postscript name is available.
     */
    virtual const char* GetFontname() const = 0;

    /**
     * \returns NULL or a 6 uppercase letter and "+" sign prefix
     *          used for font subsets
     */
    inline const char* GetSubsetFontnamePrefix() const;

    /** Get the weight of this font.
     *  Used to build the font dictionay
     *  \returns the weight of this font (500 is normal).
     */
    virtual  unsigned int GetWeight() const = 0;

    /** Get the ascent of this font in PDF
     *  units for the current font size.
     *
     *  \returns the ascender for this font
     *
     *  \see GetPdfAscent
     */
    virtual double GetAscent() const = 0;

    /** Get the ascent of this font
     *  Used to build the font dictionay
     *  \returns the ascender for this font
     *
     *  \see GetAscent
     */
    virtual double GetPdfAscent() const = 0;

    /** Get the descent of this font in PDF
     *  units for the current font size.
     *  This value is usually negative!
     *
     *  \returns the descender for this font
     *
     *  \see GetPdfDescent
     */
    virtual double GetDescent() const = 0;

    /** Get the descent of this font
     *  Used to build the font dictionay
     *  \returns the descender for this font
     *
     *  \see GetDescent
     */
    virtual double GetPdfDescent() const = 0;

    /** Get the italic angle of this font.
     *  Used to build the font dictionay
     *  \returns the italic angle of this font.
     */
    virtual int GetItalicAngle() const = 0;

    /** Set the font size of this metrics object for width and height
     *  calculations.
     *  This is typically called from PdfFont for you.
     *
     *  \param fSize font size in points
     */
    inline void SetFontSize( float fSize );

    /** Retrieve the current font size of this metrics object
     *  \returns the current font size
     */
    inline float GetFontSize() const;

    /** Set the horizontal scaling of the font for compressing (< 100) and expanding (>100)
     *  This is typically called from PdfFont for you.
     *
     *  \param fScale scaling in percent
     */
    inline void SetFontScale( float fScale );

    /** Retrieve the current horizontal scaling of this metrics object
     *  \returns the current font scaling
     */
    inline float GetFontScale() const;

    /** Set the character spacing of this metrics object
     *  \param fCharSpace character spacing in percent
     */
    inline void SetFontCharSpace( float fCharSpace );

    /** Retrieve the current character spacing of this metrics object
     *  \returns the current font character spacing
     */
    inline float GetFontCharSpace() const;

    /** Set the word spacing of this metrics object
     *  \param fWordSpace word spacing in PDF units
     */
    inline void SetWordSpace( float fWordSpace );

    /** Retrieve the current word spacing of this metrics object
     *  \returns the current font word spacing in PDF units
     */
    inline float GetWordSpace() const;

    /**
     *  \returns the fonttype of the loaded font
     */
    inline EPdfFontType GetFontType() const;

    /** Get the glyph id for a unicode character
     *  in the current font.
     *
     *  \param lUnicode the unicode character value
     *  \returns the glyhph id for the character or 0 if the glyph was not found.
     */
    virtual long GetGlyphId( long lUnicode ) const = 0;

    /** Symbol fonts do need special treatment in a few cases.
     *  Use this method to check if the current font is a symbol
     *  font. Symbold fonts are detected by checking
     *  if they use FT_ENCODING_MS_SYMBOL as internal encoding.
     *
     * \returns true if this is a symbol font
     */
    virtual bool IsSymbol() const = 0;

    /** Try to detect the internal fonttype from
     *  the file extension of a fontfile.
     *
     *  \param pszFilename must be the filename of a font file
     *
     *  \return font type
     */
    static EPdfFontType FontTypeFromFilename( const char* pszFilename );

 protected:
    /**
     *  Set the fonttype.
     *  \param eFontType fonttype
     */
    inline void SetFontType(EPdfFontType eFontType);

 protected:
    std::string   m_sFilename;
    float         m_fFontSize;
    float         m_fFontScale;
    float         m_fFontCharSpace;
    float         m_fWordSpace;

    std::vector<double> m_vecWidth;

    EPdfFontType  m_eFontType;
    std::string   m_sFontSubsetPrefix;
};

// -----------------------------------------------------
//
// -----------------------------------------------------
unsigned long PdfFontMetrics::CharWidthMM( unsigned char c ) const
{
    return static_cast<unsigned long>(this->CharWidth( c ) / PODOFO_CONVERSION_CONSTANT);
}

// -----------------------------------------------------
//
// -----------------------------------------------------
double PdfFontMetrics::StringWidth( const PdfString & rsString ) const
{
    return (rsString.IsUnicode() ?  this->StringWidth( rsString.GetUnicode() ) : this->StringWidth( rsString.GetString() ));
}

// -----------------------------------------------------
//
// -----------------------------------------------------
unsigned long PdfFontMetrics::StringWidthMM( const char* pszText, unsigned int nLength ) const
{
    return static_cast<unsigned long>(this->StringWidth( pszText, nLength ) / PODOFO_CONVERSION_CONSTANT);
}

// -----------------------------------------------------
//
// -----------------------------------------------------
unsigned long PdfFontMetrics::StringWidthMM( const pdf_utf16be* pszText, unsigned int nLength ) const
{
    return static_cast<unsigned long>(this->StringWidth( pszText, nLength ) / PODOFO_CONVERSION_CONSTANT);
}

// -----------------------------------------------------
//
// -----------------------------------------------------
#ifndef _WCHAR_T_DEFINED
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200    // not for MS Visual Studio 6
#else
unsigned long PdfFontMetrics::StringWidthMM( const wchar_t* pszText, unsigned int nLength ) const
{
    return static_cast<unsigned long>(this->StringWidth( pszText, nLength ) / PODOFO_CONVERSION_CONSTANT);
}
#endif
#endif

// -----------------------------------------------------
//
// -----------------------------------------------------
unsigned long PdfFontMetrics::GetLineSpacingMM() const
{
    return static_cast<unsigned long>(this->GetLineSpacing() / PODOFO_CONVERSION_CONSTANT);
}

// -----------------------------------------------------
//
// -----------------------------------------------------
long PdfFontMetrics::GetUnderlinePositionMM() const
{
    return static_cast<long>(this->GetUnderlinePosition() /  PODOFO_CONVERSION_CONSTANT);
}

// -----------------------------------------------------
//
// -----------------------------------------------------
unsigned long PdfFontMetrics::GetStrikeOutPositionMM() const
{
    return static_cast<long>(this->GetStrikeOutPosition() /  PODOFO_CONVERSION_CONSTANT);
}

// -----------------------------------------------------
//
// -----------------------------------------------------
unsigned long PdfFontMetrics::GetUnderlineThicknessMM() const
{
    return static_cast<unsigned long>(this->GetUnderlineThickness() / PODOFO_CONVERSION_CONSTANT);
}

// -----------------------------------------------------
//
// -----------------------------------------------------
unsigned long PdfFontMetrics::GetStrikeoutThicknessMM() const
{
    return static_cast<unsigned long>(this->GetStrikeoutThickness() / PODOFO_CONVERSION_CONSTANT);
}

// -----------------------------------------------------
//
// -----------------------------------------------------
const char* PdfFontMetrics::GetFilename() const
{
    return m_sFilename.c_str();
}

// -----------------------------------------------------
//
// -----------------------------------------------------
EPdfFontType PdfFontMetrics::GetFontType() const
{
    return m_eFontType;
}

// -----------------------------------------------------
//
// -----------------------------------------------------
void PdfFontMetrics::SetFontType(EPdfFontType eFontType)
{
    m_eFontType = eFontType;
}

// -----------------------------------------------------
//
// -----------------------------------------------------
float PdfFontMetrics::GetFontSize() const
{
    return m_fFontSize;
}

// -----------------------------------------------------
//
// -----------------------------------------------------
void PdfFontMetrics::SetFontSize( float fSize )
{
    m_fFontSize = fSize;
}

// -----------------------------------------------------
//
// -----------------------------------------------------
float PdfFontMetrics::GetFontScale() const
{
    return m_fFontScale;
}

// -----------------------------------------------------
//
// -----------------------------------------------------
float PdfFontMetrics::GetFontCharSpace() const
{
    return m_fFontCharSpace;
}

// -----------------------------------------------------
//
// -----------------------------------------------------
inline float PdfFontMetrics::GetWordSpace() const
{
    return m_fWordSpace;
}

// -----------------------------------------------------
//
// -----------------------------------------------------
const char* PdfFontMetrics::GetSubsetFontnamePrefix() const
{
    return m_sFontSubsetPrefix.c_str();
}

// -----------------------------------------------------
//
// -----------------------------------------------------
void PdfFontMetrics::SetFontScale( float fScale )
{
    m_fFontScale = fScale;
}

// -----------------------------------------------------
//
// -----------------------------------------------------
void PdfFontMetrics::SetFontCharSpace( float fCharSpace )
{
    m_fFontCharSpace = fCharSpace;
}

// -----------------------------------------------------
//
// -----------------------------------------------------
inline void PdfFontMetrics::SetWordSpace( float fWordSpace )
{
    m_fWordSpace = fWordSpace;
}


};

#endif // _PDF_FONT_METRICS_H_


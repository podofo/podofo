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

#ifndef _PDF_FONT_METRICS_H_
#define _PDF_FONT_METRICS_H_

#include "PdfDefines.h"
#include "Pdf3rdPtyForwardDecl.h"
#include "PdfString.h"

#ifndef PODOFO_CONVERSION_CONSTANT
#define PODOFO_CONVERSION_CONSTANT 0.002834645669291339
#endif // PODOFO_CONVERSION_CONSTANT

namespace PoDoFo {

class PdfArray;
class PdfObject;
class PdfVariant;

/**
 * Enum for the different font formats supported by PoDoFo
 */
enum EPdfFontType {
    ePdfFontType_TrueType,
    ePdfFontType_Type1Pfa,
    ePdfFontType_Type1Pfb,

    ePdfFontType_Unknown = 0xff
};


class PODOFO_API PdfFontMetrics {
 public:
    /** Create a font metrics object for a given true type file
     *  \param pLibrary handle to an initialized FreeType2 library handle
     *  \param pszFilename filename of a truetype file
     *  \param pszSubsetPrefix unique prefix for font subsets (see GetFontSubsetPrefix)
     */
    PdfFontMetrics( FT_Library* pLibrary, const char* pszFilename, 
		    const char* pszSubsetPrefix = NULL );

    /** Create a font metrics object for a given memory buffer
     *  \param pLibrary handle to an initialized FreeType2 library handle
     *  \param pBuffer block of memory representing the font data (PdfFontMetrics will copy the buffer)
     *  \param nBufLen the length of the buffer
     *  \param pszSubsetPrefix unique prefix for font subsets (see GetFontSubsetPrefix)
     */
    PdfFontMetrics( FT_Library* pLibrary, const char* pBuffer, unsigned int nBufLen,
		    const char* pszSubsetPrefix = NULL);

    /** Create a font metrics object for a given true type file
     *  \param pLibrary handle to an initialized FreeType2 library handle
     *  \param rBuffer a buffer containing a font file
     *  \param pszSubsetPrefix unique prefix for font subsets (see GetFontSubsetPrefix)
     */
    PdfFontMetrics( FT_Library* pLibrary, const PdfRefCountedBuffer & rBuffer,
		    const char* pszSubsetPrefix = NULL);

    /** Create a font metrics object for a given freetype font.
     *  \param pLibrary handle to an initialized FreeType2 library handle
     *  \param face a valid freetype font face
     *  \param pszSubsetPrefix unique prefix for font subsets (see GetFontSubsetPrefix)
     */
    PdfFontMetrics( FT_Library* pLibrary, FT_Face face,
		    const char* pszSubsetPrefix = NULL);

    /** Create a font metrics object based on an existing PdfObject
     *
     *  \param pLibrary handle to an initialized FreeType2 library handle
     *  \param pObject an existing font descriptor object
     */
    PdfFontMetrics( FT_Library* pLibrary, PdfObject* pDescriptor );

    virtual ~PdfFontMetrics();

    /** Create a width array for this font which is a required part
     *  of every font dictionary.
     *  \param var the final width array is written to this PdfVariant
     *  \param nFirst first character to be in the array
     *  \param nLast last character code to be in the array
     */
    void GetWidthArray( PdfVariant & var, unsigned int nFirst, unsigned int nLast ) const;

    /** Get the width of a single glyph id
     *
     *  \returns the width of a single glyph id
     */
    double GetGlyphWidth( int nGlyphId ) const;

    /** Create the bounding box array as required by the PDF reference
     *  so that it can be written directly to a PDF file.
     * 
     *  \param array write the bounding box to this array.
     */
    void GetBoundingBox( PdfArray & array ) const;

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
    double StringWidth( const char* pszText, unsigned int nLength = 0 ) const;

    /** Retrieve the width of a given text string in 1/1000th mm when
     *  drawn with the current font
     *  \param pszText a text string of which the width should be calculated
     *  \param nLength if != 0 only the width of the nLength first characters is calculated
     *  \returns the width in 1/1000th mm
     */
    unsigned long StringWidthMM( const char* pszText, unsigned int nLength = 0 ) const;

    /** Retrieve the width of the given character in PDF units in the current font
     *  \param c character
     *  \returns the width in PDF units
     */
    double CharWidth( unsigned char c ) const;

    /** Retrieve the width of the given character in 1/1000th mm in the current font
     *  \param c character
     *  \returns the width in 1/1000th mm
     */
    unsigned long CharWidthMM( unsigned char c ) const;

    /** Retrieve the line spacing for this font
     *  \returns the linespacing in PDF units
     */
    inline double GetLineSpacing() const;

    /** Retrieve the line spacing for this font
     *  \returns the linespacing in 1/1000th mm
     */
    inline unsigned long GetLineSpacingMM() const;

    /** Get the width of the underline for the current 
     *  font size in PDF units
     *  \returns the thickness of the underline in PDF units
     */
    inline double GetUnderlineThickness() const;

    /** Get the width of the underline for the current 
     *  font size in 1/1000th mm
     *  \returns the thickness of the underline in 1/1000th mm
     */
    inline unsigned long GetUnderlineThicknessMM() const;

    /** Return the position of the underline for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
    inline double GetUnderlinePosition() const;

    /** Return the position of the underline for the current font
     *  size in 1/1000th mm
     *  \returns the underline position in 1/1000th mm
     */
    inline long GetUnderlinePositionMM() const;

    /** Return the position of the strikeout for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
    inline double GetStrikeOutPosition() const;

    /** Return the position of the strikeout for the current font
     *  size in 1/1000th mm
     *  \returns the underline position in 1/1000th mm
     */
    inline unsigned long GetStrikeOutPositionMM() const;

    /** Get the width of the strikeout for the current 
     *  font size in PDF units
     *  \returns the thickness of the strikeout in PDF units
     */
    inline double GetStrikeoutThickness() const;

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
    inline const char* GetFontData() const;

    /** Get the length of the actual font data - if it was loaded from memory.
     *  \returns a the length of the font data
     */
    inline unsigned int GetFontDataLen() const;

    /** Get a string with the postscript name of the font.
     *  \returns the postscript name of the font or NULL string if no postscript name is available.
     */
    const char* GetFontname() const;

    /**
     * \returns NULL or a 6 uppercase letter and "+" sign prefix
     *          used for font subsets
     */
    const char* GetSubsetFontnamePrefix() const;

    /** Get the weight of this font.
     *  Used to build the font dictionay
     *  \returns the weight of this font (500 is normal).
     */
    inline unsigned int GetWeight() const;

    /** Get the ascent of this font in PDF
     *  units for the current font size.
     *
     *  \returns the ascender for this font
     *  
     *  \see GetPdfAscent
     */
    inline double GetAscent() const;

    /** Get the ascent of this font
     *  Used to build the font dictionay
     *  \returns the ascender for this font
     *  
     *  \see GetAscent
     */
    inline double GetPdfAscent() const;

    /** Get the descent of this font in PDF 
     *  units for the current font size.
     *  This value is usually negative!
     *
     *  \returns the descender for this font
     *
     *  \see GetPdfDescent
     */
    inline double GetDescent() const;

    /** Get the descent of this font
     *  Used to build the font dictionay
     *  \returns the descender for this font
     *
     *  \see GetDescent
     */
    inline double GetPdfDescent() const;

    /** Get the italic angle of this font.
     *  Used to build the font dictionay
     *  \returns the italic angle of this font.
     */
    inline int GetItalicAngle() const;

    /** Set the font size of this metrics object for width and height
     *  calculations.
     *  This is typically called from PdfFont for you.
     *
     *  \param fSize font size in points
     */
    void SetFontSize( float fSize );

    /** Retrieve the current font size of this metrics object 
     *  \returns the current font size
     */
    inline float GetFontSize() const;

    /** Set the horizontal scaling of the font for compressing (< 100) and expanding (>100)
     *  This is typically called from PdfFont for you.
     *
     *  \param fScale scaling in percent
     */
    void SetFontScale( float fScale );

    /** Retrieve the current horizontal scaling of this metrics object
     *  \returns the current font scaling
     */
    inline float GetFontScale() const;

    /** Set the character spacing of this metrics object
     *  \param fCharSpace character spacing in percent
     */
    void SetFontCharSpace( float fCharSpace );

    /** Retrieve the current character spacing of this metrics object
     *  \returns the current font character spacing
     */
    inline float GetFontCharSpace() const;

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
    long GetGlyphId( long lUnicode ) const;

    FT_Face GetFace() { return m_face; }; 
 private:
    
    /** Initialize this object from an in memory buffer
     *  Called internally by the constructors
     */
    void InitFromBuffer();

    /** Load the metric data from the FTFace data
     *		Called internally by the constructors
     */
    void InitFromFace();

    /** Try to detect the internal fonttype from
     *  the file extension of a fontfile.
     *
     *  This function will set the member m_eFontType.
     *
     *  \param pszFilename must be the filename of a font file
     */
    void SetFontTypeFromFilename( const char* pszFilename );

 protected:
    FT_Face       m_face;
    FT_Library*   m_pLibrary;

 private:
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

    std::string   m_sFilename;
    PdfRefCountedBuffer m_bufFontData;
    float         m_fFontSize;
    float         m_fFontScale;
    float         m_fFontCharSpace;

    std::vector<double> m_vecWidth;

    EPdfFontType  m_eFontType;
    std::string   m_sFontSubsetPrefix;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::StringWidth( const PdfString & rsString ) const
{
    return this->StringWidth( rsString.GetString() );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::GetLineSpacing() const
{
    return m_dLineSpacing;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned long PdfFontMetrics::GetLineSpacingMM() const
{
    return static_cast<unsigned long>(m_dLineSpacing / PODOFO_CONVERSION_CONSTANT);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::GetUnderlinePosition() const
{
    return m_dUnderlinePosition;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
long PdfFontMetrics::GetUnderlinePositionMM() const
{
    return static_cast<long>(m_dUnderlinePosition /  PODOFO_CONVERSION_CONSTANT);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::GetStrikeOutPosition() const
{
	return m_dStrikeOutPosition;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned long PdfFontMetrics::GetStrikeOutPositionMM() const
{
	    return static_cast<long>(m_dStrikeOutPosition /  PODOFO_CONVERSION_CONSTANT);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::GetUnderlineThickness() const
{
    return m_dUnderlineThickness;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned long PdfFontMetrics::GetUnderlineThicknessMM() const
{
    return static_cast<unsigned long>(m_dUnderlineThickness / PODOFO_CONVERSION_CONSTANT);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::GetStrikeoutThickness() const
{
    return m_dStrikeOutThickness;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned long PdfFontMetrics::GetStrikeoutThicknessMM() const
{
    return static_cast<unsigned long>(m_dStrikeOutThickness / PODOFO_CONVERSION_CONSTANT);
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
const char* PdfFontMetrics::GetFontData() const
{
    return m_bufFontData.GetBuffer();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned int PdfFontMetrics::GetFontDataLen() const
{
    return m_bufFontData.GetSize();
}  

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned int PdfFontMetrics::GetWeight() const
{
    return m_nWeight;
}  

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::GetAscent() const
{
    return m_dAscent;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::GetPdfAscent() const
{
    return m_dPdfAscent;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::GetDescent() const
{
    return m_dDescent;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::GetPdfDescent() const
{
    return m_dPdfDescent;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
int PdfFontMetrics::GetItalicAngle() const
{
    return m_nItalicAngle;
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
float PdfFontMetrics::GetFontSize() const
{
    return m_fFontSize;
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
 
};

#endif // _PDF_FONT_METRICS_H_


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

#ifndef _PDF_FONT_METRICS_FREETYPE_H_
#define _PDF_FONT_METRICS_FREETYPE_H_

#include "base/PdfDefines.h"
#include "base/Pdf3rdPtyForwardDecl.h"
#include "base/PdfString.h"
#include "PdfFontMetrics.h"

namespace PoDoFo {

class PdfArray;
class PdfObject;
class PdfVariant;

class PODOFO_API PdfFontMetricsFreetype : public PdfFontMetrics {
 public:
    /** Create a font metrics object for a given true type file
     *  \param pLibrary handle to an initialized FreeType2 library handle
     *  \param pszFilename filename of a truetype file
     *  \param pszSubsetPrefix unique prefix for font subsets (see GetFontSubsetPrefix)
     */
    PdfFontMetricsFreetype( FT_Library* pLibrary, const char* pszFilename, 
		    const char* pszSubsetPrefix = NULL );

    /** Create a font metrics object for a given memory buffer
     *  \param pLibrary handle to an initialized FreeType2 library handle
     *  \param pBuffer block of memory representing the font data (PdfFontMetricsFreetype will copy the buffer)
     *  \param nBufLen the length of the buffer
     *  \param pszSubsetPrefix unique prefix for font subsets (see GetFontSubsetPrefix)
     */
    PdfFontMetricsFreetype( FT_Library* pLibrary, const char* pBuffer, unsigned int nBufLen,
		    const char* pszSubsetPrefix = NULL);

    /** Create a font metrics object for a given true type file
     *  \param pLibrary handle to an initialized FreeType2 library handle
     *  \param rBuffer a buffer containing a font file
     *  \param pszSubsetPrefix unique prefix for font subsets (see GetFontSubsetPrefix)
     */
    PdfFontMetricsFreetype( FT_Library* pLibrary, const PdfRefCountedBuffer & rBuffer,
		    const char* pszSubsetPrefix = NULL);

    /** Create a font metrics object for a given freetype font.
     *  \param pLibrary handle to an initialized FreeType2 library handle
     *  \param face a valid freetype font face
     *  \param pszSubsetPrefix unique prefix for font subsets (see GetFontSubsetPrefix)
     */
    PdfFontMetricsFreetype( FT_Library* pLibrary, FT_Face face,
		    const char* pszSubsetPrefix = NULL);

    /** Create a font metrics object based on an existing PdfObject
     *
     *  \param pLibrary handle to an initialized FreeType2 library handle
     *  \param pObject an existing font descriptor object
     */
    PdfFontMetricsFreetype( FT_Library* pLibrary, PdfObject* pDescriptor );

    virtual ~PdfFontMetricsFreetype();

    /** Create a width array for this font which is a required part
     *  of every font dictionary.
     *  \param var the final width array is written to this PdfVariant
     *  \param nFirst first character to be in the array
     *  \param nLast last character code to be in the array
     */
    virtual void GetWidthArray( PdfVariant & var, unsigned int nFirst, unsigned int nLast ) const;

    /** Get the width of a single glyph id
     *
     *  \returns the width of a single glyph id
     */
    virtual double GetGlyphWidth( int nGlyphId ) const;

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
    virtual unsigned int GetWeight() const;

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

    /** Get direct access to the internal FreeType handle
     * 
     *  \returns the internal freetype handle
     */
    inline FT_Face GetFace();
 
 private:
    
    /** Initialize this object from an in memory buffer
     *  Called internally by the constructors
     */
    void InitFromBuffer();

    /** Load the metric data from the FTFace data
     *		Called internally by the constructors
     */
    void InitFromFace();

    void InitFontSizes();
 protected:
    FT_Library*   m_pLibrary;
    FT_Face       m_pFace;

 private:
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

    PdfRefCountedBuffer m_bufFontData;
    std::vector<double> m_vecWidth;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
FT_Face PdfFontMetricsFreetype::GetFace() 
{ 
    return m_pFace; 
} 

 
};

#endif // _PDF_FONT_METRICS_FREETYPE_H_


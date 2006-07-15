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
#ifndef _WIN32
#include <fontconfig.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H

namespace PoDoFo {

class PdfArray;
class PdfVariant;

class PdfFontMetrics {
 public:
    /** Create a font metrics object for a given true type file
     *  \param pLibrary handle to an initialized FreeType2 library handle
     *  \param pszFilename filename of a truetype file
     */
    PdfFontMetrics( FT_Library* pLibrary, const char* pszFilename );

   /** Create a font metrics object for a given memory buffer
     *  \param pLibrary handle to an initialized FreeType2 library handle
     *  \param pBuffer block of memory representing the font data
     *  \param nBufLen the length of the buffer
     */
    PdfFontMetrics( FT_Library* pLibrary, const char* pBuffer, unsigned int nBufLen );

    virtual ~PdfFontMetrics();

    /** Create a width array for this font which is a required part
     *  of every font dictionary.
     *  \param var the final width array is written to this PdfVariant
     *  \param nFirst first character to be in the array
     *  \param nLast last character code to be in the array
     */
    void GetWidthArray( PdfVariant & var, unsigned int nFirst, unsigned int nLast ) const;

    /** Create the bounding box array as required by the PDF reference
     *  so that it can be written directly to a PDF file.
     * 
     *  \param array write the bounding box to this array.
     */
    void GetBoundingBox( PdfArray & array ) const;

    /** Retrieve the width of a given text string in 1/1000th mm when
     *  drawn with the current font
     *  \param pszText a text string of which the width should be calculated
     *  \param nLength if != 0 only the width of the nLength first characters is calculated
     *  \returns the width in 1/1000th mm
     */
    unsigned long StringWidth( const char* pszText, unsigned int nLength = 0 ) const;

    /** Retrieve the width of the given character in 1/1000th mm in the current font
     *  \param c character
     *  \returns the width in 1/1000th mm
     */
    unsigned long CharWidth( char c ) const;

    /** Retrieve the line spacing for this font
     *  \returns the linespacing in 1/1000th mm
     */
    inline unsigned long LineSpacing() const;

    /** Get the width of the underline for the current 
     *  font size in 1/1000th mm
     *  \returns the thickness of the underline in 1/1000th mm
     */
    inline unsigned long UnderlineThickness() const;

    /** Return the position of the underline for the current font
     *  size in 1/1000th mm
     *  \returns the underline position in 1/1000th mm
     */
    inline long UnderlinePosition() const;

    /** Get a pointer to the path of the font file.
     *  \returns a zero terminated string containing the filename of the font file
     */
    inline const char* Filename() const;

    /** Get a pointer to the actual font data - if it was loaded from memory.
     *  \returns a binary buffer of data containing the font data
     */
    inline const char* FontData() const;

    /** Get the length of the actual font data - if it was loaded from memory.
     *  \returns a the length of the font data
     */
    inline unsigned int FontDataLen() const;

    /** Get a string with the postscript name of the font.
     *  \returns the postscript name of the font or NULL string if no postscript name is available.
     */
    const char* Fontname() const;

     /** Get a string with the postscript name of the font.
     *  \returns the postscript name of the font or empty string if no postscript name is available.
     */
    // const std::string Fontname() const;

    /** Get the weight of this font.
     *  Used to build the font dictionay
     *  \returns the weight of this font (500 is normal).
     */
    inline unsigned int Weight() const;

    /** Get the ascent of this font
     *  Used to build the font dictionay
     *  \returns the ascender for this font
     */
    inline double Ascent() const;

    /** Get the descent of this font
     *  Used to build the font dictionay
     *  \returns the descender for this font
     */
    inline double Descent() const;

    /** Get the italic angle of this font.
     *  Used to build the font dictionay
     *  \returns the italic angle of this font.
     */
    inline int ItalicAngle() const;

    /** Set the font size of this metrics object for width and height
     *  calculations.
     *  This is typically called from PdfFont for you.
     *
     *  \param fSize font size in points
     */
    void SetFontSize( float fSize );

    /** Get the filename where a font is located on the system
     *  On Unix systems the FontConfig library is used to find fonts.
     * 
     *  Everytime you call this function. The fontconfig library will be initialized
     *  and deinitialized afterwards, which is kind of slow.
     *  You should use the version below which can use an existing FcConfig object.
     *
     *  \param pszFontname name of the font e.g "Arial" or "Times New Roman"
     *  \returns the compelte absolute path to a matching font file or NULL
     *           if none was found.
     */
    static std::string GetFilenameForFont( const char* pszFontname );

    /** Get the filename where a font is located on the system
     *  On Unix systems the FontConfig library is used to find fonts.
     *  \param pConfig a handle to a fontconfig config object,
     *         using this function is fast as Fontconfig is not initialized in this function.
     *  \param pszFontname name of the font e.g "Arial" or "Times New Roman"
     *  \returns the compelte absolute path to a matching font file or NULL
     *           if none was found.
     */
#ifdef _WIN32
#else
    static std::string GetFilenameForFont( FcConfig* pConfig, const char* pszFontname );
#endif

 private:
    FT_Face       m_face;
    FT_Library*   m_pLibrary;

    unsigned int  m_nWeight;
    int           m_nItalicAngle;

    double        m_dAscent;
    double        m_dDescent;

    unsigned long m_lLineSpacing;
    unsigned long m_lUnderlineThickness;
    long          m_lUnderlinePosition;

    std::string   m_sFilename;
    char*		  m_pFontData;
    unsigned int  m_nFontDataLen;
    
    /** Load the metric data from the FTFace data
     *		Called internally by the constructors
     */
    void InitFromFace();
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned long PdfFontMetrics::LineSpacing() const
{
    return m_lLineSpacing;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
long PdfFontMetrics::UnderlinePosition() const
{
    return m_lUnderlinePosition;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned long PdfFontMetrics::UnderlineThickness() const
{
    return m_lUnderlineThickness;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const char* PdfFontMetrics::Filename() const
{
    return m_sFilename.c_str();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const char* PdfFontMetrics::FontData() const
{
    return m_pFontData;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned int PdfFontMetrics::FontDataLen() const
{
    return m_nFontDataLen;
}  

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned int PdfFontMetrics::Weight() const
{
    return m_nWeight;
}  

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::Ascent() const
{
    return m_dAscent;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::Descent() const
{
    return m_dDescent;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
int PdfFontMetrics::ItalicAngle() const
{
    return m_nItalicAngle;
}
   
};

#endif // _PDF_FONT_METRICS_H_


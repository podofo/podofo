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

#ifndef CONVERSION_CONSTANT
#define CONVERSION_CONSTANT 0.002834645669291339
#endif // CONVERSION_CONSTANT

namespace PoDoFo {

class PdfArray;
class PdfVariant;

/**
 * Enum for the different font formats supported by PoDoFo
 */
typedef enum EPdfFontType {
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
     */
    PdfFontMetrics( FT_Library* pLibrary, const char* pszFilename );

    /** Create a font metrics object for a given memory buffer
     *  \param pLibrary handle to an initialized FreeType2 library handle
     *  \param pBuffer block of memory representing the font data
     *  \param nBufLen the length of the buffer
     */
    PdfFontMetrics( FT_Library* pLibrary, const char* pBuffer, unsigned int nBufLen );

    /** Create a font metrics object for a given freetype font.
     *  \param pLibrary handle to an initialized FreeType2 library handle
     *  \param face a valid freetype font face
     */
    PdfFontMetrics( FT_Library* pLibrary, FT_Face face );

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
    double CharWidth( char c ) const;

    /** Retrieve the width of the given character in 1/1000th mm in the current font
     *  \param c character
     *  \returns the width in 1/1000th mm
     */
    unsigned long CharWidthMM( char c ) const;

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

     /** Get a string with the postscript name of the font.
     *  \returns the postscript name of the font or empty string if no postscript name is available.
     */
    // const std::string Fontname() const;

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
#if defined(_WIN32) || defined(__APPLE_CC__)
#else
    static std::string GetFilenameForFont( FcConfig* pConfig, const char* pszFontname );
#endif

    /** 
     *  \returns the fonttype of the loaded font
     */
    inline EPdfFontType GetFontType() const;

 private:
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

    std::string   m_sFilename;
    char*	  m_pFontData;
    unsigned int  m_nFontDataLen;
    float         m_fFontSize;

    std::vector<double> m_vecWidth;

    EPdfFontType  m_eFontType;
};

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
    return static_cast<unsigned long>(m_dLineSpacing / CONVERSION_CONSTANT);
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
    return static_cast<long>(m_dUnderlinePosition /  CONVERSION_CONSTANT);
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
    return static_cast<unsigned long>(m_dUnderlineThickness / CONVERSION_CONSTANT);
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
    return m_pFontData;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned int PdfFontMetrics::GetFontDataLen() const
{
    return m_nFontDataLen;
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
   
};

#endif // _PDF_FONT_METRICS_H_


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
	ePdfFontType_Type1Base14,
    ePdfFontType_Unknown = 0xff
};

// Base14changes : start changes
struct  PODOFO_Rect {
    double  left;
    double  bottom;
    double  right;
    double  top;

	PODOFO_Rect(double l,double b, double r, double t) : left(l), bottom(b), right(r), top(t) {}
	PODOFO_Rect(const PODOFO_Rect& rect) : left(rect.left), bottom(rect.bottom),
						right(rect.right), top(rect.top) {}
} ;

typedef struct  {
    pdf_int16     char_cd;
    pdf_uint16   unicode;  
    pdf_int16     width;
} PODOFO_CharData;
 
 /*
 This is the main class to handle the base14 metric data.
 The member functions are accessed only through PDFFontmetrics.
 For eg. pdffontmetrics->GetFontSize would check if it is a base14 font,
 and call PODOFO_Base14FontDefData->GetFontSize.
 
 This is done to ensure all existing paths work as is.
 The changes to Base 14 get added without affecting the existing workflow and fit in exactly.
 
 Ideally PdfFontMetrics should be abstract or the metric related interface should be seperated out
 from the implementation details - such as whether the font metric data is read from a file/buffer/hard coded.
 
 Kaushik : April 12th 2010
 
 */
struct PODOFO_Base14FontDefData 
{
	//PODOFO_Base14FontDefData(const PODOFO_Base14FontDefDataRec&  data) : base14font_data(data){}
	PODOFO_Base14FontDefData(const char      *mfont_name,
    const PODOFO_CharData  *mwidths_table,
    bool             mis_font_specific,
    pdf_int16            mascent,
    pdf_int16            mdescent,
    pdf_uint16           mx_height,
    pdf_uint16           mcap_height,
	PODOFO_Rect              mbbox) : font_name(mfont_name),widths_table(mwidths_table),
							is_font_specific(mis_font_specific),ascent(mascent), descent(mdescent),
								x_height(mx_height), cap_height(mcap_height), bbox(mbbox), 
									m_bSymbol(is_font_specific)
	{			
		m_nWeight             = 500;
		m_nItalicAngle        = 0;
		m_dLineSpacing        = 0.0;
		m_dUnderlineThickness = 0.0;
		m_dUnderlinePosition  = 0.0;
		m_dStrikeOutPosition  = 0.0;
		m_dStrikeOutThickness = 0.0;
		m_fFontSize           = 0.0f;
		units_per_EM = 1000;
		m_dPdfAscent  = ascent * 1000 / units_per_EM;
		m_dPdfDescent = descent * 1000 / units_per_EM;
 
		m_dAscent = ascent;
		m_dDescent = descent;	 

 
	}

	void GetWidthArray( PdfVariant & var, unsigned int nFirst, unsigned int nLast ) const;


    /** Get the width of a single glyph id
     *
     *  \returns the width of a single glyph id
     */
    double GetGlyphWidth( int nGlyphId) const 
	{
		return widths_table[static_cast<unsigned int>(nGlyphId)].width; 
	}

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
	inline double StringWidth( const PdfString & rsString ) const 
	{
		
		return (rsString.IsUnicode() ?  this->StringWidth( rsString.GetUnicode() ) : this->StringWidth( rsString.GetString() ));

	}

    /** Retrieve the width of a given text string in PDF units when
     *  drawn with the current font
     *  \param pszText a text string of which the width should be calculated
     *  \param nLength if != 0 only the width of the nLength first characters is calculated
     *  \returns the width in PDF units
     */
	double StringWidth( const char* pszText, pdf_long nLength = 0 ) const 
	{
	 
		double dWidth = 0.0;

		if( !pszText )
			return dWidth;

		if( !nLength )
			nLength = strlen( pszText );

		const char *localText = pszText;
		for ( pdf_long i=0; i<nLength; i++ ) 
		{
			dWidth += CharWidth( *localText );
			localText++;
		}

		return dWidth;

	}

    /** Retrieve the width of a given text string in PDF units when
     *  drawn with the current font
     *  \param pszText a text string of which the width should be calculated
     *  \param nLength if != 0 only the width of the nLength first characters is calculated
     *  \returns the width in PDF units
     */
double StringWidth( const pdf_utf16be* pszText, unsigned int nLength = 0 ) const 
{
	double dWidth = 0.0;

    if( !pszText )
        return dWidth;

    if( !nLength )
    {
		const pdf_utf16be* pszCount = pszText;
		while( *pszCount )
		{
			++pszCount;
			++nLength;
		}
    }
    
    const pdf_utf16be* localText = pszText;
    for ( unsigned int i=0; i<nLength; i++ ) 
    {
#ifdef PODOFO_IS_LITTLE_ENDIAN
        dWidth += UnicodeCharWidth(static_cast<unsigned short>(((*localText & 0x00ff) << 8 | (*localText & 0xff00) >> 8)) );
#else
        dWidth += UnicodeCharWidth(static_cast<unsigned short>(*localText) );
#endif // PODOFO_IS_LITTLE_ENDIAN
        localText++;
    }

    return dWidth;
}
#ifndef _WCHAR_T_DEFINED
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200			// nicht für Visualstudio 6
#else
    /** Retrieve the width of a given text string in PDF units when
     *  drawn with the current font
     *  \param pszText a text string of which the width should be calculated
     *  \param nLength if != 0 only the width of the nLength first characters is calculated
     *  \returns the width in PDF units
     */
    double StringWidth( const wchar_t* pszText, unsigned int nLength = 0 ) const
	{
		double dWidth = 0.0;

		if( !pszText )
			return dWidth;

		if( !nLength )
			nLength = static_cast<unsigned int>(wcslen( pszText ));

		const wchar_t *localText = pszText;
		for ( unsigned int i=0; i<nLength; i++ ) 
		{
			dWidth += CharWidth( static_cast<int>(*localText) );
			localText++;
		}

		return dWidth;
	}
#endif
#endif

    /** Retrieve the width of a given text string in 1/1000th mm when
     *  drawn with the current font
     *  \param pszText a text string of which the width should be calculated
     *  \param nLength if != 0 only the width of the nLength first characters is calculated
     *  \returns the width in 1/1000th mm
     */
	unsigned long StringWidthMM( const char* pszText, unsigned int nLength = 0 ) const 
	{
		return static_cast<unsigned long>(this->StringWidth( pszText, nLength ) / PODOFO_CONVERSION_CONSTANT);
	}

    /** Retrieve the width of a given text string in 1/1000th mm when
     *  drawn with the current font
     *  \param pszText a text string of which the width should be calculated
     *  \param nLength if != 0 only the width of the nLength first characters is calculated
     *  \returns the width in 1/1000th mm
     */
	unsigned long StringWidthMM( const pdf_utf16be* pszText, unsigned int nLength = 0 ) const 
	{
		return static_cast<unsigned long>(this->StringWidth( pszText, nLength ) / PODOFO_CONVERSION_CONSTANT);
	}

#ifndef _WCHAR_T_DEFINED
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200			// nicht für Visualstudio 6
#else
    /** Retrieve the width of a given text string in 1/1000th mm when
     *  drawn with the current font
     *  \param pszText a text string of which the width should be calculated
     *  \param nLength if != 0 only the width of the nLength first characters is calculated
     *  \returns the width in 1/1000th mm
     */
    unsigned long StringWidthMM( const wchar_t* pszText, unsigned int nLength = 0 ) const
	{
			return static_cast<unsigned long>(this->StringWidth( pszText, nLength ) / PODOFO_CONVERSION_CONSTANT);
	}
#endif
#endif
    
    /** Retrieve the width of the given character in PDF units in the current font
     *  \param c character
     *  \returns the width in PDF units
     */
	double CharWidth( unsigned char c ) const 
	{
		 
		double dWidth = widths_table[static_cast<unsigned int>(GetGlyphId(c) )].width;

		return dWidth * static_cast<double>(m_fFontSize * m_fFontScale / 100.0) / 1000.0 +
		    static_cast<double>( m_fFontSize * m_fFontScale / 100.0 * m_fFontCharSpace / 100.0);
	}

    // Peter Petrov 20 March 2009
    /** Retrieve the width of the given character in PDF units in the current font
     *  \param c character
     *  \returns the width in PDF units
     */
	double UnicodeCharWidth( unsigned short c ) const 
	{
		double   dWidth = 0.0;


		dWidth = widths_table[static_cast<unsigned int>(GetGlyphIdUnicode(c) )].width;
		 

		return dWidth * static_cast<double>(m_fFontSize * m_fFontScale / 100.0) / 1000.0 +
				static_cast<double>( m_fFontSize * m_fFontScale / 100.0 * m_fFontCharSpace / 100.0);

	 
	}

    /** Retrieve the width of the given character in 1/1000th mm in the current font
     *  \param c character
     *  \returns the width in 1/1000th mm
     */
	unsigned long CharWidthMM( unsigned char c ) const 
	{
		return static_cast<unsigned long>(this->CharWidth( c ) / PODOFO_CONVERSION_CONSTANT);
	}

    /** Retrieve the line spacing for this font
     *  \returns the linespacing in PDF units
     */
	inline double GetLineSpacing() const 
	{
		return m_dLineSpacing;
	}

    /** Retrieve the line spacing for this font
     *  \returns the linespacing in 1/1000th mm
     */
	inline unsigned long GetLineSpacingMM() const 
	{
		return static_cast<unsigned long>(m_dLineSpacing / PODOFO_CONVERSION_CONSTANT);
	}

    /** Get the width of the underline for the current 
     *  font size in PDF units
     *  \returns the thickness of the underline in PDF units
     */
	inline double GetUnderlineThickness() const 
	{
		return m_dUnderlineThickness;
	}

    /** Get the width of the underline for the current 
     *  font size in 1/1000th mm
     *  \returns the thickness of the underline in 1/1000th mm
     */
	inline unsigned long GetUnderlineThicknessMM() const 
	{
		return static_cast<unsigned long>(m_dUnderlineThickness / PODOFO_CONVERSION_CONSTANT);
	}

    /** Return the position of the underline for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
	inline double GetUnderlinePosition() const 
	{
		return m_dUnderlinePosition;
	}

    /** Return the position of the underline for the current font
     *  size in 1/1000th mm
     *  \returns the underline position in 1/1000th mm
     */
	inline long GetUnderlinePositionMM() const 
	{
		return static_cast<long>(m_dUnderlinePosition /  PODOFO_CONVERSION_CONSTANT);
	}

    /** Return the position of the strikeout for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
	inline double GetStrikeOutPosition() const 
	{
		return m_dStrikeOutPosition;
	}
	
    /** Return the position of the strikeout for the current font
     *  size in 1/1000th mm
     *  \returns the underline position in 1/1000th mm
     */
	inline unsigned long GetStrikeOutPositionMM() const 
	{
		return static_cast<long>(m_dStrikeOutPosition /  PODOFO_CONVERSION_CONSTANT);
	}

    /** Get the width of the strikeout for the current 
     *  font size in PDF units
     *  \returns the thickness of the strikeout in PDF units
     */
	inline double GetStrikeoutThickness() const 
	{
		return m_dStrikeOutThickness;
	}

    /** Get the width of the strikeout for the current 
     *  font size in 1/1000th mm
     *  \returns the thickness of the strikeout in 1/1000th mm
     */
	inline unsigned long GetStrikeoutThicknessMM() const 
	{
		return static_cast<unsigned long>(m_dStrikeOutThickness / PODOFO_CONVERSION_CONSTANT);
	}

    /** Get a pointer to the path of the font file.
     *  \returns a zero terminated string containing the filename of the font file
     */
	inline const char* GetFilename() const 
	{
#ifdef MYASSERT
		assert(0); // should not be called;
#endif
		/*
			We are getting called to  work on a font file of a base 14 font but we haven't been given a file.
			Perhaps we are trying to embed a base14 font. 
			A document.createfont call was made without giving a path to font file.
		*/

		PODOFO_RAISE_ERROR( ePdfError_FileNotFound); // throw?
		return 0;
	}

    /** Get a pointer to the actual font data - if it was loaded from memory.
     *  \returns a binary buffer of data containing the font data
     */
	inline const char* GetFontData() const 
	{
#ifdef MYASSERT
		assert(0); // should not be called;
#endif
		/*
			We are getting called to  work on a font file/memory buffer of a base 14 font but we haven't been given a file.
			Perhaps we are trying to embed a base14 font. 
			A document.createfont call was made without giving a path to font file.
		*/
		PODOFO_RAISE_ERROR(ePdfError_InvalidHandle);
		//  throw?
		return 0;
	}

    /** Get the length of the actual font data - if it was loaded from memory.
     *  \returns a the length of the font data
     */
	inline pdf_long GetFontDataLen() const 
	{
#ifdef MYASSERT
		assert(0); // should not be called;
#endif
		/*
			We are getting called to  work on a font file/memory buffer of a base 14 font but we haven't been given a file.
			Perhaps we are trying to embed a base14 font. 
			A document.createfont call was made without giving a path to font file.
		*/
		PODOFO_RAISE_ERROR(ePdfError_InvalidHandle);
		//  throw?
		return 0;
	}

    /** Get a string with the postscript name of the font.
     *  \returns the postscript name of the font or NULL string if no postscript name is available.
     */
	const char* GetFontname() const 
	{
#ifdef MYASSERT
		assert(font_name != NULL);
#endif
		return font_name;
	}

    /**
     * \returns NULL or a 6 uppercase letter and "+" sign prefix
     *          used for font subsets
     */
	const char* GetSubsetFontnamePrefix() const 
	{
		return "";
	}

    /** Get the weight of this font.
     *  Used to build the font dictionay
     *  \returns the weight of this font (500 is normal).
     */
	inline unsigned int GetWeight() const 
	{
		return m_nWeight;
	}

    /** Get the ascent of this font in PDF
     *  units for the current font size.
     *
     *  \returns the ascender for this font
     *  
     *  \see GetPdfAscent
     */
	inline double GetAscent() const 
	{
		return m_dAscent;
	}

	inline double GetCapHeight() const 
	{
		return cap_height;
	}

    /** Get the ascent of this font
     *  Used to build the font dictionay
     *  \returns the ascender for this font
     *  
     *  \see GetAscent
     */
	inline double GetPdfAscent() const 
	{
		return m_dPdfAscent;
	}

    /** Get the descent of this font in PDF 
     *  units for the current font size.
     *  This value is usually negative!
     *
     *  \returns the descender for this font
     *
     *  \see GetPdfDescent
     */
	inline double GetDescent() const 
	{
		return m_dDescent;
	}

    /** Get the descent of this font
     *  Used to build the font dictionay
     *  \returns the descender for this font
     *
     *  \see GetDescent
     */
	inline double GetPdfDescent() const 
	{
		return m_dPdfDescent;
	}

    /** Get the italic angle of this font.
     *  Used to build the font dictionay
     *  \returns the italic angle of this font.
     */
	inline int GetItalicAngle() const 
	{
		return m_nItalicAngle;
	}

    /** Set the font size of this metrics object for width and height
     *  calculations.
     *  This is typically called from PdfFont for you.
     *
     *  \param fSize font size in points
     */
	void SetFontSize( float fSize )
	{	
				// calculate the line spacing now, as it changes only with the font size
		m_dLineSpacing        = (static_cast<double>(ascent + abs(descent)) * fSize / units_per_EM);

	//	m_dUnderlineThickness = (static_cast<double>(m_face->underline_thickness) * fSize / m_face->units_per_EM);
	//	m_dUnderlinePosition  = (static_cast<double>(m_face->underline_position)  * fSize  / m_face->units_per_EM);
		
		m_dAscent  = static_cast<double>(ascent)  * fSize /  units_per_EM;
		m_dDescent = static_cast<double>(descent) * fSize /  units_per_EM;

		// Set default values for strikeout, in case the font has no direct values
		m_dStrikeOutPosition  = m_dAscent / 2.0; 
	//	m_dStrikeOutThickness = m_dUnderlineThickness;

		/*
		TT_OS2* pOs2Table = static_cast<TT_OS2*>(FT_Get_Sfnt_Table( m_face, ft_sfnt_os2 ));
		if( pOs2Table ) 
		{
			m_dStrikeOutPosition  = static_cast<double>(pOs2Table->yStrikeoutPosition)  * fSize / m_face->units_per_EM;
			m_dStrikeOutThickness = static_cast<double>(pOs2Table->yStrikeoutSize)  * fSize / m_face->units_per_EM;
		}
                */

		m_fFontSize = fSize;

		return ;
	}

    /** Retrieve the current font size of this metrics object 
     *  \returns the current font size
     */
	inline float GetFontSize() const 
	{
		return m_fFontSize;
	}

    /** Set the horizontal scaling of the font for compressing (< 100) and expanding (>100)
     *  This is typically called from PdfFont for you.
     *
     *  \param fScale scaling in percent
     */
	void SetFontScale( float fScale )
	{
		m_fFontScale = fScale;
		return ;
	}

    /** Retrieve the current horizontal scaling of this metrics object
     *  \returns the current font scaling
     */
    inline float GetFontScale() const
	{
		return m_fFontScale;
	}

    /** Set the character spacing of this metrics object
     *  \param fCharSpace character spacing in percent
     */
    void SetFontCharSpace( float fCharSpace )
	{
		m_fFontCharSpace = fCharSpace;

		return ;
	}

    /** Retrieve the current character spacing of this metrics object
     *  \returns the current font character spacing
     */
    inline float GetFontCharSpace() const
	{
		return m_fFontCharSpace;
	}

    /** 
     *  \returns the fonttype of the loaded font
     */
    inline EPdfFontType GetFontType() const
	{
		return ePdfFontType_Type1Base14; // checkbase14
	}

	long GetGlyphIdUnicode( long lUnicode ) const
	{
		long lGlyph = 0;
 
/*
		// Handle symbol fonts!
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


    /** Get the glyph id for a unicode character
     *  in the current font.
     *
     *  \param lUnicode the unicode character value
     *  \returns the glyhph id for the character or 0 if the glyph was not found.
     */
    long GetGlyphId( long charId ) const
	{
		long lGlyph = 0;
 
/*
		// Handle symbol fonts!
		if( m_bSymbol ) 
		{
			lUnicode = lUnicode | 0xf000;
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

    /** Get direct access to the internal FreeType handle
     * 
     *  \returns the internal freetype handle
     */
 //   inline FT_Face GetFace()
//	{
//		assert(0); // not to get called
//		return FT_Face  ();
//	}
 
    /** Symbol fonts do need special treatment in a few cases.
     *  Use this method to check if the current font is a symbol
     *  font. Symbold fonts are detected by checking 
     *  if they use FT_ENCODING_MS_SYMBOL as internal encoding.
     * 
     * \returns true if this is a symbol font
     */
    inline bool IsSymbol() const
	{

		return m_bSymbol;
	}

	friend  PODOFO_Base14FontDefData*
		PODOFO_Base14FontDef_FindBuiltinData  (const char  *font_name);

private :
//	const PODOFO_Base14FontDefDataRec& base14font_data;
	const char      *font_name;
    const PODOFO_CharData  *widths_table;
    bool             is_font_specific;
    pdf_int16            ascent;
    pdf_int16            descent;
    pdf_uint16           x_height;
    pdf_uint16           cap_height;
    PODOFO_Rect              bbox;

	float         m_fFontSize;
    float         m_fFontScale;
    float         m_fFontCharSpace;
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


PODOFO_Base14FontDefData*
PODOFO_Base14FontDef_FindBuiltinData  (const char  *font_name);

// Base14changes : end changes



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

	PdfFontMetrics( PODOFO_Base14FontDefData* pMetrics_base14) : m_pMetrics_base14(pMetrics_base14)
							, m_pLibrary(NULL), m_face(NULL)
	{
#ifdef MYASSERT
		assert(m_pMetrics_base14 != NULL);
#endif
	}

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
    double StringWidth( const char* pszText, pdf_long nLength = 0 ) const;

    /** Retrieve the width of a given text string in PDF units when
     *  drawn with the current font
     *  \param pszText a text string of which the width should be calculated
     *  \param nLength if != 0 only the width of the nLength first characters is calculated
     *  \returns the width in PDF units
     */
    double StringWidth( const pdf_utf16be* pszText, unsigned int nLength = 0 ) const;

#ifndef _WCHAR_T_DEFINED
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200			// nicht für Visualstudio 6
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
    unsigned long StringWidthMM( const char* pszText, unsigned int nLength = 0 ) const;

    /** Retrieve the width of a given text string in 1/1000th mm when
     *  drawn with the current font
     *  \param pszText a text string of which the width should be calculated
     *  \param nLength if != 0 only the width of the nLength first characters is calculated
     *  \returns the width in 1/1000th mm
     */
    unsigned long StringWidthMM( const pdf_utf16be* pszText, unsigned int nLength = 0 ) const;

#ifndef _WCHAR_T_DEFINED
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200			// nicht für Visualstudio 6
#else
    /** Retrieve the width of a given text string in 1/1000th mm when
     *  drawn with the current font
     *  \param pszText a text string of which the width should be calculated
     *  \param nLength if != 0 only the width of the nLength first characters is calculated
     *  \returns the width in 1/1000th mm
     */
    unsigned long StringWidthMM( const wchar_t* pszText, unsigned int nLength = 0 ) const;
#endif
#endif
    
    /** Retrieve the width of the given character in PDF units in the current font
     *  \param c character
     *  \returns the width in PDF units
     */
    double CharWidth( unsigned char c ) const;

    // Peter Petrov 20 March 2009
    /** Retrieve the width of the given character in PDF units in the current font
     *  \param c character
     *  \returns the width in PDF units
     */
    double UnicodeCharWidth( unsigned short c ) const;

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
    inline pdf_long GetFontDataLen() const;

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

    /** Get direct access to the internal FreeType handle
     * 
     *  \returns the internal freetype handle
     */
    inline FT_Face GetFace();
 
    /** Symbol fonts do need special treatment in a few cases.
     *  Use this method to check if the current font is a symbol
     *  font. Symbold fonts are detected by checking 
     *  if they use FT_ENCODING_MS_SYMBOL as internal encoding.
     * 
     * \returns true if this is a symbol font
     */
    inline bool IsSymbol() const;

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

 public:
    FT_Face       m_face;
    // Peter Petrov 19 March 2009
    std::string   m_sFilename;

 protected:
    FT_Library*   m_pLibrary;

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
    float         m_fFontSize;
    float         m_fFontScale;
    float         m_fFontCharSpace;

    std::vector<double> m_vecWidth;

    EPdfFontType  m_eFontType;
    std::string   m_sFontSubsetPrefix;

	PODOFO_Base14FontDefData* m_pMetrics_base14; //base14changes
	bool m_isBase14;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::StringWidth( const PdfString & rsString ) const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->StringWidth(rsString );

    return (rsString.IsUnicode() ?  this->StringWidth( rsString.GetUnicode() ) : this->StringWidth( rsString.GetString() ));
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::GetLineSpacing() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetLineSpacing();

    return m_dLineSpacing;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned long PdfFontMetrics::GetLineSpacingMM() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetLineSpacingMM();

    return static_cast<unsigned long>(m_dLineSpacing / PODOFO_CONVERSION_CONSTANT);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::GetUnderlinePosition() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetUnderlinePosition();

    return m_dUnderlinePosition;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
long PdfFontMetrics::GetUnderlinePositionMM() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetUnderlinePositionMM();

    return static_cast<long>(m_dUnderlinePosition /  PODOFO_CONVERSION_CONSTANT);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::GetStrikeOutPosition() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetStrikeOutPosition();

	return m_dStrikeOutPosition;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned long PdfFontMetrics::GetStrikeOutPositionMM() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetStrikeOutPositionMM();

	return static_cast<long>(m_dStrikeOutPosition /  PODOFO_CONVERSION_CONSTANT);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::GetUnderlineThickness() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetUnderlineThickness();

    return m_dUnderlineThickness;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned long PdfFontMetrics::GetUnderlineThicknessMM() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetUnderlineThicknessMM();

    return static_cast<unsigned long>(m_dUnderlineThickness / PODOFO_CONVERSION_CONSTANT);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::GetStrikeoutThickness() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetStrikeoutThickness();

    return m_dStrikeOutThickness;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned long PdfFontMetrics::GetStrikeoutThicknessMM() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetStrikeoutThicknessMM();

    return static_cast<unsigned long>(m_dStrikeOutThickness / PODOFO_CONVERSION_CONSTANT);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const char* PdfFontMetrics::GetFilename() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetFilename();

    return m_sFilename.c_str();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const char* PdfFontMetrics::GetFontData() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetFontData();

    return m_bufFontData.GetBuffer();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
pdf_long PdfFontMetrics::GetFontDataLen() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetFontDataLen();

    return m_bufFontData.GetSize();
}  

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned int PdfFontMetrics::GetWeight() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetWeight();

    return m_nWeight;
}  

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::GetAscent() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetAscent();

    return m_dAscent;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::GetPdfAscent() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetPdfAscent();

    return m_dPdfAscent;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::GetDescent() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetDescent();

    return m_dDescent;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetrics::GetPdfDescent() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetPdfDescent();

    return m_dPdfDescent;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
int PdfFontMetrics::GetItalicAngle() const
{
		
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetItalicAngle();

    return m_nItalicAngle;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
EPdfFontType PdfFontMetrics::GetFontType() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetFontType();

    return m_eFontType;
}
  
// -----------------------------------------------------
// 
// -----------------------------------------------------
float PdfFontMetrics::GetFontSize() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetFontSize();

    return m_fFontSize;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
float PdfFontMetrics::GetFontScale() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetFontScale();

    return m_fFontScale;
}
 
// -----------------------------------------------------
// 
// -----------------------------------------------------
float PdfFontMetrics::GetFontCharSpace() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->GetFontCharSpace();

    return m_fFontCharSpace;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
FT_Face PdfFontMetrics::GetFace() 
{ 
	if (m_pMetrics_base14)
	{
#ifdef MYASSERT
		assert(0);
#endif
	  // Should not be called for Base14 fonts, as we don't work with a font file or font buffer
	  PODOFO_RAISE_ERROR( ePdfError_InvalidHandle); // throw?
	}
		//		return m_pMetrics_base14->GetFace();

    return m_face; 
} 

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfFontMetrics::IsSymbol() const
{
	if (m_pMetrics_base14)
		return m_pMetrics_base14->IsSymbol();

    return m_bSymbol;
}
 
};

#endif // _PDF_FONT_METRICS_H_


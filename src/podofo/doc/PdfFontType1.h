/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
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

#ifndef _PDF_FONT_TYPE1_H_
#define _PDF_FONT_TYPE1_H_

#include "podofo/base/PdfDefines.h"
#include "PdfFontSimple.h"

namespace PoDoFo {

/** A PdfFont implementation that can be used
 *  to embedd type1 fonts into a PDF file
 *  or to draw with type1 fonts. 
 */
class PdfFontType1 : public PdfFontSimple {
 public:

    /** Create a new Type1 font object.
     *
     *  \param pMetrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is 
     *         deleted along with the font.
     *  \param pEncoding the encoding of this font. The font will take ownership of this object
     *                   depending on pEncoding->IsAutoDelete()
     *  \param pParent parent of the font object
     *  \param bEmbed if true the font will get embedded.
     *  \param bSubsetting if true the font will use subsetting.
     *  
     */
    PdfFontType1( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, 
                  PdfVecObjects* pParent, bool bEmbed, bool bSubsetting = false );

    /** Create a PdfFont based on an existing PdfObject
     *  \param pMetrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is 
     *         deleted along with the font.
     *  \param pEncoding the encoding of this font. The font will take ownership of this object
     *                   depending on pEncoding->IsAutoDelete()
     *  \param pObject an existing PdfObject
     */
    PdfFontType1( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, 
                  PdfObject* pObject );

	/** Create a PdfFont based on an existing PdfFont with a new id
     *  \param pFont pointer to existing font
     *  \param pMetrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is 
     *         deleted along with the font.
     *  \param pszSuffix Suffix to add to font-id 
     *  \param pParent parent of the font object
     */
    PdfFontType1( PdfFontType1* pFont, PdfFontMetrics* pMetrics, const char *pszSuffix, PdfVecObjects* pParent );

    void InitBase14Font();

  protected:

  	/** Remember the glyphs used in the string in case of subsetting 
	 *
     *  \param sText the text string which should be printed (is not allowed to be NULL!)
     *  \param lStringLen draw only lLen characters of pszText
	 */
	virtual void AddUsedSubsettingGlyphs( const PdfString & sText, long lStringLen );

  	/** Remember the glyphname in case of subsetting 
	 *
     *  \param sGlyphName Name of the glyph to remember
     */
    virtual void AddUsedGlyphname( const char* sGlyphName );

    /** Embeds pending subset-font into PDF page
     *
     */
    virtual void EmbedSubsetFont();

    /** Embed the font file directly into the PDF file.
     *
     *  \param pDescriptor font descriptor object
     */
    virtual void EmbedFontFile( PdfObject* pDescriptor );

 private:

	bool FindSeac( const unsigned char * buffer, int length );

    pdf_long FindInBuffer( const char* pszNeedle, const char* pszHaystack, pdf_long lLen ) const;

	int m_bUsed[8];		// bitmask for usage if char 00..ff

	std::set<std::string>	m_sUsedGlyph;	// array for special chars

};

/** Helper Class needed for parsing type1-font for subsetting
 */
class PdfType1Encrypt
{
public:
    /** Create a new PdfTypeEncrypt object.
     *
     */
	PdfType1Encrypt();
	
    /** Encrypts a character
     *
     *  \param plain the character to encrypt.
     *
     *  \return encrypted cipher
     *
     */
	unsigned char Encrypt( unsigned char plain );

    /** Decrypts a character
     *
     *  \param cipher the cipher to decrypt.
     *
     *  \return decrypted character
     *
     */
	unsigned char Decrypt( unsigned char cipher );
	
protected:
	unsigned short int m_r;
	unsigned short int m_c1;
	unsigned short int m_c2;
};

class PdfType1EncryptEexec : public PdfType1Encrypt
{
public:
    /** Create a new PdfType1EncryptEexec object.
     *
     */
    PdfType1EncryptEexec();
};

class PdfType1EncryptCharstring : public PdfType1Encrypt
{
public:
    /** Create a new PdfType1EncryptCharstring object.
     *
     */
	PdfType1EncryptCharstring();
};

};

#endif // _PDF_FONT_TYPE1_H_


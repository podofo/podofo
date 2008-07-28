/***************************************************************************
 *   Copyright (C) 2008 by Dominik Seichter                                *
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

#ifndef _PDF_FONT_TTF_SUBSET_H_
#define _PDF_FONT_TTF_SUBSET_H_

#include "PdfDefines.h"
#include "PdfFontMetrics.h"

#include <string>
#include <vector>

namespace PoDoFo {

class PdfInputDevice;
class PdfOutputDevice;

/**
 * This class is able to build a new TTF font with only
 * certain glyphs from an existing font.
 *
 *
 * It based heabily on code by ZhangYang (张杨.国际) <zhang_yang@founder.com>
 */
class PODOFO_API PdfFontTTFSubset {
 public:
    enum EFontFileType {
	eFontFileType_TTF,    ///< TrueType Font
	eFontFileType_TTC,    ///< TrueType Collection
	eFontFileType_OTF,    ///< OpenType Font
	eFontFileType_Unknown ///< Unknown
    };


    /** Create a new PdfFontTTFSubset from an existing 
     *  TTF font file.
     *
     *  @param pszFontFileName path to a TTF file
     *  @param pMetrics font metrics object for this font
     *  @param nFaceIndex index of the face inside of the font
     */
    PdfFontTTFSubset( const char* pszFontFileName, PdfFontMetrics* pMetrics, unsigned short nFaceIndex = 0 );

    /** Create a new PdfFontTTFSubset from an existing 
     *  TTF font file using an input device.
     *
     *  @param pDevice a PdfInputDevice
     *  @param pMetrics font metrics object for this font
     *  @param eType the type of the font
     *  @param nFaceIndex index of the face inside of the font
     */
    PdfFontTTFSubset( PdfInputDevice* pDevice, PdfFontMetrics* pMetrics, EFontFileType eType, unsigned short nFaceIndex = 0 );

    ~PdfFontTTFSubset();

    /**
     * Actually generate the subsetted font
     *
     * @param pOutputDevice write the font to this device
     */
    void BuildFont( PdfOutputDevice* pOutputDevice ); 

    /** Add a new glyph index to the subset.
     *
     *  @param nGlyphIndex include this glyph in the final font
     *
     *  Already existing values are ignored, so that every
     *  glyph can be added only once.
     */
    void AddGlyph( unsigned short nGlyphIndex );

    /** Add an unicode character to the subset
     *
     *  @param nCharCode unicode character code
     */
    inline void AddCharacter( pdf_utf16be nCharCode );

    /** Get the number of glyphs in this font.
     *
     *  @returns the number of glyphs in this font.
     */
    inline unsigned long GetSize() const;

 private:
    /** Hide default constructor
     */
    PdfFontTTFSubset() : m_bOwnDevice( false ) {} 

    void Init();
    
    /** Get the offset of a specified table. 
     *  @param pszTableName name of the table
     */
    unsigned long GetTableOffset( const char* pszTableName );

    void GetNumberOfTables();
    void GetNumberOfGlyphs();
    void SeeIfLongLocaOrNot();
    void InitTables();
    void GetStartOfTTFOffsets();

    /** Get sz bytes from the offset'th bytes of the input file
     *
     */
    void GetData(unsigned long offset, void* address, unsigned long sz);

 private:
    /** Information of TrueType tables.
     */
    struct TTrueTypeTable {
	unsigned char m_tableName[4];
	unsigned long m_checksum;
	unsigned long m_length;
	unsigned long m_offset;
	std::string  m_strTableName; // TODO: DS: REMOVE FIELD?
    };

    /** GlyphData contains the glyph address relative 
     *  to the beginning of the glyf table.
     */
    struct TGlyphData {
	unsigned long glyphIndex;
	unsigned long glyphLength;
	unsigned long glyphOldAddress;	//In the original truetype file.
	unsigned long glyphNewAddress;	//In the new generated truetype file.
    };

    struct TGlyphDataShort {
	unsigned long	glyphIndex;
	unsigned short	glyphLength;
	unsigned short	glyphOldAddress;
	unsigned long	glyphOldAddressLong;	//Real address, which is 2 times of glyphOldAddress.
	unsigned short	glyphNewAddress;
	unsigned long	glyphNewAddressLong;
    };

    PdfFontMetrics* m_pMetrics;                ///< FontMetrics object which is required to convert unicode character points to glyph ids
    EFontFileType   m_eFontFileType;
    bool	    m_bIsLongLoca;
    
    unsigned short  m_numTables;
    unsigned short  m_numGlyphs;
    
    std::vector<TTrueTypeTable> m_vTable;
    std::vector<unsigned short> m_vGlyphIndice;

    unsigned short  m_faceIndex;

    unsigned long   m_ulStartOfTTFOffsets;	///< Start address of the truetype offset tables, differs from ttf to ttc.

    PdfInputDevice* m_pDevice;                  ///< Read data from this input device
    const bool      m_bOwnDevice;               ///< If the input device is owned by this object
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline unsigned long PdfFontTTFSubset::GetSize() const 
{
    return m_vGlyphIndice.size();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfFontTTFSubset::AddCharacter( pdf_utf16be nCharCode )
{
#ifdef PODOFO_IS_LITTLE_ENDIAN
    this->AddGlyph( m_pMetrics->GetGlyphId( ((nCharCode & 0xff00) >> 8) | ((nCharCode & 0xff) << 8) ) );
#else
    this->AddGlyph( m_pMetrics->GetGlyphId( nCharCode ) );
#endif // PODOFO_IS_LITTLE_ENDIAN
}

}; /* PoDoFo */

#endif /* _PDF_FONT_TRUE_TYPE_H_ */

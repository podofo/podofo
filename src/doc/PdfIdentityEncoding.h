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
 ***************************************************************************/

#ifndef _PDF_IDENTITY_ENCODING_H_
#define _PDF_IDENTITY_ENCODING_H_

#include "base/PdfDefines.h"
#include "base/PdfEncoding.h"

namespace PoDoFo {

/** PdfIdentityEncoding is a two-byte encoding which can be
 *  used with TrueType fonts to represent all characters
 *  present in a font. If the font contains all unicode
 *  glyphs, PdfIdentityEncoding will support all unicode
 *  characters.
 */
class PODOFO_API PdfIdentityEncoding : public PdfEncoding {
 public:
    /** 
     *  Create a new PdfIdentityEncoding.
     *
     *  \param nFirstChar the first supported unicode character code (at least 0) 
     *  \param nLastChar the last supported unicode character code, 
     *                   must be larger than nFirstChar (max value is 0xffff) 
     *  \param bAutoDelete if true the encoding is deleted by its owning font
     */
    PdfIdentityEncoding( int nFirstChar = 0, int nLastChar = 0xffff, bool bAutoDelete = true );

    /** Add this encoding object to a dictionary
     *  usually be adding an /Encoding key in font dictionaries.
     *
     *  \param rDictionary add the encoding to this dictionary
     */
    virtual void AddToDictionary( PdfDictionary & rDictionary ) const;

    /** Convert a string that is encoded with this encoding
     *  to an unicode PdfString.
     *
     *  \param rEncodedString a string encoded by this encoding. 
     *         Usually this string was read from a content stream.
     *  \param pFont the font for which this string is converted
     *
     *  \returns an unicode PdfString.
     */
    virtual PdfString ConvertToUnicode( const PdfString & rEncodedString, const PdfFont* pFont ) const;

    /** Convert a unicode PdfString to a string encoded with this encoding.
     *
     *  \param an unicode PdfString.
     *  \param pFont the font for which this string is converted
     *
     *  \returns an encoded PdfRefCountedBuffer. The PdfRefCountedBuffer is treated as a series of bytes
     *           and is allowed to have 0 bytes. The returned buffer must not be a unicode string.
     */
    virtual PdfRefCountedBuffer ConvertToEncoding( const PdfString & rString, const PdfFont* pFont ) const;

    /** 
     * PdfIdentityEncoding is usually delete along with the font.
     *
     * \returns true if this encoding should be deleted automatically with the
     *          font.
     */
    virtual bool IsAutoDelete() const;

    /** 
     *  \returns true if this is a single byte encoding with a maximum of 256 values.
     */
    virtual bool IsSingleByteEncoding() const;

    /** Get the unicode character code for this encoding
     *  at the position nIndex. nIndex is a position between
     *  GetFirstChar() and GetLastChar()
     *
     *  \param nIndex character code at position index
     *  \returns unicode character code 
     * 
     *  \see GetFirstChar 
     *  \see GetLastChar
     *
     *  Will throw an exception if nIndex is out of range.
     */
    virtual pdf_utf16be GetCharCode( int nIndex ) const;

 protected:
    /** Get a unique ID for this encoding
     *  which can used for comparisons!
     *
     *  \returns a unique id for this encoding!
     */
    inline virtual const PdfName & GetID() const;
    
 private:

    /** Gets the unicode value from a char code in this font
     *
     *  \param lCharCode the character code (i.e. glyph id)
     *
     *  \returns an unicode value
     */
    pdf_utf16be GetUnicodeValue( long lCharCode ) const;
 
 private:
    bool    m_bAutoDelete;      ///< If true this encoding is deleted by its font.
    PdfName m_id;               ///< Unique ID of this encoding 
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const PdfName & PdfIdentityEncoding::GetID() const
{
    return m_id;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfIdentityEncoding::IsAutoDelete() const
{
    return m_bAutoDelete;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfIdentityEncoding::IsSingleByteEncoding() const
{
    return false;
}

};  /* namespace PoDoFo */

#endif // _PDF_IDENTITY_ENCODING_H_

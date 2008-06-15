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
 ***************************************************************************/

#ifndef _PDF_ENCODING_H_
#define _PDF_ENCODING_H_

#include "PdfDefines.h"
#include "PdfName.h"
#include "PdfString.h"

namespace PoDoFo {

class PdfDictionary;
class PdfFont;

/** 
 * A PdfEncoding is in PdfFont to transform a text string
 * into a representation so that it can be displayed in a
 * PDF file.
 *
 * PdfEncoding can also be used to convert strings from a
 * PDF file back into a PdfString.
 */
class PODOFO_API PdfEncoding {
 protected:

    /** 
     *  Create a new PdfEncoding.
     *
     *  \param nFirstChar the first supported character code 
     *                    (either a byte value in the current encoding or a unicode value)
     *  \param nLastChar the last supported character code, must be larger than nFirstChar 
     *                    (either a byte value in the current encoding or a unicode value)
     *
     */
    PdfEncoding( int nFirstChar, int nLastChar );

    /** Get a unique ID for this encoding
     *  which can used for comparisons!
     *
     *  \returns a unique id for this encoding!
     */
    virtual const PdfName & GetID() const = 0;

 public:
    
    virtual ~PdfEncoding();

    /** Comparison operator.
     *
     *  \param rhs the PdfEncoding to which this encoding should be compared
     *
     *  \returns true if both encodings are the same.
     */
    inline bool operator==( const PdfEncoding & rhs ) const;

    /** Comparison operator.
     *
     *  \param rhs the PdfEncoding to which this encoding should be compared
     *
     *  \returns true if this encoding is less than the specified.
     */
    inline bool operator<( const PdfEncoding & rhs ) const;

    /** Add this encoding object to a dictionary
     *  usually be adding an /Encoding key in font dictionaries.
     *
     *  \param rDictionary add the encoding to this dictionary
     */
    virtual void AddToDictionary( PdfDictionary & rDictionary ) const = 0;

    /** Convert a string that is encoded with this encoding
     *  to an unicode PdfString.
     *
     *  \param rEncodedString a string encoded by this encoding. 
     *         Usually this string was read from a content stream.
     *  \param pFont the font for which this string is converted
     *
     *  \returns an unicode PdfString.
     */
    virtual PdfString ConvertToUnicode( const PdfString & rEncodedString, const PdfFont* pFont ) const = 0;

    /** Convert a unicode PdfString to a string encoded with this encoding.
     *
     *  \param an unicode PdfString.
     *  \param pFont the font for which this string is converted
     *
     *  \returns an encoded PdfString. The PdfString is treated as a series of bytes
     *           and is allowed to have 0 bytes. The returned string must not be a unicode string.
     */
    virtual PdfString ConvertToEncoding( const PdfString & rString, const PdfFont* pFont ) const = 0;

    /** 
     * \returns true if this encoding should be deleted automatically with the
     *          font.
     */
    virtual bool IsAutoDelete() const = 0;

    /** 
     *  \returns true if this is a single byte encoding with a maximum of 256 values.
     */
    virtual bool IsSingleByteEncoding() const = 0;

    /** 
     * \returns the first character code that is defined for this encoding
     */
    inline int GetFirstChar() const;

    /** 
     * \returns the last character code that is defined for this encoding
     */
    inline int GetLastChar() const;


 private:
    int     m_nFirstChar;   ///< The first defined character code
    int     m_nLastChar;    ///< The last defined character code
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfEncoding::operator<( const PdfEncoding & rhs ) const
{
    return (this->GetID() < rhs.GetID());
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfEncoding::operator==( const PdfEncoding & rhs ) const
{
    return (this->GetID() == rhs.GetID());
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline int PdfEncoding::GetFirstChar() const
{
    return m_nFirstChar;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline int PdfEncoding::GetLastChar() const
{
    return m_nLastChar;
}

/**
 * A common base class for standard PdfEncoding which are
 * known by name.
 *
 *  - MacRomanEncoding
 *  - WinAnsiEncoding
 *  - MacExpertEncoding
 *
 *  \see PdfWinAnsiEncoding
 *  \see PdfMacRomanEncoding
 *  \see PdfMacExportEncoding
 *
 */
class PODOFO_API PdfSimpleEncoding : public PdfEncoding {
 public:

    /*
     *  Create a new simple PdfEncoding which uses 1 byte.
     *
     *  \param rName the name of a standard PdfEncoding
     *
     *  As of now possible values for rName are:
     *  - MacRomanEncoding
     *  - WinAnsiEncoding
     *  - MacExpertEncoding
     *
     *  \see PdfWinAnsiEncoding
     *  \see PdfMacRomanEncoding
     *  \see PdfMacExportEncoding
     *
     *  This will allocate a table of 65535 short values
     *  to make conversion from unicode to encoded strings
     *  faster. As this requires a lot of memory, make sure that
     *  only one object of a certain encoding exists at one
     *  time, which is no problem as all methods are const anyways!
     *
     */
    PdfSimpleEncoding( const PdfName & rName );

    ~PdfSimpleEncoding();

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
     *  \returns an encoded PdfString. The PdfString is treated as a series of bytes
     *           and is allowed to have 0 bytes. The returned string must not be a unicode string.
     */
    virtual PdfString ConvertToEncoding( const PdfString & rString, const PdfFont* pFont ) const;

    /** 
     * PdfSimpleEncoding subclasses are usuylla not auto-deleted, as
     * they are allocated statically only once.
     *
     * \returns true if this encoding should be deleted automatically with the
     *          font.
     *
     * \see PdfFont::WinAnsiEncoding
     * \see PdfFont::MacRomanEncoding
     */
    virtual bool IsAutoDelete() const;

    /** 
     *  \returns true if this is a single byte encoding with a maximum of 256 values.
     */
    inline virtual bool IsSingleByteEncoding() const;

    /** Get the name of this encoding.
     *  
     *  \returns the name of this encoding.
     */
    inline const PdfName & GetName() const;

 private:
    /** Initialize the internal table of mappings from unicode code points
     *  to encoded byte values.
     */
    void InitEncodingTable();

 protected:

    /** Get a unique ID for this encoding
     *  which can used for comparisons!
     *
     *  \returns a unique id for this encoding!
     */
    inline virtual const PdfName & GetID() const;

    /** Gets a table of 256 short values which are the 
     *  big endian unicode code points that are assigned
     *  to the 256 values of this encoding.
     *
     *  This table is used internally to convert an encoded
     *  string of this encoding to and from unicode.
     *
     *  \returns an array of 256 big endian unicode code points
     */
    virtual const pdf_utf16be* GetToUnicodeTable() const = 0;

 private:
    PdfName m_name;           ///< The name of the encoding
    char*   m_pEncodingTable; ///< The helper table for conversions into this encoding
}; 

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const PdfName & PdfSimpleEncoding::GetID() const
{
    return m_name;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfSimpleEncoding::IsAutoDelete() const
{
    return false;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfSimpleEncoding::IsSingleByteEncoding() const
{
    return true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const PdfName & PdfSimpleEncoding::GetName() const
{
    return m_name;
}


/** 
 * The WinAnsi Encoding is the default encoding in PoDoFo for 
 * contents on PDF pages.
 *
 * It is also called CP-1252 encoding.
 *
 * Do not allocate this class yourself, as allocations
 * might be expensive. Try using PdfFont::WinAnsiEncoding.
 *
 * \see PdfFont::WinAnsiEncoding
 */
class PODOFO_API PdfWinAnsiEncoding : public PdfSimpleEncoding {
 public:
   
    /** Create a new PdfWinAnsiEncoding
     */
    inline PdfWinAnsiEncoding()
        : PdfSimpleEncoding( PdfName("WinAnsiEncoding") )
    {

    }

 protected:

    /** Gets a table of 256 short values which are the 
     *  big endian unicode code points that are assigned
     *  to the 256 values of this encoding.
     *
     *  This table is used internally to convert an encoded
     *  string of this encoding to and from unicode.
     *
     *  \returns an array of 256 big endian unicode code points
     */
    inline virtual const pdf_utf16be* GetToUnicodeTable() const;

 private:
    static const pdf_utf16be s_cEncoding[256]; ///< conversion table from WinAnsiEncoding to UTF16

};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const pdf_utf16be* PdfWinAnsiEncoding::GetToUnicodeTable() const
{
    return PdfWinAnsiEncoding::s_cEncoding;
}

/** 
 * Do not allocate this class yourself, as allocations
 * might be expensive. Try using PdfFont::MacRomanEncoding.
 *
 * \see PdfFont::MacRomanEncoding
 */
class PODOFO_API PdfMacRomanEncoding : public PdfSimpleEncoding {
 public:
   
    /** Create a new PdfMacRomanEncoding
     */
    inline PdfMacRomanEncoding()
        : PdfSimpleEncoding( PdfName("MacRomanEncoding") )
    {

    }

 protected:

    /** Gets a table of 256 short values which are the 
     *  big endian unicode code points that are assigned
     *  to the 256 values of this encoding.
     *
     *  This table is used internally to convert an encoded
     *  string of this encoding to and from unicode.
     *
     *  \returns an array of 256 big endian unicode code points
     */
    inline virtual const pdf_utf16be* GetToUnicodeTable() const;

 private:
    static const pdf_utf16be s_cEncoding[256]; ///< conversion table from WinAnsiEncoding to UTF16

};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const pdf_utf16be* PdfMacRomanEncoding::GetToUnicodeTable() const
{
    return PdfMacRomanEncoding::s_cEncoding;
}

/** 
 */
class PODOFO_API PdfMacExpertEncoding : public PdfSimpleEncoding {
 public:
   
    /** Create a new PdfMacExpertEncoding
     */
    inline PdfMacExpertEncoding()
        : PdfSimpleEncoding( PdfName("MacExpertEncoding") )
    {

    }

};

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
     *  \returns an encoded PdfString. The PdfString is treated as a series of bytes
     *           and is allowed to have 0 bytes. The returned string must not be a unicode string.
     */
    virtual PdfString ConvertToEncoding( const PdfString & rString, const PdfFont* pFont ) const;

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

 protected:
    /** Get a unique ID for this encoding
     *  which can used for comparisons!
     *
     *  \returns a unique id for this encoding!
     */
    inline virtual const PdfName & GetID() const;
    
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

}; /* namespace PoDoFo */


#endif // _PDF_ENCODING_H_


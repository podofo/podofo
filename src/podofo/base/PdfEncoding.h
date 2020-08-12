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

#ifndef _PDF_ENCODING_H_
#define _PDF_ENCODING_H_

#include "PdfDefines.h"
#include "PdfName.h"
#include "PdfString.h"
#include "util/PdfMutex.h"

#include <iterator>

namespace PoDoFo {

class PdfDictionary;
class PdfFont;
class PdfObject;

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
    PdfEncoding( int nFirstChar, int nLastChar, PdfObject* = NULL );

    /** Get a unique ID for this encoding
     *  which can used for comparisons!
     *
     *  \returns a unique id for this encoding!
     */
    virtual const PdfName & GetID() const = 0;

 public:
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200			// ab Visualstudio 6
    class PODOFO_API const_iterator : public std::iterator<
                                             std::forward_iterator_tag, 
						 int, ptrdiff_t> {
#else
    class PODOFO_API const_iterator : public std::iterator<
                                             std::forward_iterator_tag, 
						 int, std::ptrdiff_t, 
						 const int *, const int &> {
#endif
    public:
	const_iterator( const PdfEncoding* pEncoding, int nCur )
	    : m_pEncoding( pEncoding ), m_nCur( nCur )
	{
	}

	const_iterator( const const_iterator & rhs ) 
	{
	    this->operator=(rhs);
	}

	const const_iterator & operator=( const const_iterator & rhs ) 
	{
	    m_nCur      = rhs.m_nCur;
	    m_pEncoding = rhs.m_pEncoding;

	    return *this;
	}

	inline bool operator==( const const_iterator & rhs ) const
	{
	    return (m_nCur == rhs.m_nCur);
	}

	inline bool operator!=( const const_iterator & rhs ) const
	{
	    return (m_nCur != rhs.m_nCur);
	}

	inline pdf_utf16be operator*() const 
	{
	    return m_pEncoding->GetCharCode( m_nCur );
	}
	
	inline const_iterator & operator++()
	{
	    m_nCur++;

	    return *this;
	}

    private:
	const PdfEncoding* m_pEncoding;
	int                m_nCur;
    };

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
    virtual PdfString ConvertToUnicode( const PdfString & rEncodedString, const PdfFont* pFont ) const;

    /** Convert a unicode PdfString to a string encoded with this encoding.
     *
     *  \param rString an unicode PdfString.
     *  \param pFont the font for which this string is converted
     *
     *  \returns an encoded PdfRefCountedBuffer. The PdfRefCountedBuffer is treated as a series of bytes
     *           and is allowed to have 0 bytes. The returned buffer must not be a unicode string.
     */
    virtual PdfRefCountedBuffer ConvertToEncoding( const PdfString & rString, const PdfFont* pFont ) const;

    virtual bool IsAutoDelete() const = 0;

    virtual bool IsSingleByteEncoding() const = 0;

    /** 
     * \returns the first character code that is defined for this encoding
     */
    inline int GetFirstChar() const;

    /** 
     * \returns the last character code that is defined for this encoding
     */
    inline int GetLastChar() const;

    /** Iterate over all unicode character points in this
     *  encoding, beginning with the first.
     *
     *  \returns iterator pointing to the first defined unicode character
     */
    inline const_iterator begin() const;

    /** Iterate over all unicode character points in this
     *  encoding, beginning with the first.
     *
     *  \returns iterator pointing at the end
     */
    inline const_iterator end() const;

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
    virtual pdf_utf16be GetCharCode( int nIndex ) const = 0;

 protected:
    bool m_bToUnicodeIsLoaded;  ///< If true, ToUnicode has been parse
                             
 private:
    int     m_nFirstChar;   ///< The first defined character code
    int     m_nLastChar;    ///< The last defined character code
    const PdfObject* m_pToUnicode;    ///< Pointer to /ToUnicode object, if any
 protected:
    std::map<pdf_utf16be, pdf_utf16be> m_toUnicode;
               
    pdf_utf16be GetUnicodeValue( pdf_utf16be ) const;
 private:
    
    /** Parse the /ToUnicode object
    */
    void ParseToUnicode();
    pdf_utf16be GetCIDValue( pdf_utf16be ) const;
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

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfEncoding::const_iterator PdfEncoding::begin() const
{
    return PdfEncoding::const_iterator( this, this->GetFirstChar() );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfEncoding::const_iterator PdfEncoding::end() const
{
    return PdfEncoding::const_iterator( this, this->GetLastChar() + 1 );
}

/**
 * A common base class for standard PdfEncoding which are
 * known by name.
 *
 *  - PdfDocEncoding (only use this for strings which are not printed 
 *                    in the document. This is for meta data in the PDF).
 *  - MacRomanEncoding
 *  - WinAnsiEncoding
 *  - MacExpertEncoding
 *  - StandardEncoding
 *  - SymbolEncoding
 *  - ZapfDingbatsEncoding
 *
 *  \see PdfWinAnsiEncoding
 *  \see PdfMacRomanEncoding
 *  \see PdfMacExportEncoding
 *..\see PdfStandardEncoding
 *  \see PdfSymbolEncoding
 *  \see PdfZapfDingbatsEncoding
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
     *  - StandardEncoding
     *  - SymbolEncoding
     *  - ZapfDingbatsEncoding
     *
     *  \see PdfWinAnsiEncoding
     *  \see PdfMacRomanEncoding
     *  \see PdfMacExportEncoding
     *  \see PdfStandardEncoding
     *  \see PdfSymbolEncoding
     *  \see PdfZapfDingbatsEncoding
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
     *  \param rString an unicode PdfString.
     *  \param pFont the font for which this string is converted
     *
     *  \returns an encoded PdfRefCountedBuffer. The PdfRefCountedBuffer is treated as a series of bytes
     *           and is allowed to have 0 bytes. The returned buffer must not be a unicode string.
     */
    virtual PdfRefCountedBuffer ConvertToEncoding( const PdfString & rString, const PdfFont* pFont ) const;

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

    char GetUnicodeCharCode(pdf_utf16be unicodeValue) const;

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

 protected:
    Util::PdfMutex * m_mutex;   ///< Mutex for the creation of the encoding table
    
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
 * The PdfDocEncoding is the default encoding for
 * all strings in PoDoFo which are data in the PDF
 * file.
 *
 * Do not allocate this class yourself, as allocations
 * might be expensive. Try using PdfFont::DocEncoding.
 *
 * \see PdfFont::DocEncoding
 */
class PODOFO_API PdfDocEncoding : public PdfSimpleEncoding {
 public:
   
    /** Create a new PdfDocEncoding
     */
    PdfDocEncoding()
        : PdfSimpleEncoding( PdfName("PdfDocEncoding") )
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
    virtual const pdf_utf16be* GetToUnicodeTable() const;

 private:
    static const pdf_utf16be s_cEncoding[256]; ///< conversion table from DocEncoding to UTF16

};

/** 
 * The WinAnsi Encoding is the default encoding in PoDoFo for 
 * contents on PDF pages.
 *
 * It is also called CP-1252 encoding.
 * This class may be used as base for derived encodings.
 *
 * \see PdfWin1250Encoding
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
    PdfWinAnsiEncoding()
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
    virtual const pdf_utf16be* GetToUnicodeTable() const;

    /** Add this encoding object to a dictionary
     *  usually be adding an /Encoding key in font dictionaries.
     *  
     *  This method generates array of differences into /Encoding
     *  dictionary if called from derived class with
     *  different unicode table.
     *
     *  \param rDictionary add the encoding to this dictionary
     */
    virtual void AddToDictionary( PdfDictionary & rDictionary ) const;

 private:
    static const pdf_utf16be s_cEncoding[256]; ///< conversion table from WinAnsiEncoding to UTF16

};

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
    PdfMacRomanEncoding()
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
    virtual const pdf_utf16be* GetToUnicodeTable() const;

 private:
    static const pdf_utf16be s_cEncoding[256]; ///< conversion table from MacRomanEncoding to UTF16

};

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
    virtual const pdf_utf16be* GetToUnicodeTable() const;

 private:
    static const pdf_utf16be s_cEncoding[256]; ///< conversion table from MacExpertEncoding to UTF16

};

// OC 13.08.2010 Neu: StandardEncoding
/** 
 * Do not allocate this class yourself, as allocations
 * might be expensive. Try using PdfFont::StandardEncoding.
 *
 * \see PdfFont::StandardEncoding
 */
class PODOFO_API PdfStandardEncoding : public PdfSimpleEncoding {
 public:
   
    /** Create a new PdfStandardEncoding
     */
    PdfStandardEncoding()
        : PdfSimpleEncoding( PdfName("StandardEncoding") )
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
    virtual const pdf_utf16be* GetToUnicodeTable() const;

 private:
    static const pdf_utf16be s_cEncoding[256]; ///< conversion table from StandardEncoding to UTF16

};

// OC 13.08.2010 Neu: SymbolEncoding
/** 
 * Do not allocate this class yourself, as allocations
 * might be expensive. Try using PdfFont::SymbolEncoding.
 *
 * \see PdfFont::SymbolEncoding
 */
class PODOFO_API PdfSymbolEncoding : public PdfSimpleEncoding {
 public:
   
    /** Create a new PdfSymbolEncoding
     */
    PdfSymbolEncoding()
        : PdfSimpleEncoding( PdfName("SymbolEncoding") )
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
    virtual const pdf_utf16be* GetToUnicodeTable() const;

 private:
    static const pdf_utf16be s_cEncoding[256]; ///< conversion table from SymbolEncoding to UTF16

};

// OC 13.08.2010 Neu: ZapfDingbatsEncoding
/** 
 * Do not allocate this class yourself, as allocations
 * might be expensive. Try using PdfFont::ZapfDingbats.
 *
 * \see PdfFont::ZapfDingbatsEncoding
 */
class PODOFO_API PdfZapfDingbatsEncoding : public PdfSimpleEncoding {
 public:
   
    /** Create a new PdfZapfDingbatsEncoding
     */
    PdfZapfDingbatsEncoding()
        : PdfSimpleEncoding( PdfName("ZapfDingbatsEncoding") )
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
    virtual const pdf_utf16be* GetToUnicodeTable() const;

 private:
    static const pdf_utf16be s_cEncoding[256]; ///< conversion table from ZapfDingbatsEncoding to UTF16

};

/**
* WINDOWS-1250 encoding
*/
class PODOFO_API PdfWin1250Encoding : public PdfWinAnsiEncoding
{
 public:
   
    /** Create a new PdfWin1250Encoding
     */
    PdfWin1250Encoding()
    {
        m_id = "Win1250Encoding";
    }

 protected:

    virtual const PdfName & GetID() const
    {
        return m_id;
    }

    /** Gets a table of 256 short values which are the 
     *  big endian unicode code points that are assigned
     *  to the 256 values of this encoding.
     *
     *  This table is used internally to convert an encoded
     *  string of this encoding to and from unicode.
     *
     *  \returns an array of 256 big endian unicode code points
     */
    virtual const pdf_utf16be* GetToUnicodeTable() const;

 private:
    static const pdf_utf16be s_cEncoding[256]; ///< conversion table from Win1250Encoding to UTF16
    PdfName m_id;
};

/**
* ISO-8859-2 encoding
*/
class PODOFO_API PdfIso88592Encoding : public PdfWinAnsiEncoding
{
 public:
   
    /** Create a new PdfIso88592Encoding
     */
    PdfIso88592Encoding()
    {
        m_id = "Iso88592Encoding";
    }

 protected:

    virtual const PdfName & GetID() const
    {
        return m_id;
    }

    /** Gets a table of 256 short values which are the 
     *  big endian unicode code points that are assigned
     *  to the 256 values of this encoding.
     *
     *  This table is used internally to convert an encoded
     *  string of this encoding to and from unicode.
     *
     *  \returns an array of 256 big endian unicode code points
     */
    virtual const pdf_utf16be* GetToUnicodeTable() const;

 private:
    static const pdf_utf16be s_cEncoding[256]; ///< conversion table from Iso88592Encoding to UTF16
    PdfName m_id;
};

}; /* namespace PoDoFo */


#endif // _PDF_ENCODING_H_


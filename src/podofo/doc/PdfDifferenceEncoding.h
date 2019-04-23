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

#ifndef _PDF_DIFFERENCE_ENCODING_H_
#define _PDF_DIFFERENCE_ENCODING_H_

#include "podofo/base/PdfDefines.h"
#include "podofo/base/PdfEncoding.h"
#include "PdfElement.h"

#include <iterator>

namespace PoDoFo {


/** A helper class for PdfDifferenceEncoding that
 *  can be used to create a differences array.
 */
class PODOFO_DOC_API PdfEncodingDifference {
    struct TDifference {
        int         nCode;
        PdfName     name;
        pdf_utf16be unicodeValue;
    };

    typedef std::vector<TDifference>                 TVecDifferences;
    typedef std::vector<TDifference>::iterator       TIVecDifferences;
    typedef std::vector<TDifference>::const_iterator TCIVecDifferences;

 public: 
    /** Create a PdfEncodingDifference object.
     */
    PdfEncodingDifference();

    /** Copy a PdfEncodingDifference object.
     */
    PdfEncodingDifference( const PdfEncodingDifference & rhs );

    /** Copy a PdfEncodingDifference object.
     */
    const PdfEncodingDifference & operator=( const PdfEncodingDifference & rhs );

    /** Add a difference to the object.
     * 
     *  \param nCode unicode code point of the difference (0 to 255 are legal values)
     *  \param unicodeValue actual unicode value for nCode; can be 0
     *
     *  \see AddDifference if you know the name of the code point
     *       use the overload below which is faster
     */
    void AddDifference( int nCode, pdf_utf16be unicodeValue );

    /** Add a difference to the object.
     * 
     *  \param nCode unicode code point of the difference (0 to 255 are legal values)
     *  \param unicodeValue actual unicode value for nCode; can be 0
     *  \param rName name of the different code point or .notdef if none
     *  \param bExplicitKeys if true, the unicode value is set to nCode as rName is meaningless (Type3 fonts)
     */
    void AddDifference( int nCode, pdf_utf16be unicodeValue, const PdfName & rName, bool bExplicitNames = false );

    /** Tests if the specified code is part of the 
     *  differences.
     *
     *  \param nCode test if the given code is part of the differences
     *  \param rName write the associated name into this object if the 
     *               code is part of the difference
     *  \param rValue write the associated unicode value of the name to this value 
     *
     *  \returns true if the code is part of the difference
     */
    bool Contains( int nCode, PdfName & rName, pdf_utf16be & rValue ) const;

    bool ContainsUnicodeValue( pdf_utf16be unicodeValue, char &rValue ) const;

    /** Convert the PdfEncodingDifference to an array
     *
     *  \param rArray write to this array
     */
    void ToArray( PdfArray & rArray );

    /** Get the number of differences in this object.
     *  If the user added .notdef as a difference it is 
     *  counted, even it is no real difference in the final encoding.
     *  
     *  \returns the number of differences in this object
     */
    inline size_t GetCount() const;

 private:
    struct DifferenceComparatorPredicate {
        public:
          inline bool operator()( const TDifference & rDif1, 
                                  const TDifference & rDif2 ) const { 
              return rDif1.nCode < rDif2.nCode;
          }
    };

    TVecDifferences m_vecDifferences;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline size_t PdfEncodingDifference::GetCount() const
{
    return m_vecDifferences.size();
}

/** PdfDifferenceEncoding is an encoding, which is based
 *  on either the fonts encoding or a predefined encoding
 *  and defines differences to this base encoding.
 */
class PODOFO_DOC_API PdfDifferenceEncoding : public PdfEncoding, private PdfElement {
 public:

    /**
     * Defines the base encoding from which a
     * PdfDifferenceEncoding differs.
     */
    enum EBaseEncoding {
        eBaseEncoding_Font,      ///< Use The fonts encoding as base
        eBaseEncoding_WinAnsi,   ///< Use WinAnsiEncoding as base encoding
        eBaseEncoding_MacRoman,  ///< Use MacRomanEncoding as base encoding
        eBaseEncoding_MacExpert  ///< Use MacExpertEncoding as base encoding
    };

    /** Create a new PdfDifferenceEncoding which is based on 
     *  the fonts encoding.
     *
     *  \param rDifference the differences in this encoding
     *  \param pParent parent PdfVecObjects.
     *                 Add a newly created object to this vector.
     *  \param bAutoDelete if true the encoding is deleted by its owning font
     */
    PdfDifferenceEncoding( const PdfEncodingDifference & rDifference, PdfDocument* pParent, bool bAutoDelete = true );

    /** Create a new PdfDifferenceEncoding which is based on 
     *  the fonts encoding.
     *
     *  \param rDifference the differences in this encoding
     *  \param pParent parent PdfDocument.
     *                 Add a newly created object to this vector.
     *  \param bAutoDelete if true the encoding is deleted by its owning font
     */
    PdfDifferenceEncoding( const PdfEncodingDifference & rDifference, PdfVecObjects* pParent, bool bAutoDelete = true );

    /** Create a new PdfDifferenceEncoding which is based on 
     *  a predefined encoding.
     *
     *  \param rDifference the differences in this encoding
     *  \param eBaseEncoding the base encoding of this font
     *  \param pParent parent PdfDocument.
     *                 Add a newly created object to this vector.
     *  \param bAutoDelete if true the encoding is deleted by its owning font
     */
    PdfDifferenceEncoding( const PdfEncodingDifference & rDifference, EBaseEncoding eBaseEncoding, 
                           PdfDocument* pParent, bool bAutoDelete = true );

    /** Create a new PdfDifferenceEncoding which is based on 
     *  a predefined encoding.
     *
     *  \param rDifference the differences in this encoding
     *  \param eBaseEncoding the base encoding of this font
     *  \param pParent parent PdfVecObjects.
     *                 Add a newly created object to this vector.
     *  \param bAutoDelete if true the encoding is deleted by its owning font
     */
    PdfDifferenceEncoding( const PdfEncodingDifference & rDifference, EBaseEncoding eBaseEncoding, 
                           PdfVecObjects* pParent, bool bAutoDelete = true );

    /** Create a new PdfDifferenceEncoding from an existing object
     *  in a PDF file.
     *
     *  \param pObject an existing differences encoding
     *  \param bAutoDelete if true the encoding is deleted by its owning font
     *  \param bExplicitNames if true, glyph names are meaningless explicit keys on the font (used for Type3 fonts)
     */
    PdfDifferenceEncoding( PdfObject* pObject, bool bAutoDelete = true, bool bExplicitNames = false );

    /** Convert a standard character name to a unicode code point
     * 
     *  \param rName a standard character name
     *  \returns an unicode code point
     */
    static pdf_utf16be NameToUnicodeID( const PdfName & rName );

    /** Convert an unicode code point to a standard character name
     * 
     *  \param inCodePoint a code point
     *  \returns a standard character name of /.notdef if none could be found
     */
    static PdfName UnicodeIDToName( pdf_utf16be inCodePoint );

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
     * \returns true if this encoding should be deleted automatically with the
     *          font.
     */
    virtual bool IsAutoDelete() const;

    /** 
     *  \returns true if this is a single byte encoding with a maximum of 256 values.
     */
    virtual bool IsSingleByteEncoding() const;

    /** 
     * Get read-only access to the object containing the actual
     * differences.
     *
     * \returns the container with the actual differences
     */
    inline const PdfEncodingDifference & GetDifferences() const;

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
    virtual const PdfName & GetID() const;

 private:
    /** Initialize this object
     */
    void Init();

    /** Create a unique ID for this encoding
     */
    void CreateID();

    /** Get an object of type baseencoding 
     * 
     *  \returns a base encoding
     */
    const PdfEncoding* GetBaseEncoding() const;

 private:
    PdfEncodingDifference m_differences;

    bool          m_bAutoDelete;  ///< If true this encoding is deleted by its font.
    PdfName       m_id;           ///< Unique ID of this encoding 
    EBaseEncoding m_baseEncoding; ///< The base encoding of this font 
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const PdfName & PdfDifferenceEncoding::GetID() const
{
    return m_id;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfDifferenceEncoding::IsAutoDelete() const
{
    return m_bAutoDelete;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfDifferenceEncoding::IsSingleByteEncoding() const
{
    return true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const PdfEncodingDifference & PdfDifferenceEncoding::GetDifferences() const
{
    return m_differences;
}


}; /* PoDoFo */

#endif // _PDF_DIFFERENCE_ENCODING_H_


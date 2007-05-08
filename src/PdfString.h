/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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

#ifndef _PDF_STRING_H_
#define _PDF_STRING_H_

#include "PdfDefines.h"
#include "PdfDataType.h"
#include "PdfRefCountedBuffer.h"

namespace PoDoFo {

#define PDF_STRING_BUFFER_SIZE 24

class PdfOutputDevice;

typedef enum EPdfStringConversion {
    ePdfStringConversion_Strict,
    ePdfStringConversion_Lenient
};


/** A string that can be written to a PDF document.
 *  If it contains binary data it is automatically 
 *  converted into a hex string, otherwise a normal PDF 
 *  string is written to the document.
 *
 *  PdfStrings representing text are either ISO Latin1
 *  encoded or UTF-16BE encoded.
 *
 *  PoDoFo contains methods to convert between these 
 *  encodings. For convinience conversion to UTF-8
 *  is possible to. Please not that strings are
 *  always stored as UTF-16BE or ISO Latin1 (PdfDocEncoding)
 *  in the PDF file.
 *
 *  UTF-16BE strings have to start with the byts 0xFE 0xFF
 *  to be recognized by PoDoFo as unicode strings.
 */
class PODOFO_API PdfString : public PdfDataType{
 public:

    /** Create an empty and invalid string 
     */
    PdfString();

   /** Construct a new PdfString from a std::string. 
    *  The input string will be copied.
    *  If the first to bytes of the string are 0xFE and 0xFF
    *  this string is treated as UTF-16BE encoded unicode string.
    *
    *  \param sString the string to copy
    */
    PdfString( const std::string& sString );

    /** Construct a new PdfString from a 0 terminated
     *  string. 
     *  The input string will be copied.
     *
     *  \param pszString the string to copy
     */
    PdfString( const char* pszString );

    /** Construct a new PdfString from a string. 
     *  The input string will be copied.
     *  If the first to bytes of the string are 0xFE and 0xFF
     *  this string is treated as UTF-16BE encoded unicode string.
     *
     *  \param pszString the string to copy
     *  \param lLen length of the string data to encode
     *  \param bHex if true the data will be 
     *              hex encoded and IsHex() will return true.
     */
    PdfString( const char* pszString, long lLen, bool bHex = false );

    /** Copy an existing PdfString 
     *  \param rhs another PdfString to copy
     */
    PdfString( const PdfString & rhs );

    ~PdfString();

    /** Set hex encoded data as the strings data. 
     *  \param pszHex must be hex encoded data.
     *  \param lLen   length of the hex encoded data.
     *                if lLen == -1 then strlen( pszHex ) will
     *                be used as length of the hex data. 
     *                pszHex has to be zero terminated in this case.
     */
    void SetHexData( const char* pszHex, long lLen = -1 );

    /** The string is valid if no error in the constructor has occurred.
     *  If it is valid it is safe to call all the other member functions.
     *  \returns true if this is a valid initialized PdfString
     */
    inline bool IsValid() const;

    /** Check if this is a hex string or a text string.
     *  
     *  If true the data returned by String() will be hexencoded
     *  and may contain '\\0' characters. You to decode the hex data
     *  by yourself.
     *
     *  \returns true if this is a hex string.
     *  \see String() \see PdfAlgorithm::HexDecode
     */
    inline bool IsHex () const;

    /**
     * PdfStrings are either Latin1 encoded or UTF-16BE
     * encoded unicode strings.
     * This functions returns true if this is a unicode
     * string object.
     *
     * \returns true if this is a unicode string.
     */
    inline bool IsUnicode () const;

    /** The contents of the strings can be read
     *  by this function.
     *  If IsHex() returns true the returned data is 
     *  hex encoded.
     *  if IsUnicode() returns true, the return value
     *  points to a UTF-16BE string buffer with Length()
     *  characters.
     * 
     *  \returns the strings contents
     *  \see IsHex
     *  \see IsUnicode
     *  \see Length
     */
    inline const char* GetString() const;

    /** The length of the string data returned by String()
     *  \returns the length of the string. 
     */
    inline long GetLength() const;

    /** The size of the string data returned by String()
     *  \returns the size of the string in bytes. Typically
     *           Length() + 1
     *  \see Length
     */
    inline long GetSize() const;
    
    /** Write this PdfString in PDF format to a PdfOutputDevice 
     *  \param pDevice the output device.
     */
    void Write ( PdfOutputDevice* pDevice ) const;

    /** Copy an existing PdfString 
     *  \param rhs another PdfString to copy
     *  \returns this object
     */
    const PdfString & operator=( const PdfString & rhs );

    /** Compare two PdfString objects
     *  \param rhs another PdfString to compare
     *  \returns this object
     */
    bool operator>( const PdfString & rhs ) const;

    /** Compare two PdfString objects
     *  \param rhs another PdfString to compare
     *  \returns this object
     */
    bool operator<( const PdfString & rhs ) const;

    /** Comparison operator
     *  \param rhs compare to this string object
     *  \returns true if both strings have the same contents
     */
    bool operator==( const PdfString & rhs ) const;

    static const PdfString StringNull;

    static long ConvertUTF8toUTF16( const pdf_utf8* pszUtf8, pdf_utf16be* pszUtf16, long lLenUtf16 );
    static long ConvertUTF8toUTF16( const pdf_utf8* pszUtf8, long lLenUtf8, 
                                    pdf_utf16be* pszUtf16, long lLenUtf16, 
                                    EPdfStringConversion eConversion = ePdfStringConversion_Strict  );

    static long ConvertUTF16toUTF8( const pdf_utf16be* pszUtf16, pdf_utf8* pszUtf8, long lLenUtf8 );
    static long ConvertUTF16toUTF8( const pdf_utf16be* pszUtf16, long lLenUtf16, 
                                    pdf_utf8* pszUtf8, long lLenUtf8, 
                                    EPdfStringConversion eConversion = ePdfStringConversion_Strict );

 private:
    /** Allocate m_lLen data for m_pszData if data
     *  does not fit into m_pBuffer.
     *  Otherwise m_pszData is set to point to 
     *  m_pBuffer.
     */
    void Allocate();

    /** Frees the internal buffer
     *  if it was allocated using malloc
     */
    void FreeBuffer();

    /** Construct a new PdfString from a 0 terminated
     *  string. 
     *  The input string will be copied.
     *  if m_bhex is true the copied data will be hex encoded.
     *
     *  \param pszString the string to copy
     *  \param lLen length of the string data to copy
     *  
     */
    void Init( const char* pszString, long lLen );

    static const int  s_nUnicodeMarkerLen = 2;
    static const char s_pszUnicodeMarker[s_nUnicodeMarkerLen];

 private:
    PdfRefCountedBuffer m_buffer;

    bool                m_bHex;
    bool                m_bUnicode;
};

bool PdfString::IsValid() const
{
    return (m_buffer.GetBuffer() != NULL);
}

bool PdfString::IsHex () const
{
    return m_bHex;
}

bool PdfString::IsUnicode () const
{
    return m_bUnicode;
}

const char* PdfString::GetString() const
{
    return m_buffer.GetBuffer();
}

long PdfString::GetLength() const
{
    return m_buffer.GetSize()-1;
}

long PdfString::GetSize() const
{
    return m_buffer.GetSize();;
}

};

#endif // _PDF_STRING_H_

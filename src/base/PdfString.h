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

#ifndef _PDF_STRING_H_
#define _PDF_STRING_H_

#include "PdfDefines.h"
#include "PdfDataType.h"
#include "PdfRefCountedBuffer.h"

namespace PoDoFo {

#define PDF_STRING_BUFFER_SIZE 24

class PdfEncoding;
class PdfOutputDevice;

enum EPdfStringConversion {
    ePdfStringConversion_Strict,
    ePdfStringConversion_Lenient
};


/** A string that can be written to a PDF document.
 *  If it contains binary data it is automatically 
 *  converted into a hex string, otherwise a normal PDF 
 *  string is written to the document.
 *
 *  PdfStrings representing text are either in PDFDocEncoding
 *  (ISO Latin1) encoded or UTF-16BE encoded.
 *
 *  PoDoFo contains methods to convert between these 
 *  encodings. For convinience conversion to UTF-8
 *  is possible to. Please not that strings are
 *  always stored as UTF-16BE or ISO Latin1 (PdfDocEncoding)
 *  in the PDF file.
 *
 *  UTF-16BE strings have to start with the byts 0xFE 0xFF
 *  to be recognized by PoDoFo as unicode strings.
 *
 *
 *  PdfStrings is an implicitly shared class. As a reason
 *  it is very fast to copy PdfString objects.
 *
 *  The internal string buffer is guaranteed to be always terminated 
 *  by 2 zeros.
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
     *  \param pEncoding the encoding of this string, if it is no unicode string.
	 *         This is ignored for unicode strings. If NULL PdfDocEncoding will be used as a default.
     */
    PdfString( const std::string& sString, const PdfEncoding * const pEncoding = NULL );

    /** Construct a new PdfString from a 0 terminated
     *  string. 
     *  The input string will be copied.
     *
     *  \param pszString the string to copy
 	 *  \param pEncoding the encoding of this string, if it is no unicode string.
	 *         This is ignored for unicode strings. If NULL PdfDocEncoding will be used as a default.
     */
    PdfString( const char* pszString, const PdfEncoding * const pEncoding = NULL );

    /** Construct a new PdfString from a 0 terminated
     *  string. 
     *  The input string will be copied.
     *
     *  \param pszString the string to copy
     */
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200    // not for MS Visual Studio 6
#else
       PdfString( const wchar_t* pszString, pdf_long lLen = -1 );
#endif
    void setFromWchar_t( const wchar_t* pszString, pdf_long lLen = -1 );

    /** Construct a new PdfString from a string. 
     *  The input string will be copied.
     *  If the first to bytes of the string are 0xFE and 0xFF
     *  this string is treated as UTF-16BE encoded unicode string.
     *
     *  \param pszString the string to copy
     *  \param lLen length of the string data to encode
     *  \param bHex if true the data will be 
     *              hex encoded during writeout of the string and IsHex() will return true.
 	 *  \param pEncoding the encoding of this string, if it is no unicode string.
	 *         This is ignored for unicode strings. If NULL PdfDocEncoding will be used as a default.
     */
    PdfString( const char* pszString, pdf_long lLen, bool bHex = false, const PdfEncoding * const pEncoding = NULL );

    /** Construct a new PdfString from an UTF-8 encoded string.
     *  
     *  The string is converted to UTF-16BE internally.
     *
     *  \param pszStringUtf8 a UTF-8 encoded string.
     */
    PdfString( const pdf_utf8* pszStringUtf8 );

    /** Construct a new PdfString from an UTF-16be encoded zero terminated string.
     *
     *  \param pszStringUtf16 a UTF-16BE encoded string.
     */
    PdfString( const pdf_utf16be* pszStringUtf16 );

    /** Construct a new PdfString from an UTF-8 encoded string.
     *  
     *  The string is converted to UTF-16BE internally.
     *
     *  \param pszStringUtf8 a UTF-8 encoded string.
     *  \param lLen number of bytes to convert
     */
    PdfString( const pdf_utf8* pszStringUtf8, pdf_long lLen );

    /** Construct a new PdfString from an UTF-16be encoded zero terminated string.
     *
     *  \param pszStringUtf16 a UTF-16BE encoded string.
     *  \param lLen number of words to convert
     */
    PdfString( const pdf_utf16be* pszStringUtf16, pdf_long lLen );

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
     *  \param pEncrypt if !NULL assume the hex data is encrypted and should be decrypted after hexdecoding 
     */
    void SetHexData( const char* pszHex, pdf_long lLen = -1, PdfEncrypt* pEncrypt = NULL );

    /** The string is valid if no error in the constructor has occurred.
     *  The default constructor PdfString() creates an invalid string, as do
     *  other constructors when passed a NULL char* or NULL wchar_t*.
     *  PdfString::StringNull uses the default constructor so is also invalid.
     *  If it is valid it is safe to call all the other member functions.
     *  \returns true if this is a valid initialized PdfString
     */
    inline bool IsValid() const;

    /** Check if this is a hex string.
     *  
     *  If true the data will be hex encoded when the string is written to
     *  a PDF file.
     *
     *  \returns true if this is a hex string.
     *  \see GetString() will return the raw string contents (not hex encoded)
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
     *
     *  The returned data is never hex encoded may contain 0 bytes.
     *
     *  if IsUnicode() returns true, the return value
     *  points to a UTF-16BE string buffer with Length()
     *  characters. Better use GetUnicode() in this case.
     * 
     *  \returns the strings contents which is guaranteed to be zero terminated
     *           but might also contain 0 bytes in the string.
     *  \see IsHex
     *  \see IsUnicode
     *  \see Length
     */
    inline const char* GetString() const;

    /** The contents of the strings can be read
     *  by this function.
     *
     *  The returned data is never hex encoded any maycontain 0 bytes.
     *
     *  if IsUnicode() returns true, the return value
     *  points to a UTF-16BE string buffer with Length()
     *  characters. Better use GetUnicode() in this case.
     * 
     *  \returns the strings contents which is guaranteed to be zero terminated
     *           but might also contain 0 bytes in the string,
     *           returns NULL if PdfString::IsValid() returns false
     *
     *  \see IsHex
     *  \see IsUnicode
     *  \see Length
     */
    inline const pdf_utf16be* GetUnicode() const;

    /** The contents of the string as UTF8 string.
     *
     *  The strings contents are always returned as
     *  UTF8 by this function. Works for unicode strings 
     *  and for non unicode strings.
     *
     *  This is the prefered way to access the strings contents.
     *
     *  \returns the string contents always as UTF8,
     *           returns NULL if PdfString::IsValid() returns false
     */
    inline const std::string & GetStringUtf8() const;

#ifdef _WIN32
    /** The contents of the string as wide character string.
     *
     *  \returns the string contents as wide character string.
     *           returns an empty string if PdfString::IsValid() returns false
     */
    const std::wstring GetStringW() const;
#endif // _WIN32

    /** The length of the string data returned by GetString() 
     *  in bytes not including terminating zeros.
     *
     *  \returns the length of the string,
     *           returns zero if PdfString::IsValid() returns false
     *
     *  \see GetCharacterLength to determine the number of characters in the string
     */
    inline pdf_long GetLength() const;

    /** The length of the string data returned by GetUnicode() 
     *  in characters not including the terminating zero 
     *
     *  \returns the length of the string,
     *           returns zero if PdfString::IsValid() returns false
     *
     *  \see GetCharacterLength to determine the number of characters in the string
     */
    inline pdf_long GetUnicodeLength() const;

    /** Get the number of characters in the string.
     *  
     *  This function returns the correct number of characters in the string
     *  for unicode and ansi strings. Always use this method if you want to 
     *  know the number of characters in the string
     *  as GetLength() will returns the number of bytes used for unicode strings!
     *
     * 
     *  \returns the number of characters in the string,
     *           returns zero if PdfString::IsValid() returns false
     */
    inline pdf_long GetCharacterLength() const;

    /** Write this PdfString in PDF format to a PdfOutputDevice 
     *  
     *  \param pDevice the output device.
     *  \param eWriteMode additional options for writing this object
     *  \param pEncrypt an encryption object which is used to encrypt this object
     *                  or NULL to not encrypt this object
     */
    void Write ( PdfOutputDevice* pDevice, EPdfWriteMode eWriteMode, const PdfEncrypt* pEncrypt = NULL ) const;

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
     *
     *  UTF-8 and UTF-16BE encoded strings of the same data compare equal. Whether
     *  the string will be written out as hex is not considered - only the real "text"
     *  is tested for equality.
     *
     *  \param rhs compare to this string object
     *  \returns true if both strings have the same contents 
     */
    bool operator==( const PdfString & rhs ) const;

    /** Comparison operator
     *  \param rhs compare to this string object
     *  \returns true if strings have different contents
     */
    bool operator!=(const PdfString& rhs) const { return !operator==(rhs); }

#ifdef PODOFO_PUBLIC_STRING_HEX_CODEC // never set, impl. even says REMOVE :(
    /** Converts this string to a hex-encoded string.
     *  
     *  If IsHex returns true, a copy of this string is returned
     *  otherwise the string's data is hex-encoded and returned.
     *
     *  \returns a hex-encoded version of this string, or this string if it is
     *           already hex-encoded.
     *
     *  \see IsHex
     */
    PdfString HexEncode() const; 

    /** Converts this string to an ASCII string (not hex-encoded)
     *  
     *  If IsHex returns false, a copy of this string is returned,
     *  otherwise the string's data is hex-decoded and returned.
     *
     *  \returns a plain version, which is not hex-encoded, of this string, or
     *           this string if it is already a plain not hex-encoded string.
     *
     *  \see IsHex
     */
    PdfString HexDecode() const; 
#endif

    /** Converts this string to a unicode string
     *  
     *  If IsUnicode() returns true a copy of this string is returned
     *  otherwise the string data is converted to UTF-16be and returned.
     *
     *  \returns a unicode version of this string,
     *           returns *this if if PdfString::IsValid() returns false
     */
    PdfString ToUnicode() const;

	 /** Returns internal buffer; do not free it, it's owned by the PdfString
	  *
	  * \returns internal buffer; do not free it, it's owned by the PdfString
      *          returns a NULL zero size buffer if PdfString::IsValid() returns false
	  */
	 PdfRefCountedBuffer &GetBuffer(void);

    static const PdfString StringNull;

    static pdf_long ConvertUTF8toUTF16( const pdf_utf8* pszUtf8, pdf_utf16be* pszUtf16, pdf_long lLenUtf16 );
    static pdf_long ConvertUTF8toUTF16( const pdf_utf8* pszUtf8, pdf_long lLenUtf8, 
                                    pdf_utf16be* pszUtf16, pdf_long lLenUtf16, 
                                    EPdfStringConversion eConversion = ePdfStringConversion_Strict  );

    static pdf_long ConvertUTF16toUTF8( const pdf_utf16be* pszUtf16, pdf_utf8* pszUtf8, pdf_long lLenUtf8 );
    static pdf_long ConvertUTF16toUTF8( const pdf_utf16be* pszUtf16, pdf_long lLenUtf16, 
                                    pdf_utf8* pszUtf8, pdf_long lLenUtf8, 
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
     *  \param pszString the string to copy, must not be NULL
     *  \param lLen length of the string data to copy
     *  
     */
    void Init( const char* pszString, pdf_long lLen );

    /** Construct a new PdfString from a UTF8
     *  string. 
     *  The input string will be copied and converted to UTF-16be.
     *
     *  \param pszStringUtf8 the string to copy, ust not be NULL
     *  \param lLen number of bytes of the string data to copy
     *  
     */
    void InitFromUtf8( const pdf_utf8* pszStringUtf8, pdf_long lLen );

    /** Swap the bytes in the buffer (UTF16be -> UTF16le)
     *  \param pBuf buffer
     *  \param lLen length of buffer
     */
    static void SwapBytes( char* pBuf, pdf_long lLen ); 

    /** Initialise the data member containing a
     *  UTF8 version of this string.
     *
     *  This is only done once and only if necessary.
     */
    void InitUtf8();

 private:
    static const char        s_pszUnicodeMarker[];   ///< The unicode marker used to indicate unicode strings in PDF
    static const char*       s_pszUnicodeMarkerHex;  ///< The unicode marker converted to HEX
    static const pdf_utf16be s_cPdfDocEncoding[256]; ///< conversion table from PDFDocEncoding to UTF16
    static const char * const m_escMap;              ///< Mapping of escape sequences to there value

 private:
    PdfRefCountedBuffer m_buffer;                    ///< String data (always binary), may contain 0 bytes

    bool                m_bHex;                      ///< This string is converted to hex during write out
    bool                m_bUnicode;                  ///< This string contains unicode data

    std::string         m_sUtf8;                     ///< The UTF8 version of the strings contents.
    const PdfEncoding*  m_pEncoding;                 ///< Encoding for non Unicode strings. NULL for unicode strings.
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfString::IsValid() const
{
    return (m_buffer.GetBuffer() != NULL);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfString::IsHex () const
{
    return m_bHex;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfString::IsUnicode () const
{
    return m_bUnicode;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const char* PdfString::GetString() const
{
    return m_buffer.GetBuffer();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const pdf_utf16be* PdfString::GetUnicode() const
{
    return reinterpret_cast<pdf_utf16be*>(m_buffer.GetBuffer());
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const std::string & PdfString::GetStringUtf8() const
{
    if( this->IsValid() && !m_sUtf8.length() && m_buffer.GetSize() - 2) 
        const_cast<PdfString*>(this)->InitUtf8();

    return m_sUtf8;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
pdf_long PdfString::GetLength() const
{
    if ( !IsValid() )
    {
        PdfError::LogMessage( eLogSeverity_Error, "PdfString::GetLength invalid PdfString" );
        return 0;
    }
    
    PODOFO_ASSERT( m_buffer.GetSize() >= 2 );
    
    return m_buffer.GetSize() - 2;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
pdf_long PdfString::GetCharacterLength() const 
{
    return this->IsUnicode() ? this->GetUnicodeLength() : this->GetLength();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
pdf_long PdfString::GetUnicodeLength() const
{
    if ( !IsValid() )
    {
        PdfError::LogMessage( eLogSeverity_Error, "PdfString::GetUnicodeLength invalid PdfString" );
        return 0;
    }
    
    PODOFO_ASSERT( (m_buffer.GetSize() / sizeof(pdf_utf16be)) >= 1 );
    
    return (m_buffer.GetSize() / sizeof(pdf_utf16be)) - 1;
}

};


#endif // _PDF_STRING_H_

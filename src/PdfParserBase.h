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

#ifndef _PDF_PARSER_BASE_H_
#define _PDF_PARSER_BASE_H_

#include "PdfDefines.h"
#include "PdfRefCountedBuffer.h"
#include "PdfRefCountedInputDevice.h"

// PDF_XREF_BUF is used in PdfParser.cpp
// to find the startxref line and has to be smaller
// than PDF_BUFFER
#define PDF_XREF_BUF           256

namespace PoDoFo {

/**
 * A simple class which collects helper functions which are 
 * required to parse a PDF file.
 */
class PODOFO_API PdfParserBase {
 public:
    /**
     * Create a new PdfParserBase with a buffer of its own
     */
    PdfParserBase();

    /** 
     * Create a new PdfParserBase that reads from a file and uses a shared buffer
     * \param rDevice a reference counted input device to read from
     * \param rBuffer a shared buffer to use temporarily during parsing
     */
    PdfParserBase( const PdfRefCountedInputDevice & rDevice, const PdfRefCountedBuffer & rBuffer );
    virtual ~PdfParserBase();

    /** Reads the next number from the current file position
     *  until the next whitespace is readched.
     *
     *  \returns the next number in the file
     */
    long GetNextNumberFromFile();

    /** Reads the next string from the current file position
     *  until the next whitespace is reached.
     *  the result is stored into m_szBuffer.
     *  Leading whitespaces are ignored!
     * 
     *  \returns a pointer to the internal buffer 
     *           where the result is stored
     *
     *  \see GetBuffer
     */
    const char* GetNextStringFromFile( );


    /** Get a handle to the internal buffer.
     *  
     *  \returns the internal buffer
     *  \see GetBufferSize
     */
    inline char* GetBuffer() const;

    /** Size of the internal buffer
     *  
     *  \returns the size of the internal buffer
     */
    inline long GetBufferSize() const;

    /** Returns true if the given character is a whitespace 
     *  according to the pdf reference
     *
     *  \returns true if it is a whitespace character otherwise false
     */
    inline static bool IsWhitespace(const unsigned char ch) throw();

    /** Returns true if the given character is a delimiter
     *  according to the pdf reference
     *
     *  \returns true if it is a delimiter character otherwise false
     */
    inline static bool IsDelimiter(const unsigned char ch) throw();

    /**
     * True if the passed character is a regular character according to the PDF
     * reference (Section 3.1.1, Character Set); ie it is neither a white-space
     * nor a delimeter character.
     */
    inline static bool IsRegular(const unsigned char ch) throw();

    /**
     * True iff the passed character is within the generally accepted "printable"
     * ASCII range.
     */
    inline static bool IsPrintable(const unsigned char ch) throw();


 protected:
    PdfRefCountedInputDevice m_device;
    PdfRefCountedBuffer      m_buffer;

 private:
    // 256-byte array mapping character ordinal values to a truth value
    // indicating whether or not they are whitespace according to the PDF
    // standard.
    static const char * const m_delimiterMap;
    static const char * const m_whitespaceMap;
};

inline bool PdfParserBase::IsWhitespace(const unsigned char ch) throw()
{
    return ( m_whitespaceMap[ch] != 0 );
}

inline bool PdfParserBase::IsDelimiter(const unsigned char ch) throw()
{
    return ( m_delimiterMap[ch] != 0 );
}

inline bool PdfParserBase::IsRegular(const unsigned char ch) throw()
{
    return !IsWhitespace(ch) && !IsDelimiter(ch);
}

inline bool PdfParserBase::IsPrintable(const unsigned char ch) throw()
{
    return ch > 32 && ch < 125;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline char* PdfParserBase::GetBuffer() const
{
    return m_buffer.GetBuffer();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline long PdfParserBase::GetBufferSize() const
{
    return m_buffer.GetSize();
}

};

#endif // _PDF_PARSER_BASE_H_

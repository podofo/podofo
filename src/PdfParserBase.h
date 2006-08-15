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
#include "PdfRefCountedFile.h"

// PDF_XREF_BUF is used in PdfParser.cpp
// to find the startxref line and has to be smaller
// than PDF_BUFFER
#define PDF_XREF_BUF           256

namespace PoDoFo {

/**
 * A simple class which collects helper functions which are 
 * required to parse a PDF file.
 */
class PdfParserBase {
 public:
    PdfParserBase();
    PdfParserBase( const PdfRefCountedFile & rFile, const PdfRefCountedBuffer & rBuffer );
    virtual ~PdfParserBase();

    /** Set the internal file handle
     *  \param hFile a file handle
     */
    //inline void SetFileHandle( FILE* hFile );

    /** Returns true if the given character is a delimiter
     *  according to the pdf reference
     *
     *  \returns true if it is a delimiter character otherwise false
     */
    static bool IsDelimiter( const char c );

    /** Returns true if the given character is a whitespace 
     *  according to the pdf reference
     *
     *  \returns true if it is a whitespace character otherwise false
     */
    static bool IsWhitespace( const char c );

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

 protected:
    PdfRefCountedFile   m_file;
    PdfRefCountedBuffer m_buffer;
};

char* PdfParserBase::GetBuffer() const
{
    return m_buffer.Buffer();
}

long PdfParserBase::GetBufferSize() const
{
    return m_buffer.Size();
}

};

#endif // _PDF_PARSER_BASE_H_

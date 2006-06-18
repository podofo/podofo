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

#ifndef _PDF_PARSER_OBJECT_H_
#define _PDF_PARSER_OBJECT_H_

#include "PdfDefines.h"
#include "PdfParserBase.h"
#include "PdfObject.h"

namespace PoDoFo {

class PdfParser;

/**
 * A PdfParserObject constructs a PdfObject from a PDF file.
 * Parsing starts always at the current file position.
 */
class PdfParserObject : public PdfObject, public PdfParserBase {
    friend class PdfParser;

 public:
    /** Parse the object data from the given file handle starting at
     *  the current position.
     *  \param hFile  an open file handle which is positioned in
     *                front of the object which is going to be parsed.
     *  \param szBuffer buffer to use for parsing to avoid reallocations
     *  \param lBufferSize size of the buffer
     */
    PdfParserObject( FILE* hFile, char* szBuffer, long lBufferSize );

    /** Parse the object data for an internal object.
     *  You have to call ParseDictionaryKeys as next function call.
     *
     *  The following two parameters are used to avoid allocation of a new
     *  buffer in PdfSimpleParser.
     *
     *  This constructor is for internal usage only!
     *
     *  \param szBuffer buffer to use for parsing to avoid reallocations
     *  \param lBufferSize size of the buffer
     */
    PdfParserObject(char* szBuffer, long lBufferSize);

    virtual ~PdfParserObject();

    /** Parse the object data from the given file handle 
     *  \param bIsTrailer wether this is a trailer dictionary or not.
     *                    trailer dictionaries do not have a object number etc.
     *  \returns ErrOk on success
     */
    PdfError ParseFile( bool bIsTrailer = false );

    /** Returns if this object has a stream object appended.
     *  which has to be parsed.
     *  \returns true if there is a stream
     */
    inline bool HasStreamToParse() const;

    /** Starts reading at the file position m_lStreamOffset and interprets all bytes
     *  as contents of the objects stream.
     *  It is assumed that the dictionary has a valid /Length key already.
     *  \param pParser a pdf parser object which is used to retrieve objects
     *                 this is needed if the /Length key is a indirect reference
     *                 to another object
     *
     *  \returns ErrOk on success
     */
    PdfError ParseStream( PdfParser* pParser );

    /** Parse the keys of a dictionary from a zero terminated buffer
     *  \param szBuffer  buffer containing the dictioniaries data
     *  \param lBufferLen length of the data buffer
     *  \param plParsedLength if non null, the length of the parsed data is returned
     *  \returns ErrOk on success
     */
    PdfError ParseDictionaryKeys( char* szBuffer, long lBufferLen, long* plParsedLength = NULL );

 private:
    /** Initialize private members in this object with their default values
     */
    void    Init();

    PdfError ParseValue( char** szBuffer, std::string & sKey, std::string & sValue  );
    PdfError GetDataType( char c, int* counter, EPdfDataType* eDataType, bool* bType );
    PdfError ReadObjectNumber();

 private:
    bool m_bStream;
    long m_lStreamOffset;
};

bool PdfParserObject::HasStreamToParse() const
{
    return m_bStream;
}

};

#endif // _PDF_PARSER_OBJECT_H_

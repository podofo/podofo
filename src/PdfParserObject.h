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
     *  \param pParent pointer to a parent to resolve object references
     *  \param rFile  an open reference counted file handle which is positioned in
     *                front of the object which is going to be parsed.
     *  \param szBuffer buffer to use for parsing to avoid reallocations
     *  \param lBufferSize size of the buffer
     *  \param lOffset the position in the file from which the object shall be read
     *                 if lOffset = -1, the object will be read from the current 
     *                 position in the file.
     */
    PdfParserObject( PdfVecObjects* pParent, const PdfRefCountedFile & rFile, char* szBuffer, long lBufferSize, long lOffset = -1 );

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
    PdfParserObject( char* szBuffer, long lBufferSize );

    virtual ~PdfParserObject();

    /** Parse the object data from the given file handle 
     *  If delayed loading is enabled, only the object and generation number
     *  is read now and everything else is read later.
     *
     *  \param bIsTrailer wether this is a trailer dictionary or not.
     *                    trailer dictionaries do not have a object number etc.
     */
    void ParseFile( bool bIsTrailer = false );

    /** Returns if this object has a stream object appended.
     *  which has to be parsed.
     *  \returns true if there is a stream
     */
    inline bool HasStreamToParse() const;

    /** Starts reading at the file position m_lStreamOffset and interprets all bytes
     *  as contents of the objects stream.
     *  It is assumed that the dictionary has a valid /Length key already.
     */
    void ParseStream();

    /** Parse the keys of a dictionary from a zero terminated buffer
     *  \param szBuffer  buffer containing the dictioniaries data
     *  \param lBufferLen length of the data buffer
     *  \param plParsedLength if non null, the length of the parsed data is returned
     */
    void ParseDictionaryKeys( char* szBuffer, long lBufferLen, long* plParsedLength = NULL );

    /** \returns true if this PdfParser loads all objects at
     *                the time they are accessed for the first time.
     *                The default is to load all object immediately.
     *                In this case false is returned.
     */
    inline bool IsLoadOnDemand() const;

    /** Sets wether this object shall be loaded on demand
     *  when it's data is accessed for the first time.
     *  \param bDelayed if true the object is loaded delayed.
     */
    inline void SetLoadOnDemand( bool bDelayed );

    /** Set the object number of this object.
     *  It is almost never necessary to use this call.
     *  It is only included for usage in the PdfParser.
     *
     *  \param nObjNo the new object number of this object
     */
    inline void SetObjectNumber( unsigned int nObjNo );

 protected:
    /** Load all data of the object if load object on demand is enabled.
     *  Reimplemented from PdfObject.
     */
    virtual void LoadOnDemand();

    /** Load the stream of the object if it has one and if loading on demand is enabled.
     *  Reimplemented from PdfObject.
     */
    virtual void LoadStreamOnDemand();

 private:
    /** Initialize private members in this object with their default values
     */
    void    Init();

    /** Parse the object data from the given file handle 
     *  \param bIsTrailer wether this is a trailer dictionary or not.
     *                    trailer dictionaries do not have a object number etc.
     */
    void ParseFileComplete( bool bIsTrailer );

    void ParseValue( char** szBuffer, std::string & sKey, std::string & sValue  );
    void GetDataType( char c, int* counter, EPdfDataType* eDataType, bool* bType ) const;
    void ReadObjectNumber();

 private:
    bool m_bIsTrailer;

    bool m_bLoadOnDemand;
    long m_lOffset;

    bool m_bStream;
    long m_lStreamOffset;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfParserObject::SetObjectNumber( unsigned int nObjNo )
{
    m_reference.SetObjectNumber( nObjNo );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfParserObject::IsLoadOnDemand() const
{
    return m_bLoadOnDemand;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfParserObject::SetLoadOnDemand( bool bDelayed )
{
    m_bLoadOnDemand = bDelayed;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfParserObject::HasStreamToParse() const
{
    return m_bStream;
}

};

#endif // _PDF_PARSER_OBJECT_H_

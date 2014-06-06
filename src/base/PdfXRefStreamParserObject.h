/***************************************************************************
 *   Copyright (C) 2009 by Dominik Seichter                                *
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

#ifndef _PDF_XREF_STREAM_PARSER_OBJECT_H_
#define _PDF_XREF_STREAM_PARSER_OBJECT_H_

#include "PdfDefines.h"
#include "PdfParserObject.h"

#define W_ARRAY_SIZE 3
#define W_MAX_BYTES  4

namespace PoDoFo {

/**
 * A utility class for PdfParser that can parse
 * an XRef stream object.
 *
 * It is mainly here to make PdfParser more modular.
 */
class PdfXRefStreamParserObject : public PdfParserObject {
public:

    /** Parse the object data from the given file handle starting at
     *  the current position.
     *  \param pCreator pointer to a PdfVecObjects to resolve object references
     *  \param rDevice an open reference counted input device which is positioned in
     *                 front of the object which is going to be parsed.
     *  \param rBuffer buffer to use for parsing to avoid reallocations
     *  \param pOffsets XRef entries are stored into this array
     */
    PdfXRefStreamParserObject(PdfVecObjects* pCreator, const PdfRefCountedInputDevice & rDevice, 
                              const PdfRefCountedBuffer & rBuffer, PdfParser::TVecOffsets* pOffsets );

    ~PdfXRefStreamParserObject();

    void Parse();

    void ReadXRefTable();

    /**
     * \returns true if there is a previous XRefStream
     */
    inline bool HasPrevious();

    /**
     * \returns the offset of the previous XRef table
     */
    inline pdf_long GetPreviousOffset();

private:
    /**
     * Read the /Index key from the current dictionary
     * and write uit to a vector.
     *
     * \param rvecIndeces store the indeces hare
     * \param size default value from /Size key
     */
    void GetIndeces( std::vector<pdf_int64> & rvecIndeces, pdf_int64 size );

    /**
     * Parse the stream contents
     *
     * \param nW /W key
     * \param rvecIndeces indeces as filled by GetIndeces
     *
     * \see GetIndeces
     */
    void ParseStream( const pdf_int64 nW[W_ARRAY_SIZE], const std::vector<pdf_int64> & rvecIndeces );

    void ReadXRefStreamEntry( char* pBuffer, pdf_long, const pdf_int64 lW[W_ARRAY_SIZE], int nObjNo );
private:
    pdf_long m_lNextOffset;

    PdfParser::TVecOffsets* m_pOffsets;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfXRefStreamParserObject::HasPrevious()
{
    return (m_lNextOffset != -1);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline pdf_long PdfXRefStreamParserObject::GetPreviousOffset()
{
    return m_lNextOffset;
}

};

#endif // _PDF_XREF_STREAM_PARSER_OBJECT_H_

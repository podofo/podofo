/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
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

#ifndef _PDF_OBJECT_STREAM_PARSER_OBJECT_H_
#define _PDF_OBJECT_STREAM_PARSER_OBJECT_H_

#include "PdfDefines.h"

#include "PdfRefCountedBuffer.h"

namespace PoDoFo {

class PdfParserObject;
class PdfVecObjects;

/**
 * A utility class for PdfParser that can parse
 * an object stream object.
 *
 * It is mainly here to make PdfParser more modular.
 */
class PdfObjectStreamParserObject {
public:
	typedef std::vector<pdf_int64> ObjectIdList;
    /**
     * Create a new PdfObjectStreamParserObject from an existing
     * PdfParserObject. The PdfParserObject will be removed and deleted.
     * All objects from the object stream will be read into memory.
     *
     * \param pParser PdfParserObject for an object stream
     * \param pVecObjects add loaded objecs to this vector of objects
     * \param rBuffer use this allocated buffer for caching
     */
    PdfObjectStreamParserObject(PdfParserObject* pParser, PdfVecObjects* pVecObjects, const PdfRefCountedBuffer & rBuffer);

    ~PdfObjectStreamParserObject();

    void Parse(ObjectIdList const &);

private:
    void ReadObjectsFromStream( char* pBuffer, pdf_long lBufferLen, pdf_int64 lNum, pdf_int64 lFirst, ObjectIdList const &);

private:
    PdfParserObject* m_pParser;
    PdfVecObjects* m_vecObjects;
    PdfRefCountedBuffer m_buffer;
};

};

#endif // _PDF_OBJECT_STREAM_PARSER_OBJECT_H_

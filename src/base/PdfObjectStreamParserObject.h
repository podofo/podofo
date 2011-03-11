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
 ***************************************************************************/

#ifndef _PDF_OBJECT_STREAM_PARSER_OBJECT_H_
#define _PDF_OBJECT_STREAM_PARSER_OBJECT_H_

#include "PdfDefines.h"

#include "PdfRefCountedBuffer.h"

namespace PoDoFo {

class PdfEncrypt;
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
	typedef std::vector<long long> ObjectIdList;
    /**
     * Create a new PdfObjectStreamParserObject from an existing
     * PdfParserObject. The PdfParserObject will be removed and deleted.
     * All objects from the object stream will be read into memory.
     *
     * \param pParser PdfParserObject for an object stream
     * \param pVecObjects add loaded objecs to this vector of objects
     * \param rBuffer use this allocated buffer for caching
     * \param pEncrypt encryption object used to decrypt streams
     */
    PdfObjectStreamParserObject(PdfParserObject* pParser, PdfVecObjects* pVecObjects, const PdfRefCountedBuffer & rBuffer, PdfEncrypt* pEncrypt );

    ~PdfObjectStreamParserObject();

    void Parse(ObjectIdList const &);

private:
    void ReadObjectsFromStream( char* pBuffer, pdf_long lBufferLen, long long lNum, long long lFirst, ObjectIdList const &);

private:
    PdfParserObject* m_pParser;
    PdfVecObjects* m_vecObjects;
    PdfRefCountedBuffer m_buffer;
    PdfEncrypt* m_pEncrypt;
};

};

#endif // _PDF_OBJECT_STREAM_PARSER_OBJECT_H_

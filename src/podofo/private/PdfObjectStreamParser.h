/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_OBJECT_STREAM_PARSER_OBJECT_H
#define PDF_OBJECT_STREAM_PARSER_OBJECT_H

#include "PdfParserObject.h"

namespace PoDoFo {

class PdfEncrypt;
class PdfIndirectObjectList;

/**
 * A utility class for PdfParser that can parse
 * an object stream object (PDF Reference 1.7 3.4.6 Object Streams)
 *
 * It is mainly here to make PdfParser more modular.
 */
class PdfObjectStreamParser
{
public:
    /**
     * Create a new PdfObjectStreamParserObject from an existing
     * PdfParserObject. The PdfParserObject will be removed and deleted.
     * All objects from the object stream will be read into memory.
     *
     * \param parser PdfParserObject for an object stream
     * \param objects add loaded objects to this vector of objects
     * \param buffer use this allocated buffer for caching
     */
    PdfObjectStreamParser(PdfParserObject& parser, PdfIndirectObjectList& objects, const std::shared_ptr<charbuff>& buffer);

    void Parse(const cspan<int64_t>& objectList);

private:
    void readObjectsFromStream(char* buffer, size_t lBufferLen, int64_t lNum, int64_t lFirst, const cspan<int64_t>& list);

private:
    PdfParserObject* m_Parser;
    PdfIndirectObjectList* m_Objects;
    std::shared_ptr<charbuff> m_buffer;
};

};

#endif // PDF_OBJECT_STREAM_PARSER_OBJECT_H

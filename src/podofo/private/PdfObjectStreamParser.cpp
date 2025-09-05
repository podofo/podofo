/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PdfDeclarationsPrivate.h"
#include "PdfObjectStreamParser.h"

#include <algorithm>

#include "PdfParser.h"

#include <podofo/main/PdfDictionary.h>
#include <podofo/main/PdfIndirectObjectList.h>
#include <podofo/auxiliary/StreamDevice.h>

using namespace std;
using namespace PoDoFo;

PdfObjectStreamParser::PdfObjectStreamParser(PdfParserObject& parser,
        PdfIndirectObjectList& objects, const shared_ptr<charbuff>& buffer)
    : m_Parser(&parser), m_Objects(&objects), m_buffer(buffer)
{
    if (buffer == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);
}

void PdfObjectStreamParser::Parse(const unordered_set<uint32_t>* objectList)
{
    int64_t num = m_Parser->GetDictionary().FindKeyAsSafe<int64_t>("N", 0);
    int64_t first = m_Parser->GetDictionary().FindKeyAsSafe<int64_t>("First", 0);
    if (num < 0 || first < 0 || num >= PdfParser::MaxObjectCount)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::BrokenFile, "Object stream has invalid object count or offset");

    charbuff buffer;
    m_Parser->GetOrCreateStream().CopyTo(buffer);

    this->readObjectsFromStream(buffer.data(), buffer.size(), (unsigned)num, (size_t)first, objectList);
    m_Parser = nullptr;
}

void PdfObjectStreamParser::readObjectsFromStream(char* buffer, size_t bufferLen,
    unsigned num, size_t first, const unordered_set<uint32_t>* objectList)
{
    SpanStreamDevice device(buffer, bufferLen);
    PdfTokenizer tokenizer(m_buffer);
    PdfVariant var;
    for (unsigned i = 0; i < num; i++)
    {
        int64_t objNo = tokenizer.ReadNextNumber(device);
        int64_t offset = tokenizer.ReadNextNumber(device);
        size_t pos = device.GetPosition();

        if (objNo < 0 || offset < 0 || objNo >= PdfParser::MaxObjectCount
            || first >= numeric_limits<size_t>::max() - (size_t)offset)
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::BrokenFile,
                "Object stream has invalid object number or offset");
        }

        // move to the position of the object in the stream
        device.Seek(first + (size_t)offset);

        // use a second tokenizer here so that anything that gets dequeued isn't left in the tokenizer that reads the offsets and lengths
        PdfTokenizer variantTokenizer(m_buffer);
        variantTokenizer.ReadNextVariant(device, var); // NOTE: The stream is already decrypted

        bool shouldRead = objectList == nullptr || objectList->find(static_cast<uint32_t>(objNo)) != objectList->end();
#ifndef VERBOSE_DEBUG_DISABLED
        std::cerr << "ReadObjectsFromStream STREAM=" << m_Parser->GetIndirectReference().ToString() <<
            ", OBJ=" << objNo <<
            ", " << (shouldRead ? "read" : "skipped") << std::endl;
#endif
        if (shouldRead)
        {
            // The generation number of an object stream and of any
            // compressed object is implicitly zero
            PdfReference reference(static_cast<uint32_t>(objNo), 0);
            auto obj = new PdfObject(std::move(var));
            obj->SetIndirectReference(reference);
            m_Objects->PushObject(obj);
        }

        // move back to the position inside of the table of contents
        device.Seek(pos);
    }
}

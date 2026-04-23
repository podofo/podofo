// SPDX-FileCopyrightText: 2010 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include "PdfDeclarationsPrivate.h"
#include "PdfObjectStreamParser.h"

#include <algorithm>

#include <numerics/checked_math.h>

#include "PdfParser.h"

#include <podofo/main/PdfDictionary.h>
#include <podofo/main/PdfIndirectObjectList.h>
#include <podofo/auxiliary/StreamDevice.h>

using namespace std;
using namespace PoDoFo;
using namespace chromium::base;

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

        if (objNo < 0 || offset < 0 || objNo >= PdfParser::MaxObjectCount)
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::BrokenFile,
                "Object stream has invalid object number or offset");
        }

        size_t target;
        if (!(CheckedNumeric<size_t>(first) + CheckedNumeric<size_t>((size_t)offset)).AssignIfValid(&target)
            || target > bufferLen)
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::BrokenFile,
                "Object stream offset overflows buffer");
        }

        // move to the position of the object in the stream
        device.Seek(target);

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
            unique_ptr<PdfObject> obj(new PdfObject(std::move(var)));
            obj->SetIndirectReference(reference);
            m_Objects->PushObject(std::move(obj));
        }

        // move back to the position inside of the table of contents
        device.Seek(pos);
    }
}

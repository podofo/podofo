// SPDX-FileCopyrightText: 2010 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include "PdfDeclarationsPrivate.h"
#include "PdfObjectStreamParser.h"

#include <algorithm>

#include <numerics/checked_math.h>

#include "PdfCompressedObject.h"
#include "PdfParser.h"

#include <podofo/main/PdfDictionary.h>
#include <podofo/main/PdfDocument.h>
#include <podofo/main/PdfIndirectObjectList.h>
#include <podofo/auxiliary/StreamDevice.h>

using namespace std;
using namespace PoDoFo;
using namespace chromium::base;

void PdfObjectStreamParser::Parse(PdfParserObject& streamObj, PdfIndirectObjectList& objects,
    const shared_ptr<charbuff>& buffer, const vector<Entry>* entries)
{
    shared_ptr<PdfObjectStreamParser> parser(new PdfObjectStreamParser(streamObj, objects, buffer));
    if (entries == nullptr)
    {
        // The stored objects are unknown: the contents must be
        // decoded now to enumerate them
        parser->decode(streamObj);
        for (unsigned i = 0; i < (unsigned)parser->m_offsets.size(); i++)
            pushObject(objects, parser, { parser->m_offsets[i].ObjectNumber, i });
    }
    else
    {
        for (auto& entry : *entries)
            pushObject(objects, parser, entry);
    }
}

PdfObjectStreamParser::PdfObjectStreamParser(const PdfObject& streamObj,
        PdfIndirectObjectList& objects, const shared_ptr<charbuff>& buffer)
    : m_objects(&objects), m_streamRef(streamObj.GetIndirectReference()),
    m_buffer(buffer), m_pendingCount(0), m_decoding(false), m_decoded(false)
{
    if (buffer == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    if (streamObj.GetDocument() != nullptr && streamObj.GetDocument()->IsStrictParsing())
        m_params.Flags |= PdfTokenizerFlags::StrictParsing;
}

void PdfObjectStreamParser::pushObject(PdfIndirectObjectList& objects,
    const shared_ptr<PdfObjectStreamParser>& parser, const Entry& entry)
{
    // The generation number of an object stream and of any
    // compressed object is implicitly zero
    PdfReference reference(entry.ObjectNumber, 0);
    parser->m_pendingCount++;
    objects.PushObject(unique_ptr<PdfObject>(new PdfCompressedObject(reference, parser, entry.Index)));
}

bool PdfObjectStreamParser::TryReadObject(uint32_t objNum, unsigned index, PdfVariant& variant)
{
    ensureDecoded();

    size_t offset;
    if (index < m_offsets.size() && m_offsets[index].ObjectNumber == objNum)
    {
        offset = m_offsets[index].Offset;
    }
    else
    {
        // The index reported by the XRef entry is not reliable,
        // fallback searching the object number
        auto found = std::find_if(m_offsets.begin(), m_offsets.end(), [objNum](const ObjectOffset& curr) {
            return curr.ObjectNumber == objNum;
        });
        if (found == m_offsets.end())
        {
            objectLoaded();
            return false;
        }

        offset = found->Offset;
    }

    SpanStreamDevice device(m_contents.data(), m_contents.size());
    device.Seek(offset);

    PdfTokenizer tokenizer(m_buffer);
    tokenizer.SetParameters(m_params);
    tokenizer.ReadNextVariant(device, variant); // NOTE: The stream is already decrypted
    objectLoaded();
    return true;
}

void PdfObjectStreamParser::notifyObjectUnloaded()
{
    m_pendingCount++;
}

void PdfObjectStreamParser::ensureDecoded()
{
    if (m_decoded)
        return;

    // The stream dictionary may reference indirect objects, /Length for example:
    // refuse to serve an object stream that stores them by itself
    if (m_decoding)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::BrokenFile, "Recursive object stream decoding");

    auto streamObj = m_objects->GetObject(m_streamRef);
    if (streamObj == nullptr)
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidObject,
            "The object stream {} is missing", m_streamRef.ToString());
    }

    m_decoding = true;
    try
    {
        decode(*streamObj);
    }
    catch (...)
    {
        m_decoding = false;
        throw;
    }

    m_decoding = false;
}

void PdfObjectStreamParser::decode(const PdfObject& streamObj)
{
    auto& dict = streamObj.GetDictionary();
    int64_t num = dict.FindKeyAsSafe<int64_t>("N", 0);
    int64_t first = dict.FindKeyAsSafe<int64_t>("First", 0);
    if (num < 0 || first < 0 || num >= PdfParser::MaxObjectCount)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::BrokenFile, "Object stream has invalid object count or offset");

    streamObj.MustGetStream().CopyTo(m_contents);

    SpanStreamDevice device(m_contents.data(), m_contents.size());
    PdfTokenizer tokenizer(m_buffer);
    tokenizer.SetParameters(m_params);

    m_offsets.clear();
    m_offsets.reserve((size_t)num);
    for (unsigned i = 0; i < (unsigned)num; i++)
    {
        int64_t objNo = tokenizer.ReadNextNumber(device);
        int64_t offset = tokenizer.ReadNextNumber(device);
        if (objNo < 0 || offset < 0 || objNo >= PdfParser::MaxObjectCount)
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::BrokenFile,
                "Object stream has invalid object number or offset");
        }

        size_t target;
        if (!(CheckedNumeric<size_t>((size_t)first) + CheckedNumeric<size_t>((size_t)offset)).AssignIfValid(&target)
            || target > m_contents.size())
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::BrokenFile,
                "Object stream offset overflows buffer");
        }

        m_offsets.push_back({ (uint32_t)objNo, target });
    }

    m_decoded = true;
}

void PdfObjectStreamParser::objectLoaded()
{
    if (m_pendingCount == 0 || --m_pendingCount != 0)
        return;

    // All the stored objects were loaded, so the decoded contents are not
    // needed anymore. They are decoded again if an object is unloaded and
    // then accessed another time
    m_contents = charbuff();
    m_offsets.clear();
    m_offsets.shrink_to_fit();
    m_decoded = false;
}

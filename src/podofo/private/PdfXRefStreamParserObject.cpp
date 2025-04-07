/**
 * SPDX-FileCopyrightText: (C) 2009 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfXRefStreamParserObject.h"

#include <numerics/checked_math.h>

#include <podofo/main/PdfArray.h>
#include <podofo/main/PdfDictionary.h>

#include "PdfParser.h"

using namespace std;
using namespace PoDoFo;
using namespace chromium::base;

PdfXRefStreamParserObject::PdfXRefStreamParserObject(PdfDocument& doc, InputStreamDevice& device, PdfXRefEntries& entries)
    : PdfXRefStreamParserObject(&doc, device, entries) { }

PdfXRefStreamParserObject::PdfXRefStreamParserObject(InputStreamDevice& device, PdfXRefEntries& entries)
    : PdfXRefStreamParserObject(nullptr, device, entries) { }

PdfXRefStreamParserObject::PdfXRefStreamParserObject(PdfDocument* doc, InputStreamDevice& device, PdfXRefEntries& entries)
    : PdfParserObject(doc, PdfReference(), device, -1), m_NextOffset(-1), m_entries(&entries)
{
}

void PdfXRefStreamParserObject::delayedLoad()
{
    // NOTE: Ignore the encryption in the XREF as the XREF stream must no be encrypted (see PDF Reference 3.4.7)

    PdfTokenizer tokenizer;
    auto reference = ReadReference(tokenizer);
    SetIndirectReference(reference);
    PdfParserObject::Parse(tokenizer);

    // Do some very basic error checking
    auto& dict = m_Variant.GetDictionary();
    auto keyObj = dict.FindKey("Type");
    if (keyObj == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidXRef);

    if (!keyObj->IsName() || keyObj->GetName() != "XRef")
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidXRef);

    if (!dict.HasKey("Size")
        || !dict.HasKey("W"))
    {
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidXRef);
    }

    if (dict.HasKey("Prev"))
        m_NextOffset = static_cast<ssize_t>(dict.FindKeyAsSafe<double>("Prev", 0));

    if (!this->HasStreamToParse())
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidXRef);
}

void PdfXRefStreamParserObject::ReadXRefTable()
{
    int64_t size = this->GetDictionary().FindKeyAsSafe<int64_t>("Size", 0);
    auto& arrObj = this->GetDictionary().MustFindKey("W");

    // The pdf reference states that W is always an array with 3 entries
    // all of them have to be integers
    const PdfArray* arr;
    if (!arrObj.TryGetArray(arr) || arr->size() != 3)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRefStream, "Invalid XRef stream /W array");

    int64_t wArray[W_ARRAY_SIZE] = { 0, 0, 0 };
    int64_t num;
    for (unsigned i = 0; i < W_ARRAY_SIZE; i++)
    {

        if (!(*arr)[i].TryGetNumber(num))
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRefStream, "Invalid XRef stream /W array");

        wArray[i] = num;
    }

    vector<int64_t> indices;
    getIndices(indices, static_cast<int64_t>(size));

    parseStream(wArray, indices);
}

void PdfXRefStreamParserObject::parseStream(const int64_t wArray[W_ARRAY_SIZE], const vector<int64_t>& indices)
{
    for (int64_t lengthSum = 0, i = 0; i < W_ARRAY_SIZE; i++)
    {
        if (wArray[i] < 0)
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRefStream,
                "Negative field length in XRef stream");
        }
        if (numeric_limits<int64_t>::max() - lengthSum < wArray[i])
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRefStream,
                "Invalid entry length in XRef stream");
        }
        else
        {
            lengthSum += wArray[i];
        }
    }

    const size_t entryLen = static_cast<size_t>(wArray[0] + wArray[1] + wArray[2]);

    charbuff buffer;
    this->GetOrCreateStream().CopyTo(buffer);

    vector<int64_t>::const_iterator it = indices.begin();
    size_t offset = 0;
    while (it != indices.end())
    {
        int64_t firstObject = *it++;
        int64_t objectCount = *it++;

        if (firstObject < 0)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRefStream, "PdfXRefStreamParserObject: First object is negative");
        if (objectCount < 0)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRefStream, "PdfXRefStreamParserObject: Object count is negative");

        if ((offset + (size_t)objectCount * entryLen) > buffer.size())
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRefStream, "Invalid count in XRef stream");

        CheckedNumeric first((uint64_t)firstObject);
        CheckedNumeric count((uint64_t)objectCount);
        unsigned newSize;
        if (!(first + count).AssignIfValid(&newSize))
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "PdfXRefStreamParserObject: Object count has reached maximum allowed size");

        m_entries->Enlarge(newSize);
        for (unsigned index = 0; index < (unsigned)objectCount; index++)
        {
            unsigned objIndex = (unsigned)firstObject + index;
            auto& entry = (*m_entries)[objIndex];
            if (objIndex < m_entries->GetSize() && !entry.Parsed)
                readXRefStreamEntry(entry, buffer.data() + offset, wArray);

            offset += entryLen;
        }
    }
}

void PdfXRefStreamParserObject::getIndices(vector<int64_t>& indices, int64_t size)
{
    // get the first object number in this crossref stream.
    // it is not required to have an index key though
    auto indexObj = this->GetDictionary().GetKey("Index");
    if (indexObj == nullptr)
    {
        // Default
        indices.push_back(static_cast<int64_t>(0));
        indices.push_back(size);
    }
    else
    {
        const PdfArray* arr;
        if (!indexObj->TryGetArray(arr))
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRefStream, "Invalid XRef Stream /Index");

        for (auto& index : *arr)
            indices.push_back(index.GetNumber());
    }

    // indices must be a multiple of 2
    if (indices.size() % 2 != 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRefStream, "Invalid XRef Stream /Index");
}

void PdfXRefStreamParserObject::readXRefStreamEntry(PdfXRefEntry& entry, char* buffer, const int64_t wArray[W_ARRAY_SIZE])
{
    uint64_t entryRaw[W_ARRAY_SIZE];
    for (unsigned i = 0; i < W_ARRAY_SIZE; i++)
    {
        if (wArray[i] > W_MAX_BYTES)
        {
            PoDoFo::LogMessage(PdfLogSeverity::Error,
                "The XRef stream dictionary has an entry in /W of size {}. The maximum supported value is {}",
                wArray[i], W_MAX_BYTES);

            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidXRefStream);
        }

        entryRaw[i] = 0;
        for (int64_t z = W_MAX_BYTES - wArray[i]; z < W_MAX_BYTES; z++)
        {
            entryRaw[i] = (entryRaw[i] << 8) + static_cast<unsigned char>(*buffer);
            buffer++;
        }
    }

    entry.Parsed = true;

    // TABLE 3.15 Additional entries specific to a cross - reference stream dictionary
    // /W array: "If the first element is zero, the type field is not present, and it defaults to type 1"
    uint64_t type;
    if (wArray[0] == 0)
        type = 1;
    else
        type = entryRaw[0]; // nData[0] contains the type information of this entry

    switch (type)
    {
        // TABLE 3.16 Entries in a cross-reference stream
        case 0:
            // a free object
            entry.ObjectNumber = entryRaw[1];
            entry.Generation = (uint32_t)entryRaw[2];
            entry.Type = PdfXRefEntryType::Free;
            break;
        case 1:
            // normal uncompressed object
            entry.Offset = entryRaw[1];
            entry.Generation = (uint32_t)entryRaw[2];
            entry.Type = PdfXRefEntryType::InUse;
            break;
        case 2:
            // object that is part of an object stream
            entry.ObjectNumber = entryRaw[1]; // object number of the stream
            entry.Index = (uint32_t)entryRaw[2]; // index in the object stream
            entry.Type = PdfXRefEntryType::Compressed;
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidXRefStream);
    }
}

bool PdfXRefStreamParserObject::TryGetPreviousOffset(size_t& previousOffset) const
{
    bool ret = m_NextOffset != -1;
    previousOffset = ret ? (size_t)m_NextOffset : 0;
    return ret;
}

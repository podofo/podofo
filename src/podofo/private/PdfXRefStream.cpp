/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PdfDeclarationsPrivate.h"
#include "PdfXRefStream.h"

#include "PdfWriter.h"
#include <podofo/main/PdfDictionary.h>

using namespace PoDoFo;

PdfXRefStream::PdfXRefStream(PdfWriter& writer) :
    PdfXRef(writer),
    m_xrefStreamEntryIndex(-1),
    m_xrefStreamObj(&writer.GetObjects().CreateDictionaryObject("XRef"_n)),
    m_offset(-1)
{
}

uint64_t PdfXRefStream::GetOffset() const
{
    if (m_offset < 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "XRefStm has not been written yet");

    return (uint64_t)m_offset;
}

bool PdfXRefStream::ShouldSkipWrite(const PdfReference& ref)
{
    // We handle writing for the XRefStm object
    if (m_xrefStreamObj->GetIndirectReference() == ref)
        return true;
    else
        return false;
}

void PdfXRefStream::BeginWrite(OutputStreamDevice& device, charbuff& buffer)
{
    (void)device;
    (void)buffer;
    // Do nothing
}

void PdfXRefStream::WriteSubSection(OutputStreamDevice& device, uint32_t first, uint32_t count, charbuff& buffer)
{
    (void)device;
    (void)buffer;
    m_indices.Add(static_cast<int64_t>(first));
    m_indices.Add(static_cast<int64_t>(count));
}

void PdfXRefStream::WriteXRefEntry(OutputStreamDevice& device, const PdfReference& ref,
    const PdfXRefEntry& entry, charbuff& buffer)
{
    (void)device;
    (void)buffer;
    XRefStreamEntry stmEntry;
    stmEntry.Type = static_cast<uint8_t>(entry.Type);

    if (m_xrefStreamObj->GetIndirectReference() == ref)
        m_xrefStreamEntryIndex = (int)m_rawEntries.size();

    switch (entry.Type)
    {
        case PdfXRefEntryType::Free:
            stmEntry.Variant = AS_BIG_ENDIAN(static_cast<uint32_t>(entry.ObjectNumber));
            break;
        case PdfXRefEntryType::InUse:
            stmEntry.Variant = AS_BIG_ENDIAN(static_cast<uint32_t>(entry.Offset));
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    stmEntry.Generation = AS_BIG_ENDIAN(static_cast<uint16_t>(entry.Generation));
    m_rawEntries.push_back(stmEntry);
}

void PdfXRefStream::EndWriteImpl(OutputStreamDevice& device, charbuff& buffer)
{
    PdfArray wArr;
    wArr.Add(static_cast<int64_t>(sizeof(XRefStreamEntry::Type)));
    wArr.Add(static_cast<int64_t>(sizeof(XRefStreamEntry::Variant)));
    wArr.Add(static_cast<int64_t>(sizeof(XRefStreamEntry::Generation)));
 
    m_xrefStreamObj->GetDictionary().AddKey("Index"_n, m_indices);
    m_xrefStreamObj->GetDictionary().AddKey("W"_n, wArr);
 
    // Set the actual offset of the XRefStm object
    uint32_t offset = (uint32_t)device.GetPosition();
    PODOFO_ASSERT(m_xrefStreamEntryIndex >= 0);
    m_rawEntries[m_xrefStreamEntryIndex].Variant = AS_BIG_ENDIAN(offset);
 
    // Write the actual entries data to the XRefStm object stream
    auto& stream = m_xrefStreamObj->GetOrCreateStream();
    stream.SetData(bufferview((const char*)m_rawEntries.data(), m_rawEntries.size() * sizeof(XRefStreamEntry)));
    GetWriter().FillTrailerObject(*m_xrefStreamObj, this->GetSize(), false);

    m_xrefStreamObj->WriteFinal(device, GetWriter().GetWriteFlags(), nullptr, buffer); // CHECK-ME: Requires encryption info??
    m_offset = offset;
}

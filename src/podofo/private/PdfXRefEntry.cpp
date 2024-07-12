/**
 * SPDX-FileCopyrightText: (C) 2009 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PdfDeclarationsPrivate.h"
#include "PdfXRefEntry.h"

#include <podofo/main/PdfCommon.h>

using namespace PoDoFo;

PdfXRefEntry::PdfXRefEntry() :
    Unknown1(0),
    Unknown2(0),
    Type(PdfXRefEntryType::Unknown),
    Parsed(false)
{ }

PdfXRefEntry PdfXRefEntry::CreateFree(uint32_t object, uint16_t generation)
{
    PdfXRefEntry ret;
    ret.ObjectNumber = object;
    ret.Generation = generation;
    ret.Type = PdfXRefEntryType::Free;
    return ret;
}

PdfXRefEntry PdfXRefEntry::CreateInUse(uint64_t offset, uint16_t generation)
{
    PdfXRefEntry ret;
    ret.Offset = offset;
    ret.Generation = generation;
    ret.Type = PdfXRefEntryType::InUse;
    return ret;
}

PdfXRefEntry PdfXRefEntry::CreateCompressed(uint32_t object, unsigned index)
{
    PdfXRefEntry ret;
    ret.ObjectNumber = object;
    ret.Index = index;
    ret.Type = PdfXRefEntryType::Compressed;
    return ret;
}

unsigned PdfXRefEntries::GetSize() const
{
    return (unsigned)m_entries.size();
}

void PdfXRefEntries::Enlarge(unsigned newSize)
{
    // allow caller to specify a max object count to avoid very slow load times on large documents
    if (newSize > PdfCommon::GetMaxObjectCount())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "New size is greater than max pdf object count");

    if (m_entries.size() >= (size_t)newSize)
        return;

    try
    {
        m_entries.resize((size_t)newSize);
    }
    catch (...)
    {
        // If ObjectCount*sizeof(TXRefEntry) > std::numeric_limits<size_t>::max() then
        // resize() throws std::length_error, for smaller allocations that fail it may throw
        // std::bad_alloc (implementation-dependent). "20.5.5.12 Restrictions on exception 
        // handling" in the C++ Standard says any function that throws an exception is allowed 
        // to throw implementation-defined exceptions derived the base type (std::exception)
        // so we need to catch all std::exceptions here
        PODOFO_RAISE_ERROR(PdfErrorCode::OutOfMemory);
    }
}

void PdfXRefEntries::Clear()
{
    m_entries.clear();
}

PdfXRefEntry& PdfXRefEntries::operator[](unsigned index)
{
    return m_entries[index];
}

const PdfXRefEntry& PdfXRefEntries::operator[](unsigned index) const
{
    return m_entries[index];
}

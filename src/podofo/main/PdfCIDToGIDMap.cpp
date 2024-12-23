/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfCIDToGIDMap.h"
#include "PdfDictionary.h"
#include "PdfDocument.h"

using namespace std;
using namespace PoDoFo;

PdfCIDToGIDMap::PdfCIDToGIDMap(CIDToGIDMap&& map)
    : m_cidToGidMap(std::move(map)) { }

PdfCIDToGIDMap PdfCIDToGIDMap::Create(const PdfObject& cidToGidMapObj)
{
    CIDToGIDMap map;
    // Table 115 — Entries in a CIDFont dictionary
    // "The glyph index for a particular CID value c shall be
    // a 2 - byte value stored in bytes 2 × c and 2 × c + 1,
    // where the first byte shall be the high - order byte"
    auto buffer = cidToGidMapObj.MustGetStream().GetCopy();
    for (unsigned i = 0, count = (unsigned)buffer.size() / 2; i < count; i++)
    {
        unsigned gid = (unsigned)((uint8_t)buffer[i * 2 + 0] << 8 | (uint8_t)buffer[i * 2 + 1]);
        map[i] = gid;
    }

    return PdfCIDToGIDMap(std::move(map));
}

bool PdfCIDToGIDMap::TryMapCIDToGID(unsigned cid, unsigned& gid) const
{
    auto found = m_cidToGidMap.find(cid);
    if (found == m_cidToGidMap.end())
    {
        gid = 0;
        return false;
    }

    gid = found->second;
    return true;
}

void PdfCIDToGIDMap::ExportTo(PdfObject& descendantFont)
{
    auto& cidToGidMap = descendantFont.MustGetDocument().GetObjects().CreateDictionaryObject();
    descendantFont.GetDictionary().AddKeyIndirect("CIDToGIDMap"_n, cidToGidMap);
    auto& stream = cidToGidMap.GetOrCreateStream();
    auto output = stream.GetOutputStream();
    unsigned previousCid = 0;
    array<char, 2> entry;
    for (auto& pair : m_cidToGidMap)
    {
        entry = { };
        unsigned cid = previousCid;
        for (; cid < pair.first; cid++)
        {
            // Write zeroes for missing mappings
            output.Write(entry.data(), 2);
        }

        utls::WriteUInt16BE(entry.data(), (uint16_t)pair.second);
        output.Write(entry.data(), 2);
        previousCid = cid;
    }
}

unsigned PdfCIDToGIDMap::GetSize() const
{
    return (unsigned)m_cidToGidMap.size();
}

PdfCIDToGIDMap::iterator PdfCIDToGIDMap::begin() const
{
    return m_cidToGidMap.begin();
}

PdfCIDToGIDMap::iterator PdfCIDToGIDMap::end() const
{
    return m_cidToGidMap.end();
}

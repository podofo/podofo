/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontCIDTrueType.h"

#include <podofo/private/FreetypePrivate.h>

#include "PdfDictionary.h"
#include "PdfDocument.h"
#include "PdfFontTrueTypeSubset.h"

using namespace std;
using namespace PoDoFo;

PdfFontCIDTrueType::PdfFontCIDTrueType(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding) : PdfFontCID(doc, metrics, encoding) { }

PdfFontType PdfFontCIDTrueType::GetType() const
{
    return PdfFontType::CIDTrueType;
}

void PdfFontCIDTrueType::embedFontSubset()
{
    auto& usedGIDs = GetUsedGIDs();
    // Prepare a CID to GID for the subsetting
    CIDToGIDMap cidToGidMap = getCIDToGIDMapSubset(usedGIDs);
    createWidths(GetDescendantFont().GetDictionary(), cidToGidMap);
    m_Encoding->ExportToFont(*this);

    // Prepare a gid list to be used for subsetting
    vector<unsigned> gids;
    for (auto& pair : cidToGidMap)
        gids.push_back(pair.second);

    charbuff buffer;
    PdfFontTrueTypeSubset::BuildFont(buffer, GetMetrics(), gids);
    EmbedFontFileTrueType(GetDescriptor(), buffer);

    // We prepare the /CIDSet content now. NOTE: The CIDSet
    // entry is optional and it's actually deprecated in PDF 2.0
    // but it's required for PDFA/1 compliance in TrueType CID fonts
    string cidSetData;
    for (auto& pair : usedGIDs)
    {
        // ISO 32000-1:2008: Table 124 – Additional font descriptor entries for CIDFonts
        // CIDSet "The stream’s data shall be organized as a table of bits
        // indexed by CID. The bits shall be stored in bytes with the
        // high - order bit first.Each bit shall correspond to a CID.
        // The most significant bit of the first byte shall correspond
        // to CID 0, the next bit to CID 1, and so on"

        static const char bits[] = { '\x80', '\x40', '\x20', '\x10', '\x08', '\x04', '\x02', '\x01' };
        unsigned gid = pair.second.Id;
        unsigned dataIndex = gid >> 3;
        if (cidSetData.size() < dataIndex + 1)
            cidSetData.resize(dataIndex + 1);

        cidSetData[dataIndex] |= bits[gid & 7];
    }

    auto& cidSetObj = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject();
    cidSetObj.GetOrCreateStream().SetData(cidSetData);
    GetDescriptor().GetDictionary().AddKeyIndirect("CIDSet", cidSetObj);
}

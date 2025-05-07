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
#include <podofo/private/FontTrueTypeSubset.h>

using namespace std;
using namespace PoDoFo;

PdfFontCIDTrueType::PdfFontCIDTrueType(PdfDocument& doc, PdfFontMetricsConstPtr&& metrics,
        const PdfEncoding& encoding)
    : PdfFontCID(doc, PdfFontType::CIDTrueType, std::move(metrics), encoding) { }

void PdfFontCIDTrueType::embedFontFileSubset(const vector<PdfCharGIDInfo>& infos,
    const PdfCIDSystemInfo& cidInfo)
{
    (void)cidInfo;
    charbuff buffer;
    FontTrueTypeSubset::BuildFont(GetMetrics(), infos, buffer);
    EmbedFontFileTrueType(GetDescriptor().GetDictionary(), buffer);
}

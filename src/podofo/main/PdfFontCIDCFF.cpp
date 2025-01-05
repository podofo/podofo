/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontCIDCFF.h"
#include <podofo/private/FontUtils.h>

using namespace std;
using namespace PoDoFo;

PdfFontCIDCFF::PdfFontCIDCFF(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
    const PdfEncoding& encoding) : PdfFontCID(doc, metrics, encoding) { }

bool PdfFontCIDCFF::SupportsSubsetting() const
{
    return true;
}

PdfFontType PdfFontCIDCFF::GetType() const
{
    return PdfFontType::CIDCFF;
}

void PdfFontCIDCFF::embedFontFileSubset(const vector<PdfCharGIDInfo>& infos,
    const PdfCIDSystemInfo& cidInfo)
{
    charbuff buffer;
    PoDoFo::SubsetFontCFF(GetMetrics(), infos, cidInfo, buffer);
    EmbedFontFileCFF(GetDescriptor(), buffer, true);
}

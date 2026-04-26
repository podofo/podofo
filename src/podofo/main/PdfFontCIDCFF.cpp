/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontCIDCFF.h"
#ifdef PODOFO_ENABLE_AFDKO
#include <podofo/private/FontUtilsAFDKO.h>
#endif

using namespace std;
using namespace PoDoFo;

PdfFontCIDCFF::PdfFontCIDCFF(PdfDocument& doc, PdfFontMetricsConstPtr&& metrics,
        const PdfEncoding& encoding)
    : PdfFontCID(doc, PdfFontType::CIDCFF, std::move(metrics), encoding) { }

bool PdfFontCIDCFF::SupportsSubsetting() const
{
#ifdef PODOFO_ENABLE_AFDKO
    return true;
#else // PODOFO_ENABLE_AFDKO
    return false;
#endif // PODOFO_ENABLE_AFDKO
}

void PdfFontCIDCFF::embedFontFileSubset(const vector<PdfCharGIDInfo>& infos,
    const PdfCIDSystemInfo& cidInfo)
{
#ifdef PODOFO_ENABLE_AFDKO
    charbuff buffer;
    afdko::SubsetFontCFF(GetMetrics(), infos, cidInfo, buffer);
    EmbedFontFileCFF(GetDescriptor().GetDictionary(), buffer, true);
#else
    (void)infos;
    (void)cidInfo;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "AFDKO support is disabled. Enable PODOFO_ENABLE_AFDKO in CMake to enable it");
#endif
}

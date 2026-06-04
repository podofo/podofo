/// SPDX-FileCopyrightText: (C) 2024 Francesco Pretto <ceztko@gmail.com>
/// SPDX-License-Identifier: Apache-2.0

#ifndef FONT_UTILS_AFDKO_H
#define FONT_UTILS_AFDKO_H

#include <podofo/main/PdfFontMetrics.h>

namespace afdko
{
    void ConvertFontType1ToCFF(const PoDoFo::bufferview & src, PoDoFo::charbuff& dst);
    /// Subset a Type1 or CFF based font to a CFF based font
    void SubsetFontCFF(const PoDoFo::PdfFontMetrics& metrics, const PoDoFo::cspan<PoDoFo::PdfCharGIDInfo>& subsetInfos,
        const PoDoFo::PdfCIDSystemInfo& cidInfo, PoDoFo::charbuff& dstCFF);
}

#endif // FONT_UTILS_AFDKO_H

/**
 * SPDX-FileCopyrightText: (C) 2024 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <podofo/main/PdfFontMetrics.h>

namespace PoDoFo
{
    void ConvertFontType1ToCFF(const bufferview& src, charbuff& dst);
    /** Subset a Type1 or CFF based font to a CFF based font 
     */
    void SubsetFontCFF(const PdfFontMetrics& metrics, const cspan<PdfCharGIDInfo>& subsetInfos,
        const PdfCIDSystemInfo& cidInfo, charbuff& dstCFF);
}

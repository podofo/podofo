/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_CMAP_ENCODING_H
#define PDF_CMAP_ENCODING_H

#include "PdfEncodingMap.h"

namespace PoDoFo
{
    class PODOFO_API PdfCMapEncoding final : public PdfEncodingMapBase
    {
        friend class PdfEncodingMapFactory;

    public:
        /** Construct a PdfCMapEncoding from a map
         */
        PdfCMapEncoding(PdfCharCodeMap&& map);

    private:
        PdfCMapEncoding(PdfCharCodeMap&& map, const PdfEncodingLimits& limits, int wmode);

    public:
        bool HasLigaturesSupport() const override;
        const PdfEncodingLimits& GetLimits() const override;
        int GetWModeRaw() const override;

    private:
        PdfEncodingLimits m_Limits;
        int m_WMode;
    };
}

#endif // PDF_CMAP_ENCODING_H

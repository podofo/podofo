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
    class PdfObject;
    class PdfObjectStream;

    class PODOFO_API PdfCMapEncoding final : public PdfEncodingMapBase
    {
        friend class PdfEncodingMap;

    public:
        /** Construct a PdfCMapEncoding from a map
         */
        PdfCMapEncoding(PdfCharCodeMap&& map);

    public:
        /** Construct an encoding map from an object
         */
        static std::unique_ptr<PdfEncodingMap> CreateFromObject(const PdfObject& cmapObj);

    private:
        PdfCMapEncoding(PdfCharCodeMap&& map, const PdfEncodingLimits& limits);

    public:
        bool HasLigaturesSupport() const override;
        const PdfEncodingLimits& GetLimits() const override;

    private:
        PdfEncodingLimits m_Limits;
    };
}

#endif // PDF_CMAP_ENCODING_H

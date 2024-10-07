/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_PREDEFINED_TO_UNICODE_CMAP_H
#define PDF_PREDEFINED_TO_UNICODE_CMAP_H

#include "PdfCMapEncoding.h"

namespace PoDoFo
{
    /**
     * Represents a predefined ToUnicode CMap as the ones described in
     * ISO 32000-2:2020 "9.10.2 Mapping character codes to Unicode values"
     * that can be downloaded from https://github.com/adobe-type-tools/mapping-resources-pdf,
     * folder "pdf2unicode"
     */
    class PODOFO_API PdfPredefinedToUnicodeCMap final : public PdfEncodingMap
    {
        friend class PdfEncodingFactory;

    private:
        PdfPredefinedToUnicodeCMap(PdfCMapEncodingConstPtr&& toUnicode, PdfCMapEncodingConstPtr&& cidEncoding);

    public:
        const PdfEncodingLimits& GetLimits() const override;

    protected:
        bool tryGetCodePoints(const PdfCharCode& codeUnit, const unsigned* cidId, CodePointSpan& codePoints) const override;
        bool tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const override;
        bool tryGetCharCodeSpan(const unicodeview& ligature, PdfCharCode& codeUnit) const override;

        void AppendToUnicodeEntries(OutputStream& stream, charbuff& temp) const override;
        void AppendCIDMappingEntries(OutputStream& stream, const PdfFont& font, charbuff& temp) const override;

    public:
        const PdfCMapEncoding& GetToUnicodeMap() { return *m_ToUnicode; }
        const PdfCMapEncoding& GetCIDEncodingMap() { return *m_CIDEncoding; }

    private:
        PdfCMapEncodingConstPtr m_ToUnicode;
        PdfCMapEncodingConstPtr m_CIDEncoding;
    };
}

#endif // PDF_PREDEFINED_TO_UNICODE_CMAP_H

/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_DYNAMIC_ENCODING_H
#define PDF_DYNAMIC_ENCODING_H

#include "PdfEncoding.h"
#include "PdfCharCodeMap.h"

namespace PoDoFo {

    class PdfFont;

    /**
     * Encoding shim class that mocks an existing encoding
     * Used by PdfFont to to wrap
     */
    class PODOFO_API PdfEncodingShim final : public PdfEncoding
    {
        friend class PdfFont;

    private:
        PdfEncodingShim(const PdfEncoding& encoding, PdfFont& font);

    public:
        PdfFont& GetFont() const override;

    private:
        PdfFont* m_Font;
    };

    /**
     * WIP: Encoding class with an external encoding map storage
     * To be used by PdfFont in case of dynamic encoding requested
     */
    class PODOFO_API PdfDynamicEncoding final : public PdfEncoding
    {
        friend class PdfFont;
        friend class PdfEncodingFactory;

    public:
        bool IsDynamicEncoding() const override;

    protected:
        PdfFont& GetFont() const override;

    private:
        /**
         * To be used by PdfFont
         */
        PdfDynamicEncoding(const std::shared_ptr<PdfCharCodeMap>& cidMap,
            const std::shared_ptr<PdfCharCodeMap>& toUnicodeMap, PdfFont& font);

    private:
        PdfFont* m_font;
    };

};

#endif // PDF_DYNAMIC_ENCODING_H

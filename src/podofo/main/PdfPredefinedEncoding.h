/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_SIMPLE_ENCODING_H
#define PDF_SIMPLE_ENCODING_H

#include "PdfEncodingMap.h"

namespace PoDoFo
{
    /**
     * A common base class for Pdf defined predefined encodings which are
     * known by name.
     *
     *  - WinAnsiEncoding
     *  - MacRomanEncoding
     *  - MacExpertEncoding
     *
     *  \see PdfWinAnsiEncoding
     *  \see PdfMacRomanEncoding
     *  \see PdfMacExportEncoding
     */
    class PODOFO_API PdfPredefinedEncoding : public PdfBuiltInEncoding
    {
        friend class PdfWinAnsiEncoding;
        friend class PdfMacRomanEncoding;
        friend class PdfMacExpertEncoding;

    private:
        PdfPredefinedEncoding(const PdfName& name);

        PdfPredefinedEncodingType GetPredefinedEncodingType() const override;

    public:
        /** Try get a latin text character name from a codepoint,
         * as listed by ISO 32000-2:2020 Table D.1 "Latin-text encodings"
         */
        static bool TryGetCharNameFromCodePoint(char32_t codepoint, const PdfName*& name);

    protected:
        void getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const override;
    };

    /**
     * The WinAnsiEncoding is the default encoding in PoDoFo for
     * contents on PDF pages.
     *
     * It is also called CP-1252 encoding.
     * This class may be used as base for derived encodings.
     *
     * \see PdfWin1250Encoding
     *
     * \see PdfFont::WinAnsiEncoding
     */
    class PODOFO_API PdfWinAnsiEncoding final : public PdfPredefinedEncoding
    {
        friend class PdfEncodingMapFactory;
        friend class PdfWin1250Encoding;
        friend class PdfIso88592Encoding;

    private:
        PdfWinAnsiEncoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from WinAnsiEncoding to UTF16

    };

    /**
     * MacRomanEncoding 
     */
    class PODOFO_API PdfMacRomanEncoding final : public PdfPredefinedEncoding
    {
        friend class PdfEncodingMapFactory;

    private:
        PdfMacRomanEncoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from MacRomanEncoding to UTF16
    };

    /**
     * MacExpertEncoding
     */
    class PODOFO_API PdfMacExpertEncoding final : public PdfPredefinedEncoding
    {
        friend class PdfEncodingMapFactory;

    private:
        PdfMacExpertEncoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from MacExpertEncoding to UTF16
    };

    /**
     * StandardEncoding
     */
    class PODOFO_API PdfStandardEncoding final : public PdfBuiltInEncoding
    {
        friend class PdfEncodingMapFactory;

    private:
        PdfStandardEncoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from StandardEncoding to UTF16
    };

    /**
     * Symbol Encoding
     */
    class PODOFO_API PdfSymbolEncoding final : public PdfBuiltInEncoding
    {
        friend class PdfEncodingMapFactory;

    private:
        PdfSymbolEncoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from SymbolEncoding to UTF16
    };

    /**
     * ZapfDingbats encoding
     */
    class PODOFO_API PdfZapfDingbatsEncoding final : public PdfBuiltInEncoding
    {
        friend class PdfEncodingMapFactory;

    private:
        PdfZapfDingbatsEncoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from ZapfDingbatsEncoding to UTF16
    };
}

#endif // PDF_SIMPLE_ENCODING_H

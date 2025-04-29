/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_ENCODING_PRIVATE_H
#define PDF_ENCODING_PRIVATE_H

#include <podofo/main/PdfEncodingCommon.h>

namespace PoDoFo
{
    class PdfCharCodeMap;
    class OutputStream;

    // Known encoding IDs
    constexpr unsigned NullEncodingId             = 0;
    constexpr unsigned WinAnsiEncodingId          = 11;
    constexpr unsigned MacRomanEncodingId         = 12;
    constexpr unsigned MacExpertEncodingId        = 13;
    constexpr unsigned StandardEncodingId         = 21;
    constexpr unsigned SymbolEncodingId           = 22;
    constexpr unsigned ZapfDingbatsEncodingId     = 23;
    constexpr unsigned CustomEncodingStartId      = 101;

    /** Check if the chars in the given utf-8 view are eligible for PdfDocEncofing conversion
     *
     * /param isPdfDocEncoding the given utf-8 string is coincident in PdfDocEncoding representation
     */
    bool CheckValidUTF8ToPdfDocEcondingChars(const std::string_view& view, bool& isAsciiEqual);
    bool IsPdfDocEncodingCoincidentToUTF8(std::string_view view);
    bool TryConvertUTF8ToPdfDocEncoding(const std::string_view& view, std::string& pdfdocencstr);
    std::string ConvertUTF8ToPdfDocEncoding(const std::string_view& view);
    std::string ConvertPdfDocEncodingToUTF8(const std::string_view& view, bool& isAsciiEqual);
    void ConvertPdfDocEncodingToUTF8(std::string_view view, std::string& u8str, bool& isAsciiEqual);

    // Map code point(s) -> code units
    struct CodePointMapNode
    {
        codepoint CodePoint;
        PdfCharCode CodeUnit;
        CodePointMapNode* Ligatures;
        CodePointMapNode* Left;
        CodePointMapNode* Right;
    };

    /**
     * Methods to maintain and query a reverse map from code points to code units
     */

    bool TryGetCodeReverseMap(const CodePointMapNode* root, const codepointview& codePoints, PdfCharCode& codeUnit);
    bool TryGetCodeReverseMap(const CodePointMapNode* root, codepoint codePoint, PdfCharCode& codeUnit);
    bool TryGetCodeReverseMap(const CodePointMapNode* root, std::string_view::iterator& it,
        const std::string_view::iterator& end, PdfCharCode& codeUnit);
    void PushMappingReverseMap(CodePointMapNode*& root, const codepointview& codePoints, const PdfCharCode& codeUnit);
    void DeleteNodeReverseMap(CodePointMapNode* node);

    /**
     * Low level serialization commodities
     */

    void AppendCodeSpaceRangeTo(OutputStream& stream, const PdfCharCodeMap& charMap, charbuff& temp);
    void AppendCIDMappingEntriesTo(OutputStream& stream, const PdfCharCodeMap& charMap, charbuff& temp);
    void AppendToUnicodeEntriesTo(OutputStream& stream, const PdfCharCodeMap& charMap, charbuff& temp);
    void WriteCIDMapping(OutputStream& stream, const PdfCharCode& unit, unsigned cid, charbuff& temp);
    void WriteCIDRange(OutputStream& stream, const PdfCharCode& srcCodeLo, const PdfCharCode& srcCodeHi, unsigned dstCidLo, charbuff& temp);
    void AppendUTF16CodeTo(OutputStream& stream, char32_t codePoint, std::u16string& u16tmp);
    void AppendUTF16CodeTo(OutputStream& stream, const unicodeview& codePoints, std::u16string& u16tmp);
}

#endif // PDF_ENCODING_PRIVATE_H

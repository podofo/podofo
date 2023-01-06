/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_ENCODING_PRIVATE_H
#define PDF_ENCODING_PRIVATE_H

namespace PoDoFo
{
    // Known encoding IDs
    constexpr size_t NullEncodingId             = 0;
    constexpr size_t WinAnsiEncodingId          = 11;
    constexpr size_t MacRomanEncodingId         = 12;
    constexpr size_t MacExpertEncodingId        = 13;
    constexpr size_t StandardEncodingId         = 21;
    constexpr size_t SymbolEncodingId           = 22;
    constexpr size_t ZapfDingbatsEncodingId     = 23;
    constexpr size_t CustomEncodingStartId      = 101;

    /** Check if the chars in the given utf-8 view are elegible for PdfDocEncofing conversion
     *
     * /param isPdfDocEncoding the given utf-8 string is coincident in PdfDocEncoding representation
     */
    bool CheckValidUTF8ToPdfDocEcondingChars(const std::string_view& view, bool& isAsciiEqual);
    bool IsPdfDocEncodingCoincidentToUTF8(const std::string_view& view);
    bool TryConvertUTF8ToPdfDocEncoding(const std::string_view& view, std::string& pdfdocencstr);
    std::string ConvertUTF8ToPdfDocEncoding(const std::string_view& view);
    std::string ConvertPdfDocEncodingToUTF8(const std::string_view& view, bool& isAsciiEqual);
    void ConvertPdfDocEncodingToUTF8(const std::string_view& view, std::string& u8str, bool& isAsciiEqual);
}

#endif // PDF_ENCODING_PRIVATE_H

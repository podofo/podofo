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
}

#endif // PDF_ENCODING_PRIVATE_H

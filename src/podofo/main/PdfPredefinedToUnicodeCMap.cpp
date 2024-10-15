/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfPredefinedToUnicodeCMap.h"

using namespace std;
using namespace PoDoFo;

PdfPredefinedToUnicodeCMap::PdfPredefinedToUnicodeCMap(PdfCMapEncodingConstPtr&& toUnicode, PdfCMapEncodingConstPtr&& cidEncoding)
    : PdfEncodingMap(PdfEncodingMapType::CMap), m_ToUnicode(std::move(toUnicode)), m_CIDEncoding(std::move(cidEncoding))
{
    PODOFO_ASSERT(m_ToUnicode != nullptr && m_CIDEncoding != nullptr);
}

const PdfEncodingLimits& PdfPredefinedToUnicodeCMap::GetLimits() const
{
    return m_CIDEncoding->GetLimits();
}

bool PdfPredefinedToUnicodeCMap::tryGetCodePoints(const PdfCharCode& codeUnit, const unsigned* cidId, CodePointSpan& codePoints) const
{
    // ISO 32000-2:2020 "9.10.2 Mapping character codes to Unicode values"
    // "e. Map the CID obtained in step (a) according to the CMap obtained in step (d), producing a
    // Unicode value"
    if (cidId == nullptr)
    {
        if (!m_CIDEncoding->GetCharMap().TryGetCodePoints(codeUnit, codePoints))
            return false;

        return m_ToUnicode->GetCharMap().TryGetCodePoints(PdfCharCode(*codePoints, 2), codePoints);
    }
    else
    {
        // Take advantage of knowing the CID in advance and map directly using
        // the ToUnicode map
        return m_ToUnicode->GetCharMap().TryGetCodePoints(PdfCharCode(*cidId, 2), codePoints);
    }
}

bool PdfPredefinedToUnicodeCMap::tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const
{
    if (!m_ToUnicode->GetCharMap().TryGetCharCode(codePoint, codeUnit))
        return false;

    return m_CIDEncoding->GetCharMap().TryGetCharCode(codeUnit.Code, codeUnit);
}

bool PdfPredefinedToUnicodeCMap::tryGetCharCodeSpan(const unicodeview& ligature, PdfCharCode& codeUnit) const
{
    if (!m_ToUnicode->GetCharMap().TryGetCharCode(ligature, codeUnit))
        return false;

    return m_CIDEncoding->GetCharMap().TryGetCharCode(codeUnit.Code, codeUnit);
}

void PdfPredefinedToUnicodeCMap::AppendToUnicodeEntries(OutputStream& stream, charbuff& temp) const
{
    (void)stream;
    (void)temp;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Unsupported serializing a predefined ToUnicode map");
}

void PdfPredefinedToUnicodeCMap::AppendCIDMappingEntries(OutputStream& stream, const PdfFont& font, charbuff& temp) const
{
    (void)stream;
    (void)font;
    (void)temp;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Unsupported serializing a predefined ToUnicode map");
}

/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontMetricsObject.h"

#include <podofo/private/PdfEncodingPrivate.h>
#include <podofo/private/FreetypePrivate.h>

#include "PdfIdentityEncoding.h"
#include "PdfEncodingMapFactory.h"
#include "PdfDifferenceEncoding.h"

using namespace std;
using namespace PoDoFo;

namespace PoDoFo
{
    /** A built-in encoding for a /Type1 font program
     */
    class PdfFontBuiltinType1Encoding final : public PdfEncodingMapSimple
    {
    public:
        PdfFontBuiltinType1Encoding(PdfCharCodeMap&& map)
            : PdfEncodingMapSimple(map.GetLimits()), m_charMap(std::move(map)) { }

        bool tryGetCharCodeSpan(const unicodeview& codePoints, PdfCharCode& codeUnit) const override
        {
            return m_charMap.TryGetCharCode(codePoints, codeUnit);
        }

        bool tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const override
        {
            return m_charMap.TryGetCharCode(codePoint, codeUnit);
        }

        bool tryGetCodePoints(const PdfCharCode& code, const unsigned* cidId, CodePointSpan& codePoints) const override
        {
            (void)cidId;
            return m_charMap.TryGetCodePoints(code, codePoints);
        }

        void AppendToUnicodeEntries(OutputStream& stream, charbuff& temp) const override
        {
            PoDoFo::AppendToUnicodeEntriesTo(stream, m_charMap, temp);
        }

        void AppendCIDMappingEntries(OutputStream& stream, const PdfFont& font, charbuff& temp) const override
        {
            (void)font;
            PoDoFo::AppendCIDMappingEntriesTo(stream, m_charMap, temp);
        }

    protected:
        PdfCharCodeMap m_charMap;
    };
}

PdfEncodingMapConstPtr PdfFontMetrics::getFontType1BuiltInEncoding(FT_Face face) const
{
    // 9.6.5 Character encoding
    // "In PDF, a font is classified as either nonsymbolic or symbolic according to whether all of its characters
    // are members of the standard Latin character set; see D.2, "Latin character set and encodings". This
    // shall be indicated by flags in the font descriptor; see 9.8.2, "Font descriptor flags".Symbolic fonts
    // contain other character sets, to which the encodings mentioned previously ordinarily do not apply.
    // Such font programs have built - in encodings that are usually unique to each font"

    // "...the font program’s built-in encoding, as described in 9.6.5, "Character
    // encoding" and further elaborated in the subclauses on specific font types.
    // Otherwise, for a nonsymbolic font, it shall be StandardEncoding, and for a
    // symbolic font, it shall be the font's built-in encoding"

    // Annex D Character sets and encodings
    // "Standard Latin-text encoding. This is the built-in encoding defined in Type 1
    // Latin - text font programs"

    if ((GetFlags() & PdfFontDescriptorFlags::NonSymbolic) != PdfFontDescriptorFlags::None)
        return PdfEncodingMapFactory::GetStandardEncodingInstancePtr();

    FT_ULong code;
    FT_UInt index;
    PdfCharCodeMap codeMap;
    if (FT_Select_Charmap(face, FT_ENCODING_ADOBE_CUSTOM) == 0)
    {
        if (!FT_HAS_GLYPH_NAMES(face))
            return nullptr;

        FT_Error rc;
        char buffer[64];
        CodePointSpan codepoints;
        code = FT_Get_First_Char(face, &index);
        while (index != 0)
        {
            rc = FT_Get_Glyph_Name(face, index, buffer, (FT_UInt)std::size(buffer));
            if (rc != 0 || !PdfDifferenceEncoding::TryGetCodePointsFromCharName(
                (const char*)buffer, codepoints))
            {
                goto Continue;
            }

            codeMap.PushMapping(PdfCharCode(code, 1), codepoints);
        Continue:
            code = FT_Get_Next_Char(face, code, &index);
        }

        return PdfEncodingMapConstPtr(new PdfFontBuiltinType1Encoding(std::move(codeMap)));
    }
    else if (FT_Select_Charmap(face, FT_ENCODING_UNICODE) == 0)
    {
        // CHECK-ME: The following is fishy
        // NOTE: Some very strange CFF fonts just supply an unicode map
        // For these, we just assume code identity with Unicode codepoint
        code = FT_Get_First_Char(face, &index);
        while (index != 0)
        {
            codeMap.PushMapping(PdfCharCode(code), (char32_t)code);
            code = FT_Get_Next_Char(face, code, &index);
        }
    }

    return nullptr;
}

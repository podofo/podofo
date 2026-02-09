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
    // shall be indicated by flags in the font descriptor; see 9.8.2, "Font descriptor flags". Symbolic fonts
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

    if (FT_Select_Charmap(face, FT_ENCODING_UNICODE) == 0)
    {
        // NOTE: It's enough to try to select an unicode charmap,
        // as freetype generates a unicode charmap for Type1 fonts
        // even if they do not contain one using the Adobe Glyph List (AGL)
        // https://gitlab.freedesktop.org/freetype/freetype/-/blob/master/devel/ftoption.h?ref_type=85c8efe0afa5ad0df35114e317a065f544943c52#L339
        // TODO: Make a fallback in case FT_CONFIG_OPTION_ADOBE_GLYPH_LIST is not defined
        // This requires selecting FT_ENCODING_ADOBE_STANDARD or FT_ENCODING_ADOBE_STANDARD,
        // iterate all codes to 0xFF, get the glyph name for the selected glyph and
        // convert to unicode with PdfDifferenceEncoding::TryGetCodePointsFromCharName()

        FT_ULong code;
        FT_UInt index;
        PdfCharCodeMap codeMap;
        CodePointSpan codepoints;

        // NOTE: It's safe to assume the base encoding is a one byte encoding.
        // Iterate all the range, as the base encoding may be narrow
        auto& standardEncoding = PdfEncodingMapFactory::GetStandardEncodingInstance();
        for (unsigned code = 0; code <= 0xFFU; code++)
        {
            index = FT_Get_Char_Index(face, code);
            if (index != 0)
            {
                // If the match succeded then by definition the code
                // is a also a valid Unicode code point, so we can just
                // add an identity mapping
                codeMap.PushMapping(PdfCharCode(code, 1), (char32_t)code);
                continue;
            }

            // If the character is not mapped by this char map, we use
            // the standard encoding as a fallback. This is not documented
            // in the specification but it's been tested with Adobe Acrobat
            if (!standardEncoding.TryGetCodePoints(PdfCharCode(code, 1), codepoints))
                continue;

            codeMap.PushMapping(PdfCharCode(code, 1), codepoints);
        }

        return PdfEncodingMapConstPtr(new PdfFontBuiltinType1Encoding(std::move(codeMap)));
    }

    return nullptr;
}

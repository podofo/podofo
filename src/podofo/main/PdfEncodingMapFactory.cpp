/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfEncodingMapFactory.h"
#include "PdfPredefinedEncoding.h"
#include "PdfIdentityEncoding.h"
#include "PdfDifferenceEncoding.h"

using namespace std;
using namespace PoDoFo;

namespace PoDoFo
{
    class AppleLatin1Encoding final : public PdfBuiltInEncoding
    {
    public:
        AppleLatin1Encoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_encoding[256]; // conversion table from SymbolEncoding to UTF16
    };
}

PdfBuiltInEncodingConstPtr PdfEncodingMapFactory::GetWinAnsiEncodingInstancePtr()
{
    return getWinAnsiEncodingInstancePtr();
}

const PdfBuiltInEncoding& PdfEncodingMapFactory::GetWinAnsiEncodingInstance()
{
    return *getWinAnsiEncodingInstancePtr();
}

PdfBuiltInEncodingConstPtr PdfEncodingMapFactory::GetMacRomanEncodingInstancePtr()
{
    return getMacRomanEncodingInstancePtr();
}

const PdfBuiltInEncoding& PdfEncodingMapFactory::GetMacRomanEncodingInstance()
{
    return *getMacRomanEncodingInstancePtr();
}

PdfBuiltInEncodingConstPtr PdfEncodingMapFactory::GetMacExpertEncodingInstancePtr()
{
    return getMacExpertEncodingInstancePtr();
}

const PdfBuiltInEncoding& PdfEncodingMapFactory::GetMacExpertEncodingInstance()
{
    return *getMacExpertEncodingInstancePtr();
}

PdfBuiltInEncodingConstPtr PdfEncodingMapFactory::GetStandardEncodingInstancePtr()
{
    return getStandardEncodingInstancePtr();
}

const PdfBuiltInEncoding& PdfEncodingMapFactory::GetStandardEncodingInstance()
{
    return *getStandardEncodingInstancePtr();
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::GetHorizontalIdentityEncodingInstancePtr()
{
    return getHorizontalIdentityEncodingInstancePtr();
}

const PdfEncodingMap& PdfEncodingMapFactory::GetHorizontalIdentityEncodingInstance()
{
    return *getHorizontalIdentityEncodingInstancePtr();
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::GetVerticalIdentityEncodingInstancePtr()
{
    return getVerticalIdentityEncodingInstancePtr();
}

const PdfEncodingMap& PdfEncodingMapFactory::GetVerticalIdentityEncodingInstance()
{
    return *getVerticalIdentityEncodingInstancePtr();
}

const PdfEncodingMapConstPtr& PdfEncodingMapFactory::GetNullEncodingInstancePtr()
{
    static PdfEncodingMapConstPtr s_instance(new PdfNullEncodingMap());
    return s_instance;
}

const PdfBuiltInEncodingConstPtr& PdfEncodingMapFactory::GetAppleLatin1EncodingInstancePtr()
{
    static PdfBuiltInEncodingConstPtr s_instance(new AppleLatin1Encoding());
    return s_instance;
}

const PdfBuiltInEncodingConstPtr& PdfEncodingMapFactory::GetSymbolEncodingInstancePtr()
{
    static PdfBuiltInEncodingConstPtr s_instance(new PdfSymbolEncoding());
    return s_instance;
}

const PdfBuiltInEncodingConstPtr& PdfEncodingMapFactory::GetZapfDingbatsEncodingInstancePtr()
{
    static PdfBuiltInEncodingConstPtr s_instance(new PdfZapfDingbatsEncoding());
    return s_instance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::GetStandard14FontEncodingInstancePtr(PdfStandard14FontType stdFont)
{
    return getStandard14FontEncodingInstancePtr(stdFont);
}

const PdfEncodingMap& PdfEncodingMapFactory::GetStandard14FontEncodingInstance(PdfStandard14FontType stdFont)
{
    return *getStandard14FontEncodingInstancePtr(stdFont);
}

const PdfBuiltInEncodingConstPtr& PdfEncodingMapFactory::getWinAnsiEncodingInstancePtr()
{
    static PdfBuiltInEncodingConstPtr s_instance(new PdfWinAnsiEncoding());
    return s_instance;
}

const PdfBuiltInEncodingConstPtr& PdfEncodingMapFactory::getMacRomanEncodingInstancePtr()
{
    static PdfBuiltInEncodingConstPtr s_instance(new PdfMacRomanEncoding());
    return s_instance;
}

const PdfBuiltInEncodingConstPtr& PdfEncodingMapFactory::getMacExpertEncodingInstancePtr()
{
    static PdfBuiltInEncodingConstPtr s_instance(new PdfMacExpertEncoding());
    return s_instance;
}

const PdfBuiltInEncodingConstPtr& PdfEncodingMapFactory::getStandardEncodingInstancePtr()
{
    static PdfBuiltInEncodingConstPtr s_instance(new PdfStandardEncoding());
    return s_instance;
}

const PdfEncodingMapConstPtr& PdfEncodingMapFactory::getHorizontalIdentityEncodingInstancePtr()
{
    static PdfEncodingMapConstPtr s_instance(new PdfIdentityEncoding(PdfIdentityOrientation::Horizontal));
    return s_instance;
}

const PdfEncodingMapConstPtr& PdfEncodingMapFactory::getVerticalIdentityEncodingInstancePtr()
{
    static PdfEncodingMapConstPtr s_instance(new PdfIdentityEncoding(PdfIdentityOrientation::Vertical));
    return s_instance;
}

const PdfBuiltInEncodingConstPtr& PdfEncodingMapFactory::getStandard14FontEncodingInstancePtr(PdfStandard14FontType stdFont)
{
    switch (stdFont)
    {
        case PdfStandard14FontType::TimesRoman:
        case PdfStandard14FontType::TimesItalic:
        case PdfStandard14FontType::TimesBold:
        case PdfStandard14FontType::TimesBoldItalic:
        case PdfStandard14FontType::Helvetica:
        case PdfStandard14FontType::HelveticaOblique:
        case PdfStandard14FontType::HelveticaBold:
        case PdfStandard14FontType::HelveticaBoldOblique:
        case PdfStandard14FontType::Courier:
        case PdfStandard14FontType::CourierOblique:
        case PdfStandard14FontType::CourierBold:
        case PdfStandard14FontType::CourierBoldOblique:
            return getStandardEncodingInstancePtr();
        case PdfStandard14FontType::Symbol:
            return GetSymbolEncodingInstancePtr();
        case PdfStandard14FontType::ZapfDingbats:
            return GetZapfDingbatsEncodingInstancePtr();
        case PdfStandard14FontType::Unknown:
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Invalid Standard14 font type");
    }
}

// https://en.wikipedia.org/wiki/PostScript_Latin_1_Encoding
AppleLatin1Encoding::AppleLatin1Encoding()
    : PdfBuiltInEncoding("ISOLatin1Encoding")
{
}

const char32_t* AppleLatin1Encoding::GetToUnicodeTable() const
{
    return s_encoding;
}

const char32_t AppleLatin1Encoding::s_encoding[256] = {
    0x0000, // 00 NULL
    0x0001, // 01 START OF HEADING
    0x0002, // 02 START OF TEXT
    0x0003, // 03 END OF TEXT
    0x0004, // 04 END OF TRANSMISSION
    0x0005, // 05 ENQUIRY
    0x0006, // 06 ACKNOWLEDGE
    0x0007, // 07 BELL
    0x0008, // 08 BACKSPACE
    0x0009, // 09 HORIZONTAL TABULATION
    0x000A, // 0A LINE FEED
    0x000B, // 0B VERTICAL TABULATION
    0x000C, // 0C FORM FEED
    0x000D, // 0D CARRIAGE RETURN
    0x000E, // 0E SHIFT OUT
    0x000F, // 0F SHIFT IN
    0x0010, // 10 DATA LINK ESCAPE
    0x0011, // 11 DEVICE CONTROL ONE
    0x0012, // 12 DEVICE CONTROL TWO
    0x0013, // 13 DEVICE CONTROL THREE
    0x0014, // 14 DEVICE CONTROL FOUR
    0x0015, // 15 NEGATIVE ACKNOWLEDGE
    0x0016, // 16 SYNCHRONOUS IDLE
    0x0017, // 17 END OF TRANSMISSION BLOCK
    0x0018, // 18 CANCEL
    0x0019, // 19 END OF MEDIUM
    0x001A, // 1A SUBSTITUTE
    0x001B, // 1B ESCAPE
    0x001C, // 1C FILE SEPARATOR
    0x001D, // 1D GROUP SEPARATOR
    0x001E, // 1E RECORD SEPARATOR
    0x001F, // 1F UNIT SEPARATOR
    0x0020, // 20 SPACE
    0x0021, // 21 EXCLAMATION MARK
    0x0022, // 22 QUOTATION MARK
    0x0023, // 23 NUMBER SIGN
    0x0024, // 24 DOLLAR SIGN
    0x0025, // 25 PERCENT SIGN
    0x0026, // 26 AMPERSAND
    0x2018, // 27 LEFT SINGLE QUOTATION MARK
    0x0028, // 28 LEFT PARENTHESIS
    0x0029, // 29 RIGHT PARENTHESIS
    0x002A, // 2A ASTERISK
    0x002B, // 2B PLUS SIGN
    0x002C, // 2C COMMA
    0x002D, // 2D HYPHEN-MINUS
    0x002E, // 2E FULL STOP
    0x002F, // 2F SOLIDUS
    0x0030, // 30 DIGIT ZERO
    0x0031, // 31 DIGIT ONE
    0x0032, // 32 DIGIT TWO
    0x0033, // 33 DIGIT THREE
    0x0034, // 34 DIGIT FOUR
    0x0035, // 35 DIGIT FIVE
    0x0036, // 36 DIGIT SIX
    0x0037, // 37 DIGIT SEVEN
    0x0038, // 38 DIGIT EIGHT
    0x0039, // 39 DIGIT NINE
    0x003A, // 3A COLON
    0x003B, // 3B SEMICOLON
    0x003C, // 3C LESS-THAN SIGN
    0x003D, // 3D EQUALS SIGN
    0x003E, // 3E GREATER-THAN SIGN
    0x003F, // 3F QUESTION MARK
    0x0040, // 40 COMMERCIAL AT
    0x0041, // 41 LATIN CAPITAL LETTER A
    0x0042, // 42 LATIN CAPITAL LETTER B
    0x0043, // 43 LATIN CAPITAL LETTER C
    0x0044, // 44 LATIN CAPITAL LETTER D
    0x0045, // 45 LATIN CAPITAL LETTER E
    0x0046, // 46 LATIN CAPITAL LETTER F
    0x0047, // 47 LATIN CAPITAL LETTER G
    0x0048, // 48 LATIN CAPITAL LETTER H
    0x0049, // 49 LATIN CAPITAL LETTER I
    0x004A, // 4A LATIN CAPITAL LETTER J
    0x004B, // 4B LATIN CAPITAL LETTER K
    0x004C, // 4C LATIN CAPITAL LETTER L
    0x004D, // 4D LATIN CAPITAL LETTER M
    0x004E, // 4E LATIN CAPITAL LETTER N
    0x004F, // 4F LATIN CAPITAL LETTER O
    0x0050, // 50 LATIN CAPITAL LETTER P
    0x0051, // 51 LATIN CAPITAL LETTER Q
    0x0052, // 52 LATIN CAPITAL LETTER R
    0x0053, // 53 LATIN CAPITAL LETTER S
    0x0054, // 54 LATIN CAPITAL LETTER T
    0x0055, // 55 LATIN CAPITAL LETTER U
    0x0056, // 56 LATIN CAPITAL LETTER V
    0x0057, // 57 LATIN CAPITAL LETTER W
    0x0058, // 58 LATIN CAPITAL LETTER X
    0x0059, // 59 LATIN CAPITAL LETTER Y
    0x005A, // 5A LATIN CAPITAL LETTER Z
    0x005B, // 5B LEFT SQUARE BRACKET
    0x005C, // 5C REVERSE SOLIDUS
    0x005D, // 5D RIGHT SQUARE BRACKET
    0x005E, // 5E CIRCUMFLEX ACCENT
    0x005F, // 5F LOW LINE
    0x2019, // 60 RIGHT SINGLE QUOTATION MARK
    0x0061, // 61 LATIN SMALL LETTER A
    0x0062, // 62 LATIN SMALL LETTER B
    0x0063, // 63 LATIN SMALL LETTER C
    0x0064, // 64 LATIN SMALL LETTER D
    0x0065, // 65 LATIN SMALL LETTER E
    0x0066, // 66 LATIN SMALL LETTER F
    0x0067, // 67 LATIN SMALL LETTER G
    0x0068, // 68 LATIN SMALL LETTER H
    0x0069, // 69 LATIN SMALL LETTER I
    0x006A, // 6A LATIN SMALL LETTER J
    0x006B, // 6B LATIN SMALL LETTER K
    0x006C, // 6C LATIN SMALL LETTER L
    0x006D, // 6D LATIN SMALL LETTER M
    0x006E, // 6E LATIN SMALL LETTER N
    0x006F, // 6F LATIN SMALL LETTER O
    0x0070, // 70 LATIN SMALL LETTER P
    0x0071, // 71 LATIN SMALL LETTER Q
    0x0072, // 72 LATIN SMALL LETTER R
    0x0073, // 73 LATIN SMALL LETTER S
    0x0074, // 74 LATIN SMALL LETTER T
    0x0075, // 75 LATIN SMALL LETTER U
    0x0076, // 76 LATIN SMALL LETTER V
    0x0077, // 77 LATIN SMALL LETTER W
    0x0078, // 78 LATIN SMALL LETTER X
    0x0079, // 79 LATIN SMALL LETTER Y
    0x007A, // 7A LATIN SMALL LETTER Z
    0x007B, // 7B LEFT CURLY BRACKET
    0x007C, // 7C VERTICAL LINE
    0x007D, // 7D RIGHT CURLY BRACKET
    0x007E, // 7E TILDE
    0x007F, // 7F DELETE
    0x0080, // 80 <control>
    0x0081, // 81 <control>
    0x0082, // 82 <control>
    0x0083, // 83 <control>
    0x0084, // 84 <control>
    0x0085, // 85 <control>
    0x0086, // 86 <control>
    0x0087, // 87 <control>
    0x0088, // 88 <control>
    0x0089, // 89 <control>
    0x008A, // 8A <control>
    0x008B, // 8B <control>
    0x008C, // 8C <control>
    0x008D, // 8D <control>
    0x008E, // 8E <control>
    0x008F, // 8F <control>
    0x0131, // 90 LATIN SMALL LETTER DOTLESS I
    0x0300, // 91 COMBINING GRAVE ACCENT
    0x0301, // 92 COMBINING ACUTE ACCENT
    0x0302, // 93 COMBINING CIRCUMFLEX ACCENT
    0x0303, // 94 COMBINING TILDE
    0x0304, // 95 COMBINING MACRON
    0x0306, // 96 COMBINING BREVE
    0x0307, // 97 COMBINING DOT ABOVE
    0x0308, // 98 COMBINING DIAERESIS
    0x0000, // 99 UNDEFINED
    0x030A, // 9A COMBINING RING ABOVE
    0x0327, // 9B COMBINING CEDILLA
    0x0000, // 9C UNDEFINED
    0x030B, // 9D COMBINING DOUBLE ACUTE ACCENT
    0x0328, // 9E COMBINING OGONEK
    0x030C, // 9F COMBINING CARON
    0x00A0, // A0 NO-BREAK SPACE
    0x00A1, // A1 INVERTED EXCLAMATION MARK
    0x00A2, // A2 CENT SIGN
    0x00A3, // A3 POUND SIGN
    0x00A4, // A4 CURRENCY SIGN
    0x00A5, // A5 YEN SIGN
    0x00A6, // A6 BROKEN BAR
    0x00A7, // A7 SECTION SIGN
    0x00A8, // A8 DIAERESIS
    0x00A9, // A9 COPYRIGHT SIGN
    0x00AA, // AA FEMININE ORDINAL INDICATOR
    0x00AB, // AB LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x00AC, // AC NOT SIGN
    0x00AD, // AD SOFT HYPHEN
    0x00AE, // AE REGISTERED SIGN
    0x00AF, // AF MACRON
    0x00B0, // B0 DEGREE SIGN
    0x00B1, // B1 PLUS-MINUS SIGN
    0x00B2, // B2 SUPERSCRIPT TWO
    0x00B3, // B3 SUPERSCRIPT THREE
    0x00B4, // B4 ACUTE ACCENT
    0x00B5, // B5 MICRO SIGN
    0x00B6, // B6 PILCROW SIGN
    0x00B7, // B7 MIDDLE DOT
    0x00B8, // B8 CEDILLA
    0x00B9, // B9 SUPERSCRIPT ONE
    0x00BA, // BA MASCULINE ORDINAL INDICATOR
    0x00BB, // BB RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x00BC, // BC VULGAR FRACTION ONE QUARTER
    0x00BD, // BD VULGAR FRACTION ONE HALF
    0x00BE, // BE VULGAR FRACTION THREE QUARTERS
    0x00BF, // BF INVERTED QUESTION MARK
    0x00C0, // C0 LATIN CAPITAL LETTER A WITH GRAVE
    0x00C1, // C1 LATIN CAPITAL LETTER A WITH ACUTE
    0x00C2, // C2 LATIN CAPITAL LETTER A WITH CIRCUMFLEX
    0x00C3, // C3 LATIN CAPITAL LETTER A WITH TILDE
    0x00C4, // C4 LATIN CAPITAL LETTER A WITH DIAERESIS
    0x00C5, // C5 LATIN CAPITAL LETTER A WITH RING ABOVE
    0x00C6, // C6 LATIN CAPITAL LETTER AE
    0x00C7, // C7 LATIN CAPITAL LETTER C WITH CEDILLA
    0x00C8, // C8 LATIN CAPITAL LETTER E WITH GRAVE
    0x00C9, // C9 LATIN CAPITAL LETTER E WITH ACUTE
    0x00CA, // CA LATIN CAPITAL LETTER E WITH CIRCUMFLEX
    0x00CB, // CB LATIN CAPITAL LETTER E WITH DIAERESIS
    0x00CC, // CC LATIN CAPITAL LETTER I WITH GRAVE
    0x00CD, // CD LATIN CAPITAL LETTER I WITH ACUTE
    0x00CE, // CE LATIN CAPITAL LETTER I WITH CIRCUMFLEX
    0x00CF, // CF LATIN CAPITAL LETTER I WITH DIAERESIS
    0x00D0, // D0 LATIN CAPITAL LETTER ETH (Icelandic)
    0x00D1, // D1 LATIN CAPITAL LETTER N WITH TILDE
    0x00D2, // D2 LATIN CAPITAL LETTER O WITH GRAVE
    0x00D3, // D3 LATIN CAPITAL LETTER O WITH ACUTE
    0x00D4, // D4 LATIN CAPITAL LETTER O WITH CIRCUMFLEX
    0x00D5, // D5 LATIN CAPITAL LETTER O WITH TILDE
    0x00D6, // D6 LATIN CAPITAL LETTER O WITH DIAERESIS
    0x00D7, // D7 MULTIPLICATION SIGN
    0x00D8, // D8 LATIN CAPITAL LETTER O WITH STROKE
    0x00D9, // D9 LATIN CAPITAL LETTER U WITH GRAVE
    0x00DA, // DA LATIN CAPITAL LETTER U WITH ACUTE
    0x00DB, // DB LATIN CAPITAL LETTER U WITH CIRCUMFLEX
    0x00DC, // DC LATIN CAPITAL LETTER U WITH DIAERESIS
    0x00DD, // DD LATIN CAPITAL LETTER Y WITH ACUTE
    0x00DE, // DE LATIN CAPITAL LETTER THORN (Icelandic)
    0x00DF, // DF LATIN SMALL LETTER SHARP S (German)
    0x00E0, // E0 LATIN SMALL LETTER A WITH GRAVE
    0x00E1, // E1 LATIN SMALL LETTER A WITH ACUTE
    0x00E2, // E2 LATIN SMALL LETTER A WITH CIRCUMFLEX
    0x00E3, // E3 LATIN SMALL LETTER A WITH TILDE
    0x00E4, // E4 LATIN SMALL LETTER A WITH DIAERESIS
    0x00E5, // E5 LATIN SMALL LETTER A WITH RING ABOVE
    0x00E6, // E6 LATIN SMALL LETTER AE
    0x00E7, // E7 LATIN SMALL LETTER C WITH CEDILLA
    0x00E8, // E8 LATIN SMALL LETTER E WITH GRAVE
    0x00E9, // E9 LATIN SMALL LETTER E WITH ACUTE
    0x00EA, // EA LATIN SMALL LETTER E WITH CIRCUMFLEX
    0x00EB, // EB LATIN SMALL LETTER E WITH DIAERESIS
    0x00EC, // EC LATIN SMALL LETTER I WITH GRAVE
    0x00ED, // ED LATIN SMALL LETTER I WITH ACUTE
    0x00EE, // EE LATIN SMALL LETTER I WITH CIRCUMFLEX
    0x00EF, // EF LATIN SMALL LETTER I WITH DIAERESIS
    0x00F0, // F0 LATIN SMALL LETTER ETH (Icelandic)
    0x00F1, // F1 LATIN SMALL LETTER N WITH TILDE
    0x00F2, // F2 LATIN SMALL LETTER O WITH GRAVE
    0x00F3, // F3 LATIN SMALL LETTER O WITH ACUTE
    0x00F4, // F4 LATIN SMALL LETTER O WITH CIRCUMFLEX
    0x00F5, // F5 LATIN SMALL LETTER O WITH TILDE
    0x00F6, // F6 LATIN SMALL LETTER O WITH DIAERESIS
    0x00F7, // F7 DIVISION SIGN
    0x00F8, // F8 LATIN SMALL LETTER O WITH STROKE
    0x00F9, // F9 LATIN SMALL LETTER U WITH GRAVE
    0x00FA, // FA LATIN SMALL LETTER U WITH ACUTE
    0x00FB, // FB LATIN SMALL LETTER U WITH CIRCUMFLEX
    0x00FC, // FC LATIN SMALL LETTER U WITH DIAERESIS
    0x00FD, // FD LATIN SMALL LETTER Y WITH ACUTE
    0x00FE, // FE LATIN SMALL LETTER THORN (Icelandic)
    0x00FF, // FF LATIN SMALL LETTER Y WITH DIAERESIS
};

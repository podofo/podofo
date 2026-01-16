/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PdfDeclarationsPrivate.h"
#include "PdfStandard14FontData.h"
#include <podofo/main/PdfFontMetricsStandard14.h>

// NOTE: Some maps and data in this file were extracted
// with the "staging/ExtractFontInfos.cpp" script

using namespace std;
using namespace PoDoFo;

constexpr const char* FONT_TIMES_ROMAN_STD = "Times-Roman";
constexpr const char* FONT_TIMES_ITALIC_STD = "Times-Italic";
constexpr const char* FONT_TIMES_BOLD_STD = "Times-Bold";
constexpr const char* FONT_TIMES_BOLD_ITALIC_STD = "Times-BoldItalic";
constexpr const char* FONT_HELVETICA_STD = "Helvetica";
constexpr const char* FONT_HELVETICA_OBLIQUE_STD =  "Helvetica-Oblique";
constexpr const char* FONT_HELVETICA_BOLD_STD = "Helvetica-Bold";
constexpr const char* FONT_HELVETICA_BOLD_OBLIQUE_STD = "Helvetica-BoldOblique";
constexpr const char* FONT_COURIER_STD = "Courier";
constexpr const char* FONT_COURIER_OBLIQUE_STD = "Courier-Oblique";
constexpr const char* FONT_COURIER_BOLD_STD = "Courier-Bold";
constexpr const char* FONT_COURIER_BOLD_OBLIQUE_STD = "Courier-BoldOblique";
constexpr const char* FONT_SYMBOL_STD = "Symbol";
constexpr const char* FONT_ZAPF_DINGBATS_STD = "ZapfDingbats";

constexpr const char* FONT_TIMES_ROMAN_ALT = "TimesNewRoman";
constexpr const char* FONT_TIMES_ITALIC_ALT = "TimesNewRoman,Italic";
constexpr const char* FONT_TIMES_BOLD_ALT = "TimesNewRoman,Bold";
constexpr const char* FONT_TIMES_BOLD_ITALIC_ALT = "TimesNewRoman,BoldItalic";
constexpr const char* FONT_HELVETICA_ALT = "Arial";
constexpr const char* FONT_HELVETICA_OBLIQUE_ALT = "Arial,Italic";
constexpr const char* FONT_HELVETICA_BOLD_ALT = "Arial,Bold";
constexpr const char* FONT_HELVETICA_BOLD_OBLIQUE_ALT = "Arial,BoldItalic";
constexpr const char* FONT_COURIER_ALT = "CourierNew";
constexpr const char* FONT_COURIER_OBLIQUE_ALT = "CourierNew,Italic";
constexpr const char* FONT_COURIER_BOLD_ALT = "CourierNew,Bold";
constexpr const char* FONT_COURIER_BOLD_OBLIQUE_ALT = "CourierNew,BoldItalic";

constexpr const char* TIMES_ROMAN_BASE_NAME = "Times";
constexpr const char* HELVETICA_BASE_NAME = "Helvetica";
constexpr const char* COURIER_BASE_NAME = "Courier";

constexpr const char* TIMES_ROMAN_FAMILY_NAME = "Times New Roman";
constexpr const char* HELVETICA_FAMILY_NAME = "Arial";
constexpr const char* COURIER_FAMILY_NAME = "Courier Std";

enum class PdfStandard14FontFamily
{
    Unknown = 0,
    Times,
    Helvetica,
    Courier,
    Symbol,
    ZapfDingbats,
};

static const unsigned short CHAR_DATA_TIMES_ROMAN[] = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Metrics/FoxitSerif.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Metrics/LiberaLeanSerif-Regular.ipp"
#endif // USE_FOXIT_FONTS
};

static const unsigned short CHAR_DATA_TIMES_ITALIC[] = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Metrics/FoxitSerifItalic.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Metrics/LiberaLeanSerif-Italic.ipp"
#endif // USE_FOXIT_FONTS
};

static const unsigned short CHAR_DATA_TIMES_BOLD[] = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Metrics/FoxitSerifBold.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Metrics/LiberaLeanSerif-Bold.ipp"
#endif // USE_FOXIT_FONTS
};

static const unsigned short CHAR_DATA_TIMES_BOLD_ITALIC[] = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Metrics/FoxitSerifBoldItalic.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Metrics/LiberaLeanSerif-BoldItalic.ipp"
#endif // USE_FOXIT_FONTS
};

static const unsigned short CHAR_DATA_HELVETICA[] = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Metrics/FoxitSans.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Metrics/LiberaLeanSans-Regular.ipp"
#endif // USE_FOXIT_FONTS
};

static const unsigned short CHAR_DATA_HELVETICA_OBLIQUE[] = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Metrics/FoxitSansItalic.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Metrics/LiberaLeanSans-Italic.ipp"
#endif // USE_FOXIT_FONTS
};

static const unsigned short CHAR_DATA_HELVETICA_BOLD[] = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Metrics/FoxitSansBold.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Metrics/LiberaLeanSans-Bold.ipp"
#endif // USE_FOXIT_FONTS
};

static const unsigned short CHAR_DATA_HELVETICA_BOLD_OBLIQUE[] = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Metrics/FoxitSansBoldItalic.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Metrics/LiberaLeanSans-BoldItalic.ipp"
#endif // USE_FOXIT_FONTS
};

static const unsigned short CHAR_DATA_COURIER[] = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Metrics/FoxitFixed.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Metrics/LiberaLeanMono-Regular.ipp"
#endif // USE_FOXIT_FONTS
};

static const unsigned short CHAR_DATA_COURIER_OBLIQUE[] = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Metrics/FoxitFixedItalic.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Metrics/LiberaLeanMono-Italic.ipp"
#endif // USE_FOXIT_FONTS
};

static const unsigned short CHAR_DATA_COURIER_BOLD[] = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Metrics/FoxitFixedBold.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Metrics/LiberaLeanMono-Bold.ipp"
#endif // USE_FOXIT_FONTS
};

static const unsigned short CHAR_DATA_COURIER_BOLD_OBLIQUE[] = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Metrics/FoxitFixedBoldItalic.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Metrics/LiberaLeanMono-BoldItalic.ipp"
#endif // USE_FOXIT_FONTS
};

static const unsigned short CHAR_DATA_SYMBOL[] = {
#include "FoxitFonts/Metrics/FoxitSymbol.ipp"
};

static const unsigned short CHAR_DATA_ZAPF_DINGBATS[] = {
#include "FoxitFonts/Metrics/FoxitDingbats.ipp"
};

string_view PoDoFo::GetStandard14FontName(PdfStandard14FontType stdFont)
{
    switch (stdFont)
    {
        case PdfStandard14FontType::TimesRoman:
            return FONT_TIMES_ROMAN_STD;
        case PdfStandard14FontType::TimesItalic:
            return FONT_TIMES_ITALIC_STD;
        case PdfStandard14FontType::TimesBold:
            return FONT_TIMES_BOLD_STD;
        case PdfStandard14FontType::TimesBoldItalic:
            return FONT_TIMES_BOLD_ITALIC_STD;
        case PdfStandard14FontType::Helvetica:
            return FONT_HELVETICA_STD;
        case PdfStandard14FontType::HelveticaOblique:
            return FONT_HELVETICA_OBLIQUE_STD;
        case PdfStandard14FontType::HelveticaBold:
            return FONT_HELVETICA_BOLD_STD;
        case PdfStandard14FontType::HelveticaBoldOblique:
            return FONT_HELVETICA_BOLD_OBLIQUE_STD;
        case PdfStandard14FontType::Courier:
            return FONT_COURIER_STD;
        case PdfStandard14FontType::CourierOblique:
            return FONT_COURIER_OBLIQUE_STD;
        case PdfStandard14FontType::CourierBold:
            return FONT_COURIER_BOLD_STD;
        case PdfStandard14FontType::CourierBoldOblique:
            return FONT_COURIER_BOLD_OBLIQUE_STD;
        case PdfStandard14FontType::Symbol:
            return FONT_SYMBOL_STD;
        case PdfStandard14FontType::ZapfDingbats:
            return FONT_ZAPF_DINGBATS_STD;
        case PdfStandard14FontType::Unknown:
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

string_view PoDoFo::GetStandard14FontFamilyName(PdfStandard14FontType stdFont)
{
    switch (stdFont)
    {
        case PdfStandard14FontType::TimesRoman:
        case PdfStandard14FontType::TimesItalic:
        case PdfStandard14FontType::TimesBold:
        case PdfStandard14FontType::TimesBoldItalic:
            return TIMES_ROMAN_FAMILY_NAME;
        case PdfStandard14FontType::Helvetica:
        case PdfStandard14FontType::HelveticaOblique:
        case PdfStandard14FontType::HelveticaBold:
        case PdfStandard14FontType::HelveticaBoldOblique:
            return HELVETICA_FAMILY_NAME;
        case PdfStandard14FontType::Courier:
        case PdfStandard14FontType::CourierOblique:
        case PdfStandard14FontType::CourierBold:
        case PdfStandard14FontType::CourierBoldOblique:
            return COURIER_FAMILY_NAME;
        case PdfStandard14FontType::Symbol:
        case PdfStandard14FontType::ZapfDingbats:
            return { }; // There's no font family name for Symbol and ZapfDingbats
        case PdfStandard14FontType::Unknown:
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

// NOTE: Adobe strictly checks for Standard14 names + alternative
// names. No other combinations are possible
// NOTE: ISO 32000-1:2008 and ISO 32000-2:2020 don't
// mention alternative names. They are mentioned
// until Adobe Pdf Reference 1.7
bool PoDoFo::IsStandard14Font(const string_view& fontName, bool useAltNames, PdfStandard14FontType& stdFont)
{
    if (fontName == FONT_TIMES_ROMAN_STD)
    {
        stdFont = PdfStandard14FontType::TimesRoman;
        return true;
    }
    else if (fontName == FONT_TIMES_ITALIC_STD)
    {
        stdFont = PdfStandard14FontType::TimesItalic;
        return true;
    }
    else if (fontName == FONT_TIMES_BOLD_STD)
    {
        stdFont = PdfStandard14FontType::TimesBold;
        return true;
    }
    else if (fontName == FONT_TIMES_BOLD_ITALIC_STD)
    {
        stdFont = PdfStandard14FontType::TimesBoldItalic;
        return true;
    }
    else if (fontName == FONT_HELVETICA_STD)
    {
        stdFont = PdfStandard14FontType::Helvetica;
        return true;
    }
    else if (fontName == FONT_HELVETICA_OBLIQUE_STD)
    {
        stdFont = PdfStandard14FontType::HelveticaOblique;
        return true;
    }
    else if (fontName == FONT_HELVETICA_BOLD_STD)
    {
        stdFont = PdfStandard14FontType::HelveticaBold;
        return true;
    }
    else if (fontName == FONT_HELVETICA_BOLD_OBLIQUE_STD)
    {
        stdFont = PdfStandard14FontType::HelveticaBoldOblique;
        return true;
    }
    else if (fontName == FONT_COURIER_STD)
    {
        stdFont = PdfStandard14FontType::Courier;
        return true;
    }
    else if (fontName == FONT_COURIER_OBLIQUE_STD)
    {
        stdFont = PdfStandard14FontType::CourierOblique;
        return true;
    }
    else if (fontName == FONT_COURIER_BOLD_STD)
    {
        stdFont = PdfStandard14FontType::CourierBold;
        return true;
    }
    else if (fontName == FONT_COURIER_BOLD_OBLIQUE_STD)
    {
        stdFont = PdfStandard14FontType::CourierBoldOblique;
        return true;
    }
    else if (fontName == FONT_SYMBOL_STD)
    {
        stdFont = PdfStandard14FontType::Symbol;
        return true;
    }
    else if (fontName == FONT_ZAPF_DINGBATS_STD)
    {
        stdFont = PdfStandard14FontType::ZapfDingbats;
        return true;
    }

    if (useAltNames)
    {
        if (fontName == FONT_TIMES_ROMAN_ALT)
        {
            stdFont = PdfStandard14FontType::TimesRoman;
            return true;
        }
        else if (fontName == FONT_TIMES_ITALIC_ALT)
        {
            stdFont = PdfStandard14FontType::TimesItalic;
            return true;
        }
        else if (fontName == FONT_TIMES_BOLD_ALT)
        {
            stdFont = PdfStandard14FontType::TimesBold;
            return true;
        }
        else if (fontName == FONT_TIMES_BOLD_ITALIC_ALT)
        {
            stdFont = PdfStandard14FontType::TimesBoldItalic;
            return true;
        }
        else if (fontName == FONT_HELVETICA_ALT)
        {
            stdFont = PdfStandard14FontType::Helvetica;
            return true;
        }
        else if (fontName == FONT_HELVETICA_OBLIQUE_ALT)
        {
            stdFont = PdfStandard14FontType::HelveticaOblique;
            return true;
        }
        else if (fontName == FONT_HELVETICA_BOLD_ALT)
        {
            stdFont = PdfStandard14FontType::HelveticaBold;
            return true;
        }
        else if (fontName == FONT_HELVETICA_BOLD_OBLIQUE_ALT)
        {
            stdFont = PdfStandard14FontType::HelveticaBoldOblique;
            return true;
        }
        else if (fontName == FONT_COURIER_ALT)
        {
            stdFont = PdfStandard14FontType::Courier;
            return true;
        }
        else if (fontName == FONT_COURIER_OBLIQUE_ALT)
        {
            stdFont = PdfStandard14FontType::CourierOblique;
            return true;
        }
        else if (fontName == FONT_COURIER_BOLD_ALT)
        {
            stdFont = PdfStandard14FontType::CourierBold;
            return true;
        }
        else if (fontName == FONT_COURIER_BOLD_OBLIQUE_ALT)
        {
            stdFont = PdfStandard14FontType::CourierBoldOblique;
            return true;
        }
    }

    stdFont = PdfStandard14FontType::Unknown;
    return false;
}

const unsigned short* PoDoFo::GetStd14FontWidths(PdfStandard14FontType stdFont, unsigned& size)
{
    switch (stdFont)
    {
        case PdfStandard14FontType::TimesRoman:
            size = (unsigned)std::size(CHAR_DATA_TIMES_ROMAN);
            return CHAR_DATA_TIMES_ROMAN;
        case PdfStandard14FontType::TimesItalic:
            size = (unsigned)std::size(CHAR_DATA_TIMES_ITALIC);
            return CHAR_DATA_TIMES_ITALIC;
        case PdfStandard14FontType::TimesBold:
            size = (unsigned)std::size(CHAR_DATA_TIMES_BOLD);
            return CHAR_DATA_TIMES_BOLD;
        case PdfStandard14FontType::TimesBoldItalic:
            size = (unsigned)std::size(CHAR_DATA_TIMES_BOLD_ITALIC);
            return CHAR_DATA_TIMES_BOLD_ITALIC;
        case PdfStandard14FontType::Helvetica:
            size = (unsigned)std::size(CHAR_DATA_HELVETICA);
            return CHAR_DATA_HELVETICA;
        case PdfStandard14FontType::HelveticaOblique:
            size = (unsigned)std::size(CHAR_DATA_HELVETICA_OBLIQUE);
            return CHAR_DATA_HELVETICA_OBLIQUE;
        case PdfStandard14FontType::HelveticaBold:
            size = (unsigned)std::size(CHAR_DATA_HELVETICA_BOLD);
            return CHAR_DATA_HELVETICA_BOLD;
        case PdfStandard14FontType::HelveticaBoldOblique:
            size = (unsigned)std::size(CHAR_DATA_HELVETICA_BOLD_OBLIQUE);
            return CHAR_DATA_HELVETICA_BOLD_OBLIQUE;
        case PdfStandard14FontType::Courier:
            size = (unsigned)std::size(CHAR_DATA_COURIER);
            return CHAR_DATA_COURIER;
        case PdfStandard14FontType::CourierOblique:
            size = (unsigned)std::size(CHAR_DATA_COURIER_OBLIQUE);
            return CHAR_DATA_COURIER_OBLIQUE;
        case PdfStandard14FontType::CourierBold:
            size = (unsigned)std::size(CHAR_DATA_COURIER_BOLD);
            return CHAR_DATA_COURIER_BOLD;
        case PdfStandard14FontType::CourierBoldOblique:
            size = (unsigned)std::size(CHAR_DATA_COURIER_BOLD_OBLIQUE);
            return CHAR_DATA_COURIER_BOLD_OBLIQUE;
        case PdfStandard14FontType::Symbol:
            size = (unsigned)std::size(CHAR_DATA_SYMBOL);
            return CHAR_DATA_SYMBOL;
        case PdfStandard14FontType::ZapfDingbats:
            size = (unsigned)std::size(CHAR_DATA_ZAPF_DINGBATS);
            return CHAR_DATA_ZAPF_DINGBATS;
        case PdfStandard14FontType::Unknown:
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

static const Std14CPToGIDMap& GetTimesRomanMap()
{
    static Std14CPToGIDMap map = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Encodings/FoxitSerif.ipp"
#else
#include "LiberaLean/Encodings/LiberaLeanSerif-Regular.ipp"
#endif // USE_FOXIT_FONTS
    };

    return map;
}

static const Std14CPToGIDMap& GetTimesItalicMap()
{
    static Std14CPToGIDMap map = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Encodings/FoxitSerifItalic.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Encodings/LiberaLeanSerif-Italic.ipp"
#endif // USE_FOXIT_FONTS
    };
    return map;
}

static const Std14CPToGIDMap& GetTimesBoldMap()
{
    static Std14CPToGIDMap map = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Encodings/FoxitSerifBold.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Encodings/LiberaLeanSerif-Bold.ipp"
#endif // USE_FOXIT_FONTS
    };
    return map;
}

static const Std14CPToGIDMap& GetTimesBoldItalicMap()
{
    static Std14CPToGIDMap map = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Encodings/FoxitSerifBoldItalic.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Encodings/LiberaLeanSerif-BoldItalic.ipp"
#endif // USE_FOXIT_FONTS
    };
    return map;
}

static const Std14CPToGIDMap& GetHelveticaMap()
{
    static Std14CPToGIDMap map = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Encodings/FoxitSans.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Encodings/LiberaLeanSans-Regular.ipp"
#endif // USE_FOXIT_FONTS
    };
    return map;
}

static const Std14CPToGIDMap& GetHelveticaObliqueMap()
{
    static Std14CPToGIDMap map = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Encodings/FoxitSansItalic.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Encodings/LiberaLeanSans-Italic.ipp"
#endif // USE_FOXIT_FONTS
    };
    return map;
}

static const Std14CPToGIDMap& GetHelveticaBoldMap()
{
    static Std14CPToGIDMap map = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Encodings/FoxitSansBold.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Encodings/LiberaLeanSans-Bold.ipp"
#endif // USE_FOXIT_FONTS
    };
    return map;
}

static const Std14CPToGIDMap& GetHelveticaBoldObliqueMap()
{
    static Std14CPToGIDMap map = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Encodings/FoxitSansBoldItalic.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Encodings/LiberaLeanSans-BoldItalic.ipp"
#endif // USE_FOXIT_FONTS
    };
    return map;
}

static const Std14CPToGIDMap& GetCourierMap()
{
    static Std14CPToGIDMap map = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Encodings/FoxitFixed.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Encodings/LiberaLeanMono-Regular.ipp"
#endif // USE_FOXIT_FONTS
    };
    return map;
}

static const Std14CPToGIDMap& GetCourierObliqueMap()
{
    static Std14CPToGIDMap map = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Encodings/FoxitFixedItalic.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Encodings/LiberaLeanMono-Italic.ipp"
#endif // USE_FOXIT_FONTS
    };
    return map;
}

static const Std14CPToGIDMap& GetCourierBoldMap()
{
    static Std14CPToGIDMap map = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Encodings/FoxitFixedBold.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Encodings/LiberaLeanMono-Bold.ipp"
#endif // USE_FOXIT_FONTS
    };
    return map;
}

static const Std14CPToGIDMap& GetCourierBoldObliqueMap()
{
    static Std14CPToGIDMap map = {
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/Encodings/FoxitFixedBoldItalic.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/Encodings/LiberaLeanMono-BoldItalic.ipp"
#endif // USE_FOXIT_FONTS
    };
    return map;
}

static const Std14CPToGIDMap& GetSymbolMap()
{
    static Std14CPToGIDMap map = {
#include "FoxitFonts/Encodings/FoxitSymbol.ipp"
    };
    return map;
}

static const Std14CPToGIDMap& GetZapfDingbatsMap()
{
    static Std14CPToGIDMap map = {
#include "FoxitFonts/Encodings/FoxitDingbats.ipp"
    };
    return map;
}

const Std14CPToGIDMap& PoDoFo::GetStd14CPToGIDMap(PdfStandard14FontType stdFont)
{
    switch (stdFont)
    {
        case PdfStandard14FontType::TimesRoman:
            return GetTimesRomanMap();
        case PdfStandard14FontType::TimesItalic:
            return GetTimesItalicMap();
        case PdfStandard14FontType::TimesBold:
            return GetTimesBoldMap();
        case PdfStandard14FontType::TimesBoldItalic:
            return GetTimesBoldItalicMap();
        case PdfStandard14FontType::Helvetica:
            return GetHelveticaMap();
        case PdfStandard14FontType::HelveticaOblique:
            return GetHelveticaObliqueMap();
        case PdfStandard14FontType::HelveticaBold:
            return GetHelveticaBoldMap();
        case PdfStandard14FontType::HelveticaBoldOblique:
            return GetHelveticaBoldObliqueMap();
        case PdfStandard14FontType::Courier:
            return GetCourierMap();
        case PdfStandard14FontType::CourierOblique:
            return GetCourierObliqueMap();
        case PdfStandard14FontType::CourierBold:
            return GetCourierBoldMap();
        case PdfStandard14FontType::CourierBoldOblique:
            return GetCourierBoldObliqueMap();
        case PdfStandard14FontType::Symbol:
            return GetSymbolMap();
        case PdfStandard14FontType::ZapfDingbats:
            return GetZapfDingbatsMap();
        case PdfStandard14FontType::Unknown:
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

string_view PoDoFo::GetStandard14FontBaseName(PdfStandard14FontType stdFont)
{
    switch (stdFont)
    {
        case PdfStandard14FontType::TimesRoman:
        case PdfStandard14FontType::TimesItalic:
        case PdfStandard14FontType::TimesBold:
        case PdfStandard14FontType::TimesBoldItalic:
            return TIMES_ROMAN_BASE_NAME;
        case PdfStandard14FontType::Helvetica:
        case PdfStandard14FontType::HelveticaOblique:
        case PdfStandard14FontType::HelveticaBold:
        case PdfStandard14FontType::HelveticaBoldOblique:
            return HELVETICA_BASE_NAME;
        case PdfStandard14FontType::Courier:
        case PdfStandard14FontType::CourierOblique:
        case PdfStandard14FontType::CourierBold:
        case PdfStandard14FontType::CourierBoldOblique:
            return COURIER_BASE_NAME;
        case PdfStandard14FontType::Symbol:
            return FONT_SYMBOL_STD;
        case PdfStandard14FontType::ZapfDingbats:
            return FONT_ZAPF_DINGBATS_STD;
        case PdfStandard14FontType::Unknown:
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

const Standard14FontData& PoDoFo::GetStandard14FontData(
    PdfStandard14FontType std14Font)
{
    // Some properties were extracted by actual font program.
    // /MissingWidth, /StemV, /Flags, /FontFamily, /FontStretch
    // values were copied from Acrobat Pro by performing font embedding
    static Standard14FontData PODOFO_BUILTIN_STD14FONT_DATA[] = {
#ifdef USE_FOXIT_FONTS
        {
            CHAR_DATA_TIMES_ROMAN,
            (unsigned short)std::size(CHAR_DATA_TIMES_ROMAN),
            PdfFontFileType::Type1CFF,
            PdfFontDescriptorFlags::Symbolic | PdfFontDescriptorFlags::Serif,
            1000,
            PdfFontStretch::Normal,
            727,
            -273,
            450,
            662,
            0,
            400,
            80,
            0,
            262,
            -100,
            Corners(-168, -218, 1000, 898)
        },
        {
            CHAR_DATA_TIMES_ITALIC,
            (unsigned short)std::size(CHAR_DATA_TIMES_ITALIC),
            PdfFontFileType::Type1CFF,
            PdfFontDescriptorFlags::Symbolic | PdfFontDescriptorFlags::Serif | PdfFontDescriptorFlags::Italic,
            1000,
            PdfFontStretch::Normal,
            727,
            -273,
            441,
            653,
            -17,
            400,
            72,
            0,
            262,
            -100,
            Corners(-169, -217, 1010, 883)
        },
        {
            CHAR_DATA_TIMES_BOLD,
            (unsigned short)std::size(CHAR_DATA_TIMES_BOLD),
            PdfFontFileType::Type1CFF,
            PdfFontDescriptorFlags::Symbolic | PdfFontDescriptorFlags::Serif,
            1000,
            PdfFontStretch::Normal,
            727,
            -273,
            461,
            676,
            0,
            700,
            136,
            0,
            262,
            -100,
            Corners(-168, -218, 1000, 935)
        },
        {
            CHAR_DATA_TIMES_BOLD_ITALIC,
            (unsigned short)std::size(CHAR_DATA_TIMES_BOLD_ITALIC),
            PdfFontFileType::Type1CFF,
            PdfFontDescriptorFlags::Symbolic | PdfFontDescriptorFlags::Serif | PdfFontDescriptorFlags::Italic,
            1000,
            PdfFontStretch::Normal,
            727,
            -273,
            462,
            669,
            -17,
            700,
            124,
            0,
            262,
            -100,
            Corners(-200, -218, 996, 921)
        },
        {
            CHAR_DATA_HELVETICA,
            (unsigned short)std::size(CHAR_DATA_HELVETICA),
            PdfFontFileType::Type1CFF,
            PdfFontDescriptorFlags::Symbolic,
            1000,
            PdfFontStretch::Normal,
            750,
            -250,
            523,
            718,
            0,
            400,
            88,
            0,
            290,
            -100,
            Corners(-166, -225, 1000, 931)
        },
        {
            CHAR_DATA_HELVETICA_OBLIQUE,
            (unsigned short)std::size(CHAR_DATA_HELVETICA_OBLIQUE),
            PdfFontFileType::Type1CFF,
            PdfFontDescriptorFlags::Symbolic | PdfFontDescriptorFlags::Italic,
            1000,
            PdfFontStretch::Normal,
            750,
            -250,
            532,
            718,
            -12,
            400,
            92,
            0,
            290,
            -100,
            Corners(-170, -225, 1116, 931)
        },
        {
            CHAR_DATA_HELVETICA_BOLD,
            (unsigned short)std::size(CHAR_DATA_HELVETICA_BOLD),
            PdfFontFileType::Type1CFF,
            PdfFontDescriptorFlags::Symbolic,
            1000,
            PdfFontStretch::Normal,
            750,
            -250,
            532,
            718,
            0,
            700,
            136,
            0,
            290,
            -100,
            Corners(-170, -228, 1003, 962)
        },
        {
            CHAR_DATA_HELVETICA_BOLD_OBLIQUE,
            (unsigned short)std::size(CHAR_DATA_HELVETICA_BOLD_OBLIQUE),
            PdfFontFileType::Type1CFF,
            PdfFontDescriptorFlags::Symbolic | PdfFontDescriptorFlags::Italic,
            1000,
            PdfFontStretch::Normal,
            750,
            -250,
            532,
            718,
            -12,
            700,
            140,
            0,
            290,
            -100,
            Corners(-174, -228, 1114, 962)
        },
        {
            CHAR_DATA_COURIER,
            (unsigned short)std::size(CHAR_DATA_COURIER),
            PdfFontFileType::Type1CFF,
            PdfFontDescriptorFlags::Symbolic | PdfFontDescriptorFlags::Serif,
            1000,
            PdfFontStretch::Normal,
            627,
            -373,
            426,
            562,
            0,
            500,
            56,
            0,
            261,
            -224,
            Corners(-23, -250, 715, 805)
        },
        {
            CHAR_DATA_COURIER_OBLIQUE,
            (unsigned short)std::size(CHAR_DATA_COURIER_OBLIQUE),
            PdfFontFileType::Type1CFF,
            PdfFontDescriptorFlags::Symbolic | PdfFontDescriptorFlags::Serif | PdfFontDescriptorFlags::Italic,
            1000,
            PdfFontStretch::Normal,
            627,
            -373,
            426,
            562,
            -11,
            500,
            56,
            0,
            261,
            -224,
            Corners(-27, -250, 849, 805)
        },
        {
            CHAR_DATA_COURIER_BOLD,
            (unsigned short)std::size(CHAR_DATA_COURIER_BOLD),
            PdfFontFileType::Type1CFF,
            PdfFontDescriptorFlags::Symbolic | PdfFontDescriptorFlags::Serif,
            1000,
            PdfFontStretch::Normal,
            627,
            -373,
            439,
            562,
            0,
            700,
            92,
            0,
            261,
            -224,
            Corners(-113, -250, 749, 801)
        },
        {
            CHAR_DATA_COURIER_BOLD_OBLIQUE,
            (unsigned short)std::size(CHAR_DATA_COURIER_BOLD_OBLIQUE),
            PdfFontFileType::Type1CFF,
            PdfFontDescriptorFlags::Symbolic | PdfFontDescriptorFlags::Serif | PdfFontDescriptorFlags::Italic,
            1000,
            PdfFontStretch::Normal,
            627,
            -373,
            439,
            562,
            -11,
            700,
            92,
            0,
            261,
            -224,
            Corners(-57, -250, 869, 801)
        },
#else // USE_LIBERA_LEAN
        {
            CHAR_DATA_TIMES_ROMAN,
            (unsigned short)std::size(CHAR_DATA_TIMES_ROMAN),
            PdfFontFileType::OpenTypeCFF,
            PdfFontDescriptorFlags::Symbolic | PdfFontDescriptorFlags::Serif,
            1000,
            PdfFontStretch::Normal,
            693,
            -216,
            459,
            655,
            0,
            400,
            80,
            0,
            205,
            -84,
            Corners(-177, -303, 1007, 981)
        },
        {
            CHAR_DATA_TIMES_ITALIC,
            (unsigned short)std::size(CHAR_DATA_TIMES_ITALIC),
            PdfFontFileType::OpenTypeCFF,
            PdfFontDescriptorFlags::Symbolic | PdfFontDescriptorFlags::Serif | PdfFontDescriptorFlags::Italic,
            1000,
            PdfFontStretch::Normal,
            694,
            -216,
            459,
            655,
            -512,
            400,
            72,
            0,
            259,
            -84,
            Corners(-177, -303, 1088, 981)
        },
        {
            CHAR_DATA_TIMES_BOLD,
            (unsigned short)std::size(CHAR_DATA_TIMES_BOLD),
            PdfFontFileType::OpenTypeCFF,
            PdfFontDescriptorFlags::Symbolic | PdfFontDescriptorFlags::Serif,
            1000,
            PdfFontStretch::Normal,
            677,
            -216,
            459,
            655,
            0,
            700,
            136,
            0,
            259,
            -61,
            Corners(-182, -303, 1085, 1008)
        },
        {
            CHAR_DATA_TIMES_BOLD_ITALIC,
            (unsigned short)std::size(CHAR_DATA_TIMES_BOLD_ITALIC),
            PdfFontFileType::OpenTypeCFF,
            PdfFontDescriptorFlags::Symbolic | PdfFontDescriptorFlags::Serif | PdfFontDescriptorFlags::Italic,
            1000,
            PdfFontStretch::Normal,
            677,
            -216,
            459,
            655,
            -512,
            700,
            124,
            0,
            259,
            -61,
            Corners(-178, -303, 1150, 981)
        },
        {
            CHAR_DATA_HELVETICA,
            (unsigned short)std::size(CHAR_DATA_HELVETICA),
            PdfFontFileType::OpenTypeCFF,
            PdfFontDescriptorFlags::Symbolic,
            1000,
            PdfFontStretch::Normal,
            728,
            -210,
            528,
            688,
            0,
            400,
            88,
            0,
            259,
            -69,
            Corners(-203, -303, 1050, 910)
        },
        {
            CHAR_DATA_HELVETICA_OBLIQUE,
            (unsigned short)std::size(CHAR_DATA_HELVETICA_OBLIQUE),
            PdfFontFileType::OpenTypeCFF,
            PdfFontDescriptorFlags::Symbolic | PdfFontDescriptorFlags::Italic,
            1000,
            PdfFontStretch::Normal,
            728,
            -208,
            528,
            688,
            0,
            400,
            92,
            0,
            259,
            -69,
            Corners(-272, -303, 1063, 1014)
        },
        {
            CHAR_DATA_HELVETICA_BOLD,
            (unsigned short)std::size(CHAR_DATA_HELVETICA_BOLD),
            PdfFontFileType::OpenTypeCFF,
            PdfFontDescriptorFlags::Symbolic,
            1000,
            PdfFontStretch::Normal,
            728,
            -210,
            528,
            688,
            0,
            700,
            136,
            0,
            259,
            -53,
            Corners(-184, -303, 1062, 1033)
        },
        {
            CHAR_DATA_HELVETICA_BOLD_OBLIQUE,
            (unsigned short)std::size(CHAR_DATA_HELVETICA_BOLD_OBLIQUE),
            PdfFontFileType::OpenTypeCFF,
            PdfFontDescriptorFlags::Symbolic | PdfFontDescriptorFlags::Italic,
            1000,
            PdfFontStretch::Normal,
            728,
            -210,
            528,
            688,
            0,
            700,
            140,
            0,
            259,
            -53,
            Corners(-209, -303, 1128, 1030)
        },
        {
            CHAR_DATA_COURIER,
            (unsigned short)std::size(CHAR_DATA_COURIER),
            PdfFontFileType::OpenTypeCFF,
            PdfFontDescriptorFlags::Symbolic | PdfFontDescriptorFlags::Serif,
            1000,
            PdfFontStretch::Normal,
            613,
            -188,
            528,
            659,
            0,
            400,
            56,
            0,
            259,
            -212,
            Corners(-24, -300, 609, 833)
        },
        {
            CHAR_DATA_COURIER_OBLIQUE,
            (unsigned short)std::size(CHAR_DATA_COURIER_OBLIQUE),
            PdfFontFileType::OpenTypeCFF,
            PdfFontDescriptorFlags::Symbolic | PdfFontDescriptorFlags::Serif | PdfFontDescriptorFlags::Italic,
            1000,
            PdfFontStretch::Normal,
            613,
            -188,
            528,
            659,
            0,
            400,
            56,
            0,
            259,
            -212,
            Corners(-94, -300, 705, 833)
        },
        {
            CHAR_DATA_COURIER_BOLD,
            (unsigned short)std::size(CHAR_DATA_COURIER_BOLD),
            PdfFontFileType::OpenTypeCFF,
            PdfFontDescriptorFlags::Symbolic | PdfFontDescriptorFlags::Serif,
            1000,
            PdfFontStretch::Normal,
            633,
            -209,
            528,
            659,
            0,
            700,
            92,
            0,
            259,
            -183,
            Corners(-27, -300, 615, 833)
        },
        {
            CHAR_DATA_COURIER_BOLD_OBLIQUE,
            (unsigned short)std::size(CHAR_DATA_COURIER_BOLD_OBLIQUE),
            PdfFontFileType::OpenTypeCFF,
            PdfFontDescriptorFlags::Symbolic | PdfFontDescriptorFlags::Serif | PdfFontDescriptorFlags::Italic,
            1000,
            PdfFontStretch::Normal,
            633,
            -209,
            528,
            659,
            0,
            700,
            92,
            0,
            259,
            -183,
            Corners(-94, -300, 698, 833)
        },
#endif // USE_FOXIT_FONTS
        {
            CHAR_DATA_SYMBOL,
            (unsigned short)std::size(CHAR_DATA_SYMBOL),
            PdfFontFileType::Type1CFF,
            PdfFontDescriptorFlags::Symbolic,
            0,
            PdfFontStretch::Unknown,
            683,
            -217,
            462,
            669,
            0,
            -1,
            75,
            92,
            341,
            -100,
            Corners(-180, -293, 1090, 1010)
        },
        {
            CHAR_DATA_ZAPF_DINGBATS,
            (unsigned short)std::size(CHAR_DATA_ZAPF_DINGBATS),
            PdfFontFileType::Type1CFF,
            PdfFontDescriptorFlags::Symbolic,
            0,
            PdfFontStretch::Unknown,
            683,
            -217,
            462,
            669,
            0,
            -1,
            75,
            50,
            341,
            -100,
            Corners(-1, -143, 981, 820)
        }
    };

    switch (std14Font)
    {
        case PdfStandard14FontType::TimesRoman:
            return PODOFO_BUILTIN_STD14FONT_DATA[0];
        case PdfStandard14FontType::TimesItalic:
            return PODOFO_BUILTIN_STD14FONT_DATA[1];
        case PdfStandard14FontType::TimesBold:
            return PODOFO_BUILTIN_STD14FONT_DATA[2];
        case PdfStandard14FontType::TimesBoldItalic:
            return PODOFO_BUILTIN_STD14FONT_DATA[3];
        case PdfStandard14FontType::Helvetica:
            return PODOFO_BUILTIN_STD14FONT_DATA[4];
        case PdfStandard14FontType::HelveticaOblique:
            return PODOFO_BUILTIN_STD14FONT_DATA[5];
        case PdfStandard14FontType::HelveticaBold:
            return PODOFO_BUILTIN_STD14FONT_DATA[6];
        case PdfStandard14FontType::HelveticaBoldOblique:
            return PODOFO_BUILTIN_STD14FONT_DATA[7];
        case PdfStandard14FontType::Courier:
            return PODOFO_BUILTIN_STD14FONT_DATA[8];
        case PdfStandard14FontType::CourierOblique:
            return PODOFO_BUILTIN_STD14FONT_DATA[9];
        case PdfStandard14FontType::CourierBold:
            return PODOFO_BUILTIN_STD14FONT_DATA[10];
        case PdfStandard14FontType::CourierBoldOblique:
            return PODOFO_BUILTIN_STD14FONT_DATA[11];
        case PdfStandard14FontType::Symbol:
            return PODOFO_BUILTIN_STD14FONT_DATA[12];
        case PdfStandard14FontType::ZapfDingbats:
            return PODOFO_BUILTIN_STD14FONT_DATA[13];
        case PdfStandard14FontType::Unknown:
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Invalid Standard14 font type");
    }
}


#include "PdfDeclarationsPrivate.h"
#include "PdfStandard14FontData.h"

using namespace std;
using namespace PoDoFo;

static const unsigned char TimesRomanRegular[] = ""
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/FoxitSerif.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/LiberaLeanSerif-Regular.ipp"
#endif // USE_FOXIT_FONTS
;

static const unsigned char TimesItalic[] = ""
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/FoxitSerifItalic.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/LiberaLeanSerif-Italic.ipp"
#endif // USE_FOXIT_FONTS
;

static const unsigned char TimesBold[] = ""
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/FoxitSerifBold.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/LiberaLeanSerif-Bold.ipp"
#endif // USE_FOXIT_FONTS
;

static const unsigned char TimesBoldItalic[] = ""
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/FoxitSerifBoldItalic.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/LiberaLeanSerif-BoldItalic.ipp"
#endif // USE_FOXIT_FONTS
;

static const unsigned char HelveticaRegular[] = ""
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/FoxitSans.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/LiberaLeanSans-Regular.ipp"
#endif // USE_FOXIT_FONTS
;

static const unsigned char HelveticaOblique[] = ""
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/FoxitSansItalic.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/LiberaLeanSans-Italic.ipp"
#endif // USE_FOXIT_FONTS
;

static const unsigned char HelveticaBold[] = ""
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/FoxitSansBold.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/LiberaLeanSans-Bold.ipp"
#endif
;

static const unsigned char HelveticaBoldOblique[] = ""
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/FoxitSansBoldItalic.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/LiberaLeanSans-BoldItalic.ipp"
#endif // USE_FOXIT_FONTS
;

static const unsigned char CourierRegular[] = ""
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/FoxitFixed.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/LiberaLeanMono-Regular.ipp"
#endif // USE_FOXIT_FONTS
;

static const unsigned char CourierOblique[] = ""
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/FoxitFixedItalic.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/LiberaLeanMono-Italic.ipp"
#endif // USE_FOXIT_FONTS
;

static const unsigned char CourierBold[] = ""
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/FoxitFixedBold.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/LiberaLeanMono-Bold.ipp"
#endif // USE_FOXIT_FONTS
;

static const unsigned char CourierBoldOblique[] = ""
#ifdef USE_FOXIT_FONTS
#include "FoxitFonts/FoxitFixedBoldItalic.ipp"
#else // USE_LIBERA_LEAN
#include "LiberaLean/LiberaLeanMono-BoldItalic.ipp"
#endif // USE_FOXIT_FONTS
;

static const unsigned char Symbol[] = ""
#include "FoxitFonts/FoxitSymbol.ipp"
;

static const unsigned char ZapfDingbats[] = ""
#include "FoxitFonts/FoxitDingbats.ipp"
;

bufferview PoDoFo::GetStandard14FontFileData(PdfStandard14FontType stdFont)
{
    switch (stdFont)
    {
        case PdfStandard14FontType::TimesRoman:
            return { (const char *)TimesRomanRegular, std::size(TimesRomanRegular) - 1 };
        case PdfStandard14FontType::TimesItalic:
            return { (const char*)TimesItalic, std::size(TimesItalic) - 1 };
        case PdfStandard14FontType::TimesBold:
            return { (const char*)TimesBold, std::size(TimesBold) - 1 };
        case PdfStandard14FontType::TimesBoldItalic:
            return { (const char*)TimesBoldItalic, std::size(TimesBoldItalic) - 1 };
        case PdfStandard14FontType::Helvetica:
            return { (const char*)HelveticaRegular, std::size(HelveticaRegular) - 1 };
        case PdfStandard14FontType::HelveticaOblique:
            return { (const char *)HelveticaOblique, std::size(HelveticaOblique) - 1 };
        case PdfStandard14FontType::HelveticaBold:
            return { (const char*)HelveticaBold, std::size(HelveticaBold) - 1 };
        case PdfStandard14FontType::HelveticaBoldOblique:
            return { (const char*)HelveticaBoldOblique, std::size(HelveticaBoldOblique) - 1 };
        case PdfStandard14FontType::Courier:
            return { (const char*)CourierRegular, std::size(CourierRegular) - 1 };
        case PdfStandard14FontType::CourierOblique:
            return { (const char*)CourierOblique, std::size(CourierOblique) - 1 };
        case PdfStandard14FontType::CourierBold:
            return { (const char*)CourierBold, std::size(CourierBold) - 1 };
        case PdfStandard14FontType::CourierBoldOblique:
            return { (const char*)CourierBoldOblique, std::size(CourierBoldOblique) - 1 };
        case PdfStandard14FontType::Symbol:
            return { (const char*)Symbol, std::size(Symbol) - 1 };
        case PdfStandard14FontType::ZapfDingbats:
            return { (const char*)ZapfDingbats, std::size(ZapfDingbats) - 1 };
        case PdfStandard14FontType::Unknown:
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

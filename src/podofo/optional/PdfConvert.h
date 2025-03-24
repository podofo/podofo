/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_CONVERT_H
#define PDF_CONVERT_H

#include <podofo/main/PdfError.h>

namespace PoDoFo
{
    template<typename T>
    struct Convert final
    {
        static std::string_view ToString(T value)
        {
            (void)value;
            static_assert(always_false<T>, "Unsupported type");
            return { };
        }

        static bool TryParse(const std::string_view& str, T& value)
        {
            (void)str;
            (void)value;
            static_assert(always_false<T>, "Unsupported type");
            return false;
        }
    };

    template<>
    struct Convert<PdfColorSpaceType>
    {
        static std::string_view ToString(PdfColorSpaceType value)
        {
            using namespace std;
            switch (value)
            {
                case PdfColorSpaceType::DeviceGray:
                    return "DeviceGray"sv;
                case PdfColorSpaceType::DeviceRGB:
                    return "DeviceRGB"sv;
                case PdfColorSpaceType::DeviceCMYK:
                    return "DeviceCMYK"sv;
                case PdfColorSpaceType::CalGray:
                    return "CalGray"sv;
                case PdfColorSpaceType::CalRGB:
                    return "CalRGB"sv;
                case PdfColorSpaceType::Lab:
                    return "Lab"sv;
                case PdfColorSpaceType::ICCBased:
                    return "ICCBased"sv;
                case PdfColorSpaceType::Indexed:
                    return "Indexed"sv;
                case PdfColorSpaceType::Pattern:
                    return "Pattern"sv;
                case PdfColorSpaceType::Separation:
                    return "Separation"sv;
                case PdfColorSpaceType::DeviceN:
                    return "DeviceN"sv;
                case PdfColorSpaceType::Unknown:
                default:
                    throw PdfError(PdfErrorCode::InvalidEnumValue, __FILE__, __LINE__);
            }
        }

        static bool TryParse(const std::string_view& str, PdfColorSpaceType& value)
        {
            if (str == "DeviceGray")
            {
                value = PdfColorSpaceType::DeviceGray;
                return true;
            }
            else if (str == "DeviceRGB")
            {
                value = PdfColorSpaceType::DeviceRGB;
                return true;
            }
            else if (str == "DeviceCMYK")
            {
                value = PdfColorSpaceType::DeviceCMYK;
                return true;
            }
            else if (str == "CalGray")
            {
                value = PdfColorSpaceType::CalGray;
                return true;
            }
            else if (str == "CalRGB")
            {
                value = PdfColorSpaceType::CalRGB;
                return true;
            }
            else if (str == "Lab")
            {
                value = PdfColorSpaceType::Lab;
                return true;
            }
            else if (str == "ICCBased")
            {
                value = PdfColorSpaceType::ICCBased;
                return true;
            }
            else if (str == "Indexed")
            {
                value = PdfColorSpaceType::Indexed;
                return true;
            }
            else if (str == "Pattern")
            {
                value = PdfColorSpaceType::Pattern;
                return true;
            }
            else if (str == "Separation")
            {
                value = PdfColorSpaceType::Separation;
                return true;
            }
            else if (str == "DeviceN")
            {
                value = PdfColorSpaceType::DeviceN;
                return true;
            }

            return false;
        }
    };

    template<>
    struct Convert<PdfAnnotationType>
    {
        static std::string_view ToString(PdfAnnotationType value)
        {
            using namespace std;
            switch (value)
            {
                case PdfAnnotationType::Text:
                    return "Text"sv;
                case PdfAnnotationType::Link:
                    return "Link"sv;
                case PdfAnnotationType::FreeText:
                    return "FreeText"sv;
                case PdfAnnotationType::Line:
                    return "Line"sv;
                case PdfAnnotationType::Square:
                    return "Square"sv;
                case PdfAnnotationType::Circle:
                    return "Circle"sv;
                case PdfAnnotationType::Polygon:
                    return "Polygon"sv;
                case PdfAnnotationType::PolyLine:
                    return "PolyLine"sv;
                case PdfAnnotationType::Highlight:
                    return "Highlight"sv;
                case PdfAnnotationType::Underline:
                    return "Underline"sv;
                case PdfAnnotationType::Squiggly:
                    return "Squiggly"sv;
                case PdfAnnotationType::StrikeOut:
                    return "StrikeOut"sv;
                case PdfAnnotationType::Stamp:
                    return "Stamp"sv;
                case PdfAnnotationType::Caret:
                    return "Caret"sv;
                case PdfAnnotationType::Ink:
                    return "Ink"sv;
                case PdfAnnotationType::Popup:
                    return "Popup"sv;
                case PdfAnnotationType::FileAttachement:
                    return "FileAttachment"sv;
                case PdfAnnotationType::Sound:
                    return "Sound"sv;
                case PdfAnnotationType::Movie:
                    return "Movie"sv;
                case PdfAnnotationType::Widget:
                    return "Widget"sv;
                case PdfAnnotationType::Screen:
                    return "Screen"sv;
                case PdfAnnotationType::PrinterMark:
                    return "PrinterMark"sv;
                case PdfAnnotationType::TrapNet:
                    return "TrapNet"sv;
                case PdfAnnotationType::Watermark:
                    return "Watermark"sv;
                case PdfAnnotationType::Model3D:
                    return "3D"sv;
                case PdfAnnotationType::RichMedia:
                    return "RichMedia"sv;
                case PdfAnnotationType::WebMedia:
                    return "WebMedia"sv;
                case PdfAnnotationType::Redact:
                    return "Redact"sv;
                case PdfAnnotationType::Projection:
                    return "Projection"sv;
                default:
                    throw PdfError(PdfErrorCode::InvalidEnumValue, __FILE__, __LINE__);
            }
        }

        static bool TryParse(const std::string_view& str, PdfAnnotationType& value)
        {
            using namespace std;
            if (str == "Text"sv)
            {
                value = PdfAnnotationType::Text;
                return true;
            }
            else if (str == "Link"sv)
            {
                value = PdfAnnotationType::Link;
                return true;
            }
            else if (str == "FreeText"sv)
            {
                value = PdfAnnotationType::FreeText;
                return true;
            }
            else if (str == "Line"sv)
            {
                value = PdfAnnotationType::Line;
                return true;
            }
            else if (str == "Square"sv)
            {
                value = PdfAnnotationType::Square;
                return true;
            }
            else if (str == "Circle"sv)
            {
                value = PdfAnnotationType::Circle;
                return true;
            }
            else if (str == "Polygon"sv)
            {
                value = PdfAnnotationType::Polygon;
                return true;
            }
            else if (str == "PolyLine"sv)
            {
                value = PdfAnnotationType::PolyLine;
                return true;
            }
            else if (str == "Highlight"sv)
            {
                value = PdfAnnotationType::Highlight;
                return true;
            }
            else if (str == "Underline"sv)
            {
                value = PdfAnnotationType::Underline;
                return true;
            }
            else if (str == "Squiggly"sv)
            {
                value = PdfAnnotationType::Squiggly;
                return true;
            }
            else if (str == "StrikeOut"sv)
            {
                value = PdfAnnotationType::StrikeOut;
                return true;
            }
            else if (str == "Stamp"sv)
            {
                value = PdfAnnotationType::Stamp;
                return true;
            }
            else if (str == "Caret"sv)
            {
                value = PdfAnnotationType::Caret;
                return true;
            }
            else if (str == "Ink"sv)
            {
                value = PdfAnnotationType::Ink;
                return true;
            }
            else if (str == "Popup"sv)
            {
                value = PdfAnnotationType::Popup;
                return true;
            }
            else if (str == "FileAttachment"sv)
            {
                value = PdfAnnotationType::FileAttachement;
                return true;
            }
            else if (str == "Sound"sv)
            {
                value = PdfAnnotationType::Sound;
                return true;
            }
            else if (str == "Movie"sv)
            {
                value = PdfAnnotationType::Movie;
                return true;
            }
            else if (str == "Widget"sv)
            {
                value = PdfAnnotationType::Widget;
                return true;
            }
            else if (str == "Screen"sv)
            {
                value = PdfAnnotationType::Screen;
                return true;
            }
            else if (str == "PrinterMark"sv)
            {
                value = PdfAnnotationType::PrinterMark;
                return true;
            }
            else if (str == "TrapNet"sv)
            {
                value = PdfAnnotationType::TrapNet;
                return true;
            }
            else if (str == "Watermark"sv)
            {
                value = PdfAnnotationType::Watermark;
                return true;
            }
            else if (str == "3D"sv)
            {
                value = PdfAnnotationType::Model3D;
                return true;
            }
            else if (str == "RichMedia"sv)
            {
                value = PdfAnnotationType::RichMedia;
                return true;
            }
            else if (str == "WebMedia"sv)
            {
                value = PdfAnnotationType::WebMedia;
                return true;
            }
            else if (str == "Redact"sv)
            {
                value = PdfAnnotationType::Redact;
                return true;
            }
            else if (str == "Projection"sv)
            {
                value = PdfAnnotationType::Projection;
                return true;
            }
            else
            {
                return false;
            }
        }
    };

    template<>
    struct Convert<PdfRenderingIntent>
    {
        static std::string_view ToString(PdfRenderingIntent value)
        {
            using namespace std;
            switch (value)
            {
                case PdfRenderingIntent::AbsoluteColorimetric:
                    return "AbsoluteColorimetric"sv;
                case PdfRenderingIntent::RelativeColorimetric:
                    return "RelativeColorimetric"sv;
                case PdfRenderingIntent::Perceptual:
                    return "Perceptual"sv;
                case PdfRenderingIntent::Saturation:
                    return "Saturation"sv;
                case PdfRenderingIntent::Unknown:
                default:
                    throw PdfError(PdfErrorCode::InvalidEnumValue, __FILE__, __LINE__);
            }
        }

        static bool TryParse(const std::string_view& str, PdfRenderingIntent& value)
        {
            if (str == "AbsoluteColorimetric")
            {
                value = PdfRenderingIntent::AbsoluteColorimetric;
                return true;
            }
            else if (str == "RelativeColorimetric")
            {
                value = PdfRenderingIntent::RelativeColorimetric;
                return true;
            }
            else if (str == "Perceptual")
            {
                value = PdfRenderingIntent::Perceptual;
                return true;
            }
            else if (str == "Saturation")
            {
                value = PdfRenderingIntent::Saturation;
                return true;
            }

            return false;
        }
    };

    template<>
    struct Convert<PdfBlendMode>
    {
        static std::string_view ToString(PdfBlendMode value)
        {
            using namespace std;
            switch (value)
            {
                case PdfBlendMode::Normal:
                    return "Normal"sv;
                case PdfBlendMode::Multiply:
                    return "Multiply"sv;
                case PdfBlendMode::Screen:
                    return "Screen"sv;
                case PdfBlendMode::Overlay:
                    return "Overlay"sv;
                case PdfBlendMode::Darken:
                    return "Darken"sv;
                case PdfBlendMode::Lighten:
                    return "Lighten"sv;
                case PdfBlendMode::ColorDodge:
                    return "ColorDodge"sv;
                case PdfBlendMode::ColorBurn:
                    return "ColorBurn"sv;
                case PdfBlendMode::HardLight:
                    return "HardLight"sv;
                case PdfBlendMode::SoftLight:
                    return "SoftLight"sv;
                case PdfBlendMode::Difference:
                    return "Difference"sv;
                case PdfBlendMode::Exclusion:
                    return "Exclusion"sv;
                case PdfBlendMode::Hue:
                    return "Hue"sv;
                case PdfBlendMode::Saturation:
                    return "Saturation"sv;
                case PdfBlendMode::Color:
                    return "Color"sv;
                case PdfBlendMode::Luminosity:
                    return "Luminosity"sv;
                case PdfBlendMode::Unknown:
                default:
                    throw PdfError(PdfErrorCode::InvalidEnumValue, __FILE__, __LINE__);
            }
        }

        static bool TryParse(const std::string_view& str, PdfBlendMode& value)
        {
            if (str == "Normal")
            {
                value = PdfBlendMode::Normal;
                return true;
            }
            else if (str == "Multiply")
            {
                value = PdfBlendMode::Multiply;
                return true;
            }
            else if (str == "Screen")
            {
                value = PdfBlendMode::Screen;
                return true;
            }
            else if (str == "Overlay")
            {
                value = PdfBlendMode::Overlay;
                return true;
            }
            else if (str == "Darken")
            {
                value = PdfBlendMode::Darken;
                return true;
            }
            else if (str == "Lighten")
            {
                value = PdfBlendMode::Lighten;
                return true;
            }
            else if (str == "ColorDodge")
            {
                value = PdfBlendMode::ColorDodge;
                return true;
            }
            else if (str == "ColorBurn")
            {
                value = PdfBlendMode::ColorBurn;
                return true;
            }
            else if (str == "HardLight")
            {
                value = PdfBlendMode::HardLight;
                return true;
            }
            else if (str == "SoftLight")
            {
                value = PdfBlendMode::SoftLight;
                return true;
            }
            else if (str == "Difference")
            {
                value = PdfBlendMode::Difference;
                return true;
            }
            else if (str == "Exclusion")
            {
                value = PdfBlendMode::Exclusion;
                return true;
            }
            else if (str == "Hue")
            {
                value = PdfBlendMode::Hue;
                return true;
            }
            else if (str == "Saturation")
            {
                value = PdfBlendMode::Saturation;
                return true;
            }
            else if (str == "Color")
            {
                value = PdfBlendMode::Color;
                return true;
            }
            else if (str == "Luminosity")
            {
                value = PdfBlendMode::Luminosity;
                return true;
            }

            return false;
        }
    };

    template<>
    struct Convert<PdfOperator>
    {
        static std::string_view ToString(PdfOperator value)
        {
            using namespace std;
            switch (value)
            {
                case PdfOperator::w:
                    return "w"sv;
                case PdfOperator::J:
                    return "J"sv;
                case PdfOperator::j:
                    return "j"sv;
                case PdfOperator::M:
                    return "M"sv;
                case PdfOperator::d:
                    return "d"sv;
                case PdfOperator::ri:
                    return "ri"sv;
                case PdfOperator::i:
                    return "i"sv;
                case PdfOperator::gs:
                    return "gs"sv;
                case PdfOperator::q:
                    return "q"sv;
                case PdfOperator::Q:
                    return "Q"sv;
                case PdfOperator::cm:
                    return "cm"sv;
                case PdfOperator::m:
                    return "m"sv;
                case PdfOperator::l:
                    return "l"sv;
                case PdfOperator::c:
                    return "c"sv;
                case PdfOperator::v:
                    return "v"sv;
                case PdfOperator::y:
                    return "y"sv;
                case PdfOperator::h:
                    return "h"sv;
                case PdfOperator::re:
                    return "re"sv;
                case PdfOperator::S:
                    return "S"sv;
                case PdfOperator::s:
                    return "s"sv;
                case PdfOperator::f:
                    return "f"sv;
                case PdfOperator::F:
                    return "F"sv;
                case PdfOperator::f_Star:
                    return "f*"sv;
                case PdfOperator::B:
                    return "B"sv;
                case PdfOperator::B_Star:
                    return "B*"sv;
                case PdfOperator::b:
                    return "b"sv;
                case PdfOperator::b_Star:
                    return "b*"sv;
                case PdfOperator::n:
                    return "n"sv;
                case PdfOperator::W:
                    return "W"sv;
                case PdfOperator::W_Star:
                    return "W*"sv;
                case PdfOperator::BT:
                    return "BT"sv;
                case PdfOperator::ET:
                    return "ET"sv;
                case PdfOperator::Tc:
                    return "Tc"sv;
                case PdfOperator::Tw:
                    return "Tw"sv;
                case PdfOperator::Tz:
                    return "Tz"sv;
                case PdfOperator::TL:
                    return "TL"sv;
                case PdfOperator::Tf:
                    return "Tf"sv;
                case PdfOperator::Tr:
                    return "Tr"sv;
                case PdfOperator::Ts:
                    return "Ts"sv;
                case PdfOperator::Td:
                    return "Td"sv;
                case PdfOperator::TD:
                    return "TD"sv;
                case PdfOperator::Tm:
                    return "Tm"sv;
                case PdfOperator::T_Star:
                    return "T*"sv;
                case PdfOperator::Tj:
                    return "Tj"sv;
                case PdfOperator::TJ:
                    return "TJ"sv;
                case PdfOperator::Quote:
                    return "'"sv;
                case PdfOperator::DoubleQuote:
                    return "\""sv;
                case PdfOperator::d0:
                    return "d0"sv;
                case PdfOperator::d1:
                    return "d1"sv;
                case PdfOperator::CS:
                    return "CS"sv;
                case PdfOperator::cs:
                    return "cs"sv;
                case PdfOperator::SC:
                    return "SC"sv;
                case PdfOperator::SCN:
                    return "SCN"sv;
                case PdfOperator::sc:
                    return "sc"sv;
                case PdfOperator::scn:
                    return "scn"sv;
                case PdfOperator::G:
                    return "G"sv;
                case PdfOperator::g:
                    return "g"sv;
                case PdfOperator::RG:
                    return "RG"sv;
                case PdfOperator::rg:
                    return "rg"sv;
                case PdfOperator::K:
                    return "K"sv;
                case PdfOperator::k:
                    return "k"sv;
                case PdfOperator::sh:
                    return "sh"sv;
                case PdfOperator::BI:
                    return "BI"sv;
                case PdfOperator::ID:
                    return "ID"sv;
                case PdfOperator::EI:
                    return "EI"sv;
                case PdfOperator::Do:
                    return "Do"sv;
                case PdfOperator::MP:
                    return "MP"sv;
                case PdfOperator::DP:
                    return "DP"sv;
                case PdfOperator::BMC:
                    return "BMC"sv;
                case PdfOperator::BDC:
                    return "BDC"sv;
                case PdfOperator::EMC:
                    return "EMC"sv;
                case PdfOperator::BX:
                    return "BX"sv;
                case PdfOperator::EX:
                    return "EX"sv;
                case PdfOperator::Unknown:
                default:
                    throw PdfError(PdfErrorCode::InvalidEnumValue, __FILE__, __LINE__);
            }
        }

        static bool TryParse(const std::string_view& str, PdfOperator& value)
        {
            if (str == "w")
            {
                value = PdfOperator::w;
                return true;
            }
            else if (str == "J")
            {
                value = PdfOperator::J;
                return true;
            }
            else if (str == "j")
            {
                value = PdfOperator::j;
                return true;
            }
            else if (str == "M")
            {
                value = PdfOperator::M;
                return true;
            }
            else if (str == "d")
            {
                value = PdfOperator::d;
                return true;
            }
            else if (str == "ri")
            {
                value = PdfOperator::ri;
                return true;
            }
            else if (str == "i")
            {
                value = PdfOperator::i;
                return true;
            }
            else if (str == "gs")
            {
                value = PdfOperator::gs;
                return true;
            }
            else if (str == "q")
            {
                value = PdfOperator::q;
                return true;
            }
            else if (str == "Q")
            {
                value = PdfOperator::Q;
                return true;
            }
            else if (str == "cm")
            {
                value = PdfOperator::cm;
                return true;
            }
            else if (str == "m")
            {
                value = PdfOperator::m;
                return true;
            }
            else if (str == "l")
            {
                value = PdfOperator::l;
                return true;
            }
            else if (str == "c")
            {
                value = PdfOperator::c;
                return true;
            }
            else if (str == "v")
            {
                value = PdfOperator::v;
                return true;
            }
            else if (str == "y")
            {
                value = PdfOperator::y;
                return true;
            }
            else if (str == "h")
            {
                value = PdfOperator::h;
                return true;
            }
            else if (str == "re")
            {
                value = PdfOperator::re;
                return true;
            }
            else if (str == "S")
            {
                value = PdfOperator::S;
                return true;
            }
            else if (str == "s")
            {
                value = PdfOperator::s;
                return true;
            }
            else if (str == "f")
            {
                value = PdfOperator::f;
                return true;
            }
            else if (str == "F")
            {
                value = PdfOperator::F;
                return true;
            }
            else if (str == "f*")
            {
                value = PdfOperator::f_Star;
                return true;
            }
            else if (str == "B")
            {
                value = PdfOperator::B;
                return true;
            }
            else if (str == "B*")
            {
                value = PdfOperator::B_Star;
                return true;
            }
            else if (str == "b")
            {
                value = PdfOperator::b;
                return true;
            }
            else if (str == "b*")
            {
                value = PdfOperator::b_Star;
                return true;
            }
            else if (str == "n")
            {
                value = PdfOperator::n;
                return true;
            }
            else if (str == "W")
            {
                value = PdfOperator::W;
                return true;
            }
            else if (str == "W*")
            {
                value = PdfOperator::W_Star;
                return true;
            }
            else if (str == "BT")
            {
                value = PdfOperator::BT;
                return true;
            }
            else if (str == "ET")
            {
                value = PdfOperator::ET;
                return true;
            }
            else if (str == "Tc")
            {
                value = PdfOperator::Tc;
                return true;
            }
            else if (str == "Tw")
            {
                value = PdfOperator::Tw;
                return true;
            }
            else if (str == "Tz")
            {
                value = PdfOperator::Tz;
                return true;
            }
            else if (str == "TL")
            {
                value = PdfOperator::TL;
                return true;
            }
            else if (str == "Tf")
            {
                value = PdfOperator::Tf;
                return true;
            }
            else if (str == "Tr")
            {
                value = PdfOperator::Tr;
                return true;
            }
            else if (str == "Ts")
            {
                value = PdfOperator::Ts;
                return true;
            }
            else if (str == "Td")
            {
                value = PdfOperator::Td;
                return true;
            }
            else if (str == "TD")
            {
                value = PdfOperator::TD;
                return true;
            }
            else if (str == "Tm")
            {
                value = PdfOperator::Tm;
                return true;
            }
            else if (str == "T*")
            {
                value = PdfOperator::T_Star;
                return true;
            }
            else if (str == "Tj")
            {
                value = PdfOperator::Tj;
                return true;
            }
            else if (str == "TJ")
            {
                value = PdfOperator::TJ;
                return true;
            }
            else if (str == "'")
            {
                value = PdfOperator::Quote;
                return true;
            }
            else if (str == "\"")
            {
                value = PdfOperator::DoubleQuote;
                return true;
            }
            else if (str == "d0")
            {
                value = PdfOperator::d0;
                return true;
            }
            else if (str == "d1")
            {
                value = PdfOperator::d1;
                return true;
            }
            else if (str == "CS")
            {
                value = PdfOperator::CS;
                return true;
            }
            else if (str == "cs")
            {
                value = PdfOperator::cs;
                return true;
            }
            else if (str == "SC")
            {
                value = PdfOperator::SC;
                return true;
            }
            else if (str == "SCN")
            {
                value = PdfOperator::SCN;
                return true;
            }
            else if (str == "sc")
            {
                value = PdfOperator::sc;
                return true;
            }
            else if (str == "scn")
            {
                value = PdfOperator::scn;
                return true;
            }
            else if (str == "G")
            {
                value = PdfOperator::G;
                return true;
            }
            else if (str == "g")
            {
                value = PdfOperator::g;
                return true;
            }
            else if (str == "RG")
            {
                value = PdfOperator::RG;
                return true;
            }
            else if (str == "rg")
            {
                value = PdfOperator::rg;
                return true;
            }
            else if (str == "K")
            {
                value = PdfOperator::K;
                return true;
            }
            else if (str == "k")
            {
                value = PdfOperator::k;
                return true;
            }
            else if (str == "sh")
            {
                value = PdfOperator::sh;
                return true;
            }
            else if (str == "BI")
            {
                value = PdfOperator::BI;
                return true;
            }
            else if (str == "ID")
            {
                value = PdfOperator::ID;
                return true;
            }
            else if (str == "EI")
            {
                value = PdfOperator::EI;
                return true;
            }
            else if (str == "Do")
            {
                value = PdfOperator::Do;
                return true;
            }
            else if (str == "MP")
            {
                value = PdfOperator::MP;
                return true;
            }
            else if (str == "DP")
            {
                value = PdfOperator::DP;
                return true;
            }
            else if (str == "BMC")
            {
                value = PdfOperator::BMC;
                return true;
            }
            else if (str == "BDC")
            {
                value = PdfOperator::BDC;
                return true;
            }
            else if (str == "EMC")
            {
                value = PdfOperator::EMC;
                return true;
            }
            else if (str == "BX")
            {
                value = PdfOperator::BX;
                return true;
            }
            else if (str == "EX")
            {
                value = PdfOperator::EX;
                return true;
            }
            else
            {
                value = PdfOperator::Unknown;
                return false;
            }
        }
    };

    template<>
    struct Convert<PdfALevel>
    {
        static std::string_view ToString(PdfALevel value)
        {
            using namespace std;
            switch (value)
            {
                case PdfALevel::L1B:
                    return "L1B"sv;
                case PdfALevel::L1A:
                    return "L1A"sv;
                case PdfALevel::L2B:
                    return "L2B"sv;
                case PdfALevel::L2A:
                    return "L2A"sv;
                case PdfALevel::L2U:
                    return "L2U"sv;
                case PdfALevel::L3B:
                    return "L3B"sv;
                case PdfALevel::L3A:
                    return "L3A"sv;
                case PdfALevel::L3U:
                    return "L3U"sv;
                case PdfALevel::L4:
                    return "L4"sv;
                case PdfALevel::L4E:
                    return "L4E"sv;
                case PdfALevel::L4F:
                    return "L4F"sv;
                default:
                    throw PdfError(PdfErrorCode::InvalidEnumValue, __FILE__, __LINE__);
            }
        }

        static bool TryParse(const std::string_view& str, PdfALevel& value)
        {
            if (str == "L1B")
            {
                value = PdfALevel::L1B;
                return true;
            }
            else if (str == "L1A")
            {
                value = PdfALevel::L1A;
                return true;
            }
            else if (str == "L2B")
            {
                value = PdfALevel::L2B;
                return true;
            }
            else if (str == "L2A")
            {
                value = PdfALevel::L2A;
                return true;
            }
            else if (str == "L2U")
            {
                value = PdfALevel::L2U;
                return true;
            }
            else if (str == "L3B")
            {
                value = PdfALevel::L3B;
                return true;
            }
            else if (str == "L3A")
            {
                value = PdfALevel::L3A;
                return true;
            }
            else if (str == "L3U")
            {
                value = PdfALevel::L3U;
                return true;
            }
            else if (str == "L4")
            {
                value = PdfALevel::L4;
                return true;
            }
            else if (str == "L4E")
            {
                value = PdfALevel::L4E;
                return true;
            }
            else if (str == "L4F")
            {
                value = PdfALevel::L4F;
                return true;
            }

            return false;
        }
    };

    template<>
    struct Convert<PdfUALevel>
    {
        static std::string_view ToString(PdfUALevel value)
        {
            using namespace std;
            switch (value)
            {
                case PdfUALevel::L1:
                    return "L1"sv;
                case PdfUALevel::L2:
                    return "L2"sv;
                default:
                    throw PdfError(PdfErrorCode::InvalidEnumValue, __FILE__, __LINE__);
            }
        }

        static bool TryParse(const std::string_view& str, PdfUALevel& value)
        {
            if (str == "L1")
            {
                value = PdfUALevel::L1;
                return true;
            }
            else if (str == "L2")
            {
                value = PdfUALevel::L2;
                return true;
            }

            return false;
        }
    };

    template<typename T>
    std::string_view ToString(T value)
    {
        return Convert<T>::ToString(value);
    }

    template<typename T>
    bool TryConvertTo(const std::string_view& str, T& value)
    {
        value = { };
        return Convert<T>::TryParse(str, value);
    }

    template<typename T>
    T ConvertTo(const std::string_view& str)
    {
        T value;
        if (!Convert<T>::TryParse(str, value))
            throw PdfError(PdfErrorCode::InvalidEnumValue, __FILE__, __LINE__);

        return value;
    }
}

#endif // PDF_CONVERT_H

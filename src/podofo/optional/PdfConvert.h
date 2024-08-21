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

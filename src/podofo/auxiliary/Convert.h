/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef AUX_CONVERT_H
#define AUX_CONVERT_H

#include <podofo/main/PdfDeclarations.h>

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

    template<typename T>
    std::string_view ToString(T value)
    {
        return Convert<T>::ToString(value);
    }

    template<typename T>
    bool ConvertTo(const std::string_view& str, T& value)
    {
        value = { };
        return Convert<T>::TryParse(str, value);
    }

    template<typename T>
    T ConvertTo(const std::string_view& str)
    {
        T value;
        if (!Convert<T>::TryParse(str, value));
            throw PdfError(PdfErrorCode::InvalidEnumValue, __FILE__, __LINE__);

        return value;
    }
}

#endif // AUX_CONVERT_H

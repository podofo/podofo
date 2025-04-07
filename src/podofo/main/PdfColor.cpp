/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfColor.h"

#include "PdfTokenizer.h"
#include <podofo/auxiliary/StreamDevice.h>

using namespace std;
using namespace PoDoFo;

namespace
{
    /** A PdfNamedColor holds
     *  a PdfColor object and a name.
     */
    class PdfNamedColor
    {
    public:
        /** Create a PdfNamedColor object.
         *
         *  \param name the name. The string must be allocated as static memory somewhere
         *         The string data will not be copied!
         *  \param color a PdfColor object
         */
        PdfNamedColor(const string_view& name, const PdfColor& color)
            : m_Name(utls::ToLower(name)), m_color(color)
        {
        }

        /** Create a PdfNamedColor object.
         *
         *  \param name the name. The string must be allocated as static memory somewhere
         *         The string data will not be copied!
         *  \param colorName RGB hex value (e.g. #FFABCD)
         */
        PdfNamedColor(const string_view& name, const string_view& colorCode)
            : m_Name(name), m_color(createFromRGBString(colorCode))
        {
        }

        /** Copy constructor
         */
        PdfNamedColor(const PdfNamedColor& rhs)
            : m_Name(rhs.m_Name), m_color(rhs.m_color)
        {
        }

        /** Compare this color object to a name
         *  The comparison is case insensitive!
         *  \returns true if the passed string is smaller than the name
         *           of this color object.
         */
        inline bool operator<(const string_view& name) const
        {
            return m_Name < name;
        }

        /** Compare this color object to a PdfNamedColor comparing only the name.
         *  The comparison is case insensitive!
         *  \returns true if the passed string is smaller than the name
         *           of this color object.
         */
        inline bool operator<(const PdfNamedColor& rhs) const
        {
            return m_Name < rhs.GetName();
        }

        /** Compare this color object to a name
         *  The comparison is case insensitive!
         *  \returns true if the passed string is the name
         *           of this color object.
         */
        inline bool operator==(const string_view& name) const
        {
            return m_Name == name;
        }

        /**
         * \returns a reference to the internal color object
         */
        inline const PdfColor& GetColor() const
        {
            return m_color;
        }

        /**
         * \returns a pointer to the name of the color
         */
        inline const string& GetName() const
        {
            return m_Name;
        }

    private:
        PdfNamedColor& operator=(const PdfNamedColor&) = delete;

        /** Creates a color object from a RGB string.
         *
         *  \param name a string describing a color.
         *
         *  Supported values are:
         *  - hex values (e.g. #FF002A (RGB))
         *
         *  \returns a PdfColor object
         */
        static PdfColor createFromRGBString(string_view name);

        string m_Name;
        PdfColor m_color;
    };

    /**
     * Predicate to allow binary search in the list
     * of PdfNamedColor's using for example std::equal_range.
     */
    class NamedColorComparatorPredicate
    {
    public:
        NamedColorComparatorPredicate()
        {
        }

        inline bool operator()(const PdfNamedColor& namedColor1, const PdfNamedColor& namedColor2) const
        {
            return namedColor1 < namedColor2;
        }
    };
}

// Table based on http://cvsweb.xfree86.org/cvsweb/xc/programs/rgb/rgb.txt?rev=1.2
// Hex values have been copied from http://en.wikipedia.org/wiki/X11_color_names (21/11/2010)
static const size_t s_NamedColorsCount = 148;
static const PdfNamedColor s_NamedColors[s_NamedColorsCount] =
{
    PdfNamedColor( "aliceblue",     "#F0F8FF" ) ,
    PdfNamedColor( "antiquewhite",  "#FAEBD7" ) ,
    PdfNamedColor( "aqua",          "#00FFFF" ) ,
    PdfNamedColor( "aquamarine",    "#7FFFD4" ) ,
    PdfNamedColor( "azure",         "#F0FFFF" ) ,
    PdfNamedColor( "beige",         "#F5F5DC" ) ,
    PdfNamedColor( "bisque",        "#FFE4C4" ) ,
    PdfNamedColor( "black",         "#000000" ) ,
    PdfNamedColor( "blanchedalmond","#FFEBCD" ) ,
    PdfNamedColor( "blue",          "#0000FF" ) ,
    PdfNamedColor( "blueviolet",    "#8A2BE2" ) ,
    PdfNamedColor( "brown",         "#A52A2A" ) ,
    PdfNamedColor( "burlywood",     "#DEB887" ) ,
    PdfNamedColor( "cadetblue",     "#5F9EA0" ) ,
    PdfNamedColor( "chartreuse",    "#7FFF00" ) ,
    PdfNamedColor( "chocolate",     "#D2691E" ) ,
    PdfNamedColor( "coral",         "#FF7F50" ) ,
    PdfNamedColor( "cornflowerblue","#6495ED" ) ,
    PdfNamedColor( "cornsilk",      "#FFF8DC" ) ,
    PdfNamedColor( "crimson",       "#DC143C" ) ,
    PdfNamedColor( "cyan",          "#00FFFF" ) ,
    PdfNamedColor( "darkblue",      "#00008B" ) ,
    PdfNamedColor( "darkcyan",      "#008B8B" ) ,
    PdfNamedColor( "darkgoldenrod", "#B8860B" ) ,
    PdfNamedColor( "darkgray",      "#A9A9A9" ) ,
    PdfNamedColor( "darkgreen",     "#006400" ) ,
    PdfNamedColor( "darkgrey",      "#A9A9A9" ) ,
    PdfNamedColor( "darkkhaki",     "#BDB76B" ) ,
    PdfNamedColor( "darkmagenta",   "#8B008B" ) ,
    PdfNamedColor( "darkolivegreen","#556B2F" ) ,
    PdfNamedColor( "darkorange",    "#FF8C00" ) ,
    PdfNamedColor( "darkorchid",    "#9932CC" ) ,
    PdfNamedColor( "darkred",       "#8B0000" ) ,
    PdfNamedColor( "darksalmon",    "#E9967A" ) ,
    PdfNamedColor( "darkseagreen",  "#8FBC8F" ) ,
    PdfNamedColor( "darkslateblue", "#483D8B" ) ,
    PdfNamedColor( "darkslategray", "#2F4F4F" ) ,
    PdfNamedColor( "darkslategrey", "#2F4F4F" ) ,
    PdfNamedColor( "darkturquoise", "#00CED1" ) ,
    PdfNamedColor( "darkviolet",    "#9400D3" ) ,
    PdfNamedColor( "deeppink",      "#FF1493" ) ,
    PdfNamedColor( "deepskyblue",   "#00BFFF" ) ,
    PdfNamedColor( "dimgray",       "#696969" ) ,
    PdfNamedColor( "dimgrey",       "#696969" ) ,
    PdfNamedColor( "dodgerblue",    "#1E90FF" ) ,
    PdfNamedColor( "firebrick",     "#B22222" ) ,
    PdfNamedColor( "floralwhite",   "#FFFAF0" ) ,
    PdfNamedColor( "forestgreen",   "#228B22" ) ,
    PdfNamedColor( "fuchsia",       "#FF00FF" ) ,
    PdfNamedColor( "gainsboro",     "#DCDCDC" ) ,
    PdfNamedColor( "ghostwhite",    "#F8F8FF" ) ,
    PdfNamedColor( "gold",          "#FFD700" ) ,
    PdfNamedColor( "goldenrod",     "#DAA520" ) ,
    PdfNamedColor( "gray",          "#BEBEBE" ) , //RG changed from W3C to X11 value
    PdfNamedColor( "green",         "#00FF00" ) ,
    PdfNamedColor( "greenyellow",   "#ADFF2F" ) ,
    PdfNamedColor( "grey",          "#BEBEBE" ) , //RG changed from W3C to X11 value
    PdfNamedColor( "honeydew",      "#F0FFF0" ) ,
    PdfNamedColor( "hotpink",       "#FF69B4" ) ,
    PdfNamedColor( "indianred",     "#CD5C5C" ) ,
    PdfNamedColor( "indigo",        "#4B0082" ) ,
    PdfNamedColor( "ivory",         "#FFFFF0" ) ,
    PdfNamedColor( "khaki",         "#F0E68C" ) ,
    PdfNamedColor( "lavender",      "#E6E6FA" ) ,
    PdfNamedColor( "lavenderblush", "#FFF0F5" ) ,
    PdfNamedColor( "lawngreen",     "#7CFC00" ) ,
    PdfNamedColor( "lemonchiffon",  "#FFFACD" ) ,
    PdfNamedColor( "lightblue",     "#ADD8E6" ) ,
    PdfNamedColor( "lightcoral",    "#F08080" ) ,
    PdfNamedColor( "lightcyan",     "#E0FFFF" ) ,
    PdfNamedColor( "lightgoldenrod", "#EEDD82" ) ,
    PdfNamedColor( "lightgoldenrodyellow", "#FAFAD2" ) ,
    PdfNamedColor( "lightgray",     "#D3D3D3" ) ,
    PdfNamedColor( "lightgreen",    "#90EE90" ) ,
    PdfNamedColor( "lightgrey",     "#D3D3D3" ) ,
    PdfNamedColor( "lightpink",     "#FFB6C1" ) ,
    PdfNamedColor( "lightsalmon",   "#FFA07A" ) ,
    PdfNamedColor( "lightseagreen", "#20B2AA" ) ,
    PdfNamedColor( "lightskyblue",  "#87CEFA" ) ,
    PdfNamedColor( "lightslategray","#778899" ) ,
    PdfNamedColor( "lightslategrey","#778899" ) ,
    PdfNamedColor( "lightsteelblue","#B0C4DE" ) ,
    PdfNamedColor( "lightyellow",   "#FFFFE0" ) ,
    PdfNamedColor( "lime",          "#00FF00" ) ,
    PdfNamedColor( "limegreen",     "#32CD32" ) ,
    PdfNamedColor( "linen",         "#FAF0E6" ) ,
    PdfNamedColor( "magenta",       "#FF00FF" ) ,
    PdfNamedColor( "maroon",        "#B03060" ) , //RG changed from W3C to X11 value
    PdfNamedColor( "mediumaquamarine", "#66CDAA" ) ,
    PdfNamedColor( "mediumblue",    "#0000CD" ) ,
    PdfNamedColor( "mediumorchid",  "#BA55D3" ) ,
    PdfNamedColor( "mediumpurple",  "#9370DB" ) ,
    PdfNamedColor( "mediumseagreen","#3CB371" ) ,
    PdfNamedColor( "mediumslateblue", "#7B68EE" ) ,
    PdfNamedColor( "mediumspringgreen", "#00FA9A" ) ,
    PdfNamedColor( "mediumturquoise", "#48D1CC" ) ,
    PdfNamedColor( "mediumvioletred", "#C71585" ) ,
    PdfNamedColor( "midnightblue",  "#191970" ) ,
    PdfNamedColor( "mintcream",     "#F5FFFA" ) ,
    PdfNamedColor( "mistyrose",     "#FFE4E1" ) ,
    PdfNamedColor( "moccasin",      "#FFE4B5" ) ,
    PdfNamedColor( "navajowhite",   "#FFDEAD" ) ,
    PdfNamedColor( "navy",          "#000080" ) ,
    PdfNamedColor( "oldlace",       "#FDF5E6" ) ,
    PdfNamedColor( "olive",         "#808000" ) ,
    PdfNamedColor( "olivedrab",     "#6B8E23" ) ,
    PdfNamedColor( "orange",        "#FFA500" ) ,
    PdfNamedColor( "orangered",     "#FF4500" ) ,
    PdfNamedColor( "orchid",        "#DA70D6" ) ,
    PdfNamedColor( "palegoldenrod", "#EEE8AA" ) ,
    PdfNamedColor( "palegreen",     "#98FB98" ) ,
    PdfNamedColor( "paleturquoise", "#AFEEEE" ) ,
    PdfNamedColor( "palevioletred", "#DB7093" ) ,
    PdfNamedColor( "papayawhip",    "#FFEFD5" ) ,
    PdfNamedColor( "peachpuff",     "#FFDAB9" ) ,
    PdfNamedColor( "peru",          "#CD853F" ) ,
    PdfNamedColor( "pink",          "#FFC0CB" ) ,
    PdfNamedColor( "plum",          "#DDA0DD" ) ,
    PdfNamedColor( "powderblue",    "#B0E0E6" ) ,
    PdfNamedColor( "purple",        "#A020F0" ) , //RG changed from W3C to X11 value
    PdfNamedColor( "red",           "#FF0000" ) ,
    PdfNamedColor( "rosybrown",     "#BC8F8F" ) ,
    PdfNamedColor( "royalblue",     "#4169E1" ) ,
    PdfNamedColor( "saddlebrown",   "#8B4513" ) ,
    PdfNamedColor( "salmon",        "#FA8072" ) ,
    PdfNamedColor( "sandybrown",    "#F4A460" ) ,
    PdfNamedColor( "seagreen",      "#2E8B57" ) ,
    PdfNamedColor( "seashell",      "#FFF5EE" ) ,
    PdfNamedColor( "sienna",        "#A0522D" ) ,
    PdfNamedColor( "silver",        "#C0C0C0" ) ,
    PdfNamedColor( "skyblue",       "#87CEEB" ) ,
    PdfNamedColor( "slateblue",     "#6A5ACD" ) ,
    PdfNamedColor( "slategray",     "#708090" ) ,
    PdfNamedColor( "slategrey",     "#708090" ) ,
    PdfNamedColor( "snow",          "#FFFAFA" ) ,
    PdfNamedColor( "springgreen",   "#00FF7F" ) ,
    PdfNamedColor( "steelblue",     "#4682B4" ) ,
    PdfNamedColor( "tan",           "#D2B48C" ) ,
    PdfNamedColor( "teal",          "#008080" ) ,
    PdfNamedColor( "thistle",       "#D8BFD8" ) ,
    PdfNamedColor( "tomato",        "#FF6347" ) ,
    PdfNamedColor( "turquoise",     "#40E0D0" ) ,
    PdfNamedColor( "violet",        "#EE82EE" ) ,
    PdfNamedColor( "wheat",         "#F5DEB3" ) ,
    PdfNamedColor( "white",         "#FFFFFF" ) ,
    PdfNamedColor( "whitesmoke",    "#F5F5F5" ) ,
    PdfNamedColor( "yellow",        "#FFFF00" ) ,
    PdfNamedColor( "yellowgreen",   "#9ACD32" ) 
};

inline void CheckDoubleRange(double val, double min, double max)
{
    if ((val < min) || (val > max))
        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);
}

PdfColor::PdfColor() :
    m_IsTransparent(false),
    m_ColorSpace(PdfColorSpaceType::DeviceGray),
    m_ComponentCount(1),
    m_RawColor{ }
{
}

PdfColor::PdfColor(double gray) :
    m_IsTransparent(false),
    m_ColorSpace(PdfColorSpaceType::DeviceGray),
    m_ComponentCount(1),
    m_RawColor{ }
{
    CheckDoubleRange(gray, 0.0, 1.0);
    m_RawColor[0] = gray;
}

PdfColor::PdfColor(double red, double green, double blue) :
    m_IsTransparent(false),
    m_ColorSpace(PdfColorSpaceType::DeviceRGB),
    m_ComponentCount(3),
    m_RawColor{ }
{
    CheckDoubleRange(red, 0.0, 1.0);
    CheckDoubleRange(green, 0.0, 1.0);
    CheckDoubleRange(blue, 0.0, 1.0);

    m_RawColor[0] = red;
    m_RawColor[1] = green;
    m_RawColor[2] = blue;
}

PdfColor::PdfColor(double cyan, double magenta, double yellow, double black) :
    m_IsTransparent(false),
    m_ColorSpace(PdfColorSpaceType::DeviceCMYK),
    m_ComponentCount(4),
    m_RawColor{ }
{
    CheckDoubleRange(cyan, 0.0, 1.0);
    CheckDoubleRange(magenta, 0.0, 1.0);
    CheckDoubleRange(yellow, 0.0, 1.0);
    CheckDoubleRange(black, 0.0, 1.0);

    m_RawColor[0] = cyan;
    m_RawColor[1] = magenta;
    m_RawColor[2] = yellow;
    m_RawColor[3] = black;
}

PdfColor::PdfColor(bool isTransparent, PdfColorSpaceType colorSpace,
    unsigned char componentCount, const PdfColorRaw& data) :
    m_IsTransparent(isTransparent),
    m_ColorSpace(colorSpace),
    m_ComponentCount(componentCount),
    m_RawColor(data)
{
}

PdfColor PdfColor::CreateTransparent()
{
    return PdfColor(true, PdfColorSpaceType::Unknown, 0, { });
}

PdfColor PdfColor::ConvertToGrayScale() const
{
    switch (m_ColorSpace)
    {
        case PdfColorSpaceType::DeviceGray:
        {
            return *this;
        }
        case PdfColorSpaceType::DeviceRGB:
        {
            return PdfColor(0.299 * m_RawColor[0] + 0.587 * m_RawColor[1] + 0.114 * m_RawColor[2]);
        }
        case PdfColorSpaceType::DeviceCMYK:
        {
            return ConvertToRGB().ConvertToGrayScale();
        }
        case PdfColorSpaceType::Separation:
        case PdfColorSpaceType::Lab:
        case PdfColorSpaceType::Indexed:
        case PdfColorSpaceType::Unknown:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
            break;
        }
        default:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
            break;
        }
    };
}

PdfColor PdfColor::ConvertToRGB() const
{
    switch (m_ColorSpace)
    {
        case PdfColorSpaceType::DeviceGray:
        {
            return PdfColor(m_RawColor[0], m_RawColor[0], m_RawColor[0]);
        }
        case PdfColorSpaceType::DeviceRGB:
        {
            return *this;
        }
        case PdfColorSpaceType::DeviceCMYK:
        {
            double cyan = m_RawColor[0];
            double magenta = m_RawColor[1];
            double yellow = m_RawColor[2];
            double black = m_RawColor[3];

            double red = cyan * (1.0 - black) + black;
            double green = magenta * (1.0 - black) + black;
            double blue = yellow * (1.0 - black) + black;

            return PdfColor(1.0 - red, 1.0 - green, 1.0 - blue);
        }
        case PdfColorSpaceType::Separation:
        case PdfColorSpaceType::Lab:
        case PdfColorSpaceType::Indexed:
        case PdfColorSpaceType::Unknown:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
            break;
        }
        default:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
            break;
        }
    };
}

PdfColor PdfColor::ConvertToCMYK() const
{
    switch (m_ColorSpace)
    {
        case PdfColorSpaceType::DeviceGray:
        {
            return ConvertToRGB().ConvertToCMYK();
        }
        case PdfColorSpaceType::DeviceRGB:
        {
            double red = m_RawColor[0];
            double green = m_RawColor[1];
            double blue = m_RawColor[2];

            double black = std::min(1.0 - red, std::min(1.0 - green, 1.0 - blue));

            double cyan = 0.0;
            double magenta = 0.0;
            double yellow = 0.0;
            if (black < 1.0)
            {
                cyan = (1.0 - red - black) / (1.0 - black);
                magenta = (1.0 - green - black) / (1.0 - black);
                yellow = (1.0 - blue - black) / (1.0 - black);
            }
            //else do nothing

            return PdfColor(cyan, magenta, yellow, black);
        }
        case PdfColorSpaceType::DeviceCMYK:
        {
            return *this;
        }
        case PdfColorSpaceType::Separation:
        case PdfColorSpaceType::Lab:
        case PdfColorSpaceType::Indexed:
        case PdfColorSpaceType::Unknown:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
            break;
        }
        default:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
            break;
        }
    }
}

PdfArray PdfColor::ToArray() const
{
    PdfArray arr;

    switch (m_ColorSpace)
    {
        case PdfColorSpaceType::DeviceGray:
        {
            arr.Add(m_RawColor[0]);
            break;
        }
        case PdfColorSpaceType::DeviceRGB:
        {
            arr.Add(m_RawColor[0]);
            arr.Add(m_RawColor[1]);
            arr.Add(m_RawColor[2]);
            break;
        }
        case PdfColorSpaceType::DeviceCMYK:
        {
            arr.Add(m_RawColor[0]);
            arr.Add(m_RawColor[1]);
            arr.Add(m_RawColor[2]);
            arr.Add(m_RawColor[3]);
            break;
        }
        case PdfColorSpaceType::Lab:
        {
            arr.Add(m_RawColor[0]);
            arr.Add(m_RawColor[1]);
            arr.Add(m_RawColor[2]);
            break;
        }
        case PdfColorSpaceType::Separation:
        {
            arr.Add(m_RawColor[0]);
            break;
        }
        case PdfColorSpaceType::Indexed:
        case PdfColorSpaceType::Unknown:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
            break;
        }
        default:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
            break;
        }
    }

    return arr;
}

PdfColor PdfColor::CreateFromString(const string_view& name)
{
    if (name.length() == 0)
        return PdfColor();

    // first see if it's a single number - if so, that's a single gray value
    if (isdigit(name[0]) || name[0] == '.')
    {
        double grayVal = 0.0;
        if (!utls::TryParse(name.substr(1), grayVal))
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidNumber, "Could not read number");

        return PdfColor(grayVal);
    }
    // now check for a hex value (#xxxxxx or #xxxxxxxx)
    else if (name[0] == '#')
    {
        if (name.size() == 7) // RGB
        {
            unsigned char R_HI;
            unsigned char R_LO;
            unsigned char G_HI;
            unsigned char G_LO;
            unsigned char B_HI;
            unsigned char B_LO;

            if (!utls::TryGetHexValue(name[1], R_HI)
                || !utls::TryGetHexValue(name[2], R_LO)
                || !utls::TryGetHexValue(name[3], G_HI)
                || !utls::TryGetHexValue(name[4], G_LO)
                || !utls::TryGetHexValue(name[5], B_HI)
                || !utls::TryGetHexValue(name[6], B_LO))
            {
                PODOFO_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
            }

            const unsigned R = (R_HI << 4) | R_LO;
            const unsigned G = (G_HI << 4) | G_LO;
            const unsigned B = (B_HI << 4) | B_LO;

            return PdfColor(static_cast<double>(R) / 255.0,
                static_cast<double>(G) / 255.0,
                static_cast<double>(B) / 255.0);
        }
        else if (name.size() == 9) // CMYK
        {
            unsigned char C_HI;
            unsigned char C_LO;
            unsigned char M_HI;
            unsigned char M_LO;
            unsigned char Y_HI;
            unsigned char Y_LO;
            unsigned char K_HI;
            unsigned char K_LO;

            if (!utls::TryGetHexValue(name[1], C_HI)
                || !utls::TryGetHexValue(name[2], C_LO)
                || !utls::TryGetHexValue(name[3], M_HI)
                || !utls::TryGetHexValue(name[4], M_LO)
                || !utls::TryGetHexValue(name[5], Y_HI)
                || !utls::TryGetHexValue(name[6], Y_LO)
                || !utls::TryGetHexValue(name[7], K_HI)
                || !utls::TryGetHexValue(name[8], K_LO))
            {
                PODOFO_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
            }

            const unsigned C = (C_HI << 4) | C_LO;
            const unsigned M = (M_HI << 4) | M_LO;
            const unsigned Y = (Y_HI << 4) | Y_LO;
            const unsigned K = (K_HI << 4) | K_LO;

            return PdfColor(static_cast<double>(C) / 255.0,
                static_cast<double>(M) / 255.0,
                static_cast<double>(Y) / 255.0,
                static_cast<double>(K) / 255.0);
        }
        else
        {
            return PdfColor();
        }

    }
    // PdfArray
    else if (name[0] == '[')
    {
        SpanStreamDevice device(name);
        PdfTokenizer tokenizer;
        PdfVariant var;

        tokenizer.ReadNextVariant(device, var); // No encryption...
        const PdfArray* arr;
        if (!var.TryGetArray(arr))
            return PdfColor();

        PdfColor color;
        (void)tryCreateFromArray(*arr, color);
        return color;
    }
    // it must be a named RGB color
    else
    {
        pair<const PdfNamedColor*, const PdfNamedColor*> iterators =
            std::equal_range(&(s_NamedColors[0]),
                s_NamedColors + s_NamedColorsCount,
                PdfNamedColor(name.data(), PdfColor()), NamedColorComparatorPredicate());

        if (iterators.first != iterators.second)
            return iterators.first->GetColor();

        return PdfColor();
    }
}

PdfColor PdfColor::CreateFromObject(const PdfObject& obj)
{
    PdfColor ret;
    if (!TryCreateFromObject(obj, ret))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "PdfColor::CreateFromObject supports only GrayScale, RGB and CMYK colors");

    return ret;
}

bool PdfColor::TryCreateFromObject(const PdfObject& obj, PdfColor& color)
{
    const PdfArray* arr;
    if (!obj.TryGetArray(arr))
        return false;

    return tryCreateFromArray(*arr, color);
}

bool PdfColor::tryCreateFromArray(const PdfArray& arr, PdfColor& color)
{
    if (arr.GetSize() == 0) // Transparent
    {
        color = CreateTransparent();
    }
    else if (arr.GetSize() == 1) // grayscale
    {
        double gray;
        if (!arr[0].TryGetReal(gray))
            return false;

        color = PdfColor(gray);
        return true;
    }
    else if (arr.GetSize() == 3) // RGB or spot
    {
        double red;
        double green;
        double blue;
        if (!arr[0].TryGetReal(red) || !arr[1].TryGetReal(green) || !arr[2].TryGetReal(blue))
            return false;

        color = PdfColor(red, green, blue);
        return true;
    }
    else if (arr.GetSize() == 4) // CMYK
    {
        double cyan;
        double magenta;
        double yellow;
        double key;
        if (!arr[0].TryGetReal(cyan) || !arr[1].TryGetReal(magenta) || !arr[2].TryGetReal(yellow) || !arr[3].TryGetReal(key))
            return false;

        color = PdfColor(cyan, magenta, yellow, key);
        return true;
    }

    return false;
}

PdfColor PdfNamedColor::createFromRGBString(string_view name)
{
    // This method cannot use PdfTokenizer::GetHexValue() as static values used there have
    // not been initialised yet. This function should used only during program startup
    // and the only purpose is use at s_NamedColors table

    if (name.length() == 7
        && name[0] == '#'
        && isxdigit(name[1]))
    {
        unsigned nameConverted;
        if (!utls::TryParse(name.substr(1), nameConverted, 16))
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidNumber, "Could not read number");

        const unsigned R = (nameConverted & 0x00FF0000) >> 16;
        const unsigned G = (nameConverted & 0x0000FF00) >> 8;
        const unsigned B = (nameConverted & 0x000000FF);

        return PdfColor(R / 255.0, G / 255.0, B / 255.0);
    }
    else
    {
        PODOFO_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
    }
}

bool PdfColor::operator==(const PdfColor& rhs) const
{
    return m_IsTransparent == rhs.m_IsTransparent
        && m_ColorSpace == rhs.m_ColorSpace
        && m_RawColor == rhs.m_RawColor;
}

bool PdfColor::operator!=(const PdfColor& rhs) const
{
    return m_IsTransparent != rhs.m_IsTransparent
        || m_ColorSpace != rhs.m_ColorSpace
        || m_RawColor != rhs.m_RawColor;
}

bool PdfColor::IsGrayScale() const
{
    return m_ColorSpace == PdfColorSpaceType::DeviceGray;
}

bool PdfColor::IsRGB() const
{
    return m_ColorSpace == PdfColorSpaceType::DeviceRGB;
}

bool PdfColor::IsCMYK() const
{
    return m_ColorSpace == PdfColorSpaceType::DeviceCMYK;
}

bool PdfColor::IsTransparent() const
{
    return m_IsTransparent;
}

double PdfColor::GetGrayScale() const
{
    PODOFO_RAISE_LOGIC_IF(!this->IsGrayScale(),
        "PdfColor::GetGrayScale cannot be called on non grayscale color objects!");

    return m_RawColor[0];
}

double PdfColor::GetRed() const
{
    PODOFO_RAISE_LOGIC_IF(!this->IsRGB(),
        "PdfColor::GetRed cannot be called on non RGB color objects!");

    return m_RawColor[0];
}

double PdfColor::GetGreen() const
{
    PODOFO_RAISE_LOGIC_IF(!this->IsRGB(),
        "PdfColor::GetGreen cannot be called on non RGB color objects!");

    return m_RawColor[1];
}

double PdfColor::GetBlue() const
{
    PODOFO_RAISE_LOGIC_IF(!this->IsRGB(),
        "PdfColor::GetBlue cannot be called on non RGB color objects!");

    return m_RawColor[2];
}

double PdfColor::GetCyan() const
{
    PODOFO_RAISE_LOGIC_IF(!this->IsCMYK(),
        "PdfColor::GetCyan cannot be called on non CMYK color objects!");

    return m_RawColor[0];
}

double PdfColor::GetMagenta() const
{
    PODOFO_RAISE_LOGIC_IF(!this->IsCMYK(),
        "PdfColor::GetMagenta cannot be called on non CMYK color objects!");

    return m_RawColor[1];
}

double PdfColor::GetYellow() const
{
    PODOFO_RAISE_LOGIC_IF(!this->IsCMYK(),
        "PdfColor::GetYellow cannot be called on non CMYK color objects!");

    return m_RawColor[2];
}

double PdfColor::GetBlack() const
{
    PODOFO_RAISE_LOGIC_IF(!this->IsCMYK(),
        "PdfColor::GetBlack cannot be called on non CMYK color objects!");

    return m_RawColor[3];
}

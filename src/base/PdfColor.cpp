/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#include "PdfColor.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfLocale.h"
#include "PdfStream.h"
#include "PdfTokenizer.h"
#include "PdfVariant.h"
#include "PdfDefinesPrivate.h"

#include <algorithm>
#include <ctype.h>

namespace PoDoFo {

/** A PdfNamedColor holds
 *  a PdfColor object and a name.
 */
class PdfNamedColor {
public:
    /** Create a PdfNamedColor object.
     *
     *  \param pszName the name. The string must be allocated as static memory somewhere
     *         The string data will not be copied!
     *  \param rColor a PdfColor object
     */
    PdfNamedColor( const char* pszName, const PdfColor & rColor )
        : m_pszName( pszName ), m_color( rColor )
    {
    }

    /** Create a PdfNamedColor object.
     *
     *  \param pszName the name. The string must be allocated as static memory somewhere
     *         The string data will not be copied!
     *  \param rColorName RGB hex value (e.g. #FFABCD)
     */
    PdfNamedColor( const char* pszName, const char* rColorName )
        : m_pszName( pszName ), m_color( FromRGBString(rColorName) )
    {
    }

    /** Copy constructor
     */
    PdfNamedColor(const PdfNamedColor& rhs)
        : m_pszName( rhs.m_pszName ), m_color( rhs.m_color )
    {
    }

    /** Class destructor.
     */
    ~PdfNamedColor()
    {
    }

    /** Compare this color object to a name 
     *  The comparison is case insensitive!
     *  \returns true if the passed string is smaller than the name
     *           of this color object.
     */
    inline bool operator<( const char* pszName ) const
    {
        return pszName ? PoDoFo::compat::strcasecmp( m_pszName, pszName ) < 0 : true; 
	}

    /** Compare this color object to a PdfNamedColor comparing only the name.
     *  The comparison is case insensitive!
     *  \returns true if the passed string is smaller than the name
     *           of this color object.
     */
    inline bool operator<( const PdfNamedColor & rhs ) const
    {
        return rhs.GetName() ? PoDoFo::compat::strcasecmp( m_pszName, rhs.GetName() ) < 0 : true; 
    }


    /** Compare this color object to a name 
     *  The comparison is case insensitive!
     *  \returns true if the passed string is the name
     *           of this color object.
     */
    inline bool operator==( const char* pszName ) const
    {
        return pszName ? PoDoFo::compat::strcasecmp( m_pszName, pszName ) == 0 : false; 
    }

    /** 
     * \returns a reference to the internal color object
     */
    inline const PdfColor & GetColor() const 
    {
        return m_color;
    }
    
    /**
     * \returns a pointer to the name of the color
     */
    inline const char* GetName() const
    {
        return m_pszName;
    }

private:
    /** default constructor, not implemented
     */
    PdfNamedColor();

    /** copy assignment operator, not implemented
     */
    PdfNamedColor& operator=(const PdfNamedColor&);

    /** Creates a color object from a RGB string.
     *
     *  \param pszName a string describing a color.
     *
     *  Supported values are:
     *  - hex values (e.g. #FF002A (RGB))
     *
     *  \returns a PdfColor object
     */
    static PdfColor FromRGBString( const char* pszName )
    {
        //This method cannot use PdfTokenizer::GetHexValue() as static values used there have 
        //not been initialised yet. This function should used only during program startup
        //and the only purpose is use at s_NamedColors table.

        size_t lLen = strlen( pszName );
        if (
            (lLen == 7) &&
            (pszName[0] == '#') &&
            isxdigit(pszName[1])
            )
        {
            const unsigned long NAME_CONVERTED_TO_LONG_HEX = 
                static_cast<unsigned long>(strtol(&pszName[1], 0, 16));

            const unsigned long R = (NAME_CONVERTED_TO_LONG_HEX & 0x00FF0000) >> 16;
            const unsigned long G = (NAME_CONVERTED_TO_LONG_HEX & 0x0000FF00) >> 8;
            const unsigned long B = (NAME_CONVERTED_TO_LONG_HEX & 0x000000FF);
            
            return PdfColor( static_cast<double>(R)/255.0, 
                             static_cast<double>(G)/255.0, 
                             static_cast<double>(B)/255.0 );
        }
        else
        {
            PODOFO_RAISE_ERROR( ePdfError_CannotConvertColor );
        }
    }

    const char* m_pszName;
    PdfColor    m_color;
};

/**
 * Predicate to allow binary search in the list
 * of PdfNamedColor's using for example std::equal_range.
 */
class NamedColorComparatorPredicate { 
public:
    NamedColorComparatorPredicate()
    {
    }

    inline bool operator()( const PdfNamedColor & rNamedColor1, const PdfNamedColor & rNamedColor2 ) const { 
        return rNamedColor1 < rNamedColor2;
    }
};

// Table based on http://cvsweb.xfree86.org/cvsweb/xc/programs/rgb/rgb.txt?rev=1.2
// Hex values have been copied from http://en.wikipedia.org/wiki/X11_color_names (21/11/2010)
static const size_t s_nNumNamedColors = 148;
static const PdfNamedColor s_NamedColors[s_nNumNamedColors] = 
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

inline void CheckDoubleRange( double val, double min, double max )
{
    if( (val < min) || (val > max) )
    {
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }
}

PdfColor::PdfColor() :
    m_uColor(),
    m_separationName(),
    m_separationDensity(0.0),
    m_eColorSpace(ePdfColorSpace_Unknown),
    m_eAlternateColorSpace(ePdfColorSpace_Unknown)
{
    m_uColor.rgb[0] = 0.0;
    m_uColor.rgb[1] = 0.0;
    m_uColor.rgb[2] = 0.0;
}

PdfColor::PdfColor( double dGray ) :
    m_uColor(),
    m_separationName(),
    m_separationDensity(0.0),
    m_eColorSpace(ePdfColorSpace_DeviceGray),
    m_eAlternateColorSpace(ePdfColorSpace_Unknown)
{
    CheckDoubleRange( dGray, 0.0, 1.0 );

    m_uColor.gray = dGray;
}

PdfColor::PdfColor( double dRed, double dGreen, double dBlue ) :
    m_uColor(),
    m_separationName(),
    m_separationDensity(0.0),
    m_eColorSpace(ePdfColorSpace_DeviceRGB),
    m_eAlternateColorSpace(ePdfColorSpace_Unknown)
{
    CheckDoubleRange( dRed,   0.0, 1.0 );
    CheckDoubleRange( dGreen, 0.0, 1.0 );
    CheckDoubleRange( dBlue,  0.0, 1.0 );

    m_uColor.rgb[0] = dRed;
    m_uColor.rgb[1] = dGreen;
    m_uColor.rgb[2] = dBlue;
}

PdfColor::PdfColor( double dCyan, double dMagenta, double dYellow, double dBlack ) :
    m_uColor(),
    m_separationName(),
    m_separationDensity(0.0),
    m_eColorSpace( ePdfColorSpace_DeviceCMYK ),
    m_eAlternateColorSpace(ePdfColorSpace_Unknown)
{
    CheckDoubleRange( dCyan,    0.0, 1.0 );
    CheckDoubleRange( dMagenta, 0.0, 1.0 );
    CheckDoubleRange( dYellow,  0.0, 1.0 );
    CheckDoubleRange( dBlack,   0.0, 1.0 );

    m_uColor.cmyk[0] = dCyan;
    m_uColor.cmyk[1] = dMagenta;
    m_uColor.cmyk[2] = dYellow;
    m_uColor.cmyk[3] = dBlack;
}

PdfColor::PdfColor( const PdfColor & rhs ) :
    m_uColor(),
    m_separationName(rhs.m_separationName),
    m_separationDensity(rhs.m_separationDensity),
    m_eColorSpace(rhs.m_eColorSpace),
    m_eAlternateColorSpace(rhs.m_eAlternateColorSpace)
{
    memcpy( &m_uColor, &rhs.m_uColor, sizeof(m_uColor) );
}

PdfColor::~PdfColor()
{
}

PdfColorGray::PdfColorGray( double dGray ) :
    PdfColor( dGray )
{
}

PdfColorGray::~PdfColorGray()
{
}

PdfColorRGB::PdfColorRGB( double dRed, double dGreen, double dBlue )
    : PdfColor( dRed, dGreen, dBlue )
{
}

PdfColorRGB::~PdfColorRGB()
{
}

PdfColorCMYK::PdfColorCMYK( double dCyan, double dMagenta, double dYellow, double dBlack )
    : PdfColor( dCyan, dMagenta, dYellow, dBlack )
{
}

PdfColorCMYK::~PdfColorCMYK()
{
}

PdfColorCieLab::PdfColorCieLab( double dCieL, double dCieA, double dCieB )
  : PdfColor()
{
    CheckDoubleRange( dCieL,    0.0, 100.0 );
    CheckDoubleRange( dCieA, -128.0, 127.0 );
    CheckDoubleRange( dCieB, -128.0, 127.0 );

    m_eColorSpace = ePdfColorSpace_CieLab;
    m_uColor.lab[0] = dCieL;
    m_uColor.lab[1] = dCieA;
    m_uColor.lab[2] = dCieB;
}

PdfColorCieLab::~PdfColorCieLab()
{
}

PdfColorSeparationAll::PdfColorSeparationAll()
  : PdfColor()
{
    m_eColorSpace = ePdfColorSpace_Separation;
    m_separationName = "All";
    m_separationDensity = 1.0;
    m_eAlternateColorSpace = ePdfColorSpace_DeviceCMYK;
    m_uColor.cmyk[0] = 1.0;
    m_uColor.cmyk[1] = 1.0;
    m_uColor.cmyk[2] = 1.0;
    m_uColor.cmyk[3] = 1.0;
}

PdfColorSeparationAll::~PdfColorSeparationAll()
{
}

PdfColorSeparationNone::PdfColorSeparationNone()
  : PdfColor()
{
    m_eColorSpace = ePdfColorSpace_Separation;
    m_separationName = "None";
    m_separationDensity = 0.0;
    m_eAlternateColorSpace = ePdfColorSpace_DeviceCMYK;
    m_uColor.cmyk[0] = 0.0;
    m_uColor.cmyk[1] = 0.0;
    m_uColor.cmyk[2] = 0.0;
    m_uColor.cmyk[3] = 0.0;
}

PdfColorSeparationNone::~PdfColorSeparationNone()
{
}

PdfColorSeparation::PdfColorSeparation( const std::string & sName, double dDensity, const PdfColor & alternateColor )
  : PdfColor()
{
    m_eAlternateColorSpace = alternateColor.GetColorSpace();
    switch( m_eAlternateColorSpace )
    {
        case ePdfColorSpace_DeviceGray:
        {
            m_uColor.gray = alternateColor.GetGrayScale();
            break;
        }
        case ePdfColorSpace_DeviceRGB:
        {
            m_uColor.rgb[0] = alternateColor.GetRed();
            m_uColor.rgb[1] = alternateColor.GetGreen();
            m_uColor.rgb[2] = alternateColor.GetBlue();
            break;
        }
        case ePdfColorSpace_DeviceCMYK:
        {
            m_uColor.cmyk[0] = alternateColor.GetCyan();
            m_uColor.cmyk[1] = alternateColor.GetMagenta();
            m_uColor.cmyk[2] = alternateColor.GetYellow();
            m_uColor.cmyk[3] = alternateColor.GetBlack();
            break;
        }
        case ePdfColorSpace_CieLab:
        {
            m_uColor.lab[0] = alternateColor.GetCieL();
            m_uColor.lab[1] = alternateColor.GetCieA();
            m_uColor.lab[2] = alternateColor.GetCieB();
            break;
        }
        case ePdfColorSpace_Separation:
        {
            PODOFO_RAISE_LOGIC_IF( true, "PdfColor::PdfColorSeparation alternateColor must be Gray, RGB, CMYK or CieLab!");
            break;
        }
        case ePdfColorSpace_Unknown:
        case ePdfColorSpace_Indexed:
        default:
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidEnumValue );
            break;
        }
    }
    m_eColorSpace = ePdfColorSpace_Separation;
    m_separationName = sName;
    m_separationDensity = dDensity;
}

PdfColorSeparation::~PdfColorSeparation()
{
}

const PdfColor & PdfColor::operator=( const PdfColor & rhs )
{
    if (this != &rhs)
    {
        memcpy( &m_uColor, &rhs.m_uColor, sizeof(m_uColor) );
        m_separationName = rhs.m_separationName;
        m_separationDensity = rhs.m_separationDensity;
        m_eColorSpace = rhs.m_eColorSpace;
        m_eAlternateColorSpace = rhs.m_eAlternateColorSpace;
    }
    else
    {
        //do nothing
    }

    return *this;
}

PdfColor PdfColor::ConvertToGrayScale() const
{
    switch(m_eColorSpace)
    {
        case ePdfColorSpace_DeviceGray:
        {
            return *this;
        }
        case ePdfColorSpace_DeviceRGB:
        {
            return PdfColor( 0.299*m_uColor.rgb[0] + 0.587*m_uColor.rgb[1] + 0.114*m_uColor.rgb[2] );
        }
        case ePdfColorSpace_DeviceCMYK:
        {
            return ConvertToRGB().ConvertToGrayScale();
        }
        case ePdfColorSpace_Separation:
        {
            if ( m_eAlternateColorSpace == ePdfColorSpace_DeviceCMYK )
            {
                double dCyan    = m_uColor.cmyk[0];
                double dMagenta = m_uColor.cmyk[1];
                double dYellow  = m_uColor.cmyk[2];
                double dBlack   = m_uColor.cmyk[3];

                double dRed   = dCyan    * (1.0 - dBlack) + dBlack;
                double dGreen = dMagenta * (1.0 - dBlack) + dBlack;
                double dBlue  = dYellow  * (1.0 - dBlack) + dBlack;

                return PdfColor( 1.0 - dRed, 1.0 - dGreen, 1.0 - dBlue );
            }
            else
            {
                PODOFO_RAISE_ERROR( ePdfError_NotImplemented );
            }
            break;
        }
        case ePdfColorSpace_CieLab:
        case ePdfColorSpace_Indexed:
        case ePdfColorSpace_Unknown:
        {
            PODOFO_RAISE_ERROR( ePdfError_CannotConvertColor );
            break;
        }
        default:
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidEnumValue );
            break;
        }
    };

    return PdfColor();
}

PdfColor PdfColor::ConvertToRGB() const
{
    switch(m_eColorSpace)
    {
        case ePdfColorSpace_DeviceGray:
        {
            return PdfColor( m_uColor.gray, m_uColor.gray, m_uColor.gray );
        }
        case ePdfColorSpace_DeviceRGB:
        {
            return *this;
        }
        case ePdfColorSpace_DeviceCMYK:
        {
            double dCyan    = m_uColor.cmyk[0];
            double dMagenta = m_uColor.cmyk[1];
            double dYellow  = m_uColor.cmyk[2];
            double dBlack   = m_uColor.cmyk[3];

            double dRed   = dCyan    * (1.0 - dBlack) + dBlack;
            double dGreen = dMagenta * (1.0 - dBlack) + dBlack;
            double dBlue  = dYellow  * (1.0 - dBlack) + dBlack;

            return PdfColor( 1.0 - dRed, 1.0 - dGreen, 1.0 - dBlue );
        }
        case ePdfColorSpace_Separation:
        {
            if ( m_eAlternateColorSpace == ePdfColorSpace_DeviceCMYK )
            {
                double dCyan    = m_uColor.cmyk[0];
                double dMagenta = m_uColor.cmyk[1];
                double dYellow  = m_uColor.cmyk[2];
                double dBlack   = m_uColor.cmyk[3];

                double dRed   = dCyan    * (1.0 - dBlack) + dBlack;
                double dGreen = dMagenta * (1.0 - dBlack) + dBlack;
                double dBlue  = dYellow  * (1.0 - dBlack) + dBlack;

                return PdfColor( 1.0 - dRed, 1.0 - dGreen, 1.0 - dBlue );
            }
            else
            {
                PODOFO_RAISE_ERROR( ePdfError_NotImplemented );
            }

            break;
        }
        case ePdfColorSpace_CieLab:
        case ePdfColorSpace_Indexed:
        case ePdfColorSpace_Unknown:
        {
            PODOFO_RAISE_ERROR( ePdfError_CannotConvertColor );
            break;
        }
        default:
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidEnumValue );
            break;
        }
    };

    return PdfColor();
}

PdfColor PdfColor::ConvertToCMYK() const
{
    switch(m_eColorSpace)
    {
        case ePdfColorSpace_DeviceGray:
        {
            return ConvertToRGB().ConvertToCMYK();
        }
        case ePdfColorSpace_DeviceRGB:
        {
            double dRed   = m_uColor.rgb[0];
            double dGreen = m_uColor.rgb[1];
            double dBlue  = m_uColor.rgb[2];

            double dBlack   = PDF_MIN( 1.0-dRed, PDF_MIN( 1.0-dGreen, 1.0-dBlue ) );

            double dCyan = 0.0;
            double dMagenta = 0.0;
            double dYellow = 0.0;
            if (dBlack < 1.0)
            {
                dCyan    = (1.0 - dRed   - dBlack) / (1.0 - dBlack);
                dMagenta = (1.0 - dGreen - dBlack) / (1.0 - dBlack);
                dYellow  = (1.0 - dBlue  - dBlack) / (1.0 - dBlack);
            }
            //else do nothing

            return PdfColor( dCyan, dMagenta, dYellow, dBlack );
        }
        case ePdfColorSpace_DeviceCMYK:
        {
            return *this;
        }
        case ePdfColorSpace_Separation:
        case ePdfColorSpace_CieLab:
        case ePdfColorSpace_Indexed:
        case ePdfColorSpace_Unknown:
        {
            PODOFO_RAISE_ERROR( ePdfError_CannotConvertColor );
            break;
        }
        default:
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidEnumValue );
            break;
        }
    };
}

PdfArray PdfColor::ToArray() const
{
    PdfArray array;

    switch(m_eColorSpace)
    {
        case ePdfColorSpace_DeviceGray:
        {
            array.push_back( m_uColor.gray );
            break;
        }
        case ePdfColorSpace_DeviceRGB:
        {
            array.push_back( m_uColor.rgb[0] ); 
            array.push_back( m_uColor.rgb[1] ); 
            array.push_back( m_uColor.rgb[2] );
            break;
        }
        case ePdfColorSpace_DeviceCMYK:
        {
            array.push_back( m_uColor.cmyk[0] ); 
            array.push_back( m_uColor.cmyk[1] ); 
            array.push_back( m_uColor.cmyk[2] ); 
            array.push_back( m_uColor.cmyk[3] ); 
            break;
        }
        case ePdfColorSpace_CieLab:
        {
            array.push_back( m_uColor.lab[0] ); 
            array.push_back( m_uColor.lab[1] ); 
            array.push_back( m_uColor.lab[2] );
            break;
        }
        case ePdfColorSpace_Separation:
        {
            array.push_back( m_separationDensity );
            break;
        }
        case ePdfColorSpace_Indexed:
        case ePdfColorSpace_Unknown:
        {
            PODOFO_RAISE_ERROR( ePdfError_CannotConvertColor );
            break;
        }
        default:
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidEnumValue );
            break;
        }
    };

    return array;
}

PdfColor PdfColor::FromString( const char* pszName )
{
    if( pszName ) 
    {
        size_t lLen = strlen( pszName );

        // first see if it's a single number - if so, that's a single gray value
        if( isdigit( pszName[0] ) || (pszName[0] == '.') ) 
        {
            double dGrayVal = 0.0;

            std::istringstream stream( pszName );
            PdfLocaleImbue(stream);

            if( !(stream >> dGrayVal) ) 
            {
                PODOFO_RAISE_ERROR( ePdfError_CannotConvertColor );
            }            
            else
            {
                return PdfColor( dGrayVal );
            }
        }
        // now check for a hex value (#xxxxxx or #xxxxxxxx)
        else if( pszName[0] == '#' )
        {
            ++pszName;
            if( lLen == 7 ) // RGB
            {
                const unsigned int R_HI = static_cast<unsigned int>(PdfTokenizer::GetHexValue( *pszName++ ));
                const unsigned int R_LO = static_cast<unsigned int>(PdfTokenizer::GetHexValue( *pszName++ ));
                const unsigned int R = (R_HI << 4) | R_LO;
                
                const unsigned int G_HI = static_cast<unsigned int>(PdfTokenizer::GetHexValue( *pszName++ ));
                const unsigned int G_LO = static_cast<unsigned int>(PdfTokenizer::GetHexValue( *pszName++ ));
                const unsigned int G = (G_HI << 4) | G_LO;

                const unsigned int B_HI = static_cast<unsigned int>(PdfTokenizer::GetHexValue( *pszName++ ));
                const unsigned int B_LO = static_cast<unsigned int>(PdfTokenizer::GetHexValue( *pszName++ ));
                const unsigned int B = (B_HI << 4) | B_LO;

                if ( 
                    (R_HI != PdfTokenizer::HEX_NOT_FOUND) && 
                    (R_LO != PdfTokenizer::HEX_NOT_FOUND) && 
                    (G_HI != PdfTokenizer::HEX_NOT_FOUND) && 
                    (G_LO != PdfTokenizer::HEX_NOT_FOUND) && 
                    (B_HI != PdfTokenizer::HEX_NOT_FOUND) &&
                    (B_LO != PdfTokenizer::HEX_NOT_FOUND)
                    )
                {
                    return PdfColor( static_cast<double>(R)/255.0, 
                                     static_cast<double>(G)/255.0, 
                                     static_cast<double>(B)/255.0 );
                }
                else
                {
                    PODOFO_RAISE_ERROR( ePdfError_CannotConvertColor );
                }
            }
            else if( lLen == 9 ) // CMYK
            {
                const unsigned int C_HI = static_cast<unsigned int>(PdfTokenizer::GetHexValue( *pszName++ ));
                const unsigned int C_LO = static_cast<unsigned int>(PdfTokenizer::GetHexValue( *pszName++ ));
                const unsigned int C = (C_HI << 4) | C_LO;
                
                const unsigned int M_HI = static_cast<unsigned int>(PdfTokenizer::GetHexValue( *pszName++ ));
                const unsigned int M_LO = static_cast<unsigned int>(PdfTokenizer::GetHexValue( *pszName++ ));
                const unsigned int M = (M_HI << 4) | M_LO;

                const unsigned int Y_HI = static_cast<unsigned int>(PdfTokenizer::GetHexValue( *pszName++ ));
                const unsigned int Y_LO = static_cast<unsigned int>(PdfTokenizer::GetHexValue( *pszName++ ));
                const unsigned int Y = (Y_HI << 4) | Y_LO;

                const unsigned int K_HI = static_cast<unsigned int>(PdfTokenizer::GetHexValue( *pszName++ ));
                const unsigned int K_LO = static_cast<unsigned int>(PdfTokenizer::GetHexValue( *pszName++ ));
                const unsigned int K = (K_HI << 4) | K_LO;

                if ( 
                    (C_HI != PdfTokenizer::HEX_NOT_FOUND) && 
                    (C_LO != PdfTokenizer::HEX_NOT_FOUND) && 
                    (M_HI != PdfTokenizer::HEX_NOT_FOUND) && 
                    (M_LO != PdfTokenizer::HEX_NOT_FOUND) && 
                    (Y_HI != PdfTokenizer::HEX_NOT_FOUND) && 
                    (Y_LO != PdfTokenizer::HEX_NOT_FOUND) && 
                    (K_HI != PdfTokenizer::HEX_NOT_FOUND) &&
                    (K_LO != PdfTokenizer::HEX_NOT_FOUND)
                    )
                {
                    return PdfColor( static_cast<double>(C)/255.0, 
                                     static_cast<double>(M)/255.0, 
                                     static_cast<double>(Y)/255.0,
                                     static_cast<double>(K)/255.0 );
                }
                else
                {
                    PODOFO_RAISE_ERROR( ePdfError_CannotConvertColor );
                }
            }
            
        }
        // PdfArray 
        else if( pszName[0] == '[' ) 
        {
            PdfTokenizer tokenizer( pszName, lLen );
            PdfVariant   var;

            tokenizer.GetNextVariant( var, NULL ); // No encryption...
            if( var.IsArray() )
                return PdfColor::FromArray( var.GetArray() );
        }
        // it must be a named RGB color
        else
        {
            std::pair<const PdfNamedColor*, const PdfNamedColor*> iterators = 
                std::equal_range( &(s_NamedColors[0]), 
                                  s_NamedColors + s_nNumNamedColors, 
                                  PdfNamedColor( pszName, PdfColor() ), NamedColorComparatorPredicate() );
            
            if( iterators.first != iterators.second )
            {
                return (*(iterators.first)).GetColor();
            }

        }
    }

    return PdfColor();
}

PdfColor PdfColor::FromArray( const PdfArray & rArray )
{
    if( rArray.GetSize() == 1 ) // grayscale
        return PdfColor( rArray[0].GetReal() );
    else if( rArray.GetSize() == 3 ) // RGB or spot
        return PdfColor( rArray[0].GetReal(), rArray[1].GetReal(), rArray[2].GetReal() );
    else if( rArray.GetSize() == 4 ) // CMYK
        return PdfColor( rArray[0].GetReal(), rArray[1].GetReal(), rArray[2].GetReal(), rArray[3].GetReal() );

    PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, "PdfColor::FromArray supports only GrayScale, RGB and CMYK colors." );
}

PdfObject* PdfColor::BuildColorSpace( PdfVecObjects* pOwner ) const
{
    switch( m_eColorSpace )
    {
        case ePdfColorSpace_Separation:
        {
            // Build color-spaces for separation
            PdfObject* csTintFunc = pOwner->CreateObject();

            csTintFunc->GetDictionary().AddKey( "BitsPerSample", static_cast<pdf_int64>(8) );

            PdfArray decode;
            decode.push_back( static_cast<pdf_int64>(0) );
            decode.push_back( static_cast<pdf_int64>(1) );
            decode.push_back( static_cast<pdf_int64>(0) );
            decode.push_back( static_cast<pdf_int64>(1) );
            decode.push_back( static_cast<pdf_int64>(0) );
            decode.push_back( static_cast<pdf_int64>(1) );
            decode.push_back( static_cast<pdf_int64>(0) );
            decode.push_back( static_cast<pdf_int64>(1) );
            csTintFunc->GetDictionary().AddKey( "Decode", decode );

            PdfArray domain;
            domain.push_back( static_cast<pdf_int64>(0) );
            domain.push_back( static_cast<pdf_int64>(1) );
            csTintFunc->GetDictionary().AddKey( "Domain", domain );

            PdfArray encode;
            encode.push_back( static_cast<pdf_int64>(0) );
            encode.push_back( static_cast<pdf_int64>(1) );
            csTintFunc->GetDictionary().AddKey( "Encode", encode );

            csTintFunc->GetDictionary().AddKey( "Filter", PdfName( "FlateDecode" ) );
            csTintFunc->GetDictionary().AddKey( "FunctionType", PdfVariant( static_cast<pdf_int64>(0L) ) );
            //csTintFunc->GetDictionary().AddKey( "FunctionType", 
            //                                    PdfVariant( static_cast<pdf_int64>(ePdfFunctionType_Sampled) ) );

            switch ( m_eAlternateColorSpace )
            {
                case ePdfColorSpace_DeviceGray:
                {
                    char data[1*2];
                    data[0] = 0;
                    data[1] = static_cast<char> (m_uColor.gray);

                    PdfArray range;
                    range.push_back( static_cast<pdf_int64>(0) );
                    range.push_back( static_cast<pdf_int64>(1) );
                    csTintFunc->GetDictionary().AddKey( "Range", range );

                    PdfArray size;
                    size.push_back( static_cast<pdf_int64>(2) );
                    csTintFunc->GetDictionary().AddKey( "Size", size );

                    PdfMemoryInputStream stream( data, 1*2 );
                    csTintFunc->GetStream()->Set( &stream );

                    PdfArray csArr;
                    csArr.push_back( PdfName("Separation") );
                    csArr.push_back( PdfName( m_separationName ) );
                    csArr.push_back( PdfName("DeviceGray") );
                    csArr.push_back( csTintFunc->Reference() );

                    PdfObject* csp = pOwner->CreateObject( csArr );

                    return csp;
                }
                break;

                case ePdfColorSpace_DeviceRGB:
                {
                    char data[3*2];
                    data[0] =
                    data[1] =
                    data[2] = 0;
                    data[3] = static_cast<char> (m_uColor.rgb[0] * 255);
                    data[4] = static_cast<char> (m_uColor.rgb[1] * 255);
                    data[5] = static_cast<char> (m_uColor.rgb[2] * 255);

                    PdfArray range;
                    range.push_back( static_cast<pdf_int64>(0) );
                    range.push_back( static_cast<pdf_int64>(1) );
                    range.push_back( static_cast<pdf_int64>(0) );
                    range.push_back( static_cast<pdf_int64>(1) );
                    range.push_back( static_cast<pdf_int64>(0) );
                    range.push_back( static_cast<pdf_int64>(1) );
                    csTintFunc->GetDictionary().AddKey( "Range", range );

                    PdfArray size;
                    size.push_back( static_cast<pdf_int64>(2) );
                    csTintFunc->GetDictionary().AddKey( "Size", size );

                    PdfMemoryInputStream stream( data, 3*2 );
                    csTintFunc->GetStream()->Set( &stream );

                    PdfArray csArr;
                    csArr.push_back( PdfName("Separation") );
                    csArr.push_back( PdfName( m_separationName ) );
                    csArr.push_back( PdfName("DeviceRGB") );
                    csArr.push_back( csTintFunc->Reference() );

                    PdfObject* csp = pOwner->CreateObject( csArr );

                    return csp;
                }
                break;

                case ePdfColorSpace_DeviceCMYK:
                {
                    char data[4*2];
                    data[0] =
                    data[1] =
                    data[2] = 
                    data[3] = 0;
                    data[4] = static_cast<char> (m_uColor.cmyk[0] * 255);
                    data[5] = static_cast<char> (m_uColor.cmyk[1] * 255);
                    data[6] = static_cast<char> (m_uColor.cmyk[2] * 255);
                    data[7] = static_cast<char> (m_uColor.cmyk[3] * 255);

                    PdfArray range;
                    range.push_back( static_cast<pdf_int64>(0) );
                    range.push_back( static_cast<pdf_int64>(1) );
                    range.push_back( static_cast<pdf_int64>(0) );
                    range.push_back( static_cast<pdf_int64>(1) );
                    range.push_back( static_cast<pdf_int64>(0) );
                    range.push_back( static_cast<pdf_int64>(1) );
                    range.push_back( static_cast<pdf_int64>(0) );
                    range.push_back( static_cast<pdf_int64>(1) );
                    csTintFunc->GetDictionary().AddKey( "Range", range );

                    PdfArray size;
                    size.push_back( static_cast<pdf_int64>(2) );
                    csTintFunc->GetDictionary().AddKey( "Size", size );

                    PdfArray csArr;
                    csArr.push_back( PdfName("Separation") );
                    csArr.push_back( PdfName( m_separationName ) );
                    csArr.push_back( PdfName("DeviceCMYK") );
                    csArr.push_back( csTintFunc->Reference() );

                    PdfMemoryInputStream stream( data, 4*2 );
                    csTintFunc->GetStream()->Set( &stream ); // set stream as last, so that it will work with PdfStreamedDocument

                    PdfObject* csp = pOwner->CreateObject( csArr );

                    return csp;
                }
                break;

                case ePdfColorSpace_CieLab:
                {
                    char data[3*2];
                    data[0] =
                    data[1] =
                    data[2] = 0;
                    data[3] = static_cast<char> (m_uColor.lab[0] * 255);
                    data[4] = static_cast<char> (m_uColor.lab[1] * 255);
                    data[5] = static_cast<char> (m_uColor.lab[2] * 255);

                    PdfArray range;
                    range.push_back( static_cast<pdf_int64>(0) );
                    range.push_back( static_cast<pdf_int64>(1) );
                    range.push_back( static_cast<pdf_int64>(0) );
                    range.push_back( static_cast<pdf_int64>(1) );
                    range.push_back( static_cast<pdf_int64>(0) );
                    range.push_back( static_cast<pdf_int64>(1) );
                    csTintFunc->GetDictionary().AddKey( "Range", range );

                    PdfArray size;
                    size.push_back( static_cast<pdf_int64>(2) );
                    csTintFunc->GetDictionary().AddKey( "Size", size );

                    PdfMemoryInputStream stream( data, 3*2 );
                    csTintFunc->GetStream()->Set( &stream );

                    PdfArray csArr;
                    csArr.push_back( PdfName("Separation") );
                    csArr.push_back( PdfName( m_separationName ) );
                    csArr.push_back( PdfName("Lab") );
                    csArr.push_back( csTintFunc->Reference() );

                    PdfObject* csp = pOwner->CreateObject( csArr );

                    return csp;
                }
                break;

                case ePdfColorSpace_Separation:
                case ePdfColorSpace_Indexed:
                {
                    break;
                }

                case ePdfColorSpace_Unknown:
                default:
                {
                    PODOFO_RAISE_ERROR( ePdfError_InvalidEnumValue );
                    break;
                }
            }
        }
        break;

        case ePdfColorSpace_CieLab:
        {
            // Build color-spaces for CIE-lab
            PdfDictionary labDict;

            // D65-whitepoint
            PdfArray wpArr;
            wpArr.push_back( 0.9505 );
            wpArr.push_back( 1.0000 );
            wpArr.push_back( 1.0890 );
            labDict.AddKey( PdfName("WhitePoint" ), wpArr );

            // Range for A,B, L is implicit 0..100
            PdfArray rangeArr;
            rangeArr.push_back( static_cast<pdf_int64>(-128) );
            rangeArr.push_back( static_cast<pdf_int64>(127) );
            rangeArr.push_back( static_cast<pdf_int64>(-128) );
            rangeArr.push_back( static_cast<pdf_int64>(127) );
            labDict.AddKey( PdfName("Range" ), rangeArr );

            PdfArray labArr;
            labArr.push_back( PdfName("Lab") );
            labArr.push_back( labDict );

            PdfObject* labp = pOwner->CreateObject( labArr );

            return labp;
        }
        break;

        case ePdfColorSpace_DeviceGray:
        case ePdfColorSpace_DeviceRGB:
        case ePdfColorSpace_DeviceCMYK:
        case ePdfColorSpace_Indexed:
        {
            break;
        }

        case ePdfColorSpace_Unknown:
        default:
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidEnumValue );
            break;
        }
    }

    return NULL;
}

EPdfColorSpace PdfColor::GetColorSpaceForName( const PdfName & rName )
{
    EPdfColorSpace ePdfColorSpace = ePdfColorSpace_Unknown;

    if( PdfName("DeviceGray") == rName ) 
    {
        ePdfColorSpace = ePdfColorSpace_DeviceGray;
    }
    else if( PdfName("DeviceRGB") == rName ) 
    {
        ePdfColorSpace = ePdfColorSpace_DeviceRGB;
    }
    else if( PdfName("DeviceCMYK") == rName ) 
    {
        ePdfColorSpace = ePdfColorSpace_DeviceCMYK;
    }
    else if( PdfName("Indexed") == rName ) 
    {
        ePdfColorSpace = ePdfColorSpace_Indexed;
    }
    else
    {
        // TODO: other are not supported at the moment
        PdfError::LogMessage( eLogSeverity_Information, "Unsupported colorspace name: %s", rName.GetName().c_str() );
    }

    return ePdfColorSpace;
}
    
PdfName PdfColor::GetNameForColorSpace( EPdfColorSpace eColorSpace )
{
    switch( eColorSpace )
    {
        case ePdfColorSpace_DeviceGray:
            return PdfName("DeviceGray");
        case ePdfColorSpace_DeviceRGB:
            return PdfName("DeviceRGB");
        case ePdfColorSpace_DeviceCMYK:
            return PdfName("DeviceCMYK");
        case ePdfColorSpace_Separation:
            return PdfName("Separation");
        case ePdfColorSpace_CieLab:
            return PdfName("Lab");
        case ePdfColorSpace_Indexed:
            return PdfName("Indexed");
        case ePdfColorSpace_Unknown:
        default:
            PdfError::LogMessage( eLogSeverity_Information, "Unsupported colorspace enum: %i", eColorSpace );
            return PdfName();
    }
    
}

};


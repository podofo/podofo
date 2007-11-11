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
 ***************************************************************************/

#include "PdfColor.h"

#include "PdfArray.h"
#include "PdfTokenizer.h"
#include "PdfVariant.h"

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

    /** Compare this color object to a name 
     *  The comparison is case insensitive!
     *  \returns true if the passed string is smaller than the name
     *           of this color object.
     */
    inline bool operator<( const char* pszName ) const
    {
#ifdef _WIN32
        return pszName ? _stricmp( m_pszName, pszName ) < 0 : true; 
#else
        return pszName ? strcasecmp( m_pszName, pszName ) < 0 : true; 
#endif // _WIN32
    }

    /** Compare this color object to a name 
     *  The comparison is case insensitive!
     *  \returns true if the passed string is the name
     *           of this color object.
     */
    inline bool operator==( const char* pszName ) const
    {
#ifdef _WIN32
        return pszName ? _stricmp( m_pszName, pszName ) == 0 : false; 
#else
        return pszName ? strcasecmp( m_pszName, pszName ) == 0 : false; 
#endif // _WIN32
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
    const char* m_pszName;
    PdfColor    m_color;
};

const int nNumNamedColors = 147;
static PdfNamedColor s_NamedColors[nNumNamedColors] = 
{
    PdfNamedColor( "aliceblue", PdfColor(0.941, 0.973, 1.000, 1.000) ) ,
    PdfNamedColor( "antiquewhite", PdfColor(0.980, 0.922, 0.843, 1.000) ) ,
    PdfNamedColor( "aqua", PdfColor(0.000, 1.000, 1.000, 1.000) ) ,
    PdfNamedColor( "aquamarine", PdfColor(0.498, 1.000, 0.831, 1.000) ) ,
    PdfNamedColor( "azure", PdfColor(0.941, 1.000, 1.000, 1.000) ) ,
    PdfNamedColor( "beige", PdfColor(0.961, 0.961, 0.863, 1.000) ) ,
    PdfNamedColor( "bisque", PdfColor(1.000, 0.894, 0.769, 1.000) ) ,
    PdfNamedColor( "black", PdfColor(0.000, 0.000, 0.000, 1.000) ) ,
    PdfNamedColor( "blanchedalmond", PdfColor(1.000, 0.922, 0.804, 1.000) ) ,
    PdfNamedColor( "blue", PdfColor(0.000, 0.000, 1.000, 1.000) ) ,
    PdfNamedColor( "blueviolet", PdfColor(0.541, 0.169, 0.886, 1.000) ) ,
    PdfNamedColor( "brown", PdfColor(0.647, 0.165, 0.165, 1.000) ) ,
    PdfNamedColor( "burlywood", PdfColor(0.871, 0.722, 0.529, 1.000) ) ,
    PdfNamedColor( "cadetblue", PdfColor(0.373, 0.620, 0.627, 1.000) ) ,
    PdfNamedColor( "chartreuse", PdfColor(0.498, 1.000, 0.000, 1.000) ) ,
    PdfNamedColor( "chocolate", PdfColor(0.824, 0.412, 0.118, 1.000) ) ,
    PdfNamedColor( "coral", PdfColor(1.000, 0.498, 0.314, 1.000) ) ,
    PdfNamedColor( "cornflowerblue", PdfColor(0.392, 0.584, 0.929, 1.000) ) ,
    PdfNamedColor( "cornsilk", PdfColor(1.000, 0.973, 0.863, 1.000) ) ,
    PdfNamedColor( "crimson", PdfColor(0.863, 0.078, 0.235, 1.000) ) ,
    PdfNamedColor( "cyan", PdfColor(0.000, 1.000, 1.000, 1.000) ) ,
    PdfNamedColor( "darkblue", PdfColor(0.000, 0.000, 0.545, 1.000) ) ,
    PdfNamedColor( "darkcyan", PdfColor(0.000, 0.545, 0.545, 1.000) ) ,
    PdfNamedColor( "darkgoldenrod", PdfColor(0.722, 0.525, 0.043, 1.000) ) ,
    PdfNamedColor( "darkgray", PdfColor(0.663, 0.663, 0.663, 1.000) ) ,
    PdfNamedColor( "darkgreen", PdfColor(0.000, 0.392, 0.000, 1.000) ) ,
    PdfNamedColor( "darkgrey", PdfColor(0.663, 0.663, 0.663, 1.000) ) ,
    PdfNamedColor( "darkkhaki", PdfColor(0.741, 0.718, 0.420, 1.000) ) ,
    PdfNamedColor( "darkmagenta", PdfColor(0.545, 0.000, 0.545, 1.000) ) ,
    PdfNamedColor( "darkolivegreen", PdfColor(0.333, 0.420, 0.184, 1.000) ) ,
    PdfNamedColor( "darkorange", PdfColor(1.000, 0.549, 0.000, 1.000) ) ,
    PdfNamedColor( "darkorchid", PdfColor(0.600, 0.196, 0.800, 1.000) ) ,
    PdfNamedColor( "darkred", PdfColor(0.545, 0.000, 0.000, 1.000) ) ,
    PdfNamedColor( "darksalmon", PdfColor(0.914, 0.588, 0.478, 1.000) ) ,
    PdfNamedColor( "darkseagreen", PdfColor(0.561, 0.737, 0.561, 1.000) ) ,
    PdfNamedColor( "darkslateblue", PdfColor(0.282, 0.239, 0.545, 1.000) ) ,
    PdfNamedColor( "darkslategray", PdfColor(0.184, 0.310, 0.310, 1.000) ) ,
    PdfNamedColor( "darkslategrey", PdfColor(0.184, 0.310, 0.310, 1.000) ) ,
    PdfNamedColor( "darkturquoise", PdfColor(0.000, 0.808, 0.820, 1.000) ) ,
    PdfNamedColor( "darkviolet", PdfColor(0.580, 0.000, 0.827, 1.000) ) ,
    PdfNamedColor( "deeppink", PdfColor(1.000, 0.078, 0.576, 1.000) ) ,
    PdfNamedColor( "deepskyblue", PdfColor(0.000, 0.749, 1.000, 1.000) ) ,
    PdfNamedColor( "dimgray", PdfColor(0.412, 0.412, 0.412, 1.000) ) ,
    PdfNamedColor( "dimgrey", PdfColor(0.412, 0.412, 0.412, 1.000) ) ,
    PdfNamedColor( "dodgerblue", PdfColor(0.118, 0.565, 1.000, 1.000) ) ,
    PdfNamedColor( "firebrick", PdfColor(0.698, 0.133, 0.133, 1.000) ) ,
    PdfNamedColor( "floralwhite", PdfColor(1.000, 0.980, 0.941, 1.000) ) ,
    PdfNamedColor( "forestgreen", PdfColor(0.133, 0.545, 0.133, 1.000) ) ,
    PdfNamedColor( "fuchsia", PdfColor(1.000, 0.000, 1.000, 1.000) ) ,
    PdfNamedColor( "gainsboro", PdfColor(0.863, 0.863, 0.863, 1.000) ) ,
    PdfNamedColor( "ghostwhite", PdfColor(0.973, 0.973, 1.000, 1.000) ) ,
    PdfNamedColor( "gold", PdfColor(1.000, 0.843, 0.000, 1.000) ) ,
    PdfNamedColor( "goldenrod", PdfColor(0.855, 0.647, 0.125, 1.000) ) ,
    PdfNamedColor( "gray", PdfColor(0.502, 0.502, 0.502, 1.000) ) ,
    PdfNamedColor( "green", PdfColor(0.000, 0.502, 0.000, 1.000) ) ,
    PdfNamedColor( "greenyellow", PdfColor(0.678, 1.000, 0.184, 1.000) ) ,
    PdfNamedColor( "grey", PdfColor(0.502, 0.502, 0.502, 1.000) ) ,
    PdfNamedColor( "honeydew", PdfColor(0.941, 1.000, 0.941, 1.000) ) ,
    PdfNamedColor( "hotpink", PdfColor(1.000, 0.412, 0.706, 1.000) ) ,
    PdfNamedColor( "indianred", PdfColor(0.804, 0.361, 0.361, 1.000) ) ,
    PdfNamedColor( "indigo", PdfColor(0.294, 0.000, 0.510, 1.000) ) ,
    PdfNamedColor( "ivory", PdfColor(1.000, 1.000, 0.941, 1.000) ) ,
    PdfNamedColor( "khaki", PdfColor(0.941, 0.902, 0.549, 1.000) ) ,
    PdfNamedColor( "lavender", PdfColor(0.902, 0.902, 0.980, 1.000) ) ,
    PdfNamedColor( "lavenderblush", PdfColor(1.000, 0.941, 0.961, 1.000) ) ,
    PdfNamedColor( "lawngreen", PdfColor(0.486, 0.988, 0.000, 1.000) ) ,
    PdfNamedColor( "lemonchiffon", PdfColor(1.000, 0.980, 0.804, 1.000) ) ,
    PdfNamedColor( "lightblue", PdfColor(0.678, 0.847, 0.902, 1.000) ) ,
    PdfNamedColor( "lightcoral", PdfColor(0.941, 0.502, 0.502, 1.000) ) ,
    PdfNamedColor( "lightcyan", PdfColor(0.878, 1.000, 1.000, 1.000) ) ,
    PdfNamedColor( "lightgoldenrodyellow", PdfColor(0.980, 0.980, 0.824, 1.000) ) ,
    PdfNamedColor( "lightgray", PdfColor(0.827, 0.827, 0.827, 1.000) ) ,
    PdfNamedColor( "lightgreen", PdfColor(0.565, 0.933, 0.565, 1.000) ) ,
    PdfNamedColor( "lightgrey", PdfColor(0.827, 0.827, 0.827, 1.000) ) ,
    PdfNamedColor( "lightpink", PdfColor(1.000, 0.714, 0.757, 1.000) ) ,
    PdfNamedColor( "lightsalmon", PdfColor(1.000, 0.627, 0.478, 1.000) ) ,
    PdfNamedColor( "lightseagreen", PdfColor(0.125, 0.698, 0.667, 1.000) ) ,
    PdfNamedColor( "lightskyblue", PdfColor(0.529, 0.808, 0.980, 1.000) ) ,
    PdfNamedColor( "lightslategray", PdfColor(0.467, 0.533, 0.600, 1.000) ) ,
    PdfNamedColor( "lightslategrey", PdfColor(0.467, 0.533, 0.600, 1.000) ) ,
    PdfNamedColor( "lightsteelblue", PdfColor(0.690, 0.769, 0.871, 1.000) ) ,
    PdfNamedColor( "lightyellow", PdfColor(1.000, 1.000, 0.878, 1.000) ) ,
    PdfNamedColor( "lime", PdfColor(0.000, 1.000, 0.000, 1.000) ) ,
    PdfNamedColor( "limegreen", PdfColor(0.196, 0.804, 0.196, 1.000) ) ,
    PdfNamedColor( "linen", PdfColor(0.980, 0.941, 0.902, 1.000) ) ,
    PdfNamedColor( "magenta", PdfColor(1.000, 0.000, 1.000, 1.000) ) ,
    PdfNamedColor( "maroon", PdfColor(0.502, 0.000, 0.000, 1.000) ) ,
    PdfNamedColor( "mediumaquamarine", PdfColor(0.400, 0.804, 0.667, 1.000) ) ,
    PdfNamedColor( "mediumblue", PdfColor(0.000, 0.000, 0.804, 1.000) ) ,
    PdfNamedColor( "mediumorchid", PdfColor(0.729, 0.333, 0.827, 1.000) ) ,
    PdfNamedColor( "mediumpurple", PdfColor(0.576, 0.439, 0.859, 1.000) ) ,
    PdfNamedColor( "mediumseagreen", PdfColor(0.235, 0.702, 0.443, 1.000) ) ,
    PdfNamedColor( "mediumslateblue", PdfColor(0.482, 0.408, 0.933, 1.000) ) ,
    PdfNamedColor( "mediumspringgreen", PdfColor(0.000, 0.980, 0.604, 1.000) ) ,
    PdfNamedColor( "mediumturquoise", PdfColor(0.282, 0.820, 0.800, 1.000) ) ,
    PdfNamedColor( "mediumvioletred", PdfColor(0.780, 0.082, 0.522, 1.000) ) ,
    PdfNamedColor( "midnightblue", PdfColor(0.098, 0.098, 0.439, 1.000) ) ,
    PdfNamedColor( "mintcream", PdfColor(0.961, 1.000, 0.980, 1.000) ) ,
    PdfNamedColor( "mistyrose", PdfColor(1.000, 0.894, 0.882, 1.000) ) ,
    PdfNamedColor( "moccasin", PdfColor(1.000, 0.894, 0.710, 1.000) ) ,
    PdfNamedColor( "navajowhite", PdfColor(1.000, 0.871, 0.678, 1.000) ) ,
    PdfNamedColor( "navy", PdfColor(0.000, 0.000, 0.502, 1.000) ) ,
    PdfNamedColor( "oldlace", PdfColor(0.992, 0.961, 0.902, 1.000) ) ,
    PdfNamedColor( "olive", PdfColor(0.502, 0.502, 0.000, 1.000) ) ,
    PdfNamedColor( "olivedrab", PdfColor(0.420, 0.557, 0.137, 1.000) ) ,
    PdfNamedColor( "orange", PdfColor(1.000, 0.647, 0.000, 1.000) ) ,
    PdfNamedColor( "orangered", PdfColor(1.000, 0.271, 0.000, 1.000) ) ,
    PdfNamedColor( "orchid", PdfColor(0.855, 0.439, 0.839, 1.000) ) ,
    PdfNamedColor( "palegoldenrod", PdfColor(0.933, 0.910, 0.667, 1.000) ) ,
    PdfNamedColor( "palegreen", PdfColor(0.596, 0.984, 0.596, 1.000) ) ,
    PdfNamedColor( "paleturquoise", PdfColor(0.686, 0.933, 0.933, 1.000) ) ,
    PdfNamedColor( "palevioletred", PdfColor(0.859, 0.439, 0.576, 1.000) ) ,
    PdfNamedColor( "papayawhip", PdfColor(1.000, 0.937, 0.835, 1.000) ) ,
    PdfNamedColor( "peachpuff", PdfColor(1.000, 0.855, 0.725, 1.000) ) ,
    PdfNamedColor( "peru", PdfColor(0.804, 0.522, 0.247, 1.000) ) ,
    PdfNamedColor( "pink", PdfColor(1.000, 0.753, 0.796, 1.000) ) ,
    PdfNamedColor( "plum", PdfColor(0.867, 0.627, 0.867, 1.000) ) ,
    PdfNamedColor( "powderblue", PdfColor(0.690, 0.878, 0.902, 1.000) ) ,
    PdfNamedColor( "purple", PdfColor(0.502, 0.000, 0.502, 1.000) ) ,
    PdfNamedColor( "red", PdfColor(1.000, 0.000, 0.000, 1.000) ) ,
    PdfNamedColor( "rosybrown", PdfColor(0.737, 0.561, 0.561, 1.000) ) ,
    PdfNamedColor( "royalblue", PdfColor(0.255, 0.412, 0.882, 1.000) ) ,
    PdfNamedColor( "saddlebrown", PdfColor(0.545, 0.271, 0.075, 1.000) ) ,
    PdfNamedColor( "salmon", PdfColor(0.980, 0.502, 0.447, 1.000) ) ,
    PdfNamedColor( "sandybrown", PdfColor(0.957, 0.643, 0.376, 1.000) ) ,
    PdfNamedColor( "seagreen", PdfColor(0.180, 0.545, 0.341, 1.000) ) ,
    PdfNamedColor( "seashell", PdfColor(1.000, 0.961, 0.933, 1.000) ) ,
    PdfNamedColor( "sienna", PdfColor(0.627, 0.322, 0.176, 1.000) ) ,
    PdfNamedColor( "silver", PdfColor(0.753, 0.753, 0.753, 1.000) ) ,
    PdfNamedColor( "skyblue", PdfColor(0.529, 0.808, 0.922, 1.000) ) ,
    PdfNamedColor( "slateblue", PdfColor(0.416, 0.353, 0.804, 1.000) ) ,
    PdfNamedColor( "slategray", PdfColor(0.439, 0.502, 0.565, 1.000) ) ,
    PdfNamedColor( "slategrey", PdfColor(0.439, 0.502, 0.565, 1.000) ) ,
    PdfNamedColor( "snow", PdfColor(1.000, 0.980, 0.980, 1.000) ) ,
    PdfNamedColor( "springgreen", PdfColor(0.000, 1.000, 0.498, 1.000) ) ,
    PdfNamedColor( "steelblue", PdfColor(0.275, 0.510, 0.706, 1.000) ) ,
    PdfNamedColor( "tan", PdfColor(0.824, 0.706, 0.549, 1.000) ) ,
    PdfNamedColor( "teal", PdfColor(0.000, 0.502, 0.502, 1.000) ) ,
    PdfNamedColor( "thistle", PdfColor(0.847, 0.749, 0.847, 1.000) ) ,
    PdfNamedColor( "tomato", PdfColor(1.000, 0.388, 0.278, 1.000) ) ,
    PdfNamedColor( "turquoise", PdfColor(0.251, 0.878, 0.816, 1.000) ) ,
    PdfNamedColor( "violet", PdfColor(0.933, 0.510, 0.933, 1.000) ) ,
    PdfNamedColor( "wheat", PdfColor(0.961, 0.871, 0.702, 1.000) ) ,
    PdfNamedColor( "white", PdfColor(1.000, 1.000, 1.000, 1.000) ) ,
    PdfNamedColor( "whitesmoke", PdfColor(0.961, 0.961, 0.961, 1.000) ) ,
    PdfNamedColor( "yellow", PdfColor(1.000, 1.000, 0.000, 1.000) ) ,
    PdfNamedColor( "yellowgreen", PdfColor(0.604, 0.804, 0.196, 1.000) ) 
};



static inline void CheckDoubleRange( double val, double min, double max )
{
    if( val < min || val > max )
    {
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }
}

PdfColor::PdfColor()
    : m_eColorSpace( ePdfColorSpace_DeviceRGB )
{
    m_uColor.rgb[0] = 0.0;
    m_uColor.rgb[1] = 0.0;
    m_uColor.rgb[2] = 0.0;
}

PdfColor::PdfColor( double dGray )
    : m_eColorSpace( ePdfColorSpace_DeviceGray )
{
    CheckDoubleRange( dGray, 0.0, 1.0 );

    m_uColor.gray = dGray;
}

PdfColor::PdfColor( double dRed, double dGreen, double dBlue )
    : m_eColorSpace( ePdfColorSpace_DeviceRGB )
{
    CheckDoubleRange( dRed,   0.0, 1.0 );
    CheckDoubleRange( dGreen, 0.0, 1.0 );
    CheckDoubleRange( dBlue,  0.0, 1.0 );

    m_uColor.rgb[0] = dRed;
    m_uColor.rgb[1] = dGreen;
    m_uColor.rgb[2] = dBlue;
}

PdfColor::PdfColor( double dCyan, double dMagenta, double dYellow, double dBlack )
    : m_eColorSpace( ePdfColorSpace_DeviceCMYK )
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

const PdfColor & PdfColor::operator=( const PdfColor & rhs )
{
    m_eColorSpace = rhs.m_eColorSpace;
    memcpy( &m_uColor, &rhs.m_uColor, sizeof(m_uColor) );

    return *this;
}

PdfColor PdfColor::ConvertToGrayScale() const
{
    switch(m_eColorSpace)
    {
        case ePdfColorSpace_DeviceGray:
            return *this;
        case ePdfColorSpace_DeviceRGB:
            return PdfColor( 0.299*m_uColor.rgb[0] + 0.587*m_uColor.rgb[1] + 0.114*m_uColor.rgb[2] );
        case ePdfColorSpace_DeviceCMYK:
            return this->ConvertToRGB().ConvertToGrayScale();
    };

    return PdfColor();
}

PdfColor PdfColor::ConvertToRGB() const
{
    switch(m_eColorSpace)
    {
        case ePdfColorSpace_DeviceGray:
            return PdfColor( m_uColor.gray, m_uColor.gray, m_uColor.gray );
        case ePdfColorSpace_DeviceRGB:
            return *this;
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
    };

    return PdfColor();
}

PdfColor PdfColor::ConvertToCMYK() const
{
    switch(m_eColorSpace)
    {
        case ePdfColorSpace_DeviceGray:
            return this->ConvertToRGB().ConvertToCMYK();
        case ePdfColorSpace_DeviceRGB:
        {
            double dRed   = m_uColor.rgb[0];
            double dGreen = m_uColor.rgb[1];
            double dBlue  = m_uColor.rgb[2];

            double dBlack   = PDF_MIN( 1.0-dRed, PDF_MIN( 1.0-dGreen, 1.0-dBlue ) );
            double dCyan    = (1-dRed-dBlack)  /(1.0-dBlack);
            double dMagenta = (1-dGreen-dBlack)/(1.0-dBlack);
            double dYellow  = (1-dBlue-dBlack) /(1.0-dBlack);
            
            return PdfColor( dCyan, dMagenta, dYellow, dBlack );
        }
        case ePdfColorSpace_DeviceCMYK:
            return *this;
            break;
    };

    return PdfColor();
}

static unsigned short GetHex(char inHex)
{
    if ( islower( inHex ) )
        inHex -= 32;
    
    if (inHex <= '9')
        return inHex - '0';

    return inHex - 'A' + 10;
}

PdfColor PdfColor::FromString( const char* pszName )
{
    if( pszName ) 
    {
        long lLen = strlen( pszName );

        // first see if it's a single number - if so, that's a single gray value
        if( isdigit( pszName[0] ) || pszName[0] == '.' ) 
        {
            char* pszEnd = const_cast<char*>(pszName);
            double dGrayVal = strtod( pszName, &pszEnd );

            if( pszEnd == pszName ) // error in conversion
                return PdfColor();
            else
                return PdfColor( dGrayVal );
        }
        // now check for a hex value (#xxxxxx or #xxxxxxxx)
        else if( pszName[0] == '#' )
	{
            ++pszName;
            if( lLen == 7 ) // RGB
            {
                int r, g, b;

                r = (GetHex( *(pszName++) ) << 4) | GetHex( *(pszName++) );
                g = (GetHex( *(pszName++) ) << 4) | GetHex( *(pszName++) );
                b = (GetHex( *(pszName++) ) << 4) | GetHex( *(pszName++) );

                return PdfColor( static_cast<double>(r)/255.0, 
                                 static_cast<double>(g)/255.0, 
                                 static_cast<double>(b)/255.0 );
            }
            else if( lLen == 9 ) // CMYK
            {
                int c, m, y, k;


                c = (GetHex( *(pszName++) ) << 4) | GetHex( *(pszName++) );
                m = (GetHex( *(pszName++) ) << 4) | GetHex( *(pszName++) );
                y = (GetHex( *(pszName++) ) << 4) | GetHex( *(pszName++) );
                k = (GetHex( *(pszName++) ) << 4) | GetHex( *(pszName++) );

                return PdfColor( static_cast<double>(c)/255.0, 
                                 static_cast<double>(m)/255.0, 
                                 static_cast<double>(y)/255.0,
                                 static_cast<double>(k)/255.0 );
            }
            
        }
        // PdfArray 
        else if( pszName[0] == '[' ) 
        {
            printf("pdf...\n");
            PdfTokenizer tokenizer( pszName, lLen );
            PdfVariant   var;

            tokenizer.GetNextVariant( var, NULL ); // No encryption...
            if( var.IsArray() )
                return PdfColor::FromArray( var.GetArray() );
        }
        // it must be a named RGB color
        else
        {
            // TODO: binary search
            for( int i=0;i<nNumNamedColors;i++ )
            {
                if( s_NamedColors[i] == pszName )
                    return s_NamedColors[i].GetColor();
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

    return PdfColor();
}

};


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

#ifndef _PDF_COLOR_H_
#define _PDF_COLOR_H_

#include "PdfDefines.h"

namespace PoDoFo {

class PdfArray;
    
/** A color object can represent a either a grayscale
 *  value, a RGB color or a CMYK color.
 *
 *  All drawing functions in PoDoFo accept a PdfColor object
 *  to specify a drawing color in one of these colorspaces.
 */
class PODOFO_API PdfColor {
 public:
    /** Create a PdfColor object that is RGB black.
     */
    PdfColor();
    
    /** Create a new PdfColor object with
     *  a grayscale value.
     *
     *  \param dGray a grayscalue value between 0.0 and 1.0
     */
    PdfColor( double dGray );

    /** Create a new PdfColor object with
     *  a RGB color
     *
     *  \param dRed the value of the red component, must be between 0.0 and 1.0
     *  \param dGreen the value of the green component, must be between 0.0 and 1.0
     *  \param dBlue the value of the blue component, must be between 0.0 and 1.0
     */
    PdfColor( double dRed, double dGreen, double dBlue );

    /** Create a new PdfColor object with
     *  a CMYK color
     *
     *  \param dCyan the value of the cyan component, must be between 0.0 and 1.0
     *  \param dMagenta the value of the magenta component, must be between 0.0 and 1.0
     *  \param dYellow the value of the yellow component, must be between 0.0 and 1.0
     *  \param dBlack the value of the black component, must be between 0.0 and 1.0
     */
    PdfColor( double dCyan, double dMagenta, double dYellow, double dBlack );

    /** Copy constructor
     *
     *  \param rhs copy rhs into this object
     */
    inline PdfColor( const PdfColor & rhs );

    /** Assignment operator
     *
     *  \param rhs copy rhs into this object
     *
     *  \returns a reference to this color object
     */    
    const PdfColor & operator=( const PdfColor & rhs );

    /** Test if this is a grayscale color.
     * 
     *  \returns true if this is a grayscale PdfColor object
     */
    inline bool IsGrayScale() const;

    /** Test if this is a RGB color.
     * 
     *  \returns true if this is a RGB PdfColor object
     */
    inline bool IsRGB() const;

    /** Test if this is a CMYK color.
     * 
     *  \returns true if this is a CMYK PdfColor object
     */
    inline bool IsCMYK() const;

    /** Get the colorspace of this PdfColor object
     *
     *  \returns the colorspace of this PdfColor object
     */
    inline EPdfColorSpace GetColorSpace() const;

    /** Get the grayscale color value 
     *  of this object.
     *
     *  Throws an exception if this is no grayscale color object.
     *
     *  \returns the grayscale color value of this object (between 0.0 and 1.0)
     *
     *  \see IsGrayScale
     */
    inline double GetGrayScale() const;

    /** Get the red color value 
     *  of this object.
     *
     *  Throws an exception if this is no RGB color object.
     *
     *  \returns the red color value of this object (between 0.0 and 1.0)
     *
     *  \see IsRGB
     */
    inline double GetRed() const;

    /** Get the green color value 
     *  of this object.
     *
     *  Throws an exception if this is no RGB color object.
     *
     *  \returns the green color value of this object (between 0.0 and 1.0)
     *
     *  \see IsRGB
     */
    inline double GetGreen() const;

    /** Get the blue color value 
     *  of this object.
     *
     *  Throws an exception if this is no RGB color object.
     *
     *  \returns the blue color value of this object (between 0.0 and 1.0)
     *
     *  \see IsRGB
     */
    inline double GetBlue() const;

    /** Get the cyan color value 
     *  of this object.
     *
     *  Throws an exception if this is no CMYK color object.
     *
     *  \returns the cyan color value of this object (between 0.0 and 1.0)
     *
     *  \see IsCMYK
     */
    inline double GetCyan() const;

    /** Get the magenta color value 
     *  of this object.
     *
     *  Throws an exception if this is no CMYK color object.
     *
     *  \returns the magenta color value of this object (between 0.0 and 1.0)
     *
     *  \see IsCMYK
     */
    inline double GetMagenta() const;

    /** Get the yellow color value 
     *  of this object.
     *
     *  Throws an exception if this is no CMYK color object.
     *
     *  \returns the yellow color value of this object (between 0.0 and 1.0)
     *
     *  \see IsCMYK
     */
    inline double GetYellow() const;

    /** Get the black color value 
     *  of this object.
     *
     *  Throws an exception if this is no CMYK color object.
     *
     *  \returns the black color value of this object (between 0.0 and 1.0)
     *
     *  \see IsCMYK
     */
    inline double GetBlack() const;

    /** Converts the color object into a grayscale
     *  color object.
     *
     *  This is only a convinience function. It might be useful
     *  for on screen display but is in NO WAY suitable to
     *  professional printing!
     *
     *  \returns a grayscale color object
     *  \see IsGrayScale()
     */
    PdfColor ConvertToGrayScale() const;

    /** Converts the color object into a RGB
     *  color object.
     *
     *  This is only a convinience function. It might be useful
     *  for on screen display but is in NO WAY suitable to
     *  professional printing!
     *
     *  \returns a RGB color object
     *  \see IsRGB()
     */
    PdfColor ConvertToRGB() const;

    /** Converts the color object into a CMYK
     *  color object.
     *
     *  This is only a convinience function. It might be useful
     *  for on screen display but is in NO WAY suitable to
     *  professional printing!
     *
     *  \returns a CMYK color object
     *  \see IsCMYK()
     */
    PdfColor ConvertToCMYK() const;

    /** Creates a color object from a string.
     *
     *  \param pszName a string describing a color.
     *
     *  Supported values are:
     *  - single gray values as string (e.g. '0.5')
     *  - a named color (e.g. 'auquamarine' or 'magenta')
     *  - hex values (e.g. #FF002A (RGB) or #FF12AB3D (CMYK))
     *  - PdfArray's
     *
     *  \returns a PdfColor object
     */
    static PdfColor FromString( const char* pszName );

    /** Creates a color object from a PdfArray which represents a color.
     *
     *  Raises an exception if this is no PdfColor!
     *
     *  \param rArray an array that must be a color PdfArray
     *  \returns a PdfColor object
     */
    static PdfColor FromArray( const PdfArray & rArray );

 private:
    union {
        double cmyk[4];
        double rgb[3];
        double gray;
    }  m_uColor; 

    EPdfColorSpace m_eColorSpace;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfColor::PdfColor( const PdfColor & rhs )
{
    this->operator=( rhs );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfColor::IsGrayScale() const
{
    return (m_eColorSpace == ePdfColorSpace_DeviceGray);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfColor::IsRGB() const
{
    return (m_eColorSpace == ePdfColorSpace_DeviceRGB);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfColor::IsCMYK() const
{
    return (m_eColorSpace == ePdfColorSpace_DeviceCMYK);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
EPdfColorSpace PdfColor::GetColorSpace() const
{
    return m_eColorSpace;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfColor::GetGrayScale() const
{
    PODOFO_RAISE_LOGIC_IF( (!this->IsGrayScale()), "PdfColor::GetGrayScale cannot be called on non grayscale color objects!");

    return m_uColor.gray;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfColor::GetRed() const
{
    PODOFO_RAISE_LOGIC_IF( !this->IsRGB(), "PdfColor::GetRed cannot be called on non RGB color objects!");

    return m_uColor.rgb[0];
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfColor::GetGreen() const
{
    PODOFO_RAISE_LOGIC_IF( !this->IsRGB(), "PdfColor::GetGreen cannot be called on non RGB color objects!");

    return m_uColor.rgb[1];
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfColor::GetBlue() const
{
    PODOFO_RAISE_LOGIC_IF( !this->IsRGB(), "PdfColor::GetBlue cannot be called on non RGB color objects!");

    return m_uColor.rgb[2];
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfColor::GetCyan() const
{
    PODOFO_RAISE_LOGIC_IF( !this->IsCMYK(), "PdfColor::GetCyan cannot be called on non CMYK color objects!");

    return m_uColor.cmyk[0];
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfColor::GetMagenta() const
{
    PODOFO_RAISE_LOGIC_IF( !this->IsCMYK(), "PdfColor::GetMagenta cannot be called on non CMYK color objects!");

    return m_uColor.cmyk[1];
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfColor::GetYellow() const
{
    PODOFO_RAISE_LOGIC_IF( !this->IsCMYK(), "PdfColor::GetYellow cannot be called on non CMYK color objects!");

    return m_uColor.cmyk[2];
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfColor::GetBlack() const
{
    PODOFO_RAISE_LOGIC_IF( !this->IsCMYK(), "PdfColor::GetBlack cannot be called on non CMYK color objects!");

    return m_uColor.cmyk[3];
}

};

#endif // _PDF_COLOR_H_

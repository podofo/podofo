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
    
/** A color object can represent either a grayscale
 *  value, a RGB color, a CMYK color, a separation color or
 *  a CieLab color.
 *
 *  All drawing functions in PoDoFo accept a PdfColor object
 *  to specify a drawing color in one of these colorspaces.
 *
 *  Derived classes PdfColorGray, PdfColorRGB, PdfColorCMYK, PdfColorSeparation
 *  and PdfColorCieLab are available for easy construction
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

    /** Test for equality of colors.
     * 
     *  \param rhs color to compare ro
	 *
     *  \returns true if object color is equal to rhs
     */
    inline bool operator==( const PdfColor & rhs ) const;

    /** Test for inequality of colors.
     * 
     *  \param rhs color to compare ro
	 *
     *  \returns true if object color is not equal to rhs
     */
    inline bool operator!=( const PdfColor & rhs ) const;

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

    /** Test if this is a separation color.
     * 
     *  \returns true if this is a separation PdfColor object
     */
    inline bool IsSeparation() const;

    /** Test if this is a CIE-Lab color.
     * 
     *  \returns true if this is a lab Color object
     */
    inline bool IsCieLab() const;

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
     *  Throws an exception if this is no CMYK or separation color object.
     *
     *  \returns the cyan color value of this object (between 0.0 and 1.0)
     *
     *  \see IsCMYK
     */
    inline double GetCyan() const;

    /** Get the magenta color value 
     *  of this object.
     *
     *  Throws an exception if this is no CMYK or separation color object.
     *
     *  \returns the magenta color value of this object (between 0.0 and 1.0)
     *
     *  \see IsCMYK
     */
    inline double GetMagenta() const;

    /** Get the yellow color value 
     *  of this object.
     *
     *  Throws an exception if this is no CMYK or separation color object.
     *
     *  \returns the yellow color value of this object (between 0.0 and 1.0)
     *
     *  \see IsCMYK
     */
    inline double GetYellow() const;

    /** Get the black color value 
     *  of this object.
     *
     *  Throws an exception if this is no CMYK or separation color object.
     *
     *  \returns the black color value of this object (between 0.0 and 1.0)
     *
     *  \see IsCMYK
     */
    inline double GetBlack() const;

    /** Get the separation name of this object.
     *
     *  Throws an exception if this is no separation color object.
     *
     *  \returns the name of this object
     *
     *  \see IsSeparation
     */
	inline const std::string GetName() const;

    /** Get the L color value 
     *  of this object.
     *
     *  Throws an exception if this is no CIE-Lab color object.
     *
     *  \returns the L color value of this object (between 0.0 and 100.0)
     *
     *  \see IsCieLab
     */
    inline double GetCieL() const;

    /** Get the A color value 
     *  of this object.
     *
     *  Throws an exception if this is no CIE-Lab color object.
     *
     *  \returns the A color value of this object (between -128.0 and 127.0)
     *
     *  \see IsCieLab
     */
    inline double GetCieA() const;

    /** Get the B color value 
     *  of this object.
     *
     *  Throws an exception if this is no CIE-Lab color object.
     *
     *  \returns the B color value of this object (between -128.0 and 127.0)
     *
     *  \see IsCieLab
     */
    inline double GetCieB() const;

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

    /** Creates a PdfArray which represents a color from a color.
     *  \returns a PdfArray object
     */
    PdfArray ToArray() const;

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

 protected:
    union {
        double cmyk[4];
        double rgb[3];
        double lab[3];
        double gray;
    }  m_uColor; 
	std::string m_separationName;
    EPdfColorSpace m_eColorSpace;
};

class PODOFO_API PdfColorGray : public PdfColor {
 public:
    
    /** Create a new PdfColor object with
     *  a grayscale value.
     *
     *  \param dGray a grayscalue value between 0.0 and 1.0
     */
    PdfColorGray( double dGray );
};

class PODOFO_API PdfColorRGB : public PdfColor {
 public:
	/** Create a new PdfColor object with
     *  a RGB color
     *
     *  \param dRed the value of the red component, must be between 0.0 and 1.0
     *  \param dGreen the value of the green component, must be between 0.0 and 1.0
     *  \param dBlue the value of the blue component, must be between 0.0 and 1.0
     */
    PdfColorRGB( double dRed, double dGreen, double dBlue );
};

class PODOFO_API PdfColorCMYK : public PdfColor {
 public:

    /** Create a new PdfColor object with
     *  a CMYK color
     *
     *  \param dCyan the value of the cyan component, must be between 0.0 and 1.0
     *  \param dMagenta the value of the magenta component, must be between 0.0 and 1.0
     *  \param dYellow the value of the yellow component, must be between 0.0 and 1.0
     *  \param dBlack the value of the black component, must be between 0.0 and 1.0
     */
    PdfColorCMYK( double dCyan, double dMagenta, double dYellow, double dBlack );
};

class PODOFO_API PdfColorSeparationAll : public PdfColor {
 public:

	 /** Create a new PdfColor object with
     *  Separation color All.
     *
     */
    PdfColorSeparationAll();
};

class PODOFO_API PdfColorSeparationNone : public PdfColor {
 public:

	 /** Create a new PdfColor object with
     *  Separation color None.
     *
     */
    PdfColorSeparationNone();
};

class PODOFO_API PdfColorSeparation : public PdfColor {
 public:

    /** Create a new PdfColor object with
     *  a separation-name and an equivalent CMYK color
     *
     *  \param sName Name of the separation color
     *  \param dCyan the value of the cyan component, must be between 0.0 and 1.0
     *  \param dMagenta the value of the magenta component, must be between 0.0 and 1.0
     *  \param dYellow the value of the yellow component, must be between 0.0 and 1.0
     *  \param dBlack the value of the black component, must be between 0.0 and 1.0
     */
	PdfColorSeparation( const std::string & sName, double dCyan, double dMagenta, double dYellow, double dBlack );
};

class PODOFO_API PdfColorCieLab : public PdfColor {
 public:

    /** Create a new PdfColor object with
     *  a CIE-LAB-values
     *
     *  \param dCieL the value of the L component, must be between 0.0 and 100.0
     *  \param dCieA the value of the A component, must be between -128.0 and 127.0
     *  \param dCieB the value of the B component, must be between -128.0 and 127.0
     */
	PdfColorCieLab( double dCieL, double dCieA, double dCieB );
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
inline bool PdfColor::operator==( const PdfColor & rhs ) const
{
	if ( m_eColorSpace == rhs.m_eColorSpace	)
	{
		if ( 
			m_eColorSpace == ePdfColorSpace_DeviceGray			&&
			m_uColor.gray == rhs.m_uColor.gray
		   )
		   return true;

		if ( 
			m_eColorSpace == ePdfColorSpace_DeviceRGB			&&
			m_uColor.rgb[0] == rhs.m_uColor.rgb[0]				&&
			m_uColor.rgb[1] == rhs.m_uColor.rgb[1]				&&
			m_uColor.rgb[2] == rhs.m_uColor.rgb[2]
		   )
		   return true;

		if ( 
			m_eColorSpace == ePdfColorSpace_DeviceCMYK			&&
			m_uColor.cmyk[0] == rhs.m_uColor.cmyk[0]			&&
			m_uColor.cmyk[1] == rhs.m_uColor.cmyk[1]			&&
			m_uColor.cmyk[2] == rhs.m_uColor.cmyk[2]			&&
			m_uColor.cmyk[3] == rhs.m_uColor.cmyk[3]
		   )
		   return true;

		if ( 
			m_eColorSpace == ePdfColorSpace_Separation			&&
			m_uColor.cmyk[0] == rhs.m_uColor.cmyk[0]			&&
			m_uColor.cmyk[1] == rhs.m_uColor.cmyk[1]			&&
			m_uColor.cmyk[2] == rhs.m_uColor.cmyk[2]			&&
			m_uColor.cmyk[3] == rhs.m_uColor.cmyk[3]			&&
			m_separationName == rhs.m_separationName
		   )
		   return true;

		if ( 
			m_eColorSpace == ePdfColorSpace_CieLab				&&
			m_uColor.lab[0] == rhs.m_uColor.lab[0]				&&
			m_uColor.lab[1] == rhs.m_uColor.lab[1]				&&
			m_uColor.lab[2] == rhs.m_uColor.lab[2]
		   )
		   return true;
	}
	return false;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfColor::operator!=( const PdfColor & rhs ) const
{
	return ! (*this == rhs);
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
bool PdfColor::IsSeparation() const
{
    return (m_eColorSpace == ePdfColorSpace_Separation);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfColor::IsCieLab() const
{
    return (m_eColorSpace == ePdfColorSpace_CieLab);
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
	PODOFO_RAISE_LOGIC_IF( !this->IsCMYK()  &&  !this->IsSeparation(), "PdfColor::GetCyan cannot be called on non CMYK/separation color objects!");

    return m_uColor.cmyk[0];
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfColor::GetMagenta() const
{
	PODOFO_RAISE_LOGIC_IF( !this->IsCMYK()  &&  !this->IsSeparation(), "PdfColor::GetMagenta cannot be called on non CMYK/separation color objects!");

    return m_uColor.cmyk[1];
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfColor::GetYellow() const
{
	PODOFO_RAISE_LOGIC_IF( !this->IsCMYK()  &&  !this->IsSeparation(), "PdfColor::GetYellow cannot be called on non CMYK/separation color objects!");

    return m_uColor.cmyk[2];
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfColor::GetBlack() const
{
	PODOFO_RAISE_LOGIC_IF( !this->IsCMYK()  &&  !this->IsSeparation(), "PdfColor::GetBlack cannot be called on non CMYK/separation color objects!");

    return m_uColor.cmyk[3];
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const std::string PdfColor::GetName() const
{
    PODOFO_RAISE_LOGIC_IF( !this->IsSeparation(), "PdfColor::GetName cannot be called on non separation color objects!");

    return m_separationName;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfColor::GetCieL() const
{
    PODOFO_RAISE_LOGIC_IF( !this->IsCieLab(), "PdfColor::GetCieL cannot be called on non CIE-Lab color objects!");

    return m_uColor.lab[0];
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfColor::GetCieA() const
{
    PODOFO_RAISE_LOGIC_IF( !this->IsCieLab(), "PdfColor::GetCieA cannot be called on non CIE-Lab color objects!");

    return m_uColor.lab[1];
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfColor::GetCieB() const
{
    PODOFO_RAISE_LOGIC_IF( !this->IsCieLab(), "PdfColor::GetCieB cannot be called on non CIE-Lab color objects!");

    return m_uColor.lab[2];
}

};

#endif // _PDF_COLOR_H_

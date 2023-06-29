/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_COLOR_H
#define PDF_COLOR_H

#include "PdfDeclarations.h"

#include "PdfName.h"
#include "PdfArray.h"

namespace PoDoFo {

class PdfObject;
class PdfDocument;

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
class PODOFO_API PdfColor final
{
public:
    /** Create a PdfColor object that is grayscale black.
     */
    PdfColor();

    /** Create a new PdfColor object with
     *  a grayscale value.
     *
     *  \param gray a grayscale value between 0.0 and 1.0
     */
    explicit PdfColor(double gray);

    /** Create a new PdfColor object with
     *  a RGB color
     *
     *  \param red the value of the red component, must be between 0.0 and 1.0
     *  \param green the value of the green component, must be between 0.0 and 1.0
     *  \param blue the value of the blue component, must be between 0.0 and 1.0
     */
    PdfColor(double red, double green, double blue);

    /** Create a new PdfColor object with
     *  a CMYK color
     *
     *  \param cyan the value of the cyan component, must be between 0.0 and 1.0
     *  \param magenta the value of the magenta component, must be between 0.0 and 1.0
     *  \param yellow the value of the yellow component, must be between 0.0 and 1.0
     *  \param black the value of the black component, must be between 0.0 and 1.0
     */
    PdfColor(double cyan, double magenta, double yellow, double black);

    /** Copy constructor
     *
     *  \param rhs copy rhs into this object
     */
    PdfColor(const PdfColor& rhs) = default;

public:
    /** Creates a color object from a PdfArray which represents a color.
     *
     *  \param obj an object that must be a color PdfArray
     */
    static bool TryCreateFromObject(const PdfObject& obj, PdfColor& color);
    static PdfColor FromObject(const PdfObject& obj);
    static bool TryCreateFromArray(const PdfArray& arr, PdfColor& color);
    static PdfColor FromArray(const PdfArray& arr);

    /** Create a new PdfColor object with a CIE-LAB-value
     *
     *  \param cieL the value of the L component, must be between 0.0 and 100.0
     *  \param cieA the value of the A component, must be between -128.0 and 127.0
     *  \param cieB the value of the B component, must be between -128.0 and 127.0
     */
    static PdfColor CreateCieLab(double cieL, double cieA, double cieB);

    /** Create a new PdfColor object with
     *  a separation-name and an equivalent color
     *
     *  \param name Name of the separation color
     *  \param density the density value of the separation color
     *  \param alternateColor the alternate color, must be of type gray, rgb, cmyk or cie
     */
    static PdfColor CreateSeparation(const std::string_view& name, double density, const PdfColor& alternateColor);

    /** Create a new PdfColor object with
     *  Separation color None.
     *
     */
    static PdfColor CreateSeparationNone();

   /** Create a new PdfColor object with
    *  Separation color All.
    *
    */
    static PdfColor CreateSeparationAll();

    static PdfColor CreateTransparent();

public:
    /** Test if this is a grayscale color.
     *
     *  \returns true if this is a grayscale PdfColor object
     */
    bool IsGrayScale() const;

    /** Test if this is a RGB color.
     *
     *  \returns true if this is a RGB PdfColor object
     */
    bool IsRGB() const;

    /** Test if this is a CMYK color.
     *
     *  \returns true if this is a CMYK PdfColor object
     */
    bool IsCMYK() const;

    /** Test if this is a separation color.
     *
     *  \returns true if this is a separation PdfColor object
     */
    bool IsSeparation() const;

    /** Test if this is a CIE-Lab color.
     *
     *  \returns true if this is a lab Color object
     */
    bool IsCieLab() const;

    bool IsTransparent() const;

    /** Get the colorspace of this PdfColor object
     *
     *  \returns the colorspace of this PdfColor object
     */
    inline PdfColorSpace GetColorSpace() const { return m_ColorSpace; }

    /** Get the alternate colorspace of this PdfColor object
     *
     *  \returns the colorspace of this PdfColor object (must be separation)
     */
    PdfColorSpace GetAlternateColorSpace() const;

    /** Get the grayscale color value
     *  of this object.
     *
     *  Throws an exception if this is no grayscale color object.
     *
     *  \returns the grayscale color value of this object (between 0.0 and 1.0)
     *
     *  \see IsGrayScale
     */
    double GetGrayScale() const;

    /** Get the red color value
     *  of this object.
     *
     *  Throws an exception if this is no RGB color object.
     *
     *  \returns the red color value of this object (between 0.0 and 1.0)
     *
     *  \see IsRGB
     */
    double GetRed() const;

    /** Get the green color value
     *  of this object.
     *
     *  Throws an exception if this is no RGB color object.
     *
     *  \returns the green color value of this object (between 0.0 and 1.0)
     *
     *  \see IsRGB
     */
    double GetGreen() const;

    /** Get the blue color value
     *  of this object.
     *
     *  Throws an exception if this is no RGB color object.
     *
     *  \returns the blue color value of this object (between 0.0 and 1.0)
     *
     *  \see IsRGB
     */
    double GetBlue() const;

    /** Get the cyan color value
     *  of this object.
     *
     *  Throws an exception if this is no CMYK or separation color object.
     *
     *  \returns the cyan color value of this object (between 0.0 and 1.0)
     *
     *  \see IsCMYK
     */
    double GetCyan() const;

    /** Get the magenta color value
     *  of this object.
     *
     *  Throws an exception if this is no CMYK or separation color object.
     *
     *  \returns the magenta color value of this object (between 0.0 and 1.0)
     *
     *  \see IsCMYK
     */
    double GetMagenta() const;

    /** Get the yellow color value
     *  of this object.
     *
     *  Throws an exception if this is no CMYK or separation color object.
     *
     *  \returns the yellow color value of this object (between 0.0 and 1.0)
     *
     *  \see IsCMYK
     */
    double GetYellow() const;

    /** Get the black color value
     *  of this object.
     *
     *  Throws an exception if this is no CMYK or separation color object.
     *
     *  \returns the black color value of this object (between 0.0 and 1.0)
     *
     *  \see IsCMYK
     */
    double GetBlack() const;

    /** Get the separation name of this object.
     *
     *  Throws an exception if this is no separation color object.
     *
     *  \returns the name of this object
     *
     *  \see IsSeparation
     */
    const std::string& GetName() const;

    /** Get the density color value
     *  of this object.
     *
     *  Throws an exception if this is no separation color object.
     *
     *  \returns the density value of this object (between 0.0 and 1.0)
     *
     *  \see IsSeparation
     */
    double GetDensity() const;

    /** Get the L color value
     *  of this object.
     *
     *  Throws an exception if this is no CIE-Lab color object.
     *
     *  \returns the L color value of this object (between 0.0 and 100.0)
     *
     *  \see IsCieLab
     */
    double GetCieL() const;

    /** Get the A color value
     *  of this object.
     *
     *  Throws an exception if this is no CIE-Lab color object.
     *
     *  \returns the A color value of this object (between -128.0 and 127.0)
     *
     *  \see IsCieLab
     */
    double GetCieA() const;

    /** Get the B color value
     *  of this object.
     *
     *  Throws an exception if this is no CIE-Lab color object.
     *
     *  \returns the B color value of this object (between -128.0 and 127.0)
     *
     *  \see IsCieLab
     */
    double GetCieB() const;

    /** Converts the color object into a grayscale
     *  color object.
     *
     *  This is only a convenience function. It might be useful
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
     *  This is only a convenience function. It might be useful
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
     *  This is only a convenience function. It might be useful
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
     *  \param name a string describing a color.
     *
     *  Supported values are:
     *  - single gray values as string (e.g. '0.5')
     *  - a named color (e.g. 'aquamarine' or 'magenta')
     *  - hex values (e.g. #FF002A (RGB) or #FF12AB3D (CMYK))
     *  - PdfArray's
     *
     *  \returns a PdfColor object
     */
    static PdfColor FromString(const std::string_view& name);

    /** Creates a colorspace object from a color to insert into resources.
     *
     *  \param document a pointer to the owner document of the generated object
     *  \returns a PdfObject pointer, which can be insert into resources, nullptr if not needed
     */
    PdfObject* BuildColorSpace(PdfDocument& document) const;

public:
    /** Assignment operator
     *
     *  \param rhs copy rhs into this object
     *
     *  \returns a reference to this color object
     */
    PdfColor& operator=(const PdfColor& rhs) = default;

    /** Test for equality of colors.
     *
     *  \param rhs color to compare to
     *
     *  \returns true if object color is equal to rhs
     */
    bool operator==(const PdfColor& rhs) const;

    /** Test for inequality of colors.
     *
     *  \param rhs color to compare to
     *
     *  \returns true if object color is not equal to rhs
     */
    bool operator!=(const PdfColor& rhs) const;

private:
    union Color
    {
        double cmyk[4];
        double rgb[3];
        double lab[3];
        double gray;
    };

    PdfColor(bool isTransaprent, PdfColorSpace colorSpace, const Color& data,
        std::string m_separationName, double separationDensity,
        PdfColorSpace alternateColorSpace);

private:
    bool m_IsTransparent;
    PdfColorSpace m_ColorSpace;
    Color m_Color;
    std::string m_SeparationName;
    double m_SeparationDensity;
    PdfColorSpace m_AlternateColorSpace;

private:
    static const unsigned* const m_hexDigitMap; // Mapping of hex sequences to int value
};

};

#endif // PDF_COLOR_H

/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_SHADING_PATTERN_H
#define PDF_SHADING_PATTERN_H

#include <podofo/main/PdfDeclarations.h>
#include <podofo/main/PdfName.h>
#include <podofo/main/PdfElement.h>

namespace PoDoFo {

class PdfColor;
class PdfObject;
class PdfPage;
class PdfWriter;

enum class PdfShadingPatternType
{
    FunctionBase = 1,
    Axial = 2,
    Radial = 3,
    FreeForm = 4,
    LatticeForm = 5,
    CoonsPatch = 6,
    TensorProduct = 7
};

/**
 * This class defined a shading pattern which can be used
 * to fill abitrary shapes with a pattern using PdfPainter.
 */
class PODOFO_API PdfShadingPattern : public PdfDictionaryElement
{
public:
    /** Returns the identifier of this ShadingPattern how it is known
     *  in the pages resource dictionary.
     *  \returns PdfName containing the identifier (e.g. /Sh13)
     */
    inline const PdfName& GetIdentifier() const;

protected:
    /** Create a new PdfShadingPattern object which will introduce itself
     *  automatically to every page object it is used on.
     *
     *  \param doc parent document
     *  \param eShadingType the type of this shading pattern
     *
     */
    PdfShadingPattern(PdfDocument& doc, PdfShadingPatternType shadingType);

private:
    /** Initialize the object
     *
     *  \param eShadingType the type of this shading pattern
     */
    void Init(PdfShadingPatternType shadingType);

private:
    PdfName m_Identifier;
};

const PdfName& PdfShadingPattern::GetIdentifier() const
{
    return m_Identifier;
}

/** A shading pattern that is a simple axial
 *  shading between two colors.
 */
class PODOFO_API PdfAxialShadingPattern final : public PdfShadingPattern
{
public:
    /** Create an axial shading pattern
     *
     *  \param doc the parent
     *  \param x0 the starting x coordinate
     *  \param y0 the starting y coordinate
     *  \param x1 the ending x coordinate
     *  \param y1 the ending y coordinate
     *  \param start the starting color
     *  \param end the ending color
     */
    PdfAxialShadingPattern(PdfDocument& doc, double x0, double y0, double x1, double y1, const PdfColor& start, const PdfColor& end);

private:

    /** Initialize an axial shading pattern
     *
     *  \param x0 the starting x coordinate
     *  \param y0 the starting y coordinate
     *  \param x1 the ending x coordinate
     *  \param y1 the ending y coordinate
     *  \param start the starting color
     *  \param end the ending color
     */
    void Init(double x0, double y0, double x1, double y1, const PdfColor& start, const PdfColor& end);
};

/** A shading pattern that is an 2D
 *  shading between four colors.
 */
class PODOFO_API PdfFunctionBaseShadingPattern final : public PdfShadingPattern
{
public:
    /** Create an 2D shading pattern
     *
     *  \param doc the parent
     *  \param llCol the color on lower left corner
     *  \param ulCol the color on upper left corner
     *  \param lrCol the color on lower right corner
     *  \param urCol the color on upper right corner
     *  \param matrix the transformation matrix mapping the coordinate space
     *         specified by the Domain entry into the shading's target coordinate space
     */
    PdfFunctionBaseShadingPattern(PdfDocument& doc, const PdfColor& llCol, const PdfColor& ulCol, const PdfColor& lrCol, const PdfColor& urCol, const PdfArray& matrix);

private:

    /** Initialize an 2D shading pattern
     *
     *  \param llCol the color on lower left corner
     *  \param ulCol the color on upper left corner
     *  \param lrCol the color on lower right corner
     *  \param urCol the color on upper right corner
     *  \param matrix the transformation matrix mapping the coordinate space
     *         specified by the Domain entry into the shading's target coordinate space
     */
    void Init(const PdfColor& llCol, const PdfColor& ulCol, const PdfColor& lrCol, const PdfColor& urCol, const PdfArray& matrix);
};

/** A shading pattern that is a simple radial
 *  shading between two colors.
 */
class PODOFO_API PdfRadialShadingPattern final : public PdfShadingPattern
{
public:
    /** Create an radial shading pattern
     *
     *  \param doc the parent
     *  \param x0 the inner circles x coordinate
     *  \param y0 the inner circles y coordinate
     *  \param r0 the inner circles radius
     *  \param x1 the outer circles x coordinate
     *  \param y1 the outer circles y coordinate
     *  \param r1 the outer circles radius
     *  \param start the starting color
     *  \param end the ending color
     */
    PdfRadialShadingPattern(PdfDocument& doc, double x0, double y0, double r0, double x1, double y1, double r1, const PdfColor& start, const PdfColor& end);

private:

    /** Initialize an radial shading pattern
     *
     *  \param x0 the inner circles x coordinate
     *  \param y0 the inner circles y coordinate
     *  \param r0 the inner circles radius
     *  \param x1 the outer circles x coordinate
     *  \param y1 the outer circles y coordinate
     *  \param r1 the outer circles radius
     *  \param start the starting color
     *  \param end the ending color
     */
    void Init(double x0, double y0, double r0, double x1, double y1, double r1, const PdfColor& start, const PdfColor& end);
};

/** A shading pattern that is a simple triangle
 *  shading between three colors. It's a single-triangle
 *  simplified variation of a FreeForm shadding pattern.
 */
class PODOFO_API PdfTriangleShadingPattern final : public PdfShadingPattern
{
public:
    /** Create a triangle shading pattern
     *
     *  \param x0 triangle x coordinate of point 0
     *  \param y0 triangle y coordinate of point 0
     *  \param color0 color of point 0
     *  \param x1 triangle x coordinate of point 1
     *  \param y1 triangle y coordinate of point 1
     *  \param color1 color of point 1
     *  \param x2 triangle x coordinate of point 2
     *  \param y2 triangle y coordinate of point 2
     *  \param color2 color of point 2
     *  \param parent the parent
     */
    PdfTriangleShadingPattern(PdfDocument& doc, double x0, double y0, const PdfColor& color0, double x1, double y1, const PdfColor& color1, double x2, double y2, const PdfColor& color2);

private:

    /** Initialize a triangle shading pattern
     *
     *  \param x0 triangle x coordinate of point 0
     *  \param y0 triangle y coordinate of point 0
     *  \param color0 color of point 0
     *  \param x1 triangle x coordinate of point 1
     *  \param y1 triangle y coordinate of point 1
     *  \param color1 color of point 1
     *  \param x2 triangle x coordinate of point 2
     *  \param y2 triangle y coordinate of point 2
     *  \param color2 color of point 2
     */
    void Init(double x0, double y0, const PdfColor& color0, double x1, double y1, const PdfColor& color1, double x2, double y2, const PdfColor& color2);
};

};

#endif // PDF_SHADING_PATTERN_H


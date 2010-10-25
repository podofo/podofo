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

#ifndef _PDF_SHADING_PATTERN_H_
#define _PDF_SHADING_PATTERN_H_

#include "base/PdfDefines.h"
#include "base/PdfName.h"
#include "PdfElement.h"

namespace PoDoFo {

class PdfColor;
class PdfObject;
class PdfPage;
class PdfWriter;

enum EPdfShadingPatternType {
    ePdfShadingPatternType_FunctionBase  = 1,    
    ePdfShadingPatternType_Axial         = 2,
    ePdfShadingPatternType_Radial        = 3,
    ePdfShadingPatternType_FreeForm      = 4,
    ePdfShadingPatternType_LatticeForm   = 5,
    ePdfShadingPatternType_CoonsPatch    = 6,
    ePdfShadingPatternType_TensorProduct = 7
};

/** 
 * This class defined a shading pattern which can be used
 * to fill abitrary shapes with a pattern using PdfPainter.
 */
class PODOFO_DOC_API PdfShadingPattern : public PdfElement {
 public:
    virtual ~PdfShadingPattern();

    /** Returns the identifier of this ShadingPattern how it is known
     *  in the pages resource dictionary.
     *  \returns PdfName containing the identifier (e.g. /Sh13)
     */
    inline const PdfName & GetIdentifier() const;

  protected:
    /** Create a new PdfShadingPattern object which will introduce itself
     *  automatically to every page object it is used on.
     *
     *  \param pParent parent vector of objects
     *  \param eShadingType the type of this shading pattern
     *  
     */
    PdfShadingPattern( EPdfShadingPatternType eShadingType, PdfVecObjects* pParent );

    /** Create a new PdfShadingPattern object which will introduce itself
     *  automatically to every page object it is used on.
     *
     *  \param pParent parent document 
     *  \param eShadingType the type of this shading pattern
     *  
     */
    PdfShadingPattern( EPdfShadingPatternType eShadingType, PdfDocument* pParent );


 private:
    /** Initialize the object
     *
     *  \param eShadingType the type of this shading pattern
     */
    void Init( EPdfShadingPatternType eShadingType );

 private: 
    PdfName m_Identifier;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfName & PdfShadingPattern::GetIdentifier() const
{
    return m_Identifier;
}

/** A shading pattern that is a simple axial
 *  shading between two colors.
 */
class PODOFO_DOC_API PdfAxialShadingPattern : public PdfShadingPattern {
public:
    /** Create an axial shading pattern
     *
     *  \param dX0 the starting x coordinate
     *  \param dY0 the starting y coordinate
     *  \param dX1 the ending x coordinate
     *  \param dY1 the ending y coordinate
     *  \param rStart the starting color
     *  \param rEnd the ending color
     *  \param pParent the parent
     */
    PdfAxialShadingPattern( double dX0, double dY0, double dX1, double dY1, const PdfColor & rStart, const PdfColor & rEnd, PdfVecObjects* pParent );

    /** Create an axial shading pattern
     *
     *  \param dX0 the starting x coordinate
     *  \param dY0 the starting y coordinate
     *  \param dX1 the ending x coordinate
     *  \param dY1 the ending y coordinate
     *  \param rStart the starting color
     *  \param rEnd the ending color
     *  \param pParent the parent
     */
    PdfAxialShadingPattern( double dX0, double dY0, double dX1, double dY1, const PdfColor & rStart, const PdfColor & rEnd, PdfDocument* pParent );

private:

    /** Initialize an axial shading pattern
     *
     *  \param dX0 the starting x coordinate
     *  \param dY0 the starting y coordinate
     *  \param dX1 the ending x coordinate
     *  \param dY1 the ending y coordinate
     *  \param rStart the starting color
     *  \param rEnd the ending color
     */
    void Init( double dX0, double dY0, double dX1, double dY1, const PdfColor & rStart, const PdfColor & rEnd );
};

/** A shading pattern that is an 2D
 *  shading between four colors.
 */
class PODOFO_DOC_API PdfFunctionBaseShadingPattern : public PdfShadingPattern {
public:
    /** Create an 2D shading pattern
     *
     *  \param rLL the color on lower left corner
     *  \param rUL the color on upper left corner
     *  \param rLR the color on lower right corner
     *  \param rUR the color on upper right corner
     *  \param rMatrix the transformation matrix mapping the coordinate space 
     *         specified by the Domain entry into the shading’s target coordinate space
     *  \param pParent the parent
     */
    PdfFunctionBaseShadingPattern( const PdfColor & rLL, const PdfColor & rUL, const PdfColor & rLR, const PdfColor & rUR, const PdfArray & rMatrix, PdfVecObjects* pParent );

    /** Create an 2D shading pattern
     *
     *  \param rLL the color on lower left corner
     *  \param rUL the color on upper left corner
     *  \param rLR the color on lower right corner
     *  \param rUR the color on upper right corner
     *  \param rMatrix the transformation matrix mapping the coordinate space 
     *         specified by the Domain entry into the shading’s target coordinate space
     *  \param pParent the parent
     */
    PdfFunctionBaseShadingPattern( const PdfColor & rLL, const PdfColor & rUL, const PdfColor & rLR, const PdfColor & rUR, const PdfArray & rMatrix, PdfDocument* pParent );

private:

    /** Initialize an 2D shading pattern
     *
     *  \param rLL the color on lower left corner
     *  \param rUL the color on upper left corner
     *  \param rLR the color on lower right corner
     *  \param rUR the color on upper right corner
     *  \param rMatrix the transformation matrix mapping the coordinate space 
     *         specified by the Domain entry into the shading’s target coordinate space
     */
    void Init( const PdfColor & rLL, const PdfColor & rUL, const PdfColor & rLR, const PdfColor & rUR, const PdfArray & rMatrix );
};

/** A shading pattern that is a simple radial
 *  shading between two colors.
 */
class PODOFO_DOC_API PdfRadialShadingPattern : public PdfShadingPattern {
public:
    /** Create an radial shading pattern
     *
     *  \param dX0 the inner circles x coordinate
     *  \param dY0 the inner circles y coordinate
     *  \param dR0 the inner circles radius
     *  \param dX1 the outer circles x coordinate
     *  \param dY1 the outer circles y coordinate
     *  \param dR1 the outer circles radius
     *  \param rStart the starting color
     *  \param rEnd the ending color
     *  \param pParent the parent
     */
    PdfRadialShadingPattern( double dX0, double dY0, double dR0, double dX1, double dY1, double dR1, const PdfColor & rStart, const PdfColor & rEnd, PdfVecObjects* pParent );

    /** Create an radial shading pattern
     *
     *  \param dX0 the inner circles x coordinate
     *  \param dY0 the inner circles y coordinate
     *  \param dR0 the inner circles radius
     *  \param dX1 the outer circles x coordinate
     *  \param dY1 the outer circles y coordinate
     *  \param dR1 the outer circles radius
     *  \param rStart the starting color
     *  \param rEnd the ending color
     *  \param pParent the parent
     */
    PdfRadialShadingPattern( double dX0, double dY0, double dR0, double dX1, double dY1, double dR1, const PdfColor & rStart, const PdfColor & rEnd, PdfDocument* pParent );

private:

    /** Initialize an radial shading pattern
     *
     *  \param dX0 the inner circles x coordinate
     *  \param dY0 the inner circles y coordinate
     *  \param dR0 the inner circles radius
     *  \param dX1 the outer circles x coordinate
     *  \param dY1 the outer circles y coordinate
     *  \param dR1 the outer circles radius
     *  \param rStart the starting color
     *  \param rEnd the ending color
     */
    void Init( double dX0, double dY0, double dR0, double dX1, double dY1, double dR1, const PdfColor & rStart, const PdfColor & rEnd );
};

};

#endif // _PDF_SHADING_PATTERN_H_


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

#ifndef _PDF_FUNCTION_H_
#define _PDF_FUNCTION_H_

#include "PdfDefines.h"
#include "PdfElement.h"

#include <list>

namespace PoDoFo {

class PdfArray;

enum EPdfFunctionType {
    ePdfFunctionType_Sampled     = 0, ///< A sampled function (Type1)
    ePdfFunctionType_Exponential = 2, ///< An exponential interpolation function (Type2)
    ePdfFunctionType_Stitching   = 3, ///< A stitching function (Type3)
    ePdfFunctionType_PostScript  = 4  ///< A PostScript calculator function (Type4)
};

/** 
 * This class defines a PdfFunction.
 */
class PODOFO_API PdfFunction : public PdfElement {
public:

    typedef std::list<PdfFunction> List;

    virtual ~PdfFunction();

protected:
    /** Create a new PdfFunction object.
     *
     *  \param eType the function type 
     *  \param rDomain this array describes the input parameters of this PdfFunction. If this
     *                 function has m input parameters, this array has to contain 2*m numbers
     *                 where each number describes either the lower or upper boundary of the input range.
     *  \param pParent parent vector of objects
     *  
     */
    PdfFunction( EPdfFunctionType eType, const PdfArray & rDomain, PdfVecObjects* pParent );

    /** Create a new PdfFunction object.
     *
     *  \param eType the function type 
     *  \param rDomain this array describes the input parameters of this PdfFunction. If this
     *                 function has m input parameters, this array has to contain 2*m numbers
     *                 where each number describes either the lower or upper boundary of the input range.
     *  \param pParent parent document 
     *  
     */
    PdfFunction( EPdfFunctionType eType, const PdfArray & rDomain, PdfDocument* pParent );

private:
    /** Initialize this object.
     *
     *  \param eType the function type 
     *  \param rDomain this array describes the input parameters of this PdfFunction. If this
     *                 function has m input parameters, this array has to contain 2*m numbers
     *                 where each number describes either the lower or upper boundary of the input range.
     */
    void Init( EPdfFunctionType eType, const PdfArray & rDomain );

};

/** This class is a PdfExponentialFunction.
 */
class PODOFO_API PdfExponentialFunction : public PdfFunction {
public:
    /** Create a new PdfExponentialFunction object.
     *
     *  \param rDomain this array describes the input parameters of this PdfFunction. If this
     *                 function has m input parameters, this array has to contain 2*m numbers
     *                 where each number describes either the lower or upper boundary of the input range.
     *  \param rC0
     *  \param rC1
     *  \param dExponent
     *  \param pParent parent vector of objects
     */
    PdfExponentialFunction( const PdfArray & rDomain, const PdfArray & rC0, const PdfArray & rC1, double dExponent, PdfVecObjects* pParent );

    /** Create a new PdfExponentialFunction object.
     *
     *  \param rDomain this array describes the input parameters of this PdfFunction. If this
     *                 function has m input parameters, this array has to contain 2*m numbers
     *                 where each number describes either the lower or upper boundary of the input range.
     *  \param rC0
     *  \param rC1
     *  \param dExponent
     *  \param pParent parent document 
     */
    PdfExponentialFunction( const PdfArray & rDomain, const PdfArray & rC0, const PdfArray & rC1, double dExponent, PdfDocument* pParent );

private:
    /** Initialize this object.
     */
    void Init( const PdfArray & rC0, const PdfArray & rC1, double dExponent );

};

/** This class is a PdfStitchingFunction, i.e. a PdfFunction that combines
 *  more than one PdfFunction into one.
 *
 *  It combines several PdfFunctions that take 1 input parameter to
 *  a new PdfFunction taking again only 1 input parameter.
 */
class PODOFO_API PdfStitchingFunction : public PdfFunction {
public:
    /** Create a new PdfStitchingFunction object.
     *
     *  \param rlstFunctions a list of functions which are used to built up this function object
     *  \param rDomain this array describes the input parameters of this PdfFunction. If this
     *                 function has m input parameters, this array has to contain 2*m numbers
     *                 where each number describes either the lower or upper boundary of the input range.
     *  \param rBounds the bounds array
     *  \param rEncode the encode array
     *  \param pParent parent vector of objects
     *  
     */
    PdfStitchingFunction( const PdfFunction::List & rlstFunctions, const PdfArray & rDomain, const PdfArray & rBounds, const PdfArray & rEncode, PdfVecObjects* pParent );

    /** Create a new PdfStitchingFunction object.
     *
     *  \param rlstFunctions a list of functions which are used to built up this function object
     *  \param rDomain this array describes the input parameters of this PdfFunction. If this
     *                 function has m input parameters, this array has to contain 2*m numbers
     *                 where each number describes either the lower or upper boundary of the input range.
     *  \param rBounds the bounds array
     *  \param rEncode the encode array
     *  \param pParent parent document 
     *  
     */
    PdfStitchingFunction( const PdfFunction::List & rlstFunctions, const PdfArray & rDomain, const PdfArray & rBounds, const PdfArray & rEncode, PdfDocument* pParent );

private:
    /** Initialize this object.
     *
     *  \param rlstFunctions a list of functions which are used to built up this function object
     *  \param rBounds the bounds array
     *  \param rEncode the encode array
     */
    void Init( const PdfFunction::List & rlstFunctions, const PdfArray & rBounds, const PdfArray & rEncode );

};

};

#endif // _PDF_FUNCTION_H_

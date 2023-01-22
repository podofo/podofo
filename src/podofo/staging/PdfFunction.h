/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FUNCTION_H
#define PDF_FUNCTION_H

#include <podofo/main/PdfDeclarations.h>

#include <list>

#include <podofo/main/PdfElement.h>

namespace PoDoFo {

class PdfArray;

/**
 * The function type of a mathematical function in a PDF file.
 */
enum class PdfFunctionType
{
    Sampled = 0, ///< A sampled function (Type1)
    Exponential = 2, ///< An exponential interpolation function (Type2)
    Stitching = 3, ///< A stitching function (Type3)
    PostScript = 4  ///< A PostScript calculator function (Type4)
};

/**
 * This class defines a PdfFunction.
 * A function can be used in various ways in a PDF file.
 * Examples are device dependent rasterization for high quality
 * printing or color transformation functions for certain colorspaces.
 */
class PODOFO_API PdfFunction : public PdfDictionaryElement
{
public:
    using List = std::list<PdfFunction>;
    using Sample = std::list<char>;

protected:
    /** Create a new PdfFunction object.
     *
     *  \param functionType the function type
     *  \param domain this array describes the input parameters of this PdfFunction. If this
     *                 function has m input parameters, this array has to contain 2*m numbers
     *                 where each number describes either the lower or upper boundary of the input range.
     *  \param parent parent document
     *
     */
    PdfFunction(PdfDocument& doc, PdfFunctionType functionType, const PdfArray& domain);

private:
    /** Initialize this object.
     *
     *  \param functionType the function type
     *  \param domain this array describes the input parameters of this PdfFunction. If this
     *                 function has m input parameters, this array has to contain 2*m numbers
     *                 where each number describes either the lower or upper boundary of the input range.
     */
    void Init(PdfFunctionType functionType, const PdfArray& domain);

};

/** This class is a PdfSampledFunction.
 */
class PODOFO_API PdfSampledFunction : public PdfFunction
{
public:
    /** Create a new PdfSampledFunction object.
     *
     *  \param doc parent document
     *  \param domain this array describes the input parameters of this PdfFunction. If this
     *                 function has m input parameters, this array has to contain 2*m numbers
     *                 where each number describes either the lower or upper boundary of the input range.
     *  \param range  this array describes the output parameters of this PdfFunction. If this
     *                 function has n input parameters, this array has to contain 2*n numbers
     *                 where each number describes either the lower or upper boundary of the output range.
     *  \param samples a list of bytes which are used to build up this function sample data
     */
    PdfSampledFunction(PdfDocument& doc, const PdfArray& domain, const PdfArray& range, const PdfFunction::Sample& samples);

private:
    /** Initialize this object.
     */
    void Init(const PdfArray& domain, const PdfArray& range, const PdfFunction::Sample& samples);

};

/** This class is a PdfExponentialFunction.
 */
class PODOFO_API PdfExponentialFunction : public PdfFunction
{
public:
    /** Create a new PdfExponentialFunction object.
     *
     *  \param doc parent document
     *  \param domain this array describes the input parameters of this PdfFunction. If this
     *                 function has m input parameters, this array has to contain 2*m numbers
     *                 where each number describes either the lower or upper boundary of the input range.
     *  \param c0
     *  \param c1
     *  \param exponent
     */
    PdfExponentialFunction(PdfDocument& doc, const PdfArray& domain, const PdfArray& c0, const PdfArray& c1, double exponent);

private:
    /** Initialize this object.
     */
    void Init(const PdfArray& c0, const PdfArray& c1, double exponent);

};

/** This class is a PdfStitchingFunction, i.e. a PdfFunction that combines
 *  more than one PdfFunction into one.
 *
 *  It combines several PdfFunctions that take 1 input parameter to
 *  a new PdfFunction taking again only 1 input parameter.
 */
class PODOFO_API PdfStitchingFunction : public PdfFunction
{
public:
    /** Create a new PdfStitchingFunction object.
     *
     *  \param functions a list of functions which are used to built up this function object
     *  \param domain this array describes the input parameters of this PdfFunction. If this
     *                 function has m input parameters, this array has to contain 2*m numbers
     *                 where each number describes either the lower or upper boundary of the input range.
     *  \param bounds the bounds array
     *  \param encode the encode array
     *  \param parent parent document
     *
     */
    PdfStitchingFunction(PdfDocument& doc, const PdfFunction::List& functions, const PdfArray& domain, const PdfArray& bounds, const PdfArray& encode);

private:
    /** Initialize this object.
     *
     *  \param functions a list of functions which are used to built up this function object
     *  \param bounds the bounds array
     *  \param encode the encode array
     */
    void Init(const PdfFunction::List& functions, const PdfArray& bounds, const PdfArray& encode);

};

};

#endif // PDF_FUNCTION_H

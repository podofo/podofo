/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFunction.h"

#include <podofo/main/PdfArray.h>
#include <podofo/main/PdfDictionary.h>
#include <podofo/main/PdfObjectStream.h>

using namespace PoDoFo;

PdfFunction::PdfFunction(PdfDocument& doc, PdfFunctionType functionType, const PdfArray& domain)
    : PdfDictionaryElement(doc)
{
    Init(functionType, domain);
}

void PdfFunction::Init(PdfFunctionType functionType, const PdfArray& domain)
{
    this->GetObject().GetDictionary().AddKey("FunctionType", static_cast<int64_t>(functionType));
    this->GetObject().GetDictionary().AddKey("Domain", domain);

}

PdfSampledFunction::PdfSampledFunction(PdfDocument& doc, const PdfArray& domain, const PdfArray& range, const PdfFunction::Sample& samples)
    : PdfFunction(doc, PdfFunctionType::Sampled, domain)
{
    Init(domain, range, samples);
}

void PdfSampledFunction::Init(const PdfArray& domain, const PdfArray& range, const PdfFunction::Sample& samples)
{
    PdfArray Size;
    for (unsigned i = 0; i < domain.GetSize() / 2; i++)
        Size.Add(PdfObject(static_cast<int64_t>(domain.GetSize() / 2)));

    this->GetDictionary().AddKey("Domain", domain);
    this->GetDictionary().AddKey("Range", range);
    this->GetDictionary().AddKey("Size", Size);
    this->GetDictionary().AddKey("Order", static_cast<int64_t>(1));
    this->GetDictionary().AddKey("BitsPerSample", static_cast<int64_t>(8));

    auto& stream = this->GetObject().GetOrCreateStream();
    auto output = stream.GetOutputStream();
    for (char c : samples)
        output.Write(c);
}

PdfExponentialFunction::PdfExponentialFunction(PdfDocument& doc, const PdfArray& domain, const PdfArray& c0, const PdfArray& c1, double exponent)
    : PdfFunction(doc, PdfFunctionType::Exponential, domain)
{
    Init(c0, c1, exponent);
}

void PdfExponentialFunction::Init(const PdfArray& c0, const PdfArray& c1, double exponent)
{
    this->GetDictionary().AddKey("C0", c0);
    this->GetDictionary().AddKey("C1", c1);
    this->GetDictionary().AddKey("N", exponent);
}

PdfStitchingFunction::PdfStitchingFunction(PdfDocument& doc, const PdfFunction::List& functions, const PdfArray& domain, const PdfArray& bounds, const PdfArray& encode)
    : PdfFunction(doc, PdfFunctionType::Stitching, domain)
{
    Init(functions, bounds, encode);
}

void PdfStitchingFunction::Init(const PdfFunction::List& functions, const PdfArray& bounds, const PdfArray& encode)
{
    PdfArray functionsArr;
    functionsArr.reserve(functions.size());

    for (auto& fun : functions)
        functionsArr.Add(fun.GetObject().GetIndirectReference());

    this->GetObject().GetDictionary().AddKey("Functions", functionsArr);
    this->GetObject().GetDictionary().AddKey("Bounds", bounds);
    this->GetObject().GetDictionary().AddKey("Encode", encode);
}

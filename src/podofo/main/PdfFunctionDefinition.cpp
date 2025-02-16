/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFunctionDefinition.h"
#include "PdfFunction.h"
#include "PdfIndirectObjectList.h"
#include "PdfDictionary.h"
#include "PdfArray.h"

using namespace std;
using namespace PoDoFo;

PdfFunctionDefinition::~PdfFunctionDefinition() { }

PdfFunctionDefinition::PdfFunctionDefinition(PdfFunctionType type, vector<double>&& domain, vector<double>&& range)
    : m_Domain(std::move(domain)), m_Range(std::move(range)), m_Type(type)
{
    if (m_Domain.size() % 2 != 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidInput, "The domain range must be even");

    if (m_Range.size() % 2 != 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidInput, "The domain range must be even");
}

void PdfFunctionDefinition::FillExportDictionary(PdfDictionary& dict) const
{
    dict.AddKey("Domain"_n, PdfArray::FromReals((cspan<double>)m_Domain));
    if (m_Range.size() != 0)
        dict.AddKey("Domain"_n, PdfArray::FromReals((cspan<double>)m_Range));
    fillExportDictionary(dict);
}

unsigned PdfFunctionDefinition::GetInputCount() const
{
    return (unsigned)m_Domain.size() / 2;
}

unsigned PdfFunctionDefinition::GetOutputCount() const
{
    return (unsigned)m_Range.size() / 2;
}

PdfFunctionListInitializer::PdfFunctionListInitializer() { }

PdfFunctionListInitializer::PdfFunctionListInitializer(const PdfFunction& func)
    : m_ExpVar(func.GetObject().GetIndirectReference())
{
    m_Definitions.emplace_back(func.GetDefinitionPtr());
}

PdfFunctionListInitializer::PdfFunctionListInitializer(cspan<reference_wrapper<const PdfFunction>> funcs)
    : m_Definitions(funcs.size()), m_ExpVar(PdfArray())
{
    m_ExpVar.GetArray().reserve(funcs.size());
    for (unsigned i = 0; i < funcs.size(); i++)
        pushBack(funcs[i].get());
}

PdfFunctionListInitializer::PdfFunctionListInitializer(size_t reserveSize)
    : m_ExpVar(PdfArray())
{
    m_Definitions.reserve(reserveSize);
    m_ExpVar.GetArray().reserve(reserveSize);
}

void PdfFunctionListInitializer::pushBack(const PdfFunction& func)
{
    m_Definitions.emplace_back(func.GetDefinitionPtr());
    m_ExpVar.GetArray().Add(func.GetObject().GetIndirectReference());
}

vector<PdfFunctionDefinitionPtr> PdfFunctionListInitializer::Take(PdfVariant& expVar)
{
    expVar = std::move(m_ExpVar);
    return std::move(m_Definitions);
}

PdfSampledFunctionDefinition::PdfSampledFunctionDefinition(vector<unsigned> size, unsigned char bitsPerSample,
        vector<unsigned> samples, vector<double> domain, vector<double> range,
        PdfSampledFunctionOrder order, vector<double> encode, vector<double> decode) :
    PdfFunctionDefinition(PdfFunctionType::Sampled, std::move(domain), std::move(range)),
    m_BitsPerSample(bitsPerSample),
    m_Order(order),
    m_Size(std::move(size)),
    m_Samples(std::move(samples)),
    m_Encode(std::move(encode)),
    m_Decode(std::move(decode))
{
    if (m_Size.size() != GetInputCount())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidInput, "The size array size doesn't match the input count");

    unsigned sampleCount = 1;
    for (unsigned i = 0; i < m_Size.size(); i++)
        sampleCount *= m_Size[i];

    if (m_Samples.size() != sampleCount)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidInput, "The sample size must match what defined by the size argument");

    if (m_Encode.size() == 0)
    {
        m_Encode.resize(m_Size.size() * 2);
        for (unsigned i = 0; i < m_Size.size(); i++)
        {
            // Default value: [0 (Size0 -1) 0 (Size1 - 1) ... ]
            m_Encode[i * 2 + 0] = 0;
            m_Encode[i * 2 + 1] = m_Size[i] - 1;
        }
    }
    else if (m_Encode.size() != GetDomain().size())
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidInput, "The encode array size doesn't match the domain size");
    }

    if (m_Decode.size() == 0)
    {
        // Default value: same as the value of Range.
        m_Decode = GetRange();
    }
    else if (m_Decode.size() != GetRange().size())
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidInput, "The decode array size is invalid ior doesn't match the range size");
    }
}

unsigned PdfSampledFunctionDefinition::GetSampleCount() const
{
    return (unsigned)m_Samples.size();
}

void PdfSampledFunctionDefinition::fillExportDictionary(PdfDictionary& dict) const
{
    dict.AddKey("FunctionType"_n, static_cast<int64_t>(0));
    dict.AddKey("Size"_n, PdfArray::FromNumbers((cspan<unsigned>)m_Size));
    dict.AddKey("BitsPerSample"_n, static_cast<int64_t>(m_BitsPerSample));
    if (m_Order != PdfSampledFunctionOrder::Linear)
        dict.AddKey("Order"_n, static_cast<int64_t>(m_Order));

    for (unsigned i = 0; i < GetRange().size(); i++)
    {
        // Write /Encode if different than default
        if (m_Encode[i + 0] != 0)
            goto WriteEncode;
        if (m_Encode[i + 1] != GetRange()[i] - 1)
            goto WriteEncode;

        continue;

    WriteEncode:
        dict.AddKey("Encode"_n, PdfArray::FromReals((cspan<double>)m_Encode));
        break;
    }

    if (m_Decode != GetRange())
        dict.AddKey("Decode"_n, PdfArray::FromReals((cspan<double>)m_Decode));
}

PdfExponentialFunctionDefinition::PdfExponentialFunctionDefinition(double interpolationExponent, vector<double> domain,
        vector<double> c0, vector<double> c1, vector<double> range) :
    PdfFunctionDefinition(PdfFunctionType::Exponential, std::move(domain), std::move(range)), m_InterpolationExponent(interpolationExponent),
    m_C0(std::move(c0)),
    m_C1(std::move(c1))
{
    if (m_C0.size() == 0)
        m_C0.push_back(0);

    if (m_C0.size() == 0)
        m_C0.push_back(1);

    if (m_C0.size() != m_C1.size())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidInput, "The c0 array size must match c1");
}

unsigned PdfExponentialFunctionDefinition::GetOutputCount() const
{
    return (unsigned)m_C0.size();
}

void PdfExponentialFunctionDefinition::fillExportDictionary(PdfDictionary& dict) const
{
    dict.AddKey("FunctionType"_n, static_cast<int64_t>(2));
    dict.AddKey("N"_n, m_InterpolationExponent);
    if (m_C0.size() != 1 || m_C0[0] != 0)
        dict.AddKey("C0"_n, PdfArray::FromReals((cspan<double>)m_C0));

    if (m_C1.size() != 1 || m_C1[0] != 1)
        dict.AddKey("C1"_n, PdfArray::FromReals((cspan<double>)m_C1));
}

PdfStitchingFunctionDefinition::PdfStitchingFunctionDefinition(PdfFunctionListInitializer&& functions, vector<double> bounds,
        vector<double> encode, vector<double> domain, vector<double> range) :
    PdfFunctionDefinition(PdfFunctionType::Stitching, std::move(domain), std::move(range)),
    m_Bounds(std::move(bounds)),
    m_Encode(std::move(encode))
{
    m_Functions = functions.Take(m_functionsExpVar);

    if (m_Functions.size() == 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidInput, "Functions list must be non empty");

    if (m_Bounds.size() != m_Functions.size() - 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidInput, "Bounds list must be of size k − 1, where k is the number of input functions");

    if (m_Encode.size() != m_Functions.size() * 2)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidInput, "Encode list must be of size 2 * k, where k is the number of input functions");
}

PdfStitchingFunctionDefinition::PdfStitchingFunctionDefinition(vector<PdfFunctionDefinitionPtr>&& functions,
        vector<double>&& bounds, vector<double>&& encode,
        vector<double>&& domain, vector<double>&& range) :
    PdfFunctionDefinition(PdfFunctionType::Stitching, std::move(domain), std::move(range)),
    m_Functions(std::move(functions)),
    m_Bounds(std::move(bounds)),
    m_Encode(std::move(encode))
{
}

void PdfStitchingFunctionDefinition::fillExportDictionary(PdfDictionary& dict) const
{
    dict.AddKey("FunctionType"_n, static_cast<int64_t>(3));
    dict.AddKey("Bounds"_n, PdfArray::FromReals((cspan<double>)m_Bounds));
    dict.AddKey("Encode"_n, PdfArray::FromReals((cspan<double>)m_Encode));
    if (m_functionsExpVar.IsNull())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Not implemented serialize functions from raw definitions");

    dict.AddKey("Functions"_n, m_functionsExpVar);
}

PdfPostScriptCalculatorFunctionDefinition::PdfPostScriptCalculatorFunctionDefinition(vector<double> domain, vector<double> range)
    : PdfFunctionDefinition(PdfFunctionType::PostScriptCalculator, std::move(domain), std::move(range))
{
}

void PdfPostScriptCalculatorFunctionDefinition::fillExportDictionary(PdfDictionary& dictionary) const
{
    dictionary.AddKey("FunctionType"_n, static_cast<int64_t>(4));
}

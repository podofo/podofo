/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_FUNCTION_DEFINITION_H
#define PDF_FUNCTION_DEFINITION_H

#include "PdfEncodingCommon.h"
#include "PdfObject.h"

namespace PoDoFo
{
    class PdfIndirectObjectList;
    class PdfFunction;

    enum class PdfFunctionType : uint8_t
    {
        Unknown = 0,
        Sampled = 1,
        Exponential = 2,
        Stitching = 3,
        PostScriptCalculator = 4
    };

    class PODOFO_API PdfFunctionDefinition
    {
        friend class PdfFunction;
        friend class PdfSampledFunctionDefinition;
        friend class PdfExponentialFunctionDefinition;
        friend class PdfStitchingFunctionDefinition;
        friend class PdfPostScriptCalculatorFunctionDefinition;
    public:
        virtual ~PdfFunctionDefinition();
    private:
        PdfFunctionDefinition(PdfFunctionType type, std::vector<double>&& domain, std::vector<double>&& range);
    public:
        unsigned GetInputCount() const;
        virtual unsigned GetOutputCount() const;
        const std::vector<double>& GetDomain() const { return m_Domain; }
        const std::vector<double>& GetRange() const { return m_Range; }
        PdfFunctionType GetType() const { return m_Type; }
    protected:
        PdfFunctionDefinition(const PdfFunctionDefinition&) = default;
        virtual void fillExportDictionary(PdfDictionary& dict) const = 0;
    private:
        const PdfFunctionDefinition& operator=(const PdfFunctionDefinition&) = delete;
        void FillExportDictionary(PdfDictionary& dict) const;
    private:
        std::vector<double> m_Domain;
        std::vector<double> m_Range;
        PdfFunctionType m_Type;
    };

    /** Convenience alias for a constant PdfFunction shared ptr
     */
    using PdfFunctionDefinitionPtr = std::shared_ptr<const PdfFunctionDefinition>;

    class PODOFO_API PdfFunctionListInitializer final
    {
        friend class PdfStitchingFunctionDefinition;
        friend class PdfShadingDefinition;

    public:
        PdfFunctionListInitializer();

        PdfFunctionListInitializer(const PdfFunction& func);

        PdfFunctionListInitializer(cspan<std::reference_wrapper<const PdfFunction>> funcs);

        template <typename... Args>
        PdfFunctionListInitializer(const PdfFunction& func, const Args& ...more);

    private:
        PdfFunctionListInitializer(size_t reserveSize);

        void pushBack(const PdfFunction& func);

        PdfFunctionListInitializer(const PdfFunctionListInitializer&) = delete;
        PdfFunctionListInitializer& operator=(const PdfFunctionListInitializer&) = delete;

    private:
        std::vector<PdfFunctionDefinitionPtr> Take(PdfVariant& expVar);

    private:
        std::vector<PdfFunctionDefinitionPtr> m_Definitions;
        PdfVariant m_ExpVar;
    };

    enum class PdfSampledFunctionOrder : uint8_t
    {
        Linear = 1,
        Cubic = 3,
    };

    class PODOFO_API PdfSampledFunctionDefinition final : public PdfFunctionDefinition
    {
    public:
        PdfSampledFunctionDefinition(std::vector<unsigned> size, unsigned char bitsPerSample, std::vector<unsigned> samples,
            std::vector<double> domain, std::vector<double> range,
            PdfSampledFunctionOrder order = PdfSampledFunctionOrder::Linear,
            std::vector<double> encode = { }, std::vector<double> decode = { });

        PdfSampledFunctionDefinition(const PdfSampledFunctionDefinition&) = default;
    public:
        unsigned GetSampleCount() const;
        unsigned char GetBitsPerSample() const { return m_BitsPerSample; }
        PdfSampledFunctionOrder GetOrder() const { return m_Order; }
        const std::vector<unsigned>& GetSize() const { return m_Size; }
        const std::vector<unsigned>& GetSamples() const { return m_Samples; }
        const std::vector<double>& GetEncode() const { return m_Encode; }
        const std::vector<double>& GetDecode() const { return m_Decode; }
    protected:
        void fillExportDictionary(PdfDictionary& dict) const override;
    private:
        unsigned char m_BitsPerSample;
        PdfSampledFunctionOrder m_Order;
        std::vector<unsigned> m_Size;
        std::vector<unsigned> m_Samples;
        std::vector<double> m_Encode;
        std::vector<double> m_Decode;
    };

    class PODOFO_API PdfExponentialFunctionDefinition final : public PdfFunctionDefinition
    {
    public:
        /**
         * \param c0 an array of n numbers that shall define the function result when x = 0.0
         * \param c1 an array of n numbers that shall define the function result when x = 1.0
         */
        PdfExponentialFunctionDefinition(double interpolationExponent, std::vector<double> domain,
            std::vector<double> c0 = { },
            std::vector<double> c1 = { },
            std::vector<double> range = { });

        PdfExponentialFunctionDefinition(const PdfExponentialFunctionDefinition&) = default;
    public:
        unsigned GetOutputCount() const override;
        double GetInterpolationExponent() const { return m_InterpolationExponent; }
        const std::vector<double>& GetC0() const { return m_C0; }
        const std::vector<double>& GetC1() const { return m_C1; }
    protected:
        void fillExportDictionary(PdfDictionary& dict) const override;
    private:
        double m_InterpolationExponent;
        std::vector<double> m_C0;
        std::vector<double> m_C1;
    };

    class PODOFO_API PdfStitchingFunctionDefinition final : public PdfFunctionDefinition
    {
    public:
        PdfStitchingFunctionDefinition(PdfFunctionListInitializer&& functions,
            std::vector<double> bounds, std::vector<double> encode,
            std::vector<double> domain, std::vector<double> range = { });

        /**
         * \remarks Deserialization constructor
         */
        PdfStitchingFunctionDefinition(std::vector<PdfFunctionDefinitionPtr>&& functions,
            std::vector<double>&& bounds, std::vector<double>&& encode,
            std::vector<double>&& domain, std::vector<double>&& range);

        PdfStitchingFunctionDefinition(const PdfStitchingFunctionDefinition&) = default;

    public:
        const std::vector<std::shared_ptr<const PdfFunctionDefinition>>& GetFunctions() const { return m_Functions; }
        const std::vector<double>& GetBounds() const { return m_Bounds; }
        const std::vector<double>& GetEncode() const { return m_Encode; }
    protected:
        void fillExportDictionary(PdfDictionary& dict) const override;
    private:
        std::vector<std::shared_ptr<const PdfFunctionDefinition>> m_Functions;
        PdfVariant m_functionsExpVar;
        std::vector<double> m_Bounds;
        std::vector<double> m_Encode;
    };

    class PODOFO_API PdfPostScriptCalculatorFunctionDefinition final : public PdfFunctionDefinition
    {
    public:
        PdfPostScriptCalculatorFunctionDefinition(std::vector<double> domain, std::vector<double> range);

        PdfPostScriptCalculatorFunctionDefinition(const PdfPostScriptCalculatorFunctionDefinition&) = default;
    protected:
        void fillExportDictionary(PdfDictionary& dict) const override;
    };

    template<typename ...Args>
    PdfFunctionListInitializer::PdfFunctionListInitializer(const PdfFunction& func, const Args& ...more)
        : PdfFunctionListInitializer(1 + sizeof...(more))
    {
        pushBack(func);
        (pushBack(more), ...);
    }
}

#endif // PDF_FUNCTION_DEFINITION_H

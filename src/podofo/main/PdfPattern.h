// SPDX-FileCopyrightText: 2022 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#ifndef PDF_PATTERN_H
#define PDF_PATTERN_H

#include "PdfCanvas.h"
#include "PdfPatternDefinition.h"

namespace PoDoFo
{
    class PODOFO_API PdfPattern : public PdfDictionaryElement
    {
        friend class PdfTilingPattern;
        friend class PdfShadingPattern;
    private:
        PdfPattern(PdfDocument& doc, PdfPatternDefinitionPtr&& definition);
        PdfPattern(PdfObject& obj, PdfPatternDefinitionPtr&& definition);

    public:
        static bool TryCreateFromObject(PdfObject& obj, std::unique_ptr<PdfPattern>& pattern);

        static bool TryCreateFromObject(const PdfObject& obj, std::unique_ptr<const PdfPattern>& pattern);

        template <typename PatternT>
        static bool TryCreateFromObject(PdfObject& obj, std::unique_ptr<PatternT>& pattern);

        template <typename PatternT>
        static bool TryCreateFromObject(const PdfObject& obj, std::unique_ptr<const PatternT>& pattern);

        PdfPatternType GetType() const { return m_Definition->GetType(); }
        const PdfPatternDefinition& GetDefinition() const { return *m_Definition; }
        PdfPatternDefinitionPtr GetDefinitionPtr() const { return m_Definition; }

    private:
        static PdfPattern* createFromObject(const PdfObject& obj, PdfPatternType reqType, PdfPatternType& detectedType);
        template <typename TPattern>
        static constexpr PdfPatternType GetPatternType();

    protected:
        PdfPatternDefinitionPtr m_Definition;
    };

    class PODOFO_API PdfTilingPattern : public PdfPattern, public PdfCanvas
    {
        friend class PdfColouredTilingPattern;
        friend class PdfUncolouredTilingPattern;

    private:
        PdfTilingPattern(PdfDocument& doc, std::shared_ptr<PdfTilingPatternDefinition>&& definition);
        PdfTilingPattern(PdfObject& obj, PdfPatternDefinitionPtr&& definition);

    public:
        const PdfTilingPatternDefinition& GetDefinition() const;
        std::shared_ptr<const PdfTilingPatternDefinition> GetDefinitionPtr() const;
        inline PdfResources* GetResources() { return m_Resources.get(); }
        inline const PdfResources* GetResources() const { return m_Resources.get(); }

    private:
        PdfObjectStream& GetOrCreateContentsStream(PdfStreamAppendFlags flags) override;
        PdfResources& GetOrCreateResources() override;
        PdfObjectStream& ResetContentsStream() override;
        void CopyContentsTo(OutputStream& stream) const override;
        Corners GetRectRaw() const override;
        bool TryGetRotationRadians(double& teta) const override;
        PdfObject* getContentsObject() override;
        PdfResources* getResources() override;
        PdfDictionaryElement& getElement() override;

    private:
        // Remove some PdfCanvas methods to maintain the class API surface clean
        PdfElement& GetElement() = delete;
        const PdfElement& GetElement() const = delete;
        PdfObject* GetContentsObject() = delete;
        const PdfObject* GetContentsObject() const = delete;

    private:
        std::unique_ptr<PdfResources> m_Resources;
    };

    class PODOFO_API PdfColouredTilingPattern final : public PdfTilingPattern
    {
        friend class PdfDocument;
        friend class PdfPattern;

    private:
        PdfColouredTilingPattern(PdfDocument& doc, std::shared_ptr<PdfColouredTilingPatternDefinition>&& definition);
        PdfColouredTilingPattern(PdfObject& obj);

    public:
        const PdfColouredTilingPatternDefinition& GetDefinition() const;
        std::shared_ptr<const PdfColouredTilingPatternDefinition> GetDefinitionPtr() const;
    };

    class PODOFO_API PdfUncolouredTilingPattern final : public PdfTilingPattern
    {
        friend class PdfDocument;
        friend class PdfPattern;

    private:
        PdfUncolouredTilingPattern(PdfDocument& doc, std::shared_ptr<PdfUncolouredTilingPatternDefinition>&& definition);
        PdfUncolouredTilingPattern(PdfObject& obj);

    public:
        const PdfUncolouredTilingPatternDefinition& GetDefinition() const;
        std::shared_ptr<const PdfUncolouredTilingPatternDefinition> GetDefinitionPtr() const;
    };

    class PODOFO_API PdfShadingPattern final : public PdfPattern
    {
        friend class PdfDocument;
        friend class PdfPattern;

    private:
        PdfShadingPattern(PdfDocument& doc, PdfShadingPatternDefinitionPtr&& definition);
        PdfShadingPattern(PdfObject& obj);

    public:
        const PdfShadingPatternDefinition& GetDefinition() const;
        PdfShadingPatternDefinitionPtr GetDefinitionPtr() const;
    };

    class PODOFO_API PdfShadingDictionary final : public PdfDictionaryElement
    {
        friend class PdfDocument;

    private:
        PdfShadingDictionary(PdfDocument& doc, PdfShadingDefinitionPtr&& definition);

    public:
        const PdfShadingDefinition& GetDefinition() const { return *m_Definition; }
        PdfShadingDefinitionPtr GetDefinitionPtr() const { return m_Definition; }

    private:
        PdfShadingDefinitionPtr m_Definition;
    };

    template<typename PatternT>
    inline bool PdfPattern::TryCreateFromObject(PdfObject& obj, std::unique_ptr<PatternT>& pattern)
    {
        PdfPatternType detectedType;
        pattern.reset((PatternT*)createFromObject(obj, GetPatternType<PatternT>(), detectedType));
        return pattern != nullptr;
    }

    template<typename PatternT>
    inline bool PdfPattern::TryCreateFromObject(const PdfObject& obj, std::unique_ptr<const PatternT>& pattern)
    {
        PdfPatternType detectedType;
        pattern.reset((const PatternT*)createFromObject(obj, GetPatternType<PatternT>(), detectedType));
        return pattern != nullptr;
    }

    template<typename TPattern>
    constexpr PdfPatternType PdfPattern::GetPatternType()
    {
        if (std::is_same_v<TPattern, PdfColouredTilingPattern>)
            return PdfPatternType::Tiling;
        else if (std::is_same_v<TPattern, PdfUncolouredTilingPattern>)
            return PdfPatternType::Tiling;
        else if (std::is_same_v<TPattern, PdfShadingPattern>)
            return PdfPatternType::Shading;
        else
            return PdfPatternType::Unknown;
    }
}

#endif // PDF_PATTERN_H

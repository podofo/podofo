/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

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

    public:
        const PdfPatternDefinition& GetDefinition() const { return *m_Definition; }
        PdfPatternDefinitionPtr GetDefinitionPtr() const { return m_Definition; }
    protected:
        PdfPatternDefinitionPtr m_Definition;
    };

    class PODOFO_API PdfTilingPattern : public PdfPattern, public PdfCanvas
    {
        friend class PdfColouredTilingPattern;
        friend class PdfUncolouredTilingPattern;

    private:
        PdfTilingPattern(PdfDocument& doc, std::shared_ptr<PdfTilingPatternDefinition>&& definition);

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

    private:
        PdfColouredTilingPattern(PdfDocument& doc, std::shared_ptr<PdfColouredTilingPatternDefinition>&& definition);

    public:
        const PdfColouredTilingPatternDefinition& GetDefinition() const;
        std::shared_ptr<const PdfColouredTilingPatternDefinition> GetDefinitionPtr() const;
    };

    class PODOFO_API PdfUncolouredTilingPattern final : public PdfTilingPattern
    {
        friend class PdfDocument;

    private:
        PdfUncolouredTilingPattern(PdfDocument& doc, std::shared_ptr<PdfUncolouredTilingPatternDefinition>&& definition);

    public:
        const PdfUncolouredTilingPatternDefinition& GetDefinition() const;
        std::shared_ptr<const PdfUncolouredTilingPatternDefinition> GetDefinitionPtr() const;
    };

    class PODOFO_API PdfShadingPattern final : public PdfPattern
    {
        friend class PdfDocument;

    private:
        PdfShadingPattern(PdfDocument& doc, PdfShadingPatternDefinitionPtr&& definition);

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
}

#endif // PDF_PATTERN_H

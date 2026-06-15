// SPDX-FileCopyrightText: 2022 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0


#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfPattern.h"
#include "PdfDocument.h"

using namespace std;
using namespace PoDoFo;

static PdfPatternType getPdfPatternType(const PdfDictionary& dict);

PdfPattern::PdfPattern(PdfDocument& doc, PdfPatternDefinitionPtr&& definition)
    : PdfDictionaryElement(doc, "Pattern"_n), m_Definition(std::move(definition))
{
    m_Definition->FillExportDictionary(GetDictionary());
}

PdfPattern::PdfPattern(PdfObject& obj, PdfPatternDefinitionPtr&& definition)
    : PdfDictionaryElement(obj), m_Definition(std::move(definition))
{
}

bool PdfPattern::TryCreateFromObject(PdfObject& obj, unique_ptr<PdfPattern>& pattern)
{
    PdfPatternType detectedType;
    pattern.reset(createFromObject(obj, PdfPatternType::Unknown, detectedType));
    return pattern != nullptr;
}

bool PdfPattern::TryCreateFromObject(const PdfObject& obj, unique_ptr<const PdfPattern>& pattern)
{
    PdfPatternType detectedType;
    pattern.reset(createFromObject(obj, PdfPatternType::Unknown, detectedType));
    return pattern != nullptr;
}

PdfPattern* PdfPattern::createFromObject(const PdfObject& obj, PdfPatternType reqType, PdfPatternType& detectedType)
{
    const PdfDictionary* dict;
    if (!obj.TryGetDictionary(dict)
        || (detectedType = getPdfPatternType(*dict)) == PdfPatternType::Unknown
        || (reqType != PdfPatternType::Unknown && detectedType != reqType))
    {
        return nullptr;
    }

    switch (detectedType)
    {
        case PdfPatternType::Tiling:
        {
            int64_t paintType;
            if (!dict->TryFindKeyAs("PaintType", paintType))
                return nullptr;

            if (paintType == static_cast<int64_t>(PdfTilingPaintType::Coloured))
                return new PdfColouredTilingPattern(const_cast<PdfObject&>(obj));
            else if (paintType == static_cast<int64_t>(PdfTilingPaintType::Uncoloured))
                return new PdfUncolouredTilingPattern(const_cast<PdfObject&>(obj));
            else
                return nullptr;
        }
        case PdfPatternType::Shading:
            return new PdfShadingPattern(const_cast<PdfObject&>(obj));
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
    }
}

PdfTilingPattern::PdfTilingPattern(PdfDocument& doc, shared_ptr<PdfTilingPatternDefinition>&& definition)
    : PdfPattern(doc, std::move(definition)), m_Resources(new PdfResources(*this))
{
}

PdfTilingPattern::PdfTilingPattern(PdfObject& obj, PdfPatternDefinitionPtr&& definition)
    : PdfPattern(obj, std::move(definition))
{
    auto resources = obj.GetDictionary().FindKey("Resources");
    if (resources != nullptr)
        m_Resources.reset(new PdfResources(*resources));
}

PdfObjectStream& PdfTilingPattern::GetOrCreateContentsStream(PdfStreamAppendFlags flags)
{
    (void)flags; // Flags have no use here
    return GetObject().GetOrCreateStream();
}

PdfResources& PdfTilingPattern::GetOrCreateResources()
{
    if (m_Resources != nullptr)
        return *m_Resources;

    m_Resources.reset(new PdfResources(*this));
    return *m_Resources;
}

PdfObjectStream& PdfTilingPattern::ResetContentsStream()
{
    auto& ret = GetObject().GetOrCreateStream();
    ret.Clear();
    return ret;
}

void PdfTilingPattern::CopyContentsTo(OutputStream& stream) const
{
    auto objStream = GetObject().GetStream();
    if (objStream == nullptr)
        return;

    objStream->CopyTo(stream);
}

Corners PdfTilingPattern::GetRectRaw() const
{
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

bool PdfTilingPattern::TryGetRotationRadians(double& teta) const
{
    teta = 0;
    return false;
}

PdfObject* PdfTilingPattern::getContentsObject()
{
    return &GetObject();
}

PdfResources* PdfTilingPattern::getResources()
{
    return m_Resources.get();
}

PdfDictionaryElement& PdfTilingPattern::getElement()
{
    return *this;
}

const PdfTilingPatternDefinition& PdfTilingPattern::GetDefinition() const
{
    return static_cast<const PdfTilingPatternDefinition&>(*m_Definition);
}

shared_ptr<const PdfTilingPatternDefinition> PdfTilingPattern::GetDefinitionPtr() const
{
    return std::static_pointer_cast<const PdfTilingPatternDefinition>(m_Definition);
}

PdfShadingPattern::PdfShadingPattern(PdfDocument& doc, PdfShadingPatternDefinitionPtr&& definition)
    : PdfPattern(doc, std::move(definition)) { }

PdfShadingPattern::PdfShadingPattern(PdfObject& obj)
    : PdfPattern(obj, PdfPatternDefinitionPtr(new PdfShadingPatternDefinition(obj.GetDictionary()))) { }

const PdfShadingPatternDefinition& PdfShadingPattern::GetDefinition() const
{
    return static_cast<const PdfShadingPatternDefinition&>(*m_Definition);
}

shared_ptr<const PdfShadingPatternDefinition> PdfShadingPattern::GetDefinitionPtr() const
{
    return std::static_pointer_cast<const PdfShadingPatternDefinition>(m_Definition);
}

PdfColouredTilingPattern::PdfColouredTilingPattern(PdfDocument& doc, shared_ptr<PdfColouredTilingPatternDefinition>&& definition)
    : PdfTilingPattern(doc, std::move(definition))
{
}

PdfColouredTilingPattern::PdfColouredTilingPattern(PdfObject& obj)
    : PdfTilingPattern(obj, PdfPatternDefinitionPtr(new PdfColouredTilingPatternDefinition(obj.GetDictionary())))
{
}

const PdfColouredTilingPatternDefinition& PdfColouredTilingPattern::GetDefinition() const
{
    return static_cast<const PdfColouredTilingPatternDefinition&>(*m_Definition);
}

shared_ptr<const PdfColouredTilingPatternDefinition> PdfColouredTilingPattern::GetDefinitionPtr() const
{
    return std::static_pointer_cast<const PdfColouredTilingPatternDefinition>(m_Definition);
}

PdfUncolouredTilingPattern::PdfUncolouredTilingPattern(PdfDocument& doc, shared_ptr<PdfUncolouredTilingPatternDefinition>&& definition)
    : PdfTilingPattern(doc, std::move(definition))
{
}

PdfUncolouredTilingPattern::PdfUncolouredTilingPattern(PdfObject& obj)
    : PdfTilingPattern(obj, PdfPatternDefinitionPtr(new PdfUncolouredTilingPatternDefinition(obj.GetDictionary())))
{
}

const PdfUncolouredTilingPatternDefinition& PdfUncolouredTilingPattern::GetDefinition() const
{
    return static_cast<const PdfUncolouredTilingPatternDefinition&>(*m_Definition);
}

shared_ptr<const PdfUncolouredTilingPatternDefinition> PdfUncolouredTilingPattern::GetDefinitionPtr() const
{
    return std::static_pointer_cast<const PdfUncolouredTilingPatternDefinition>(m_Definition);
}

PdfShadingDictionary::PdfShadingDictionary(PdfDocument& doc, PdfShadingDefinitionPtr&& definition)
    : PdfDictionaryElement(doc), m_Definition(std::move(definition))
{
    if (m_Definition == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Invalid null definition");

    m_Definition->FillExportDictionary(GetDictionary());
}

PdfPatternType getPdfPatternType(const PdfDictionary& dict)
{
    int64_t patternType;
    if (!dict.TryFindKeyAs("PatternType", patternType))
        return PdfPatternType::Unknown;

    switch (patternType)
    {
        case static_cast<int64_t>(PdfPatternType::Tiling):
            return PdfPatternType::Tiling;
        case static_cast<int64_t>(PdfPatternType::Shading):
            return PdfPatternType::Shading;
        default:
            return PdfPatternType::Unknown;
    }
}

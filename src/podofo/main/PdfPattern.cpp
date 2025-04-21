/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */


#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfPattern.h"
#include "PdfDocument.h"

using namespace std;
using namespace PoDoFo;

PdfPattern::PdfPattern(PdfDocument& doc, PdfPatternDefinitionPtr&& definition)
    : PdfDictionaryElement(doc, "Pattern"_n), m_Definition(std::move(definition))
{
    m_Definition->FillExportDictionary(GetDictionary());
}

PdfTilingPattern::PdfTilingPattern(PdfDocument& doc, shared_ptr<PdfTilingPatternDefinition>&& definition)
    : PdfPattern(doc, std::move(definition)), m_Resources(new PdfResources(*this))
{
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

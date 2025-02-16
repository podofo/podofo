/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfPatternDefinition.h"
#include "PdfFunction.h"
#include "PdfIndirectObjectList.h"
#include "PdfDictionary.h"
#include "PdfPattern.h"

using namespace std;
using namespace PoDoFo;

PdfPatternDefinition::PdfPatternDefinition(nullable<const Matrix&> matrix)
{
    if (matrix.has_value())
        m_Matrix = *matrix;
}

PdfPatternDefinition::~PdfPatternDefinition() { }

void PdfPatternDefinition::FillExportDictionary(PdfDictionary& dict) const
{
    dict.AddKey("Type"_n, "Pattern"_n);
    dict.AddKey("PatternType"_n, static_cast<int64_t>(GetType()));
    if (m_Matrix != Matrix::Identity)
    {
        PdfArray arr;
        m_Matrix.ToArray(arr);
        dict.AddKey("Matrix"_n, std::move(arr));
    }
    fillExportDictionary(dict);
}

PdfTilingPatternDefinition::PdfTilingPatternDefinition(PdfTilingSpacingType spacingType,
        const Rect& bbox, double xStep, double yStep, nullable<const Matrix&> matrix) :
    PdfPatternDefinition(matrix),
    m_SpacingType(spacingType),
    m_BBox(bbox),
    m_XStep(xStep),
    m_YStep(yStep)
{
}

PdfPatternType PdfTilingPatternDefinition::GetType() const
{
    return PdfPatternType::Tiling;
}

void PdfTilingPatternDefinition::fillExportDictionary(PdfDictionary& dict) const
{
    dict.AddKey("TilingType"_n, static_cast<int64_t>(m_SpacingType));
    dict.AddKey("PaintType"_n, static_cast<int64_t>(GetPaintType()));
    PdfArray arr;
    m_BBox.ToArray(arr);
    dict.AddKey("BBox"_n, std::move(arr));
    dict.AddKey("XStep"_n, m_XStep);
    dict.AddKey("YStep"_n, m_YStep);
}

PdfShadingPatternDefinition::PdfShadingPatternDefinition(const PdfShadingDictionary& shading,
		nullable<const Matrix&> matrix, nullable<const PdfExtGState&> extGState) :
	PdfPatternDefinition(matrix),
	m_Shading(shading.GetDefinitionPtr()),
	m_shadingExpRef(shading.GetObject().GetIndirectReference()),
	m_ExtGState(extGState == nullptr ? nullptr : extGState->GetDefinitionPtr()),
	m_extGStateExpRef(extGState == nullptr ? PdfReference() : extGState->GetObject().GetIndirectReference())
{
}

PdfShadingPatternDefinition::PdfShadingPatternDefinition(PdfShadingDefinitionPtr&& shading, const Matrix& matrix,
		PdfExtGStateDefinitionPtr&& extGState) :
    PdfPatternDefinition(matrix),
    m_Shading(std::move(shading)),
    m_ExtGState(std::move(extGState))
{
}

void PdfShadingPatternDefinition::fillExportDictionary(PdfDictionary& dict) const
{
    if (m_extGStateExpRef.IsIndirect())
        dict.AddKey("ExtGState"_n, m_extGStateExpRef);

    if (!m_shadingExpRef.IsIndirect())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Not supported serializing from null shading reference");

    dict.AddKey("Shading"_n, m_shadingExpRef);

}

PdfPatternType PdfShadingPatternDefinition::GetType() const
{
    return PdfPatternType::Shading;
}

PdfColouredTilingPatternDefinition::PdfColouredTilingPatternDefinition(PdfTilingSpacingType spacingType,
        const Rect& bbox, double xStep, double yStep, nullable<const Matrix&> matrix) :
    PdfTilingPatternDefinition(spacingType, bbox, xStep, yStep, matrix)
{
}

PdfTilingPaintType PdfColouredTilingPatternDefinition::GetPaintType() const
{
    return PdfTilingPaintType::Coloured;
}

PdfUncolouredTilingPatternDefinition::PdfUncolouredTilingPatternDefinition(PdfTilingSpacingType spacingType,
        const Rect& bbox, double xStep, double yStep, nullable<const Matrix&> matrix) :
    PdfTilingPatternDefinition(spacingType, bbox, xStep, yStep, matrix)
{
}

PdfTilingPaintType PdfUncolouredTilingPatternDefinition::GetPaintType() const
{
    return PdfTilingPaintType::Uncoloured;
}

PdfShadingDefinition::PdfShadingDefinition(PdfColorSpaceInitializer&& colorSpace, PdfFunctionListInitializer&& functions,
        nullable<const PdfColorRaw&> background, nullable<const Rect&> bbox, bool antiAlias) :
    m_AntiAlias(antiAlias)
{
    m_ColorSpace = colorSpace.Take(m_colorSpaceExpVar);
    if (m_ColorSpace == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The function color space must be not null");

    m_Functions = functions.Take(m_functionsExpVar);

    if (background != nullptr)
        m_Background = *background;

    if (bbox != nullptr)
        m_BBox = *bbox;
}

PdfShadingDefinition::PdfShadingDefinition(PdfColorSpaceFilterPtr&& colorSpace, vector<PdfFunctionDefinitionPtr>&& functions,
        nullable<const PdfColorRaw&> background, const Rect& bbox, bool antiAlias) :
    m_ColorSpace(std::move(colorSpace)),
    m_Functions(std::move(functions)),
    m_BBox(bbox),
    m_AntiAlias(antiAlias)
{
    if (background != nullptr)
        m_Background = *background;
}

PdfShadingDefinition::~PdfShadingDefinition() {}

void PdfShadingDefinition::FillExportDictionary(PdfDictionary& dict) const
{
    dict.AddKey("ShadingType", static_cast<int64_t>(GetShadingType()));

    if (m_colorSpaceExpVar.IsNull())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Unsupported serializing null /ColorSpace");

    if (m_Functions.size() != 0)
    {
        // NOTE: Functions may be optional
        if (m_functionsExpVar.IsNull())
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Unsupported serializing undefined /Function");

        dict.AddKey("Function", m_functionsExpVar);
    }

    dict.AddKey("ColorSpace", m_colorSpaceExpVar);

    if (m_BBox.IsValid())
        dict.AddKey("BBox", m_BBox.ToArray());

    if (m_Background != nullptr)
        dict.AddKey("Background", PdfArray::FromReals(cspan<double>(*m_Background)));

    if (m_AntiAlias)
        dict.AddKey("AntiAlias", true);

    fillExportDictionary(dict);
}

PdfFunctionBasedShadingDefinition::PdfFunctionBasedShadingDefinition(
        PdfColorSpaceInitializer&& colorSpace, PdfFunctionListInitializer&& functions,
        nullable<const array<double, 4>&> domain, nullable<const Matrix&> matrix,
        nullable<const PdfColorRaw&> background, nullable<const Rect&> bbox, bool antiAlias) :
    PdfShadingDefinition(std::move(colorSpace), std::move(functions), background, bbox, antiAlias),
    m_Domain(domain == nullptr ? array<double, 4>{ 0, 1, 0, 1 } : *domain)
{
    if (m_Functions.size() == 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidInput, "Functions must be non empty");

    if (matrix != nullptr)
        m_Matrix = *matrix;
}

PdfFunctionBasedShadingDefinition::PdfFunctionBasedShadingDefinition(
        PdfColorSpaceFilterPtr&& colorSpace, vector<PdfFunctionDefinitionPtr>&& functions,
        const array<double, 4>& domain, const Matrix& matrix,
        const PdfColorRaw& background, const Rect& bbox, bool antiAlias) :
    PdfShadingDefinition(std::move(colorSpace), std::move(functions), background, bbox, antiAlias),
    m_Domain(domain),
    m_Matrix(matrix)
{
}

void PdfFunctionBasedShadingDefinition::fillExportDictionary(PdfDictionary& dict) const
{
    if (m_Domain != array<double, 4>{ 0, 1, 0, 1})
        dict.AddKey("Domain", PdfArray::FromReals(cspan<double>(m_Domain)));

    if (m_Matrix != Matrix::Identity)
        dict.AddKey("Matrix", m_Matrix.ToArray());
}

PdfShadingType PdfFunctionBasedShadingDefinition::GetShadingType() const
{
    return PdfShadingType::FunctionBased;
}

PdfAxialShadingDefinition::PdfAxialShadingDefinition(
        PdfColorSpaceInitializer&& colorSpace, PdfFunctionListInitializer&& functions,
        const array<double, 4>& coords, nullable<const array<bool, 2>&> extend, nullable<const array<double, 2>&> domain,
        nullable<const PdfColorRaw&> background, nullable<const Rect&> bbox, bool antiAlias) :
    PdfShadingDefinition(std::move(colorSpace), std::move(functions), background, bbox, antiAlias),
    m_Coords(coords),
    m_Extend(extend == nullptr ? array<bool, 2>{ false, false } : *extend),
    m_Domain(domain == nullptr ? array<double, 2>{ 0, 1 } : *domain)
{
    if (m_Functions.size() == 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidInput, "Functions must be non empty");
}

PdfAxialShadingDefinition::PdfAxialShadingDefinition(
        PdfColorSpaceFilterPtr&& colorSpace, vector<PdfFunctionDefinitionPtr>&& functions,
        const array<double, 4>& coords, const array<bool, 2>& extend,
        const array<double, 2> domain, const PdfColorRaw& background,
        const Rect& bbox, bool antiAlias) :
    PdfShadingDefinition(std::move(colorSpace), std::move(functions), background, bbox, antiAlias),
    m_Coords(coords),
    m_Extend(extend),
    m_Domain(domain)
{
}

void PdfAxialShadingDefinition::fillExportDictionary(PdfDictionary& dict) const
{
    dict.AddKey("Coords", PdfArray::FromReals(cspan<double>(m_Coords)));
    if (m_Domain != array<double, 2>{ 0, 1 })
        dict.AddKey("Domain", PdfArray::FromReals(cspan<double>(m_Domain)));

    if (m_Extend != array<bool, 2>{ false, false })
        dict.AddKey("Extend", PdfArray::FromBools(cspan<bool>(m_Extend)));
}

PdfShadingType PdfAxialShadingDefinition::GetShadingType() const
{
    return PdfShadingType::Axial;
}

PdfRadialShadingDefinition::PdfRadialShadingDefinition(
        PdfColorSpaceInitializer&& colorSpace, PdfFunctionListInitializer&& functions,
        const array<double, 6>& coords, nullable<const array<bool, 2>&> extend, nullable<const array<double, 2>&> domain,
        nullable<const PdfColorRaw&> background, nullable<const Rect&> bbox, bool antiAlias) :
    PdfShadingDefinition(std::move(colorSpace), std::move(functions), background, bbox, antiAlias),
    m_Coords(coords),
    m_Extend(extend == nullptr ? array<bool, 2>{ false, false } : *extend),
    m_Domain(domain == nullptr ? array<double, 2>{ 0, 1 } : *domain)
{
    if (m_Functions.size() == 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidInput, "Functions must be non empty");
}

PdfRadialShadingDefinition::PdfRadialShadingDefinition(
        PdfColorSpaceFilterPtr&& colorSpace, vector<PdfFunctionDefinitionPtr>&& functions,
        const array<double, 6>& coords, const array<bool, 2>& extend,
        const array<double, 2>& domain, const PdfColorRaw& background,
        const Rect& bbox, bool antiAlias) :
    PdfShadingDefinition(std::move(colorSpace), std::move(functions), background, bbox, antiAlias),
    m_Coords(coords),
    m_Extend(extend),
    m_Domain(domain)
{
}

void PdfRadialShadingDefinition::fillExportDictionary(PdfDictionary& dict) const
{
    dict.AddKey("Coords", PdfArray::FromReals(cspan<double>(m_Coords)));
    if (m_Domain != array<double, 2>{ 0, 1 })
        dict.AddKey("Domain", PdfArray::FromReals(cspan<double>(m_Domain)));

    if (m_Extend != array<bool, 2>{ false, false })
        dict.AddKey("Extend", PdfArray::FromBools(cspan<bool>(m_Extend)));
}

PdfShadingType PdfRadialShadingDefinition::GetShadingType() const
{
    return PdfShadingType::Radial;
}

PdfFreeFormMeshShadingDefinition::PdfFreeFormMeshShadingDefinition(
        PdfColorSpaceInitializer&& colorSpace, vector<double> decode,
        unsigned bitsPerCoordinate, unsigned bitsPerComponent, unsigned bitsPerFlag,
        PdfFunctionListInitializer&& functions, nullable<const PdfColorRaw&> background,
        nullable<const Rect&> bbox, bool antiAlias) :
    PdfShadingDefinition(std::move(colorSpace), std::move(functions), background, bbox, antiAlias),
    m_Decode(std::move(decode)),
    m_BitsPerCoordinate(bitsPerCoordinate),
    m_BitsPerComponent(bitsPerComponent),
    m_BitsPerFlag(bitsPerFlag)
{
}

PdfFreeFormMeshShadingDefinition::PdfFreeFormMeshShadingDefinition(
        PdfColorSpaceFilterPtr&& colorSpace, vector<double>&& decode,
        unsigned bitsPerCoordinate, unsigned bitsPerComponent, unsigned bitsPerFlag,
        vector<PdfFunctionDefinitionPtr>&& functions, const PdfColorRaw& background,
        const Rect& bbox, bool antiAlias) :
    PdfShadingDefinition(std::move(colorSpace), std::move(functions), background, bbox, antiAlias),
    m_Decode(std::move(decode)),
    m_BitsPerCoordinate(bitsPerCoordinate),
    m_BitsPerComponent(bitsPerComponent),
    m_BitsPerFlag(bitsPerFlag)
{
}

void PdfFreeFormMeshShadingDefinition::fillExportDictionary(PdfDictionary& dict) const
{
    dict.AddKey("Decode", PdfArray::FromReals(cspan<double>(m_Decode)));
    dict.AddKey("BitsPerCoordinate", static_cast<int64_t>(m_BitsPerCoordinate));
    dict.AddKey("BitsPerComponent", static_cast<int64_t>(m_BitsPerComponent));
    dict.AddKey("BitsPerFlag", static_cast<int64_t>(m_BitsPerFlag));
}

PdfShadingType PdfFreeFormMeshShadingDefinition::GetShadingType() const
{
    return PdfShadingType::FreeFormMesh;
}

PdfLatticeFormMeshShadingDefinition::PdfLatticeFormMeshShadingDefinition(
        PdfColorSpaceInitializer&& colorSpace, vector<double> decode,
        unsigned bitsPerCoordinate, unsigned bitsPerComponent, unsigned verticesPerRow,
        PdfFunctionListInitializer&& functions, nullable<const PdfColorRaw&> background,
        nullable<const Rect&> bbox, bool antiAlias) :
    PdfShadingDefinition(std::move(colorSpace), std::move(functions), background, bbox, antiAlias),
    m_Decode(std::move(decode)),
    m_BitsPerCoordinate(bitsPerCoordinate),
    m_BitsPerComponent(bitsPerComponent),
    m_VerticesPerRow(verticesPerRow)
{
}

PdfLatticeFormMeshShadingDefinition::PdfLatticeFormMeshShadingDefinition(
        PdfColorSpaceFilterPtr&& colorSpace, vector<double>&& decode,
        unsigned bitsPerCoordinate, unsigned bitsPerComponent, unsigned verticesPerRow,
        vector<PdfFunctionDefinitionPtr>&& functions, const PdfColorRaw& background,
        const Rect& bbox, bool antiAlias) :
    PdfShadingDefinition(std::move(colorSpace), std::move(functions), background, bbox, antiAlias),
    m_Decode(std::move(decode)),
    m_BitsPerCoordinate(bitsPerCoordinate),
    m_BitsPerComponent(bitsPerComponent),
    m_VerticesPerRow(verticesPerRow)
{
}

void PdfLatticeFormMeshShadingDefinition::fillExportDictionary(PdfDictionary& dict) const
{
    dict.AddKey("Decode", PdfArray::FromReals(cspan<double>(m_Decode)));
    dict.AddKey("BitsPerCoordinate", static_cast<int64_t>(m_BitsPerCoordinate));
    dict.AddKey("BitsPerComponent", static_cast<int64_t>(m_BitsPerComponent));
    dict.AddKey("VerticesPerRow", static_cast<int64_t>(m_VerticesPerRow));
}

PdfShadingType PdfLatticeFormMeshShadingDefinition::GetShadingType() const
{
    return PdfShadingType::LatticeFormMesh;
}

PdfCoonsPatchMeshShadingDefinition::PdfCoonsPatchMeshShadingDefinition(
        PdfColorSpaceInitializer&& colorSpace, vector<double> decode,
        unsigned bitsPerCoordinate, unsigned bitsPerComponent, unsigned bitsPerFlag,
        PdfFunctionListInitializer&& functions, nullable<const PdfColorRaw&> background,
        nullable<const Rect&> bbox, bool antiAlias) :
    PdfShadingDefinition(std::move(colorSpace), std::move(functions), background, bbox, antiAlias),
    m_Decode(std::move(decode)),
    m_BitsPerCoordinate(bitsPerCoordinate),
    m_BitsPerComponent(bitsPerComponent),
    m_BitsPerFlag(bitsPerFlag)
{
}

PdfCoonsPatchMeshShadingDefinition::PdfCoonsPatchMeshShadingDefinition(
        PdfColorSpaceFilterPtr&& colorSpace, vector<double>&& decode,
        unsigned bitsPerCoordinate, unsigned bitsPerComponent, unsigned bitsPerFlag,
        vector<PdfFunctionDefinitionPtr>&& functions, const PdfColorRaw& background,
        const Rect& bbox, bool antiAlias) :
    PdfShadingDefinition(std::move(colorSpace), std::move(functions), background, bbox, antiAlias),
    m_Decode(std::move(decode)),
    m_BitsPerCoordinate(bitsPerCoordinate),
    m_BitsPerComponent(bitsPerComponent),
    m_BitsPerFlag(bitsPerFlag)
{
}

void PdfCoonsPatchMeshShadingDefinition::fillExportDictionary(PdfDictionary& dict) const
{
    dict.AddKey("Decode", PdfArray::FromReals(cspan<double>(m_Decode)));
    dict.AddKey("BitsPerCoordinate", static_cast<int64_t>(m_BitsPerCoordinate));
    dict.AddKey("BitsPerComponent", static_cast<int64_t>(m_BitsPerComponent));
    dict.AddKey("BitsPerFlag", static_cast<int64_t>(m_BitsPerFlag));
}

PdfShadingType PdfCoonsPatchMeshShadingDefinition::GetShadingType() const
{
    return PdfShadingType::CoonsPatchMesh;
}

PdfTensorProductMeshShadingDefinition::PdfTensorProductMeshShadingDefinition(
        PdfColorSpaceInitializer&& colorSpace, vector<double> decode,
        unsigned bitsPerCoordinate, unsigned bitsPerComponent, unsigned bitsPerFlag,
        PdfFunctionListInitializer&& functions, nullable<const PdfColorRaw&> background,
        nullable<const Rect&> bbox, bool antiAlias) :
    PdfShadingDefinition(std::move(colorSpace), std::move(functions), background, bbox, antiAlias),
    m_Decode(std::move(decode)),
    m_BitsPerCoordinate(bitsPerCoordinate),
    m_BitsPerComponent(bitsPerComponent),
    m_BitsPerFlag(bitsPerFlag)
{
}

PdfTensorProductMeshShadingDefinition::PdfTensorProductMeshShadingDefinition(
        PdfColorSpaceFilterPtr&& colorSpace, vector<double>&& decode,
        unsigned bitsPerCoordinate, unsigned bitsPerComponent, unsigned bitsPerFlag,
        vector<PdfFunctionDefinitionPtr>&& functions, const PdfColorRaw& background, const Rect& bbox, bool antiAlias) :
    PdfShadingDefinition(std::move(colorSpace), std::move(functions), background, bbox, antiAlias),
    m_Decode(std::move(decode)),
    m_BitsPerCoordinate(bitsPerCoordinate),
    m_BitsPerComponent(bitsPerComponent),
    m_BitsPerFlag(bitsPerFlag)
{
}

void PdfTensorProductMeshShadingDefinition::fillExportDictionary(PdfDictionary& dict) const
{
    dict.AddKey("Decode", PdfArray::FromReals(cspan<double>(m_Decode)));
    dict.AddKey("BitsPerCoordinate", static_cast<int64_t>(m_BitsPerCoordinate));
    dict.AddKey("BitsPerComponent", static_cast<int64_t>(m_BitsPerComponent));
    dict.AddKey("BitsPerFlag", static_cast<int64_t>(m_BitsPerFlag));
}

PdfShadingType PdfTensorProductMeshShadingDefinition::GetShadingType() const
{
    return PdfShadingType::TensorProductMesh;
}

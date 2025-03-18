/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_PATTERN_DEFINITION_H
#define PDF_PATTERN_DEFINITION_H

#include "PdfFunctionDefinition.h"
#include "PdfVariant.h"
#include "PdfExtGState.h"
#include "PdfColorSpace.h"
#include "podofo/auxiliary/Rect.h"
#include <podofo/auxiliary/Matrix.h>

namespace PoDoFo
{
    class PdfIndirectObjectList;
    class PdfShadingDictionary;

    enum class PdfPatternType : uint8_t
    {
        Unknown = 0,
        Tiling,
        Shading
    };

    class PODOFO_API PdfPatternDefinition
    {
        friend class PdfTilingPatternDefinition;
        friend class PdfShadingPatternDefinition;
        friend class PdfPattern;
    public:
        virtual ~PdfPatternDefinition();
    private:
        PdfPatternDefinition(nullable<const Matrix&> matrix);
    public:
        virtual PdfPatternType GetType() const = 0;
        const Matrix& GetMatrix() { return m_Matrix; }
    protected:
        virtual void fillExportDictionary(PdfDictionary& dict) const = 0;
        PdfPatternDefinition(const PdfPatternDefinition&) = delete;
        PdfPatternDefinition& operator=(const PdfPatternDefinition&) = delete;
    private:
        virtual void FillExportDictionary(PdfDictionary& dict) const;
    private:
        Matrix m_Matrix;
    };

    enum class PdfTilingPaintType : uint8_t
    {
        Unknown = 0,
        Coloured,
        Uncoloured
    };

    enum class PdfTilingSpacingType : uint8_t
    {
        Unknown = 0,
        ConstantSpacing,
        NoDistortion,
        ConstantSpacingFasterTiling,
    };

    class PODOFO_API PdfTilingPatternDefinition : public PdfPatternDefinition
    {
        friend class PdfColouredTilingPatternDefinition;
        friend class PdfUncolouredTilingPatternDefinition;

    private:
        PdfTilingPatternDefinition(PdfTilingSpacingType spacingType, const Rect& bbox,
            double xStep, double yStep, nullable<const Matrix&> matrix);

    public:
        PdfPatternType GetType() const override;

        virtual PdfTilingPaintType GetPaintType() const = 0;
        PdfTilingSpacingType GetSpacingType() const { return m_SpacingType; }
        const Rect& GetBBox() const { return m_BBox; }
        double GetXStep() const { return m_XStep; }
        double GetYStep() const { return m_YStep; }

    protected:
        void fillExportDictionary(PdfDictionary& dict) const override;

    private:
        PdfTilingSpacingType m_SpacingType;
        Rect m_BBox;
        double m_XStep;
        double m_YStep;
    };

    class PODOFO_API PdfColouredTilingPatternDefinition final : public PdfTilingPatternDefinition
    {
    public:
        PdfColouredTilingPatternDefinition(PdfTilingSpacingType spacingType,
            const Rect& bbox, double xStep, double yStep, nullable<const Matrix&> matrix = { });
    public:
        PdfTilingPaintType GetPaintType() const override;
    };

    class PODOFO_API PdfUncolouredTilingPatternDefinition final : public PdfTilingPatternDefinition
    {
    public:
        PdfUncolouredTilingPatternDefinition(PdfTilingSpacingType spacingType,
            const Rect& bbox, double xStep, double yStep, nullable<const Matrix&> matrix = { });
    public:
        PdfTilingPaintType GetPaintType() const override;
    };

    enum class PdfShadingType : uint8_t
    {
        FunctionBased = 1,
        Axial = 2,
        Radial = 3,
        FreeFormMesh = 4,
        LatticeFormMesh = 5,
        CoonsPatchMesh = 6,
        TensorProductMesh = 7
    };

    class PODOFO_API PdfShadingDefinition
    {
        friend class PdfShadingDictionary;
        friend class PdfFunctionBasedShadingDefinition;
        friend class PdfAxialShadingDefinition;
        friend class PdfRadialShadingDefinition;
        friend class PdfFreeFormMeshShadingDefinition;
        friend class PdfLatticeFormMeshShadingDefinition;
        friend class PdfCoonsPatchMeshShadingDefinition;
        friend class PdfTensorProductMeshShadingDefinition;

    private:
        PdfShadingDefinition(PdfColorSpaceInitializer&& colorSpace, PdfFunctionListInitializer&& functions,
            nullable<const PdfColorRaw&> background, nullable<const Rect&> bbox, bool antiAlias);

        PdfShadingDefinition(PdfColorSpaceFilterPtr&& colorSpace, std::vector<PdfFunctionDefinitionPtr>&& functions,
            nullable<const PdfColorRaw&> background, const Rect& bbox, bool antiAlias);
    public:
        virtual ~PdfShadingDefinition();
        virtual PdfShadingType GetShadingType() const = 0;
    protected:
        virtual void fillExportDictionary(PdfDictionary& dict) const = 0;
    public:
        const PdfColorSpaceFilter& GetColorSpace() const { return *m_ColorSpace; }
        const std::vector<PdfFunctionDefinitionPtr>& GetFunctions() const { return m_Functions; }
        bool GetAntiAlias() const { return m_AntiAlias; }
        const nullable<PdfColorRaw>& GetBackground() const { return m_Background; }
        const Rect& GetBBox() const { return m_BBox; }
    private:
        PdfShadingDefinition(const PdfShadingDefinition&) = delete;
        PdfShadingDefinition& operator=(const PdfShadingDefinition&) = delete;
        virtual void FillExportDictionary(PdfDictionary& dict) const;
    private:
        PdfColorSpaceFilterPtr m_ColorSpace;
        PdfVariant m_colorSpaceExpVar;
        std::vector<PdfFunctionDefinitionPtr> m_Functions;
        PdfVariant m_functionsExpVar;
        nullable<PdfColorRaw> m_Background;
        Rect m_BBox;
        bool m_AntiAlias;
    };

    /** Convenience alias for a constant PdfShadingDefinition shared ptr
     */
    using PdfShadingDefinitionPtr = std::shared_ptr<const PdfShadingDefinition>;

    class PODOFO_API PdfShadingPatternDefinition final : public PdfPatternDefinition
    {
    public:
        PdfShadingPatternDefinition(const PdfShadingDictionary& shading, nullable<const Matrix&> matrix = { },
            nullable<const PdfExtGState&> extGState = { });

        /**
         * \remarks Deserialization constructor
         */
        PdfShadingPatternDefinition(PdfShadingDefinitionPtr&& shading, const Matrix& matrix,
            PdfExtGStateDefinitionPtr&& extGState);

    public:
        PdfPatternType GetType() const override;
        const PdfShadingDefinition& GetShading() const { return *m_Shading; }
        PdfShadingDefinitionPtr GetShadingPtr() const { return m_Shading; }
        const Matrix& GetMatrix() const { return m_Matrix; }
        const PdfExtGStateDefinition* GetExtGState() const { return m_ExtGState.get(); }
        PdfExtGStateDefinitionPtr GetExtGStatePtr() const { return m_ExtGState; }

    protected:
        void fillExportDictionary(PdfDictionary& dict) const override;

    private:
        PdfShadingDefinitionPtr m_Shading;
        PdfReference m_shadingExpRef;
        Matrix m_Matrix;
        PdfExtGStateDefinitionPtr m_ExtGState;
        PdfReference m_extGStateExpRef;
    };

    class PODOFO_API PdfFunctionBasedShadingDefinition final : public PdfShadingDefinition
    {
    public:
        PdfFunctionBasedShadingDefinition(PdfColorSpaceInitializer&& colorSpace, PdfFunctionListInitializer&& functions,
            nullable<const std::array<double, 4>&> domain = { }, nullable<const Matrix&> matrix = { },
            nullable<const PdfColorRaw&> background = { }, nullable<const Rect&> bbox = { },
            bool antiAlias = false);

        /**
         * \remarks Deserialization constructor
         */
        PdfFunctionBasedShadingDefinition(PdfColorSpaceFilterPtr&& colorSpace, std::vector<PdfFunctionDefinitionPtr>&& functions,
            const std::array<double, 4>& domain, const Matrix& matrix,
            const PdfColorRaw& background, const Rect& bbox, bool antiAlias);

    public:
        void fillExportDictionary(PdfDictionary& dict) const override;
        PdfShadingType GetShadingType() const override;
        const std::array<double, 4>& GetDomain() const { return m_Domain; }
        const Matrix& GetMatrix() const { return m_Matrix; }
    private:
        std::array<double, 4> m_Domain;
        Matrix m_Matrix;
    };

    class PODOFO_API PdfAxialShadingDefinition final : public PdfShadingDefinition
    {
    public:
        PdfAxialShadingDefinition(PdfColorSpaceInitializer&& colorSpace, PdfFunctionListInitializer&& functions,
            const std::array<double, 4>& coords, nullable<const std::array<bool, 2>&> extend = { },
            nullable<const std::array<double, 2>&> domain = { }, nullable<const PdfColorRaw&> background = { },
            nullable<const Rect&> bbox = { }, bool antiAlias = false);

        /**
         * \remarks Deserialization constructor
         */
        PdfAxialShadingDefinition(PdfColorSpaceFilterPtr&& colorSpace, std::vector<PdfFunctionDefinitionPtr>&& functions,
            const std::array<double, 4>& coords, const std::array<bool, 2>& extend,
            const std::array<double, 2> domain, const PdfColorRaw& background,
            const Rect& bbox, bool antiAlias);
    public:
        void fillExportDictionary(PdfDictionary& dict) const override;
        PdfShadingType GetShadingType() const override;
        const std::vector<PdfFunctionDefinitionPtr>& GetFunctions() const { return m_Functions; }
        const std::array<double, 4>& GetCoords() const { return m_Coords; }
        const std::array<bool, 2>& GetExtend() const { return m_Extend; }
        const std::array<double, 2>& GetDomain() const { return m_Domain; }

    private:
        std::array<double, 4> m_Coords;
        std::array<bool, 2> m_Extend;
        std::array<double, 2> m_Domain;
    };

    class PODOFO_API PdfRadialShadingDefinition final : public PdfShadingDefinition
    {
    public:
        PdfRadialShadingDefinition(PdfColorSpaceInitializer&& colorSpace, PdfFunctionListInitializer&& functions,
            const std::array<double, 6>& coords, nullable<const std::array<bool, 2>&> extend = { },
            nullable<const std::array<double, 2>&> domain = { }, nullable<const PdfColorRaw&> background = { },
            nullable<const Rect&> bbox = { }, bool antiAlias = false);

        /**
         * \remarks Deserialization constructor 
         */
        PdfRadialShadingDefinition(PdfColorSpaceFilterPtr&& colorSpace, std::vector<PdfFunctionDefinitionPtr>&& functions,
            const std::array<double, 6>& coords, const std::array<bool, 2>& extend,
            const std::array<double, 2>& domain, const PdfColorRaw& background,
            const Rect& bbox, bool antiAlias);
    public:
        void fillExportDictionary(PdfDictionary& dict) const override;
        PdfShadingType GetShadingType() const override;
        const std::vector<PdfFunctionDefinitionPtr>& GetFunctions() const { return m_Functions; }
        const std::array<double, 6>& GetCoords() const { return m_Coords; }
        const std::array<bool, 2>& GetExtend() const { return m_Extend; }
        const std::array<double, 2>& GetDomain() const { return m_Domain; }
    private:
        std::array<double, 6> m_Coords;
        std::array<bool, 2> m_Extend;
        std::array<double, 2> m_Domain;
    };

    class PODOFO_API PdfFreeFormMeshShadingDefinition final : public PdfShadingDefinition
    {
    public:
        PdfFreeFormMeshShadingDefinition(PdfColorSpaceInitializer&& colorSpace, std::vector<double> decode,
            unsigned bitsPerCoordinate, unsigned bitsPerComponent, unsigned bitsPerFlag,
            PdfFunctionListInitializer&& functions = { }, nullable<const PdfColorRaw&> background = { },
            nullable<const Rect&> bbox = { }, bool antiAlias = false);

        /**
         * \remarks Deserialization constructor
         */
        PdfFreeFormMeshShadingDefinition(PdfColorSpaceFilterPtr&& colorSpace, std::vector<double>&& decode,
            unsigned bitsPerCoordinate, unsigned bitsPerComponent, unsigned bitsPerFlag,
            std::vector<PdfFunctionDefinitionPtr>&& functions, const PdfColorRaw& background,
            const Rect& bbox, bool antiAlias);
    public:
        void fillExportDictionary(PdfDictionary& dict) const override;
        PdfShadingType GetShadingType() const override;
        unsigned GetBitsPerCoordinate() const { return m_BitsPerCoordinate; }
        unsigned GetBitsPerComponent() const { return m_BitsPerComponent; }
        unsigned GetBitsPerFlag() const { return m_BitsPerFlag; }
        const std::vector<double>& GetDecode() const { return m_Decode; }
        const std::vector<PdfFunctionDefinitionPtr>& GetFunctions() const { return m_Functions; }
    private:
        std::vector<double> m_Decode;
        unsigned m_BitsPerCoordinate;
        unsigned m_BitsPerComponent;
        unsigned m_BitsPerFlag;
    };

    class PODOFO_API PdfLatticeFormMeshShadingDefinition final : public PdfShadingDefinition
    {
    public:
        PdfLatticeFormMeshShadingDefinition(PdfColorSpaceInitializer&& colorSpace, std::vector<double> decode,
            unsigned bitsPerCoordinate, unsigned bitsPerComponent, unsigned verticesPerRow,
            PdfFunctionListInitializer&& functions = { }, nullable<const PdfColorRaw&> background = { },
            nullable<const Rect&> bbox = { }, bool antiAlias = false);

        /**
         * \remarks Deserialization constructor
         */
        PdfLatticeFormMeshShadingDefinition(PdfColorSpaceFilterPtr&& colorSpace, std::vector<double>&& decode,
            unsigned bitsPerCoordinate, unsigned bitsPerComponent, unsigned verticesPerRow,
            std::vector<PdfFunctionDefinitionPtr>&& functions, const PdfColorRaw& background,
            const Rect& bbox, bool antiAlias);

    public:
        void fillExportDictionary(PdfDictionary& dict) const override;
        PdfShadingType GetShadingType() const override;
        unsigned GetBitsPerCoordinate() const { return m_BitsPerCoordinate; }
        unsigned GetBitsPerComponent() const { return m_BitsPerComponent; }
        unsigned GetVerticesPerRow() const { return m_VerticesPerRow; }
        const std::vector<double>& GetDecode() const { return m_Decode; }
        const std::vector<PdfFunctionDefinitionPtr>& GetFunctions() const { return m_Functions; }
    private:
        std::vector<double> m_Decode;
        unsigned m_BitsPerCoordinate;
        unsigned m_BitsPerComponent;
        unsigned m_VerticesPerRow;
    };

    class PODOFO_API PdfCoonsPatchMeshShadingDefinition final : public PdfShadingDefinition
    {
    public:
        PdfCoonsPatchMeshShadingDefinition(PdfColorSpaceInitializer&& colorSpace, std::vector<double> decode,
            unsigned bitsPerCoordinate, unsigned bitsPerComponent, unsigned bitsPerFlag,
            PdfFunctionListInitializer&& functions = { }, nullable<const PdfColorRaw&> background = { },
            nullable<const Rect&> bbox = { }, bool antiAlias = false);

        /**
         * \remarks Deserialization constructor
         */
        PdfCoonsPatchMeshShadingDefinition(PdfColorSpaceFilterPtr&& colorSpace, std::vector<double>&& decode,
            unsigned bitsPerCoordinate, unsigned bitsPerComponent, unsigned bitsPerFlag,
            std::vector<PdfFunctionDefinitionPtr>&& functions, const PdfColorRaw& background,
            const Rect& bbox, bool antiAlias);
    public:
        void fillExportDictionary(PdfDictionary& dict) const override;
        PdfShadingType GetShadingType() const override;
        unsigned GetBitsPerCoordinate() const { return m_BitsPerCoordinate; }
        unsigned GetBitsPerComponent() const { return m_BitsPerComponent; }
        unsigned GetBitsPerFlag() const { return m_BitsPerFlag; }
        const std::vector<double>& GetDecode() const { return m_Decode; }
        const std::vector<PdfFunctionDefinitionPtr>& GetFunctions() const { return m_Functions; }
    private:
        std::vector<double> m_Decode;
        unsigned m_BitsPerCoordinate;
        unsigned m_BitsPerComponent;
        unsigned m_BitsPerFlag;
    };

    class PODOFO_API PdfTensorProductMeshShadingDefinition final : public PdfShadingDefinition
    {
    public:
        PdfTensorProductMeshShadingDefinition(PdfColorSpaceInitializer&& colorSpace, std::vector<double> decode,
            unsigned bitsPerCoordinate, unsigned bitsPerComponent, unsigned bitsPerFlag,
            PdfFunctionListInitializer&& functions = { }, nullable<const PdfColorRaw&> background = { },
            nullable<const Rect&> bbox = { }, bool antiAlias = false);

        /**
         * \remarks Deserialization constructor
         */
        PdfTensorProductMeshShadingDefinition(PdfColorSpaceFilterPtr&& colorSpace, std::vector<double>&& decode,
            unsigned bitsPerCoordinate, unsigned bitsPerComponent, unsigned bitsPerFlag,
            std::vector<PdfFunctionDefinitionPtr>&& functions, const PdfColorRaw& background,
            const Rect& bbox, bool antiAlias);

    public:
        void fillExportDictionary(PdfDictionary& dict) const override;
        PdfShadingType GetShadingType() const override;
        unsigned GetBitsPerCoordinate() const { return m_BitsPerCoordinate; }
        unsigned GetBitsPerComponent() const { return m_BitsPerComponent; }
        unsigned GetBitsPerFlag() const { return m_BitsPerFlag; }
        const std::vector<double>& GetDecode() const { return m_Decode; }
        const std::vector<PdfFunctionDefinitionPtr>& GetFunctions() const { return m_Functions; }
    private:
        std::vector<double> m_Decode;
        unsigned m_BitsPerCoordinate;
        unsigned m_BitsPerComponent;
        unsigned m_BitsPerFlag;
    };

    /** Convenience alias for a constant PdfTilingPatternDefinition shared ptr
     */
    using PdfPatternDefinitionPtr = std::shared_ptr<const PdfPatternDefinition>;

    /** Convenience alias for a constant PdfTilingPatternDefinition shared ptr
     */
    using PdfTilingPatternDefinitionPtr = std::shared_ptr<const PdfTilingPatternDefinition>;

    /** Convenience alias for a constant PdfShadingPatternDefinition shared ptr
     */
    using PdfShadingPatternDefinitionPtr = std::shared_ptr<const PdfShadingPatternDefinition>;
}

#endif // PDF_PATTERN_DEFINITION_H

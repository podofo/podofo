/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FONT_METRICS_OBJECT_H
#define PDF_FONT_METRICS_OBJECT_H

#include "PdfDeclarations.h"

#include <array>

#include "PdfFontMetrics.h"
#include "PdfArray.h"
#include "PdfName.h"
#include "PdfString.h"


namespace PoDoFo {

class PODOFO_API PdfFontMetricsObject final : public PdfFontMetricsBase
{
    friend class PdfFont;

private:
    /** Create a font metrics object based on an existing PdfObject
     *
     *  \param obj an existing font descriptor object
     *  \param pEncoding a PdfEncoding which will NOT be owned by PdfFontMetricsObject
     */
    PdfFontMetricsObject(const PdfObject& font, const PdfObject* descriptor);

public:
    static std::unique_ptr<const PdfFontMetricsObject> Create(const PdfObject& font);

    bool HasUnicodeMapping() const override;

    bool TryGetGID(char32_t codePoint, unsigned& gid) const override;

    bool TryGetFlags(PdfFontDescriptorFlags& value) const override;

    bool TryGetBoundingBox(Corners& value) const override;

    bool TryGetItalicAngle(double& value) const override;

    bool TryGetAscent(double& value) const override;

    bool TryGetDescent(double& value) const override;

    bool TryGetCapHeight(double& value) const override;

    bool TryGetStemV(double& value) const override;

    double GetDefaultWidthRaw() const override;

    double GetLineSpacing() const override;

    double GetUnderlineThickness() const override;

    double GetUnderlinePosition() const override;

    double GetStrikeThroughPosition() const override;

    double GetStrikeThroughThickness() const override;

    std::string_view GetFontName() const override;

    std::string_view GetFontNameRaw() const override;

    std::string_view GetFontFamilyName() const override;

    unsigned char GetSubsetPrefixLength() const override;

    PdfFontStretch GetFontStretch() const override;

    double GetLeadingRaw() const override;

    int GetWeightRaw() const override;

    double GetXHeightRaw() const override;

    double GetStemHRaw() const override;

    double GetAvgWidthRaw() const override;

    double GetMaxWidthRaw() const override;

    PdfFontFileType GetFontFileType() const override;

    const PdfObject* GetFontFileObject() const override;

    unsigned GetFontFileLength1() const override;

    unsigned GetFontFileLength2() const override;

    unsigned GetFontFileLength3() const override;

    const Matrix& GetMatrix() const override;

    bool IsObjectLoaded() const override;

protected:
    std::string_view GetBaseFontName() const override;

    PdfFontType GetFontType() const override;

    PdfCIDToGIDMapConstPtr GetBuiltinCIDToGIDMap() const override;

    bool getIsBoldHint() const override;

    bool getIsItalicHint() const override;

    datahandle getFontFileDataHandle() const override;

private:
    static std::unique_ptr<const PdfFontMetricsObject> Create(const PdfObject& font, const PdfObject* descriptor);

    void processFontName();

    Corners getBBox(const PdfObject& obj);

private:
    std::shared_ptr<charbuff> m_Data;
    PdfCIDToGIDMapConstPtr m_CIDToGIDMap;
    Matrix m_Matrix;

    std::string m_FontName;
    std::string m_FontNameRaw;
    std::string m_FontBaseName;
    std::string m_FontFamilyName;
    unsigned char m_SubsetPrefixLength;
    bool m_IsItalicHint;
    bool m_IsBoldHint;
    bool m_HasBBox;
    PdfFontStretch m_FontStretch;
    short m_Weight;

    nullable<PdfFontDescriptorFlags> m_Flags;
    Corners m_BBox;
    double m_ItalicAngle;
    double m_Ascent;
    double m_Descent;
    double m_Leading;
    double m_CapHeight;
    double m_XHeight;
    double m_StemV;
    double m_StemH;
    double m_AvgWidth;
    double m_MaxWidth;
    double m_DefaultWidth;

    const PdfObject* m_FontFileObject;
    PdfFontType m_FontType;
    nullable<PdfFontFileType> m_FontFileType;

    unsigned m_Length1;
    unsigned m_Length2;
    unsigned m_Length3;

    double m_LineSpacing;
    double m_UnderlineThickness;
    double m_UnderlinePosition;
    double m_StrikeThroughThickness;
    double m_StrikeThroughPosition;
};

};

#endif // PDF_FONT_METRICS_OBJECT_H

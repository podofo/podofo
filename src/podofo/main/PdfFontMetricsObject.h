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

class PdfArray;
class PdfObject;
class PdfVariant;

class PODOFO_API PdfFontMetricsObject final : public PdfFontMetricsBase
{
private:
    /** Create a font metrics object based on an existing PdfObject
     *
     *  \param obj an existing font descriptor object
     *  \param pEncoding a PdfEncoding which will NOT be owned by PdfFontMetricsObject
     */
    PdfFontMetricsObject(const PdfObject& font, const PdfObject* descriptor);

public:
    static std::unique_ptr<PdfFontMetricsObject> Create(const PdfObject& font, const PdfObject* descriptor = nullptr);

    unsigned GetGlyphCount() const override;

    bool TryGetGlyphWidth(unsigned gid, double& width) const override;

    bool HasUnicodeMapping() const override;

    bool TryGetGID(char32_t codePoint, unsigned& gid) const override;

    double GetDefaultWidthRaw() const override;

    double GetLineSpacing() const override;

    double GetUnderlineThickness() const override;

    double GetUnderlinePosition() const override;

    double GetStrikeOutPosition() const override;

    double GetStrikeOutThickness() const override;

    std::string_view GetFontName() const override;

    std::string_view GetFontNameRaw() const override;

    std::string_view GetBaseFontName() const override;

    std::string_view GetFontFamilyName() const override;

    PdfFontStretch GetFontStretch() const override;

    PdfFontDescriptorFlags GetFlags() const override;

    void GetBoundingBox(std::vector<double>& bbox) const override;

    double GetItalicAngle() const override;

    double GetAscent() const override;

    double GetDescent() const override;

    double GetLeadingRaw() const override;

    int GetWeightRaw() const override;

    double GetCapHeight() const override;

    double GetXHeightRaw() const override;

    double GetStemV() const override;

    double GetStemHRaw() const override;

    double GetAvgWidthRaw() const override;

    double GetMaxWidthRaw() const override;

    PdfFontFileType GetFontFileType() const override;

    const PdfObject* GetFontFileObject() const override;

    unsigned GetFontFileLength1() const override;

    unsigned GetFontFileLength2() const override;

    unsigned GetFontFileLength3() const override;

    const Matrix2D& GetMatrix() const override;

protected:
    bool getIsBoldHint() const override;

    bool getIsItalicHint() const override;

    datahandle getFontFileDataHandle() const override;

    const PdfCIDToGIDMapConstPtr& getCIDToGIDMap() const override;

private:
    std::vector<double> getBBox(const PdfObject& obj);

    void tryLoadBuiltinCIDToGIDMap();

private:
    std::shared_ptr<charbuff> m_Data;
    PdfCIDToGIDMapConstPtr m_CIDToGIDMap;
    std::vector<double> m_BBox;
    Matrix2D m_Matrix;
    std::vector<double> m_Widths;

    std::string m_FontName;
    std::string m_FontNameRaw;
    std::string m_FontBaseName;
    std::string m_FontFamilyName;
    PdfFontStretch m_FontStretch;
    int m_Weight;
    PdfFontDescriptorFlags m_Flags;
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
    PdfFontFileType m_FontFileType;

    unsigned m_Length1;
    unsigned m_Length2;
    unsigned m_Length3;

    double m_LineSpacing;
    double m_UnderlineThickness;
    double m_UnderlinePosition;
    double m_StrikeOutThickness;
    double m_StrikeOutPosition;

    bool m_IsBoldHint;
    bool m_IsItalicHint;
};

};

#endif // PDF_FONT_METRICS_OBJECT_H

/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FONT_METRICS_FREETYPE_H
#define PDF_FONT_METRICS_FREETYPE_H

#include "PdfDeclarations.h"

#include "PdfFontMetrics.h"
#include "PdfString.h"

namespace PoDoFo {

struct PdfEncodingLimits;

class PODOFO_API PdfFontMetricsFreetype final : public PdfFontMetrics
{
    friend class PdfFontMetrics;

public:
    ~PdfFontMetricsFreetype();

    std::unique_ptr<PdfCMapEncoding> CreateToUnicodeMap(const PdfEncodingLimits& limitHints) const override;

    bool HasUnicodeMapping() const override;

    bool TryGetGID(char32_t codePoint, unsigned& gid) const override;

    double GetDefaultWidthRaw() const override;

    double GetLineSpacing() const override;

    double GetUnderlineThickness() const override;

    double GetUnderlinePosition() const override;

    double GetStrikeThroughPosition() const override;

    double GetStrikeThroughThickness() const override;

    std::string_view GetFontName() const override;

    std::string_view GetFontFamilyName() const override;

    unsigned char GetSubsetPrefixLength() const override;

    PdfFontStretch GetFontStretch() const override;

    bool TryGetFlags(PdfFontDescriptorFlags& value) const override;

    bool TryGetBoundingBox(Corners& value) const override;

    bool TryGetItalicAngle(double& value) const override;

    bool TryGetAscent(double& value) const override;

    bool TryGetDescent(double& value) const override;

    bool TryGetCapHeight(double& value) const override;

    bool TryGetStemV(double& value) const override;

    double GetLeadingRaw() const override;

    int GetWeightRaw() const override;

    double GetXHeightRaw() const override;

    double GetStemHRaw() const override;

    double GetAvgWidthRaw() const override;

    double GetMaxWidthRaw() const override;

    PdfFontFileType GetFontFileType() const override;

    unsigned GetFontFileLength1() const override;

    unsigned GetFontFileLength2() const override;

    unsigned GetFontFileLength3() const override;

    const datahandle& GetFontFileDataHandle() const override;

    FT_Face GetFaceHandle() const override;

protected:
    std::string_view GetBaseFontName() const override;

    unsigned GetGlyphCountFontProgram() const override;

    bool TryGetGlyphWidthFontProgram(unsigned gid, double& width) const override;

    bool getIsBoldHint() const override;

    bool getIsItalicHint() const override;

private:
    PdfFontMetricsFreetype(FT_Face face, const datahandle& data, const PdfFontMetrics* refMetrics = nullptr);

    void init(const PdfFontMetrics* refMetrics);

    void ensureLengthsReady();

    void initType1Lengths(const bufferview& view);

    bool tryBuildFallbackUnicodeMap();

private:
    FT_Face m_Face;
    datahandle m_Data;
    PdfFontFileType m_FontFileType;

    unsigned char m_SubsetPrefixLength;
    bool m_HasUnicodeMapping;
    std::unique_ptr<std::unordered_map<uint32_t, unsigned>> m_fallbackUnicodeMap;

    std::string m_FontBaseName;
    std::string m_FontName;
    std::string m_FontFamilyName;

    // Conditionally required metrics
    PdfFontDescriptorFlags m_Flags;
    Corners m_BBox;
    double m_ItalicAngle;
    double m_Ascent;
    double m_Descent;
    double m_CapHeight;
    double m_StemV;

    // Optional metrics
    PdfFontStretch m_FontStretch;
    int m_Weight;
    double m_Leading;
    double m_XHeight;
    double m_StemH;
    double m_AvgWidth;
    double m_MaxWidth;
    double m_DefaultWidth;

    // Computed metrics
    double m_LineSpacing;
    double m_UnderlineThickness;
    double m_UnderlinePosition;
    double m_StrikeThroughThickness;
    double m_StrikeThroughPosition;

    bool m_LengthsReady;
    unsigned m_Length1;
    unsigned m_Length2;
    unsigned m_Length3;
};

};

#endif // PDF_FONT_METRICS_FREETYPE_H

/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FONT_METRICS_STANDARD14_H
#define PDF_FONT_METRICS_STANDARD14_H

#include "PdfDeclarations.h"

#include "PdfFontMetrics.h"
#include <podofo/auxiliary/Rect.h>

namespace PoDoFo {

struct Standard14FontData;

/**
 * This is the main class to handle the Standard14 metric data.
 */
class PODOFO_API PdfFontMetricsStandard14 final : public PdfFontMetricsBase
{
    friend class PdfFont;
private:
    PdfFontMetricsStandard14(PdfStandard14FontType fontType,
        const Standard14FontData& data,
        GlyphMetricsListConstPtr parsedWidths = { });

public:
    /** Create a Standard14 font metrics
     * \param fontObj optionally try to read a /Widths entry from the supplied
     */
    static std::unique_ptr<const PdfFontMetricsStandard14> Create(
        PdfStandard14FontType fontType);
    static std::unique_ptr<const PdfFontMetricsStandard14> Create(
        PdfStandard14FontType fontType, const PdfObject& fontObj);

private:
    static std::unique_ptr<const PdfFontMetricsStandard14> Create(
        PdfStandard14FontType fontType, GlyphMetricsListConstPtr&& parsedWidths);

public:
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

    std::string_view GetFontFamilyName() const override;

    PdfFontStretch GetFontStretch() const override;

    int GetWeightRaw() const override;

    double GetLeadingRaw() const override;

    double GetXHeightRaw() const override;

    double GetStemHRaw() const override;

    double GetAvgWidthRaw() const override;

    double GetMaxWidthRaw() const override;

    PdfFontFileType GetFontFileType() const override;

    bool IsStandard14FontMetrics(PdfStandard14FontType& std14Font) const override;

    unsigned GetFontFileLength1() const override;

    unsigned GetFontFileLength2() const override;

    unsigned GetFontFileLength3() const override;

    inline const Standard14FontData& GetRawData() const { return m_data; }

protected:
    std::string_view GetBaseFontName() const override;

    unsigned GetGlyphCountFontProgram() const override;

    bool TryGetGlyphWidthFontProgram(unsigned gid, double& width) const override;

    PdfFontType GetFontType() const override;

    bool getIsItalicHint() const override;

    bool getIsBoldHint() const override;

    datahandle getFontFileDataHandle() const override;

private:
    static std::unique_ptr<PdfFontMetricsStandard14> create(
        PdfStandard14FontType fontType, const PdfObject* fontObj = nullptr);

public:
    static std::shared_ptr<const PdfFontMetricsStandard14> GetInstance(PdfStandard14FontType std14Font);

private:
    PdfStandard14FontType m_Std14FontType;
    const Standard14FontData& m_data;

    double m_Ascent;
    double m_Descent;

    double m_LineSpacing;
    double m_UnderlineThickness;
    double m_UnderlinePosition;
    double m_StrikeThroughThickness;
    double m_StrikeThroughPosition;
};

}

#endif // PDF_FONT_METRICS_STANDARD14_H

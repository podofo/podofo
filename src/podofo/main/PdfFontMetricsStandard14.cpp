/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontMetricsStandard14.h"

#include <podofo/private/PdfStandard14FontData.h>

#include "PdfArray.h"
#include "PdfDictionary.h"

using namespace std;
using namespace PoDoFo;

PdfFontMetricsStandard14::PdfFontMetricsStandard14(
        PdfStandard14FontType fontType, const Standard14FontData& data,
        std::unique_ptr<std::vector<double>> parsedWidths) :
    m_Std14FontType(fontType),
    m_data(data),
    m_parsedWidths(std::move(parsedWidths))
{
    m_LineSpacing = 0.0;
    m_UnderlineThickness = 0.05;
    m_StrikeOutThickness = m_UnderlineThickness;
    m_Ascent = data.Ascent / 1000.0;
    m_Descent = data.Descent / 1000.0;

    m_UnderlinePosition = data.UnderlinePos / 1000.0;
    m_StrikeOutPosition = data.StrikeoutPos / 1000.0;

    // calculate the line spacing now, as it changes only with the font size
    m_LineSpacing = (data.Ascent + abs(data.Descent)) / 1000.0;
}

unique_ptr<PdfFontMetricsStandard14> PdfFontMetricsStandard14::Create(
    PdfStandard14FontType fontType)
{
    return create(fontType, nullptr);
}

unique_ptr<PdfFontMetricsStandard14> PdfFontMetricsStandard14::Create(
    PdfStandard14FontType fontType, const PdfObject& fontObj)
{
    return create(fontType, &fontObj);
}

unique_ptr<PdfFontMetricsStandard14> PdfFontMetricsStandard14::create(
    PdfStandard14FontType fontType, const PdfObject* fontObj)
{
    // CHECK-ME: Some standard14 fonts indeed have a /Widths
    // entry, but is it actually honored by Adobe products?
    unique_ptr<vector<double>> parsedWidths;
    if (fontObj != nullptr)
    {
        auto widthsObj = fontObj->GetDictionary().FindKey("Widths");
        if (widthsObj != nullptr)
        {
            auto& arrWidths = widthsObj->GetArray();
            parsedWidths.reset(new vector<double>(arrWidths.size()));
            for (auto& obj : arrWidths)
                parsedWidths->push_back(obj.GetReal());
        }
    }

    return unique_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
        fontType, GetInstance(fontType)->GetRawData(), std::move(parsedWidths)));
}

unsigned PdfFontMetricsStandard14::GetGlyphCount() const
{
    if (m_parsedWidths == nullptr)
        return m_data.WidthsSize;
    else
        return (unsigned)m_parsedWidths->size();
}

bool PdfFontMetricsStandard14::TryGetGlyphWidth(unsigned gid, double& width) const
{
    if (m_parsedWidths == nullptr)
    {
        if (gid >= m_data.WidthsSize)
        {
            width = -1;
            return false;
        }

        width = m_data.Widths[gid] / 1000.0; // Convert to PDF units
        return true;
    }
    else
    {
        if (gid >= m_parsedWidths->size())
        {
            width = -1;
            return false;
        }

        width = (*m_parsedWidths)[gid];
        return true;
    }
}

bool PdfFontMetricsStandard14::HasUnicodeMapping() const
{
    return true;
}

bool PdfFontMetricsStandard14::TryGetGID(char32_t codePoint, unsigned& gid) const
{
    if (codePoint >= 0xFFFF)
    {
        gid = { };
        return false;
    }

    auto& map = GetStd14CPToGIDMap(m_Std14FontType);
    auto found = map.find((unsigned short)codePoint);
    if (found == map.end())
    {
        gid = { };
        return false;
    }

    gid = found->second;
    return true;
}

PdfFontDescriptorFlags PdfFontMetricsStandard14::GetFlags() const
{
    return m_data.Flags;
}

double PdfFontMetricsStandard14::GetDefaultWidthRaw() const
{
    return m_data.DefaultWidth / 1000.0;
}

double PdfFontMetricsStandard14::GetLineSpacing() const
{
    return m_LineSpacing;
}

double PdfFontMetricsStandard14::GetUnderlineThickness() const
{
    return m_UnderlineThickness;
}

double PdfFontMetricsStandard14::GetUnderlinePosition() const
{
    return m_UnderlinePosition;
}

double PdfFontMetricsStandard14::GetStrikeOutPosition() const
{
    return m_StrikeOutPosition;
}

double PdfFontMetricsStandard14::GetStrikeOutThickness() const
{
    return m_StrikeOutThickness;
}

double PdfFontMetricsStandard14::GetAscent() const
{
    return m_Ascent;
}

double PdfFontMetricsStandard14::GetDescent() const
{
    return m_Descent;
}

double PdfFontMetricsStandard14::GetLeadingRaw() const
{
    return -1;
}

string_view PdfFontMetricsStandard14::GetFontName() const
{
    return GetStandard14FontName(m_Std14FontType);
}

string_view PdfFontMetricsStandard14::GetBaseFontName() const
{
    return GetStandard14FontBaseName(m_Std14FontType);
}

string_view PdfFontMetricsStandard14::GetFontFamilyName() const
{
    return GetStandard14FontFamilyName(m_Std14FontType);
}

PdfFontStretch PdfFontMetricsStandard14::GetFontStretch() const
{
    return m_data.Stretch;
}

int PdfFontMetricsStandard14::GetWeightRaw() const
{
    return m_data.Weight;
}

double PdfFontMetricsStandard14::GetCapHeight() const
{
    return m_data.CapHeight / 1000.0;
}

double PdfFontMetricsStandard14::GetXHeightRaw() const
{
    return m_data.XHeight / 1000.0;
}

double PdfFontMetricsStandard14::GetStemV() const
{
    return m_data.StemV / 1000.0;
}

double PdfFontMetricsStandard14::GetStemHRaw() const
{
    return m_data.StemH / 1000.0;
}

double PdfFontMetricsStandard14::GetAvgWidthRaw() const
{
    return -1;
}

double PdfFontMetricsStandard14::GetMaxWidthRaw() const
{
    return -1;
}

double PdfFontMetricsStandard14::GetItalicAngle() const
{
    return m_data.ItalicAngle;
}

PdfFontFileType PdfFontMetricsStandard14::GetFontFileType() const
{
    return PdfFontFileType::Type1CCF;
}

bool PdfFontMetricsStandard14::IsStandard14FontMetrics(PdfStandard14FontType& std14Font) const
{
    std14Font = m_Std14FontType;
    return true;
}

void PdfFontMetricsStandard14::GetBoundingBox(std::vector<double>& bbox) const
{
    // Convert to PDF units
    bbox.clear();
    bbox.push_back(m_data.BBox.GetLeft() / 1000.0);
    bbox.push_back(m_data.BBox.GetBottom() / 1000.0);
    bbox.push_back(m_data.BBox.GetWidth() / 1000.0);
    bbox.push_back(m_data.BBox.GetHeight() / 1000.0);
}

datahandle PdfFontMetricsStandard14::getFontFileDataHandle() const
{
    return datahandle(PoDoFo::GetStandard14FontFileData(m_Std14FontType));
}

unsigned PdfFontMetricsStandard14::GetFontFileLength1() const
{
    // No need for /Length1
    return 0;
}

unsigned PdfFontMetricsStandard14::GetFontFileLength2() const
{
    // No need for /Length2
    return 0;
}

unsigned PdfFontMetricsStandard14::GetFontFileLength3() const
{
    // No need for /Length3
    return 0;
}

shared_ptr<const PdfFontMetricsStandard14> PdfFontMetricsStandard14::GetInstance(
    PdfStandard14FontType std14Font)
{
    static shared_ptr<PdfFontMetricsStandard14> PODOFO_BUILTIN_FONTS[] = {
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::TimesRoman, PoDoFo::GetStandard14FontData(PdfStandard14FontType::TimesRoman)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::TimesItalic, PoDoFo::GetStandard14FontData(PdfStandard14FontType::TimesItalic)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::TimesBold, PoDoFo::GetStandard14FontData(PdfStandard14FontType::TimesBold)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::TimesBoldItalic, PoDoFo::GetStandard14FontData(PdfStandard14FontType::TimesBoldItalic)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::Helvetica, PoDoFo::GetStandard14FontData(PdfStandard14FontType::Helvetica)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::HelveticaOblique, PoDoFo::GetStandard14FontData(PdfStandard14FontType::HelveticaOblique)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::HelveticaBold, PoDoFo::GetStandard14FontData(PdfStandard14FontType::HelveticaBold)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::HelveticaBoldOblique, PoDoFo::GetStandard14FontData(PdfStandard14FontType::HelveticaBoldOblique)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::Courier, PoDoFo::GetStandard14FontData(PdfStandard14FontType::Courier)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::CourierOblique, PoDoFo::GetStandard14FontData(PdfStandard14FontType::CourierOblique)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::CourierBold, PoDoFo::GetStandard14FontData(PdfStandard14FontType::CourierBold)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::CourierBoldOblique, PoDoFo::GetStandard14FontData(PdfStandard14FontType::CourierBoldOblique)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::Symbol, PoDoFo::GetStandard14FontData(PdfStandard14FontType::Symbol)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::ZapfDingbats, PoDoFo::GetStandard14FontData(PdfStandard14FontType::ZapfDingbats)
        ))
    };

    switch (std14Font)
    {
        case PdfStandard14FontType::TimesRoman:
            return PODOFO_BUILTIN_FONTS[0];
        case PdfStandard14FontType::TimesItalic:
            return PODOFO_BUILTIN_FONTS[1];
        case PdfStandard14FontType::TimesBold:
            return PODOFO_BUILTIN_FONTS[2];
        case PdfStandard14FontType::TimesBoldItalic:
            return PODOFO_BUILTIN_FONTS[3];
        case PdfStandard14FontType::Helvetica:
            return PODOFO_BUILTIN_FONTS[4];
        case PdfStandard14FontType::HelveticaOblique:
            return PODOFO_BUILTIN_FONTS[5];
        case PdfStandard14FontType::HelveticaBold:
            return PODOFO_BUILTIN_FONTS[6];
        case PdfStandard14FontType::HelveticaBoldOblique:
            return PODOFO_BUILTIN_FONTS[7];
        case PdfStandard14FontType::Courier:
            return PODOFO_BUILTIN_FONTS[8];
        case PdfStandard14FontType::CourierOblique:
            return PODOFO_BUILTIN_FONTS[9];
        case PdfStandard14FontType::CourierBold:
            return PODOFO_BUILTIN_FONTS[10];
        case PdfStandard14FontType::CourierBoldOblique:
            return PODOFO_BUILTIN_FONTS[11];
        case PdfStandard14FontType::Symbol:
            return PODOFO_BUILTIN_FONTS[12];
        case PdfStandard14FontType::ZapfDingbats:
            return PODOFO_BUILTIN_FONTS[13];
        case PdfStandard14FontType::Unknown:
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Invalid Standard14 font type");
    }
}


bool PdfFontMetricsStandard14::getIsBoldHint() const
{
    switch (m_Std14FontType)
    {
        case PdfStandard14FontType::TimesBold:
        case PdfStandard14FontType::TimesBoldItalic:
        case PdfStandard14FontType::HelveticaBold:
        case PdfStandard14FontType::HelveticaBoldOblique:
        case PdfStandard14FontType::CourierBold:
        case PdfStandard14FontType::CourierBoldOblique:
            return true;
        default:
            return false;
    }
}

bool PdfFontMetricsStandard14::getIsItalicHint() const
{
    switch (m_Std14FontType)
    {
        case PdfStandard14FontType::TimesItalic:
        case PdfStandard14FontType::TimesBoldItalic:
        case PdfStandard14FontType::HelveticaOblique:
        case PdfStandard14FontType::HelveticaBoldOblique:
        case PdfStandard14FontType::CourierOblique:
        case PdfStandard14FontType::CourierBoldOblique:
            return true;
        default:
            return false;
    }
}


/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontMetricsFreetype.h"

#include <podofo/private/FreetypePrivate.h>
#include FT_TRUETYPE_TABLES_H
#include FT_TYPE1_TABLES_H

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfVariant.h"
#include "PdfFont.h"
#include "PdfCMapEncoding.h"
#include "PdfEncodingMapFactory.h"

using namespace std;
using namespace PoDoFo;

static void collectCharCodeToGIDMap(FT_Face face, bool symbolFont, unordered_map<unsigned, unsigned>& codeToGidMap);
static int determineType1FontWeight(const string_view& weight);
static string getPostscriptName(FT_Face face, string& fontFamilyName);

namespace
{
    class MetricsFetcher
    {
    public:
        MetricsFetcher(FT_Face face);

        PdfFontDescriptorFlags GetFlags() const;
        Corners GetBoundingBox() const;
        int GetWeight() const;
        double GetAscent() const;
        double GetDescent() const;
        double GetLeading() const;
        double GetXHeight() const;
        double GetItalicAngle() const;
        double GetLineSpacing() const;
        double GetUnderlineThickness() const;
        double GetUnderlinePosition() const;
        double GetStrikeThroughPosition() const;
        double GetStrikeThroughThickness() const;
        double GetCapHeight() const;
        double GetStemV() const;
        double GetStemH() const;
        double GetAvgWidth() const;
        double GetMaxWidth() const;
        double GetDefaultWidth() const;

    private:
        double getMaxHeight() const;

    private:
        FT_Face m_face;
        TT_OS2* m_os2Table;         // OS2 Table is available only in TT fonts
        TT_Postscript* m_psTable;   // Postscript Table is available only in TT fonts
        PS_FontInfoRec m_type1Info; // FontInfo Table is available only in type1 fonts
        bool m_hasType1Info;
    };
}

PdfFontMetricsFreetype::PdfFontMetricsFreetype(FT_Face face, const datahandle& data,
        const PdfFontMetrics* refMetrics) :
    m_Face(face),
    m_Data(data),
    m_SubsetPrefixLength(0),
    m_LengthsReady(false),
    m_Length1(0),
    m_Length2(0),
    m_Length3(0)
{
    if (face == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The face can't be null");

    init(refMetrics);
}

PdfFontMetricsFreetype::~PdfFontMetricsFreetype()
{
    FT_Done_Face(m_Face);
}

void PdfFontMetricsFreetype::init(const PdfFontMetrics* refMetrics)
{
    if (!FT::TryGetFontFileFormat(m_Face, m_FontFileType))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Unsupported font type");

    // Try to select an unicode charmap
    if (FT_Select_Charmap(m_Face, FT_ENCODING_UNICODE) == 0)
    {
        m_HasUnicodeMapping = true;
    }
    else if (refMetrics == nullptr || !refMetrics->IsObjectLoaded())
    {
        // Avoid try to create fallback maps from loaded metrics,
        // they may be fake char maps for subsets
        m_HasUnicodeMapping = tryBuildFallbackUnicodeMap();
    }
    else
    {
        m_HasUnicodeMapping = false;
    }

    unique_ptr<MetricsFetcher> fetcher;
    if (refMetrics == nullptr)
    {
        m_FontName = getPostscriptName(m_Face, m_FontFamilyName);
        m_FontBaseName = PoDoFo::ExtractBaseFontName(m_FontName, true);

        fetcher.reset(new MetricsFetcher(m_Face));

        // Required metrics
        m_Flags = fetcher->GetFlags();
        m_BBox = fetcher->GetBoundingBox();
        m_ItalicAngle = fetcher->GetItalicAngle();
        m_Ascent = fetcher->GetAscent();
        m_Descent = fetcher->GetDescent();
        m_CapHeight = fetcher->GetCapHeight();
        m_StemV = fetcher->GetStemV();

        // Optional metrics
        m_FontStretch = PdfFontStretch::Unknown;
        m_Weight = fetcher->GetWeight();
        m_Leading = fetcher->GetLeading();
        m_XHeight = fetcher->GetXHeight();
        m_StemH = fetcher->GetStemH();
        m_AvgWidth = fetcher->GetAvgWidth();
        m_MaxWidth = fetcher->GetMaxWidth();
        m_DefaultWidth = fetcher->GetDefaultWidth();

        // Computed metrics
        m_LineSpacing = fetcher->GetLineSpacing();
        m_UnderlineThickness = fetcher->GetUnderlineThickness();
        m_UnderlinePosition = fetcher->GetUnderlinePosition();
        m_StrikeThroughPosition = fetcher->GetStrikeThroughPosition();
        m_StrikeThroughThickness = fetcher->GetStrikeThroughThickness();

        // NOTE2: It is not correct to write flags ForceBold if the
        // font is already bold. The ForceBold flag is just an hint
        // for the viewer to draw glyphs with more pixels
        // TODO: Infer more characateristics
        if ((GetStyle() & PdfFontStyle::Italic) == PdfFontStyle::Italic)
            m_Flags |= PdfFontDescriptorFlags::Italic;
    }
    else
    {
        // If no postscript name was extracted by the font program
        // try to recover it from the reference metrics
        m_FontName = refMetrics->GetFontName();
        m_FontFamilyName = refMetrics->GetFontFamilyName();
        if (m_FontName.empty())
            m_FontName = getPostscriptName(m_Face, m_FontFamilyName);
        else
            m_SubsetPrefixLength = refMetrics->GetSubsetPrefixLength();

        m_FontBaseName = PoDoFo::ExtractBaseFontName(m_FontName);

        // Required metrics
        if (!refMetrics->TryGetFlags(m_Flags))
        {
            if (fetcher == nullptr)
                fetcher.reset(new MetricsFetcher(m_Face));
            m_Flags = fetcher->GetFlags();
        }

        if (!refMetrics->TryGetBoundingBox(m_BBox))
        {
            if (fetcher == nullptr)
                fetcher.reset(new MetricsFetcher(m_Face));
            m_BBox = fetcher->GetBoundingBox();
        }

        if (!refMetrics->TryGetItalicAngle(m_ItalicAngle))
        {
            if (fetcher == nullptr)
                fetcher.reset(new MetricsFetcher(m_Face));
            m_ItalicAngle = fetcher->GetItalicAngle();
        }

        if (!refMetrics->TryGetAscent(m_Ascent))
        {
            if (fetcher == nullptr)
                fetcher.reset(new MetricsFetcher(m_Face));
            m_Ascent = fetcher->GetAscent();
        }

        if (!refMetrics->TryGetDescent(m_Descent))
        {
            if (fetcher == nullptr)
                fetcher.reset(new MetricsFetcher(m_Face));
            m_Descent = fetcher->GetDescent();
        }

        if (!refMetrics->TryGetCapHeight(m_CapHeight))
        {
            if (fetcher == nullptr)
                fetcher.reset(new MetricsFetcher(m_Face));
            m_CapHeight = fetcher->GetCapHeight();
        }

        if (!refMetrics->TryGetStemV(m_StemV))
        {
            if (fetcher == nullptr)
                fetcher.reset(new MetricsFetcher(m_Face));
            m_StemV = fetcher->GetStemV();
        }

        // Optional metrics
        m_FontStretch = refMetrics->GetFontStretch();
        m_Weight = refMetrics->GetWeightRaw();
        m_Leading = refMetrics->GetLeadingRaw();
        m_XHeight = refMetrics->GetXHeightRaw();
        m_StemH = refMetrics->GetStemHRaw();
        m_AvgWidth = refMetrics->GetAvgWidthRaw();
        m_MaxWidth = refMetrics->GetMaxWidthRaw();
        m_DefaultWidth = refMetrics->GetDefaultWidthRaw();

        // Computed metrics
        m_LineSpacing = refMetrics->GetLineSpacing();
        m_StrikeThroughPosition = refMetrics->GetStrikeThroughPosition();
        m_StrikeThroughThickness = refMetrics->GetStrikeThroughThickness();
        m_UnderlineThickness = refMetrics->GetUnderlineThickness();
        m_UnderlinePosition = refMetrics->GetUnderlinePosition();

        // Enforce parsed metrics from reference
        SetParsedWidths(refMetrics->GetParsedWidths());
    }
}

void PdfFontMetricsFreetype::ensureLengthsReady()
{
    if (m_LengthsReady)
        return;

    switch (m_FontFileType)
    {
        case PdfFontFileType::Type1:
            initType1Lengths(m_Data.view());
            break;
        case PdfFontFileType::TrueType:
            m_Length1 = (unsigned)m_Data.view().size();
            break;
        default:
            // Other font types don't need lengths
            break;
    }

    m_LengthsReady = true;
}

void PdfFontMetricsFreetype::initType1Lengths(const bufferview& view)
{
    // Specification: "Adobe Type 1 Font Format" : 2.3 Explanation of a Typical Font Program
    
    // Method taken from matplotlib
    // https://github.com/matplotlib/matplotlib/blob/a6da11eebcfe3bbdb0b6e0f24348be63a06280db/lib/matplotlib/_type1font.py#L404
    string_view sview = string_view(view.data(), view.size());
    size_t found = sview.find("eexec");
    if (found == string_view::npos)
        return;

    m_Length1 = (unsigned)found + 5;
    while (true)
    {
        PODOFO_INVARIANT(m_Length1 <= sview.length());
        if (m_Length1 == sview.length())
            return;

        switch (sview[m_Length1])
        {
            case '\n':
            case '\r':
            case '\t':
            case ' ':
                // Skip all whitespaces
                m_Length1++;
                continue;
            default:
                // Non whitespace, goto break the enclosing loop
                goto Exit;
        }
    Exit:
        break;
    }

    found = sview.rfind("cleartomark");
    if (found == string_view::npos || found == 0)
    {
        m_Length2 = (unsigned)sview.length() - m_Length1;
        return;
    }

    unsigned zeros = 512;
    size_t currIdx = found - 1;
    while (true)
    {
        PODOFO_INVARIANT(currIdx > 0);
        switch (sview[currIdx])
        {
            case '\n':
            case '\r':
                // Skip all newlines
                break;
            case '0':
                zeros--;
                break;
            default:
                // Found unexpected content
                zeros = 0;
                break;
        }

        if (zeros == 0)
            break;

        currIdx--;
        if (currIdx == 0)
            return;
    }

    m_Length2 = (unsigned)currIdx - m_Length1;
    m_Length3 = (unsigned)sview.length() - m_Length2;
}

string_view PdfFontMetricsFreetype::GetFontName() const
{
    return m_FontName;
}

string_view PdfFontMetricsFreetype::GetBaseFontName() const
{
    return m_FontBaseName;
}

string_view PdfFontMetricsFreetype::GetFontFamilyName() const
{
    return m_FontFamilyName;
}

unsigned char PdfFontMetricsFreetype::GetSubsetPrefixLength() const
{
    return m_SubsetPrefixLength;
}

PdfFontStretch PdfFontMetricsFreetype::GetFontStretch() const
{
    return m_FontStretch;
}

bool PdfFontMetricsFreetype::TryGetFlags(PdfFontDescriptorFlags& value) const
{
    value = m_Flags;
    return true;
}

bool PdfFontMetricsFreetype::TryGetBoundingBox(Corners& value) const
{
    value = m_BBox;
    return true;
}

bool PdfFontMetricsFreetype::TryGetItalicAngle(double& value) const
{
    value = m_ItalicAngle;
    return true;
}

bool PdfFontMetricsFreetype::TryGetAscent(double& value) const
{
    value = m_Ascent;
    return true;
}

bool PdfFontMetricsFreetype::TryGetDescent(double& value) const
{
    value = m_Descent;
    return true;
}

bool PdfFontMetricsFreetype::TryGetCapHeight(double& value) const
{
    value = m_CapHeight;
    return true;
}

bool PdfFontMetricsFreetype::TryGetStemV(double& value) const
{
    value = m_StemV;
    return true;
}

unsigned PdfFontMetricsFreetype::GetGlyphCountFontProgram() const
{
    return (unsigned)m_Face->num_glyphs;
}

bool PdfFontMetricsFreetype::TryGetGlyphWidthFontProgram(unsigned gid, double& width) const
{
    if (FT_Load_Glyph(m_Face, gid, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP) != 0)
    {
        width = -1;
        return false;
    }

    // zero return code is success!
    width = m_Face->glyph->metrics.horiAdvance / (double)m_Face->units_per_EM;
    return true;
}

bool PdfFontMetricsFreetype::HasUnicodeMapping() const
{
    return m_HasUnicodeMapping;
}

bool PdfFontMetricsFreetype::TryGetGID(char32_t codePoint, unsigned& gid) const
{
    if (!m_HasUnicodeMapping)
    {
        gid = 0;
        return false;
    }

    if (m_fallbackUnicodeMap != nullptr)
    {
        auto found = m_fallbackUnicodeMap->find(codePoint);
        if (found == m_fallbackUnicodeMap->end())
        {
            gid = 0;
            return false;
        }

        gid = found->second;
        return true;
    }

    gid = FT_Get_Char_Index(m_Face, codePoint);
    return gid != 0;
}


unique_ptr<PdfCMapEncoding> PdfFontMetricsFreetype::CreateToUnicodeMap(const PdfEncodingLimits& limitHints) const
{
    PdfCharCodeMap map;
    FT_ULong charcode;
    FT_UInt gid;

    charcode = FT_Get_First_Char(m_Face, &gid);
    while (gid != 0)
    {
        map.PushMapping({ gid, limitHints.MinCodeSize }, (char32_t)charcode);
        charcode = FT_Get_Next_Char(m_Face, charcode, &gid);
    }

    return std::make_unique<PdfCMapEncoding>(std::move(map));
}

bool PdfFontMetricsFreetype::tryBuildFallbackUnicodeMap()
{
    auto os2Table = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(m_Face, FT_SFNT_OS2));
    if (os2Table != nullptr)
    {
        // https://learn.microsoft.com/en-us/typography/opentype/spec/recom#panose-values
        // "If the font is a symbol font, the first byte of the PANOSE
        // value must be set to 'Latin Pictorial' (value = 5)"
        constexpr unsigned char LatinPictorial = 5;
        if (os2Table->panose[0] == LatinPictorial)
        {
            // For symbol encodings we will interpret Unicode code points
            // as character codes with 1:1 mapping when mapping to GID.
            // This appears to be what Adobe actually does in its products
            m_fallbackUnicodeMap.reset(new unordered_map<uint32_t, unsigned>());
            if (FT_Select_Charmap(m_Face, FT_ENCODING_MS_SYMBOL) == 0)
            {
                // If a symbol encoding is available, just collect that
                collectCharCodeToGIDMap(m_Face, true, *m_fallbackUnicodeMap);
            }
            else
            {
                // If the symbol encoding is not available, just collect
                // the default selected charmap
                collectCharCodeToGIDMap(m_Face, false, *m_fallbackUnicodeMap);
            }

            return true;
        }
    }

    // Try to create an Unicode to GID char map from legacy "encodings"
    // (or better charmaps), as reported by FreeType

    if (FT_Select_Charmap(m_Face, FT_ENCODING_APPLE_ROMAN) == 0)
    {
        unordered_map<unsigned, unsigned> codeToGIDmap;
        collectCharCodeToGIDMap(m_Face, false, codeToGIDmap);
        m_fallbackUnicodeMap.reset(new unordered_map<uint32_t, unsigned>());
        auto encoding = PdfEncodingMapFactory::GetMacRomanEncodingInstancePtr();
        encoding->CreateUnicodeToGIDMap(codeToGIDmap, *m_fallbackUnicodeMap);
        return true;
    }

    if (FT_Select_Charmap(m_Face, FT_ENCODING_ADOBE_LATIN_1) == 0)
    {
        unordered_map<unsigned, unsigned> codeToGIDmap;
        collectCharCodeToGIDMap(m_Face, false, codeToGIDmap);
        m_fallbackUnicodeMap.reset(new unordered_map<uint32_t, unsigned>());
        auto encoding = PdfEncodingMapFactory::GetAppleLatin1EncodingInstancePtr();
        encoding->CreateUnicodeToGIDMap(codeToGIDmap, *m_fallbackUnicodeMap);
        return true;
    }

    if (FT_Select_Charmap(m_Face, FT_ENCODING_ADOBE_STANDARD) == 0)
    {
        unordered_map<unsigned, unsigned> codeToGIDmap;
        collectCharCodeToGIDMap(m_Face, false, codeToGIDmap);
        m_fallbackUnicodeMap.reset(new unordered_map<uint32_t, unsigned>());
        auto encoding = PdfEncodingMapFactory::GetStandardEncodingInstancePtr();
        encoding->CreateUnicodeToGIDMap(codeToGIDmap, *m_fallbackUnicodeMap);
        return true;
    }

    if (FT_Select_Charmap(m_Face, FT_ENCODING_ADOBE_EXPERT) == 0)
    {
        unordered_map<unsigned, unsigned> codeToGIDmap;
        collectCharCodeToGIDMap(m_Face, false, codeToGIDmap);
        m_fallbackUnicodeMap.reset(new unordered_map<uint32_t, unsigned>());
        auto encoding = PdfEncodingMapFactory::GetMacExpertEncodingInstancePtr();
        encoding->CreateUnicodeToGIDMap(codeToGIDmap, *m_fallbackUnicodeMap);
        return true;
    }

    // CHECK-ME1: Try to merge maps if multiple encodings?
    // CHECK-ME2: Support more encodings as reported by FreeType?
    PoDoFo::LogMessage(PdfLogSeverity::Warning, "Could not create an unicode map for the font {}", m_FontName);
    return false;
}

double PdfFontMetricsFreetype::GetDefaultWidthRaw() const
{
    return m_DefaultWidth;
}

bool PdfFontMetricsFreetype::getIsBoldHint() const
{
    return (m_Face->style_flags & FT_STYLE_FLAG_BOLD) != 0;
}

bool PdfFontMetricsFreetype::getIsItalicHint() const
{
    return (m_Face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;
}

double PdfFontMetricsFreetype::GetLineSpacing() const
{
    return m_LineSpacing;
}

double PdfFontMetricsFreetype::GetUnderlinePosition() const
{
    return m_UnderlinePosition;
}

double PdfFontMetricsFreetype::GetStrikeThroughPosition() const
{
    return m_StrikeThroughPosition;
}

double PdfFontMetricsFreetype::GetUnderlineThickness() const
{
    return m_UnderlineThickness;
}

double PdfFontMetricsFreetype::GetStrikeThroughThickness() const
{
    return m_StrikeThroughThickness;
}

double PdfFontMetricsFreetype::GetLeadingRaw() const
{
    return m_Leading;
}

unsigned PdfFontMetricsFreetype::GetFontFileLength1() const
{
    switch (m_FontFileType)
    {
        case PdfFontFileType::Type1:
        case PdfFontFileType::TrueType:
            return (unsigned)m_Data.view().size();
        default:
            return 0;
    }
}

unsigned PdfFontMetricsFreetype::GetFontFileLength2() const
{
    const_cast<PdfFontMetricsFreetype&>(*this).ensureLengthsReady();
    return m_Length1;
}

unsigned PdfFontMetricsFreetype::GetFontFileLength3() const
{
    const_cast<PdfFontMetricsFreetype&>(*this).ensureLengthsReady();
    return m_Length1;
}

const datahandle& PdfFontMetricsFreetype::GetFontFileDataHandle() const
{
    return m_Data;
}

FT_Face PdfFontMetricsFreetype::GetFaceHandle() const
{
    return m_Face;
}

int PdfFontMetricsFreetype::GetWeightRaw() const
{
    return m_Weight;
}

double PdfFontMetricsFreetype::GetXHeightRaw() const
{
    return m_XHeight;
}

double PdfFontMetricsFreetype::GetStemHRaw() const
{
    return m_StemH;
}

double PdfFontMetricsFreetype::GetAvgWidthRaw() const
{
    return m_AvgWidth;
}

double PdfFontMetricsFreetype::GetMaxWidthRaw() const
{
    return m_MaxWidth;
}

PdfFontFileType PdfFontMetricsFreetype::GetFontFileType() const
{
    return m_FontFileType;
}

void collectCharCodeToGIDMap(FT_Face face, bool symbolFont, unordered_map<unsigned, unsigned>& codeToGidMap)
{
    FT_ULong charcode;
    FT_UInt gid;

    if (symbolFont)
    {
        charcode = FT_Get_First_Char(face, &gid);
        while (gid != 0)
        {
            // https://learn.microsoft.com/en-us/typography/opentype/spec/recom#non-standard-symbol-fonts
            // "The character codes should start at 0xF000". We recover the intended code
            codeToGidMap[(unsigned)charcode ^ 0xF000U] = gid;
            charcode = FT_Get_Next_Char(face, charcode, &gid);
        }
    }
    else
    {
        charcode = FT_Get_First_Char(face, &gid);
        while (gid != 0)
        {
            codeToGidMap[(unsigned)charcode] = gid;
            charcode = FT_Get_Next_Char(face, charcode, &gid);
        }
    }
}

int determineType1FontWeight(const string_view& weightraw)
{
    string weight = utls::ToLower(weightraw);
    weight = utls::Trim(weight, ' ');
    weight = utls::Trim(weight, '-');

    // The following table was found randomly on gamedev.net, but seems
    // to be consistent with PDF range [100,900] in ISO 32000-1:2008
    // Table 122 – Entries common to all font descriptors /FontWeight
    // https://www.gamedev.net/forums/topic/690570-font-weights-and-thickness-classification-in-freetype/
    if (weight == "extralight" || weight == "ultralight")
        return 100;
    else if (weight == "light" || weight == "thin")
        return 200;
    else if (weight == "book" || weight == "demi")
        return 300;
    else if (weight == "normal" || weight == "regular")
        return 400;
    else if (weight == "medium")
        return 500;
    else if (weight == "semibold" || weight == "demibold")
        return 600;
    else if (weight == "bold")
        return 700;
    else if (weight == "black" || weight == "extrabold" || weight == "heavy")
        return 800;
    else if (weight == "extrablack" || weight == "fat" || weight == "poster" || weight == "ultrablack")
        return 900;
    else
        return -1;
}

string getPostscriptName(FT_Face face, string& fontFamilyName)
{
    if (face->family_name != nullptr)
        fontFamilyName = face->family_name;

    auto psname = FT_Get_Postscript_Name(face);
    if (psname == nullptr)
        return { };

    // Get the postscript name of the font and ensures it has no space:
    // 5.5.2 TrueType Fonts, "If the name contains any spaces, the spaces
    // are removed"
    string ret = psname;
    if (ret.empty())
    {
        // Determine a fallback for the font name
        if (fontFamilyName.empty())
            ret = "FreeTypeFont";
        else
            ret = fontFamilyName;
    }
    else
    {
        ret.erase(std::remove(ret.begin(), ret.end(), ' '), ret.end());
    }

    return ret;
}

MetricsFetcher::MetricsFetcher(FT_Face face) :
    m_face(face),
    m_os2Table(static_cast<TT_OS2*>(FT_Get_Sfnt_Table(face, FT_SFNT_OS2))),
    m_psTable(static_cast<TT_Postscript*>(FT_Get_Sfnt_Table(face, FT_SFNT_POST)))
{
    m_hasType1Info = FT_Get_PS_Font_Info(m_face, &m_type1Info) == 0;

    // CHECK-ME: Try to read Type1 tables as well?
    // https://freetype.org/freetype2/docs/reference/ft2-type1_tables.html
}

PdfFontDescriptorFlags MetricsFetcher::GetFlags() const
{
    PdfFontDescriptorFlags ret = PdfFontDescriptorFlags::Symbolic;
    if (m_psTable != nullptr && m_psTable->isFixedPitch != 0)
        ret |= PdfFontDescriptorFlags::FixedPitch;
    else if (m_hasType1Info && m_type1Info.is_fixed_pitch != 0)
        ret |= PdfFontDescriptorFlags::FixedPitch;

    return ret;
}

Corners MetricsFetcher::GetBoundingBox() const
{
    return Corners(
        m_face->bbox.xMin / (double)m_face->units_per_EM,
        m_face->bbox.yMin / (double)m_face->units_per_EM,
        m_face->bbox.xMax / (double)m_face->units_per_EM,
        m_face->bbox.yMax / (double)m_face->units_per_EM
    );
}

int MetricsFetcher::GetWeight() const
{
    if (m_os2Table != nullptr)
        return m_os2Table->usWeightClass;
    else if (m_hasType1Info && m_type1Info.weight != nullptr)
        return determineType1FontWeight(m_type1Info.weight);

    return -1;
}

double MetricsFetcher::GetAscent() const
{
    return m_face->ascender / (double)m_face->units_per_EM;
}

double MetricsFetcher::GetDescent() const
{
    return m_face->descender / (double)m_face->units_per_EM;
}

double MetricsFetcher::GetLeading() const
{
    return -1;
}

double MetricsFetcher::GetItalicAngle() const
{
    if (m_psTable)
        return m_psTable->italicAngle;
    else if (m_hasType1Info)
        return m_type1Info.italic_angle;

    return 0;
}

double MetricsFetcher::GetLineSpacing() const
{
    return m_face->height / (double)m_face->units_per_EM;
}

double MetricsFetcher::GetUnderlineThickness() const
{
    return m_face->underline_thickness / (double)m_face->units_per_EM;
}

double MetricsFetcher::GetUnderlinePosition() const
{
    return m_face->underline_position / (double)m_face->units_per_EM;
}

double MetricsFetcher::GetStrikeThroughPosition() const
{
    if (m_os2Table != nullptr)
        return m_os2Table->yStrikeoutPosition / (double)m_face->units_per_EM;

    return GetAscent() / 2.0;
}

double MetricsFetcher::GetStrikeThroughThickness() const
{
    if (m_os2Table != nullptr)
        return m_os2Table->yStrikeoutSize / (double)m_face->units_per_EM;

    return GetUnderlineThickness();
}

double MetricsFetcher::GetCapHeight() const
{
    if (m_os2Table != nullptr)
        return m_os2Table->sCapHeight / (double)m_face->units_per_EM;

    return getMaxHeight();
}

double MetricsFetcher::GetXHeight() const
{
    if (m_os2Table != nullptr)
        return m_os2Table->sxHeight / (double)m_face->units_per_EM;

    return 0;
}

double MetricsFetcher::GetStemV() const
{
    // ISO 32000-2:2017, Table 120 — Entries common to all font descriptors
    // says: "A value of 0 indicates an unknown stem thickness". No mention
    // is done about this in ISO 32000-1:2008, but we assume 0 is a safe
    // value for all implementations
    return 0;
}

double MetricsFetcher::GetStemH() const
{
    return -1;
}

double MetricsFetcher::GetAvgWidth() const
{
    return -1;
}

double MetricsFetcher::GetMaxWidth() const
{
    return (m_face->bbox.xMax - m_face->bbox.xMin) / (double)m_face->units_per_EM;
}

double MetricsFetcher::GetDefaultWidth() const
{
    return GetMaxWidth();
}

double MetricsFetcher::getMaxHeight() const
{
    return (m_face->bbox.yMax - m_face->bbox.yMin) / (double)m_face->units_per_EM;
}

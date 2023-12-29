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

PdfFontMetricsFreetype::PdfFontMetricsFreetype(const FreeTypeFacePtr& face, const datahandle& data,
        const PdfFontMetrics* refMetrics) :
    m_Face(face),
    m_Data(data),
    m_LengthsReady(false),
    m_Length1(0),
    m_Length2(0),
    m_Length3(0)
{
    if (face == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The buffer can't be null");

    init(refMetrics);
}

unique_ptr<PdfFontMetricsFreetype> PdfFontMetricsFreetype::CreateSubstituteMetrics(
    const PdfFontMetrics& metrics)
{
    return unique_ptr<PdfFontMetricsFreetype>(new PdfFontMetricsFreetype(metrics.GetFaceHandle(),
        metrics.GetFontFileDataHandle(), &metrics));
}

unique_ptr<PdfFontMetricsFreetype> PdfFontMetricsFreetype::CreateFromFace(FT_Face face)
{
    if (face == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The face can't be null");

    if (!FT::IsPdfSupported(face))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The face is not PDF supported");

    shared_ptr<const charbuff> buffer(new charbuff(FT::GetDataFromFace(face)));
    FT_Face newface = FT::CreateFaceFromBuffer(*buffer);
    return unique_ptr<PdfFontMetricsFreetype>(new PdfFontMetricsFreetype(newface, buffer));
}

void PdfFontMetricsFreetype::init(const PdfFontMetrics* refMetrics)
{
    if (!FT::TryGetFontFileFormat(m_Face.get(), m_FontFileType))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Unsupported font type");

    // Get the postscript name of the font and ensures it has no space:
    // 5.5.2 TrueType Fonts, "If the name contains any spaces, the spaces are removed"
    auto psname = FT_Get_Postscript_Name(m_Face.get());
    if (psname != nullptr)
    {
        m_FontName = psname;
        m_FontName.erase(std::remove(m_FontName.begin(), m_FontName.end(), ' '), m_FontName.end());
    }

    if (m_Face->family_name != nullptr)
        m_FontFamilyName = m_Face->family_name;

    // calculate the line spacing now, as it changes only with the font size
    m_LineSpacing = m_Face->height / (double)m_Face->units_per_EM;
    m_UnderlineThickness = m_Face->underline_thickness / (double)m_Face->units_per_EM;
    m_UnderlinePosition = m_Face->underline_position / (double)m_Face->units_per_EM;
    m_Ascent = m_Face->ascender / (double)m_Face->units_per_EM;
    m_Descent = m_Face->descender / (double)m_Face->units_per_EM;

    // Try to select an unicode charmap
    if (FT_Select_Charmap(m_Face.get(), FT_ENCODING_UNICODE) == 0)
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

    // Set some default values, in case the font has no direct values
    if (refMetrics == nullptr)
    {
        // Get maximal width and height
        double width = (m_Face->bbox.xMax - m_Face->bbox.xMin) / (double)m_Face->units_per_EM;
        double height = (m_Face->bbox.yMax - m_Face->bbox.yMin) / (double)m_Face->units_per_EM;

        m_FontStretch = PdfFontStretch::Unknown;
        m_Weight = -1;
        m_Flags = PdfFontDescriptorFlags::Symbolic;
        m_ItalicAngle = 0;
        m_Leading = -1;
        m_CapHeight = height;
        m_XHeight = 0;
        // ISO 32000-2:2017, Table 120 — Entries common to all font descriptors
        // says: "A value of 0 indicates an unknown stem thickness". No mention
        // is done about this in ISO 32000-1:2008, but we assume 0 is a safe
        // value for all implementations
        m_StemV = 0;
        m_StemH = -1;
        m_AvgWidth = -1;
        m_MaxWidth = -1;
        m_DefaultWidth = width;

        m_StrikeThroughPosition = m_Ascent / 2.0;
        m_StrikeThroughThickness = m_UnderlineThickness;
    }
    else
    {
        m_CIDToGIDMap = refMetrics->GetCIDToGIDMap();

        if (m_FontName.empty())
            m_FontName = refMetrics->GetFontName();
        if (m_FontFamilyName.empty())
            m_FontFamilyName = refMetrics->GetFontFamilyName();

        m_FontStretch = refMetrics->GetFontStretch();
        m_Weight = refMetrics->GetWeightRaw();
        m_Flags = refMetrics->GetFlags();
        m_ItalicAngle = refMetrics->GetItalicAngle();
        m_Leading = refMetrics->GetLeadingRaw();
        m_CapHeight = refMetrics->GetCapHeight();
        m_XHeight = refMetrics->GetXHeightRaw();
        m_StemV = refMetrics->GetStemV();
        m_StemH = refMetrics->GetStemHRaw();
        m_AvgWidth = refMetrics->GetAvgWidthRaw();
        m_MaxWidth = refMetrics->GetMaxWidthRaw();
        m_DefaultWidth = refMetrics->GetDefaultWidthRaw();

        m_StrikeThroughPosition = refMetrics->GetStrikeThroughPosition();
        m_StrikeThroughThickness = refMetrics->GetStrikeThroughThickness();
    }

    // OS2 Table is available only in TT fonts
    TT_OS2* os2Table = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(m_Face.get(), FT_SFNT_OS2));
    if (os2Table != nullptr)
    {
        m_StrikeThroughPosition = os2Table->yStrikeoutPosition / (double)m_Face->units_per_EM;
        m_StrikeThroughThickness = os2Table->yStrikeoutSize / (double)m_Face->units_per_EM;
        m_CapHeight = os2Table->sCapHeight / (double)m_Face->units_per_EM;
        m_XHeight = os2Table->sxHeight / (double)m_Face->units_per_EM;
        m_Weight = os2Table->usWeightClass;
    }

    // Postscript Table is available only in TT fonts
    TT_Postscript* psTable = static_cast<TT_Postscript*>(FT_Get_Sfnt_Table(m_Face.get(), FT_SFNT_POST));
    if (psTable != nullptr)
    {
        m_ItalicAngle = (double)psTable->italicAngle;
        if (psTable->isFixedPitch != 0)
            m_Flags |= PdfFontDescriptorFlags::FixedPitch;
    }

    if (m_FontName.empty())
    {
        // Determine a fallback for the font name
        if (m_FontFamilyName.empty())
            m_FontName = "FreeTypeFont";
        else
            m_FontName = m_FontFamilyName;
    }

    m_FontBaseName = PoDoFo::NormalizeFontName(m_FontName);

    // FontInfo Table is available only in type1 fonts
    PS_FontInfoRec type1Info;
    if (FT_Get_PS_Font_Info(m_Face.get(), &type1Info) == 0)
    {
        m_ItalicAngle = (double)type1Info.italic_angle;
        if (type1Info.weight != nullptr)
            m_Weight = determineType1FontWeight(type1Info.weight);
        if (type1Info.is_fixed_pitch != 0)
            m_Flags |= PdfFontDescriptorFlags::FixedPitch;
    }

    // CHECK-ME: Try to read Type1 tables as well?
    // https://freetype.org/freetype2/docs/reference/ft2-type1_tables.html

    // NOTE2: It is not correct to write flags ForceBold if the
    // font is already bold. The ForceBold flag is just an hint
    // for the viewer to draw glyphs with more pixels
    // TODO: Infer more characateristics
    if ((GetStyle() & PdfFontStyle::Italic) == PdfFontStyle::Italic)
        m_Flags |= PdfFontDescriptorFlags::Italic;
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
                break;
        }
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

PdfFontStretch PdfFontMetricsFreetype::GetFontStretch() const
{
    return m_FontStretch;
}

unsigned PdfFontMetricsFreetype::GetGlyphCount() const
{
    return (unsigned)m_Face.get()->num_glyphs;
}

bool PdfFontMetricsFreetype::TryGetGlyphWidth(unsigned gid, double& width) const
{
    if (FT_Load_Glyph(m_Face.get(), gid, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP) != 0)
    {
        width = -1;
        return false;
    }

    // zero return code is success!
    width = m_Face.get()->glyph->metrics.horiAdvance / (double)m_Face.get()->units_per_EM;
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

    gid = FT_Get_Char_Index(m_Face.get(), codePoint);
    return gid != 0;
}


unique_ptr<PdfCMapEncoding> PdfFontMetricsFreetype::CreateToUnicodeMap(const PdfEncodingLimits& limitHints) const
{
    PdfCharCodeMap map;
    FT_ULong charcode;
    FT_UInt gid;

    charcode = FT_Get_First_Char(m_Face.get(), &gid);
    while (gid != 0)
    {
        map.PushMapping({ gid, limitHints.MinCodeSize }, (char32_t)charcode);
        charcode = FT_Get_Next_Char(m_Face.get(), charcode, &gid);
    }

    return std::make_unique<PdfCMapEncoding>(std::move(map));
}

bool PdfFontMetricsFreetype::tryBuildFallbackUnicodeMap()
{
    auto os2Table = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(m_Face.get(), FT_SFNT_OS2));
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
            if (FT_Select_Charmap(m_Face.get(), FT_ENCODING_MS_SYMBOL) == 0)
            {
                // If a symbol encoding is available, just collect that
                collectCharCodeToGIDMap(m_Face.get(), true, *m_fallbackUnicodeMap);
            }
            else
            {
                // If the symbol encoding is not available, just collect
                // the default selected charmap
                collectCharCodeToGIDMap(m_Face.get(), false, *m_fallbackUnicodeMap);
            }

            return true;
        }
    }

    // Try to create an Unicode to GID char map from legacy "encodings"
    // (or better charmaps), as reported by FreeType

    if (FT_Select_Charmap(m_Face.get(), FT_ENCODING_APPLE_ROMAN) == 0)
    {
        unordered_map<unsigned, unsigned> codeToGIDmap;
        collectCharCodeToGIDMap(m_Face.get(), false, codeToGIDmap);
        m_fallbackUnicodeMap.reset(new unordered_map<uint32_t, unsigned>());
        auto encoding = PdfEncodingMapFactory::MacRomanEncodingInstance();
        encoding->CreateUnicodeToGIDMap(codeToGIDmap, *m_fallbackUnicodeMap);
        return true;
    }

    if (FT_Select_Charmap(m_Face.get(), FT_ENCODING_ADOBE_LATIN_1) == 0)
    {
        unordered_map<unsigned, unsigned> codeToGIDmap;
        collectCharCodeToGIDMap(m_Face.get(), false, codeToGIDmap);
        m_fallbackUnicodeMap.reset(new unordered_map<uint32_t, unsigned>());
        auto encoding = PdfEncodingMapFactory::AppleLatin1EncodingInstance();
        encoding->CreateUnicodeToGIDMap(codeToGIDmap, *m_fallbackUnicodeMap);
        return true;
    }

    if (FT_Select_Charmap(m_Face.get(), FT_ENCODING_ADOBE_STANDARD) == 0)
    {
        unordered_map<unsigned, unsigned> codeToGIDmap;
        collectCharCodeToGIDMap(m_Face.get(), false, codeToGIDmap);
        m_fallbackUnicodeMap.reset(new unordered_map<uint32_t, unsigned>());
        auto encoding = PdfEncodingMapFactory::StandardEncodingInstance();
        encoding->CreateUnicodeToGIDMap(codeToGIDmap, *m_fallbackUnicodeMap);
        return true;
    }

    if (FT_Select_Charmap(m_Face.get(), FT_ENCODING_ADOBE_EXPERT) == 0)
    {
        unordered_map<unsigned, unsigned> codeToGIDmap;
        collectCharCodeToGIDMap(m_Face.get(), false, codeToGIDmap);
        m_fallbackUnicodeMap.reset(new unordered_map<uint32_t, unsigned>());
        auto encoding = PdfEncodingMapFactory::MacExpertEncodingInstance();
        encoding->CreateUnicodeToGIDMap(codeToGIDmap, *m_fallbackUnicodeMap);
        return true;
    }

    // CHECK-ME1: Try to merge maps if multiple encodings?
    // CHECK-ME2: Support more encodings as reported by FreeType?
    PoDoFo::LogMessage(PdfLogSeverity::Warning, "Could not create an unicode map for the font {}", m_FontName);
    return false;
}

PdfFontDescriptorFlags PdfFontMetricsFreetype::GetFlags() const
{
    return m_Flags;
}

double PdfFontMetricsFreetype::GetDefaultWidthRaw() const
{
    return m_DefaultWidth;
}

void PdfFontMetricsFreetype::GetBoundingBox(vector<double>& bbox) const
{
    bbox.clear();
    bbox.push_back(m_Face->bbox.xMin / (double)m_Face->units_per_EM);
    bbox.push_back(m_Face->bbox.yMin / (double)m_Face->units_per_EM);
    bbox.push_back(m_Face->bbox.xMax / (double)m_Face->units_per_EM);
    bbox.push_back(m_Face->bbox.yMax / (double)m_Face->units_per_EM);
}

bool PdfFontMetricsFreetype::getIsBoldHint() const
{
    return (m_Face->style_flags & FT_STYLE_FLAG_BOLD) != 0;
}

bool PdfFontMetricsFreetype::getIsItalicHint() const
{
    return (m_Face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;
}

const PdfCIDToGIDMapConstPtr& PdfFontMetricsFreetype::getCIDToGIDMap() const
{
    return m_CIDToGIDMap;
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

double PdfFontMetricsFreetype::GetAscent() const
{
    return m_Ascent;
}

double PdfFontMetricsFreetype::GetDescent() const
{
    return m_Descent;
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

const FreeTypeFacePtr& PdfFontMetricsFreetype::GetFaceHandle() const
{
    return m_Face;
}

int PdfFontMetricsFreetype::GetWeightRaw() const
{
    return m_Weight;
}

double PdfFontMetricsFreetype::GetCapHeight() const
{
    return m_CapHeight;
}

double PdfFontMetricsFreetype::GetXHeightRaw() const
{
    return m_XHeight;
}

double PdfFontMetricsFreetype::GetStemV() const
{
    return m_StemV;
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

double PdfFontMetricsFreetype::GetItalicAngle() const
{
    return m_ItalicAngle;
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

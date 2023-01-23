/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontMetricsFreetype.h"

#include <podofo/private/FreetypePrivate.h>
#include FT_TRUETYPE_TAGS_H
#include FT_TRUETYPE_TABLES_H
#include FT_TYPE1_TABLES_H
#include FT_FONT_FORMATS_H

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfVariant.h"
#include "PdfFont.h"
#include "PdfCMapEncoding.h"

using namespace std;
using namespace PoDoFo;

static PdfFontFileType determineTrueTypeFormat(FT_Face face);
static int determineType1FontWeight(const string_view& weight);

PdfFontMetricsFreetype::PdfFontMetricsFreetype(const datahandle& data, const FreeTypeFacePtr& face,
        const PdfFontMetrics* refMetrics) :
    m_Data(data),
    m_Face(face),
    m_LengthsReady(false),
    m_Length1(0),
    m_Length2(0),
    m_Length3(0)
{
    if (face == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The buffer can't be null");

    initFromFace(refMetrics);
}

PdfFontMetricsFreetype::PdfFontMetricsFreetype(const datahandle& data, const FreeTypeFacePtr& face)
    : PdfFontMetricsFreetype(data, face, nullptr)
{
}

unique_ptr<PdfFontMetricsFreetype> PdfFontMetricsFreetype::FromMetrics(const PdfFontMetrics& metrics)
{
    return unique_ptr<PdfFontMetricsFreetype>(new PdfFontMetricsFreetype(metrics.GetFontFileDataHandle(),
        metrics.GetFaceHandle(), &metrics));
}

unique_ptr<PdfFontMetricsFreetype> PdfFontMetricsFreetype::FromBuffer(const std::shared_ptr<const charbuff>& buffer)
{
    FreeTypeFacePtr face = FT::CreateFaceFromBuffer(*buffer);
    return unique_ptr<PdfFontMetricsFreetype>(new PdfFontMetricsFreetype(buffer, face));
}

unique_ptr<PdfFontMetricsFreetype> PdfFontMetricsFreetype::FromFace(FT_Face face)
{
    if (face == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Face can't be null");

    // Increment the refcount for the face
    FT_Reference_Face(face);
    return unique_ptr<PdfFontMetricsFreetype>(new PdfFontMetricsFreetype(
        shared_ptr<const charbuff>(new charbuff(FT::GetDataFromFace(face))), face));
}

void PdfFontMetricsFreetype::initFromFace(const PdfFontMetrics* refMetrics)
{
    FT_Error rc;

    string format = FT_Get_Font_Format(m_Face.get());
    if (format == "TrueType")
        m_FontFileType = determineTrueTypeFormat(m_Face.get());
    else if (format == "Type 1")
        m_FontFileType = PdfFontFileType::Type1;
    else if (format == "CID Type 1")
        m_FontFileType = PdfFontFileType::CIDType1;
    else if (format == "CFF")
        m_FontFileType = PdfFontFileType::Type1CCF;
    else
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Unsupported font type");

    // Get the postscript name of the font and ensures it has no space:
    // 5.5.2 TrueType Fonts, "If the name contains any spaces, the spaces are removed"
    auto psname = FT_Get_Postscript_Name(m_Face.get());
    if (psname != nullptr)
        m_FontName = psname;
    m_FontName.erase(std::remove(m_FontName.begin(), m_FontName.end(), ' '), m_FontName.end());
    m_FontBaseName = PdfFont::ExtractBaseName(m_FontName);
    if (m_Face->family_name != nullptr)
        m_FontFamilyName = m_Face->family_name;

    m_HasUnicodeMapping = false;
    m_HasSymbolCharset = false;

    // Try to get a unicode charmap
    rc = FT_Select_Charmap(m_Face.get(), FT_ENCODING_UNICODE);
    if (rc == 0)
    {
        m_HasUnicodeMapping = true;
    }
    else
    {
        // Try to determine if it is a symbol font
        for (int c = 0; c < m_Face->num_charmaps; c++)
        {
            FT_CharMap charmap = m_Face->charmaps[c];
            if (charmap->encoding == FT_ENCODING_MS_SYMBOL)
            {
                m_HasUnicodeMapping = true;
                m_HasSymbolCharset = true;
                rc = FT_Set_Charmap(m_Face.get(), charmap);
                break;
            }
        }
    }

    // calculate the line spacing now, as it changes only with the font size
    m_LineSpacing = m_Face->height / (double)m_Face->units_per_EM;
    m_UnderlineThickness = m_Face->underline_thickness / (double)m_Face->units_per_EM;
    m_UnderlinePosition = m_Face->underline_position / (double)m_Face->units_per_EM;
    m_Ascent = m_Face->ascender / (double)m_Face->units_per_EM;
    m_Descent = m_Face->descender / (double)m_Face->units_per_EM;

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

        m_StrikeOutPosition = m_Ascent / 2.0;
        m_StrikeOutThickness = m_UnderlineThickness;
    }
    else
    {
        m_CIDToGIDMap = refMetrics->GetCIDToGIDMap();

        if (m_FontName.empty())
            m_FontName = refMetrics->GetFontName();
        if (m_FontBaseName.empty())
            m_FontBaseName = refMetrics->GetBaseFontName();
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

        m_StrikeOutPosition = refMetrics->GetStrikeOutPosition();
        m_StrikeOutThickness = refMetrics->GetStrikeOutThickness();
    }

    // OS2 Table is available only in TT fonts
    TT_OS2* os2Table = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(m_Face.get(), FT_SFNT_OS2));
    if (os2Table != nullptr)
    {
        m_StrikeOutPosition = os2Table->yStrikeoutPosition / (double)m_Face->units_per_EM;
        m_StrikeOutThickness = os2Table->yStrikeoutSize / (double)m_Face->units_per_EM;
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

    // FontInfo Table is available only in type1 fonts
    PS_FontInfoRec type1Info;
    rc = FT_Get_PS_Font_Info(m_Face.get(), &type1Info);
    if (rc == 0)
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
            // Other font types dont't need lengths
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
    if (m_HasSymbolCharset)
        codePoint = codePoint | 0xF000;

    // NOTE: FT_Get_Char_Index returns 0 when no map is selected
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

double PdfFontMetricsFreetype::GetStrikeOutPosition() const
{
    return m_StrikeOutPosition;
}

double PdfFontMetricsFreetype::GetUnderlineThickness() const
{
    return m_UnderlineThickness;
}

double PdfFontMetricsFreetype::GetStrikeOutThickness() const
{
    return m_StrikeOutThickness;
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

// Determines if the font is legacy TrueType or OpenType
PdfFontFileType determineTrueTypeFormat(FT_Face face)
{
    FT_Error rc;
    FT_ULong size;
    FT_ULong tag;
    rc = FT_Sfnt_Table_Info(face, 0, nullptr, &size);
    CHECK_FT_RC(rc, FT_Sfnt_Table_Info);
    for (FT_ULong i = 0, count = size; i < count; i++)
    {
        rc = FT_Sfnt_Table_Info(face, i, &tag, &size);
        switch (tag)
        {
            // Legacy TrueType tables
            // https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6.html
            case TTAG_acnt:
            case TTAG_ankr:
            case TTAG_avar:
            case TTAG_bdat:
            case TTAG_bhed:
            case TTAG_bloc:
            case TTAG_bsln:
            case TTAG_cmap:
            case TTAG_cvar:
            case TTAG_cvt:
            case TTAG_EBSC:
            case TTAG_fdsc:
            case TTAG_feat:
            case TTAG_fmtx:
            case TTAG_fond:
            case TTAG_fpgm:
            case TTAG_fvar:
            case TTAG_gasp:
            case TTAG_gcid:
            case TTAG_glyf:
            case TTAG_gvar:
            case TTAG_hdmx:
            case TTAG_head:
            case TTAG_hhea:
            case TTAG_hmtx:
            case TTAG_just:
            case TTAG_kern:
            case TTAG_kerx:
            case TTAG_lcar:
            case TTAG_loca:
            case TTAG_ltag:
            case TTAG_maxp:
            case TTAG_meta:
            case TTAG_mort:
            case TTAG_morx:
            case TTAG_name:
            case TTAG_opbd:
            case TTAG_OS2:
            case TTAG_post:
            case TTAG_prep:
            case TTAG_prop:
            case TTAG_sbix:
            case TTAG_trak:
            case TTAG_vhea:
            case TTAG_vmtx:
            case TTAG_xref:
            case TTAG_Zapf:
                // Continue on legacy tables
                break;
            default:
                // Return OpenType on all other tables
                return PdfFontFileType::OpenType;
        }
    }

    // Default legay TrueType
    return PdfFontFileType::TrueType;
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

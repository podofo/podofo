/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontMetrics.h"

#include <podofo/private/FreetypePrivate.h>

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfVariant.h"
#include "PdfEncodingMapFactory.h"
#include "PdfFont.h"
#include "PdfIdentityEncoding.h"

using namespace std;
using namespace PoDoFo;

// Default matrix: thousands of PDF units
static Matrix2D s_DefaultMatrix = { 1e-3, 0.0, 0.0, 1e-3, 0, 0 };

PdfFontMetrics::PdfFontMetrics() : m_FaceIndex(0) { }

PdfFontMetrics::~PdfFontMetrics() { }

double PdfFontMetrics::GetGlyphWidth(unsigned gid) const
{
    double width;
    if (!TryGetGlyphWidth(gid, width))
        return GetDefaultWidth();

    return width;
}

void PdfFontMetrics::SubstituteGIDs(vector<unsigned>& gids, vector<unsigned char>& backwardMap) const
{
    // By default do nothing and return a map to
    backwardMap.resize(gids.size(), 1);
    // TODO: Try to implement the mechanism in some font type
}

bool PdfFontMetrics::HasFontFileData() const
{
    return GetOrLoadFontFileData().size() != 0;
}

bufferview PdfFontMetrics::GetOrLoadFontFileData() const
{
    return GetFontFileDataHandle().view();
}

FT_Face PdfFontMetrics::GetOrLoadFace() const
{
    FT_Face face;
    if (!TryGetOrLoadFace(face))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::FreeType, "Error loading FreeType face");

    return face;
}

bool PdfFontMetrics::TryGetOrLoadFace(FT_Face& face) const
{
    face = GetFaceHandle().get();
    return face != nullptr;
}

const PdfObject* PdfFontMetrics::GetFontFileObject() const
{
    // Return nullptr by default
    return nullptr;
}

string_view PdfFontMetrics::GetFontNameSafe(bool familyFirst) const
{
    if (familyFirst)
    {
        auto baseFontName = GetFontFamilyName();
        if (!baseFontName.empty())
            return baseFontName;

        return GetFontName();
    }
    else
    {
        auto fontName = GetFontName();
        if (!fontName.empty())
            return fontName;

        return GetFontFamilyName();
    }
}

string_view PdfFontMetrics::GetBaseFontNameSafe() const
{
    const_cast<PdfFontMetrics&>(*this).initBaseFontNameSafe();
    return *m_BaseFontNameSafe;
}

void PdfFontMetrics::initBaseFontNameSafe()
{
    if (m_BaseFontNameSafe != nullptr)
        return;

    m_BaseFontNameSafe.reset(new string(GetBaseFontName()));
    if (m_BaseFontNameSafe->length() == 0)
        *m_BaseFontNameSafe = PdfFont::ExtractBaseName(GetFontFamilyName());;
}

string_view PdfFontMetrics::GetFontNameRaw() const
{
    return GetFontName();
}

unsigned PdfFontMetrics::GetWeight() const
{
    int weight = GetWeightRaw();
    if (weight < 0)
    {
        if ((GetStyle() & PdfFontStyle::Bold) == PdfFontStyle::Bold)
            return 700;
        else
            return 400;
    }

    return (unsigned)weight;
}

double PdfFontMetrics::GetLeading() const
{
    double leading = GetLeadingRaw();
    if (leading < 0)
        return 0;

    return leading;
}

double PdfFontMetrics::GetXHeight() const
{
    double xHeight = GetXHeightRaw();
    if (xHeight < 0)
        return 0;

    return xHeight;
}

double PdfFontMetrics::GetStemH() const
{
    double stemH = GetStemHRaw();
    if (stemH < 0)
        return 0;

    return stemH;
}

double PdfFontMetrics::GetAvgWidth() const
{
    double avgWidth = GetAvgWidthRaw();
    if (avgWidth < 0)
        return 0;

    return avgWidth;
}

double PdfFontMetrics::GetMaxWidth() const
{
    double maxWidth = GetMaxWidthRaw();
    if (maxWidth < 0)
        return 0;

    return maxWidth;
}

double PdfFontMetrics::GetDefaultWidth() const
{
    double defaultWidth = GetDefaultWidthRaw();
    if (defaultWidth < 0)
        return 0;

    return defaultWidth;
}

PdfFontStyle PdfFontMetrics::GetStyle() const
{
    if (m_Style.has_value())
        return *m_Style;

    // ISO 32000-1:2008: Table 122 – Entries common to all font descriptors
    // The possible values shall be 100, 200, 300, 400, 500, 600, 700, 800,
    // or 900, where each number indicates a weight that is at least as dark
    // as its predecessor. A value of 400 shall indicate a normal weight;
    // 700 shall indicate bold
    bool isBold = getIsBoldHint()
        || GetWeightRaw() >= 700;
    bool isItalic = getIsItalicHint()
        || (GetFlags() & PdfFontDescriptorFlags::Italic) != PdfFontDescriptorFlags::None
        || GetItalicAngle() != 0;
    PdfFontStyle style = PdfFontStyle::Regular;
    if (isBold)
        style |= PdfFontStyle::Bold;
    if (isItalic)
        style |= PdfFontStyle::Italic;
    const_cast<PdfFontMetrics&>(*this).m_Style = style;
    return *m_Style;
}

bool PdfFontMetrics::IsStandard14FontMetrics() const
{
    PdfStandard14FontType std14Font;
    return IsStandard14FontMetrics(std14Font);
}

bool PdfFontMetrics::IsStandard14FontMetrics(PdfStandard14FontType& std14Font) const
{
    std14Font = PdfStandard14FontType::Unknown;
    return false;
}

const Matrix2D& PdfFontMetrics::GetMatrix() const
{
    return s_DefaultMatrix;
}

bool PdfFontMetrics::IsType1Kind() const
{
    switch (GetFontFileType())
    {
        case PdfFontFileType::Type1:
        case PdfFontFileType::Type1CCF:
        case PdfFontFileType::CIDType1:
            return true;
        default:
            return false;
    }
}

bool PdfFontMetrics::IsTrueTypeKind() const
{
    switch (GetFontFileType())
    {
        case PdfFontFileType::TrueType:
        case PdfFontFileType::OpenType:
            return true;
        default:
            return false;
    }
}

bool PdfFontMetrics::IsPdfSymbolic() const
{
    auto flags = GetFlags();
    return (flags & PdfFontDescriptorFlags::Symbolic) != PdfFontDescriptorFlags::None
        || (flags & PdfFontDescriptorFlags::NonSymbolic) == PdfFontDescriptorFlags::None;
}

bool PdfFontMetrics::IsPdfNonSymbolic() const
{
    auto flags = GetFlags();
    return (flags & PdfFontDescriptorFlags::Symbolic) == PdfFontDescriptorFlags::None
        && (flags & PdfFontDescriptorFlags::NonSymbolic) != PdfFontDescriptorFlags::None;
}

unique_ptr<PdfCMapEncoding> PdfFontMetrics::CreateToUnicodeMap(const PdfEncodingLimits& limitHints) const
{
    (void)limitHints;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

bool PdfFontMetrics::TryGetImplicitEncoding(PdfEncodingMapConstPtr& encoding) const
{
    PdfStandard14FontType std14Font;
    // Implicit base encoding can be :
    // 1) The implicit encoding of a standard 14 font
    if (IsStandard14FontMetrics(std14Font))
    {
        encoding = PdfEncodingMapFactory::GetStandard14FontEncodingMap(std14Font);
        return true;
    }
    else if (IsType1Kind())
    {
        // 2.1) An encoding stored in the font program (Type1)
        // ISO 32000-1:2008 9.6.6.2 "Encodings for Type 1 Fonts"
        FT_Face face;
        if (TryGetOrLoadFace(face))
        {
            encoding = getFontType1Encoding(face);
            return true;
        }
    }
    else if (IsTrueTypeKind())
    {
        // 2.2) An encoding stored in the font program (TrueType)
        // ISO 32000-1:2008 9.6.6.4 "Encodings for TrueType Fonts"
        // NOTE: We just take the inferred builtin CID to GID map and we create
        // a identity encoding of the maximum code size. It should always be 1
        // anyway
        auto& map = getCIDToGIDMap();
        if (map != nullptr)
        {
            // Find the maximum CID code size
            unsigned maxCID = 0;
            for (auto& pair : *map)
            {
                if (pair.first > maxCID)
                    maxCID = pair.first;
            }

            encoding = std::make_shared<PdfIdentityEncoding>(utls::GetCharCodeSize(maxCID));
            return true;
        }
    }

    // As a last chance, try check if the font name is actually a Standard14
    if (PdfFont::IsStandard14Font(GetFontNameSafe(), std14Font))
    {
        encoding = PdfEncodingMapFactory::GetStandard14FontEncodingMap(std14Font);
        return true;
    }

    encoding = nullptr;
    return false;
}

PdfCIDToGIDMapConstPtr PdfFontMetrics::GetCIDToGIDMap() const
{
    return getCIDToGIDMap();
}

const PdfCIDToGIDMapConstPtr& PdfFontMetrics::getCIDToGIDMap() const
{
    static PdfCIDToGIDMapConstPtr s_null;
    return s_null;
}

PdfFontMetricsBase::PdfFontMetricsBase()
    : m_dataInit(false), m_faceInit(false) { }

const datahandle& PdfFontMetricsBase::GetFontFileDataHandle() const
{
    if (!m_dataInit)
    {
        auto& rthis = const_cast<PdfFontMetricsBase&>(*this);
        rthis.m_Data = getFontFileDataHandle();
        rthis.m_dataInit = true;
    }

    return m_Data;
}

const FreeTypeFacePtr& PdfFontMetricsBase::GetFaceHandle() const
{
    if (!m_faceInit)
    {
        auto& rthis = const_cast<PdfFontMetricsBase&>(*this);
        auto view = GetFontFileDataHandle().view();
        FT_Face face;
        if (view.size() != 0 && FT::TryCreateFaceFromBuffer(view, face))
            rthis.m_Face = FreeTypeFacePtr(face);
        else
            rthis.m_Face = FreeTypeFacePtr();

        rthis.m_faceInit = true;
    }

    return m_Face;
}

FreeTypeFacePtr::FreeTypeFacePtr() { }

FreeTypeFacePtr::FreeTypeFacePtr(FT_Face face)
    : shared_ptr<FT_FaceRec_>(face, FT_Done_Face) {}

void FreeTypeFacePtr::reset(FT_Face face)
{
    shared_ptr<FT_FaceRec_>::reset(face, FT_Done_Face);
}

void PdfFontMetrics::SetFilePath(std::string&& filepath, unsigned faceIndex)
{
    m_FilePath = std::move(filepath);
    m_FaceIndex = faceIndex;
}

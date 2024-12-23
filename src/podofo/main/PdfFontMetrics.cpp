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
#include "PdfFontMetricsFreetype.h"

using namespace std;
using namespace PoDoFo;

static FT_Face getFontFaceFromFile(const string_view& filepath, unsigned faceIndex, unique_ptr<charbuff>& data);
static FT_Face getFontFaceFromBuffer(const bufferview& view, unsigned faceIndex, unique_ptr<charbuff>& data);

// Default matrix: thousands of PDF units
static Matrix s_DefaultMatrix = { 1e-3, 0.0, 0.0, 1e-3, 0, 0 };

PdfFontMetrics::PdfFontMetrics() : m_FaceIndex(0) { }

PdfFontMetrics::~PdfFontMetrics() { }

unique_ptr<const PdfFontMetrics> PdfFontMetrics::Create(const string_view& filepath, unsigned faceIndex)
{
    return Create(filepath, faceIndex, nullptr);
}
unique_ptr<const PdfFontMetrics> PdfFontMetrics::Create(const string_view& filepath, unsigned faceIndex, const PdfFontMetrics* refMetrics)
{
    unique_ptr<charbuff> data;
    auto face = getFontFaceFromFile(filepath, faceIndex, data);
    if (face == nullptr)
        return nullptr;

    unique_ptr<PdfFontMetrics> ret(new PdfFontMetricsFreetype(face, std::move(data), refMetrics));
    ret->m_FilePath = filepath;
    ret->m_FaceIndex = faceIndex;
    return ret;
}

unique_ptr<const PdfFontMetrics> PdfFontMetrics::CreateFromBuffer(const bufferview& buffer, unsigned faceIndex)
{
    unique_ptr<charbuff> data;
    auto face = getFontFaceFromBuffer(buffer, faceIndex, data);
    if (face == nullptr)
        return nullptr;

    unique_ptr<PdfFontMetrics> metrics(new PdfFontMetricsFreetype(face, std::move(data)));
    metrics->m_FaceIndex = faceIndex;
    return metrics;
}

unsigned PdfFontMetrics::GetGlyphCount() const
{
    return GetGlyphCountFontProgram();
}

unsigned PdfFontMetrics::GetGlyphCount(PdfGlyphAccess access) const
{
    switch (access)
    {
        case PdfGlyphAccess::ReadMetrics:
        {
            if (m_ParsedWidths == nullptr)
                return 0;

            return (unsigned)m_ParsedWidths->size();
        }
        case PdfGlyphAccess::FontProgram:
            return GetGlyphCountFontProgram();
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

double PdfFontMetrics::GetGlyphWidth(unsigned gid) const
{
    double width;
    if (!TryGetGlyphWidth(gid, width))
        return GetDefaultWidth();

    return width;
}

double PdfFontMetrics::GetGlyphWidth(unsigned gid, PdfGlyphAccess access) const
{
    double width;
    if (!TryGetGlyphWidth(gid, access, width))
        return GetDefaultWidth();

    return width;
}

bool PdfFontMetrics::TryGetGlyphWidth(unsigned gid, double& width) const
{
    if (m_ParsedWidths != nullptr)
    {
        if (gid >= m_ParsedWidths->size())
        {
            width = -1;
            return false;
        }

        width = (*m_ParsedWidths)[gid];
        return true;
    }

    return TryGetGlyphWidthFontProgram(gid, width);
}

bool PdfFontMetrics::TryGetGlyphWidth(unsigned gid, PdfGlyphAccess access, double& width) const
{
    switch (access)
    {
        case PdfGlyphAccess::ReadMetrics:
        {
            if (m_ParsedWidths == nullptr || gid >= m_ParsedWidths->size())
            {
                width = -1;
                return false;
            }

            width = (*m_ParsedWidths)[gid];
            return true;
        }
        case PdfGlyphAccess::FontProgram:
            return TryGetGlyphWidthFontProgram(gid, width);
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
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

const PdfObject* PdfFontMetrics::GetFontFileObject() const
{
    // Return nullptr by default
    return nullptr;
}

string_view PdfFontMetrics::GeFontFamilyNameSafe() const
{
    const_cast<PdfFontMetrics&>(*this).initFamilyFontNameSafe();
    return m_FamilyFontNameSafe;
}

unsigned char PdfFontMetrics::GetSubsetPrefixLength() const
{
    // By default return no prefix
    return 0;
}

string_view PdfFontMetrics::GetPostScriptNameRough() const
{
    return GetFontName().substr(GetSubsetPrefixLength());
}

void PdfFontMetrics::SetParsedWidths(GlyphMetricsListConstPtr&& parsedWidths)
{
    m_ParsedWidths = std::move(parsedWidths);
}

void PdfFontMetrics::initFamilyFontNameSafe()
{
    if (m_FamilyFontNameSafe.length() != 0)
        return;

    m_FamilyFontNameSafe = GetFontFamilyName();
    if (m_FamilyFontNameSafe.length() == 0)
        m_FamilyFontNameSafe = GetBaseFontName();

    PODOFO_ASSERT(m_FamilyFontNameSafe.length() != 0);
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

bool PdfFontMetrics::IsObjectLoaded() const
{
    return false;
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

const Matrix& PdfFontMetrics::GetMatrix() const
{
    return s_DefaultMatrix;
}

bool PdfFontMetrics::IsType1Kind() const
{
    switch (GetFontFileType())
    {
        case PdfFontFileType::Type1:
        case PdfFontFileType::Type1CFF:
            return true;
        default:
            return false;
    }
}

bool PdfFontMetrics::IsTrueTypeKind() const
{
    return GetFontFileType() == PdfFontFileType::TrueType;
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
        auto face = GetFaceHandle();
        if (face != nullptr)
        {
            encoding = getFontType1ImplicitEncoding(face);
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
    if (PdfFont::IsStandard14Font(GetFontName(), std14Font))
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

unsigned PdfFontMetrics::GetGlyphCountFontProgram() const
{
    auto face = GetFaceHandle();
    return (unsigned)face->num_glyphs;
}

bool PdfFontMetrics::TryGetGlyphWidthFontProgram(unsigned gid, double& width) const
{
    auto face = GetFaceHandle();
    if (face == nullptr || FT_Load_Glyph(face, gid, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP) != 0)
    {
        width = -1;
        return false;
    }

    // zero return code is success!
    width = face->glyph->metrics.horiAdvance / (double)face->units_per_EM;
    return true;
}

bool PdfFontMetrics::HasParsedWidths() const
{
    return m_ParsedWidths != nullptr;
}

PdfFontMetricsBase::PdfFontMetricsBase()
    : m_dataInit(false), m_faceInit(false), m_Face(nullptr) { }

PdfFontMetricsBase::~PdfFontMetricsBase()
{
    FT_Done_Face(m_Face);
}

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

FT_Face PdfFontMetricsBase::GetFaceHandle() const
{
    if (!m_faceInit)
    {
        auto& rthis = const_cast<PdfFontMetricsBase&>(*this);
        auto view = GetFontFileDataHandle().view();
        // NOTE: The data always represent a face, collections are not 
        if (view.size() != 0)
            rthis.m_Face = FT::CreateFaceFromBuffer(view);

        rthis.m_faceInit = true;
    }

    return m_Face;
}

FT_Face getFontFaceFromFile(const string_view& filepath, unsigned faceIndex, unique_ptr<charbuff>& data)
{
    charbuff buffer;
    auto face = FT::CreateFaceFromFile(filepath, faceIndex, buffer);
    if (face == nullptr)
    {
        PoDoFo::LogMessage(PdfLogSeverity::Error, "Error when loading the face from buffer");
        return nullptr;
    }

    if (!FT::IsPdfSupported(face))
        return nullptr;

    data.reset(new charbuff(std::move(buffer)));
    return face;
}

FT_Face getFontFaceFromBuffer(const bufferview& view, unsigned faceIndex, unique_ptr<charbuff>& data)
{
    charbuff buffer;
    auto face = FT::CreateFaceFromBuffer(view, faceIndex, buffer);
    if (face == nullptr)
    {
        PoDoFo::LogMessage(PdfLogSeverity::Error, "Error when loading the face from buffer");
        return nullptr;
    }

    if (!FT::IsPdfSupported(face))
        return nullptr;

    data.reset(new charbuff(std::move(buffer)));
    return face;
}

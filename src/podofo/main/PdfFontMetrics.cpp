/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontMetrics.h"

#include <podofo/private/FreetypePrivate.h>
#include <podofo/private/FontUtils.h>

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfVariant.h"
#include "PdfEncodingMapFactory.h"
#include "PdfFont.h"
#include "PdfIdentityEncoding.h"
#include "PdfFontMetricsFreetype.h"

using namespace std;
using namespace PoDoFo;

// Default matrix: thousands of PDF units
static Matrix s_DefaultMatrix = { 1e-3, 0.0, 0.0, 1e-3, 0, 0 };

PdfFontMetrics::PdfFontMetrics() : m_FaceIndex(0) { }

PdfFontMetrics::~PdfFontMetrics() { }

unique_ptr<const PdfFontMetrics> PdfFontMetrics::Create(const string_view& filepath, unsigned faceIndex)
{
    return CreateFromFile(filepath, faceIndex, nullptr, false);
}
unique_ptr<const PdfFontMetrics> PdfFontMetrics::CreateFromFile(const string_view& filepath, unsigned faceIndex,
    const PdfFontMetrics* refMetrics, bool skipNormalization)
{
    charbuff buffer;
    unique_ptr<FT_FaceRec_, decltype(&FT_Done_Face)> face(FT::CreateFaceFromFile(filepath, faceIndex, buffer), FT_Done_Face);
    if (face == nullptr)
    {
        PoDoFo::LogMessage(PdfLogSeverity::Error, "Error when loading the face from buffer");
        return nullptr;
    }
    auto ret = CreateFromFace(face.get(), std::make_unique<charbuff>(std::move(buffer)), refMetrics, skipNormalization);
    if (ret != nullptr)
    {
        ret->m_FilePath = filepath;
        ret->m_FaceIndex = faceIndex;
    }

    (void)face.release();
    return ret;
}

unique_ptr<const PdfFontMetrics> PdfFontMetrics::CreateFromBuffer(const bufferview& buffer, unsigned faceIndex)
{
    return CreateFromBuffer(buffer, faceIndex, nullptr, false);
}

unique_ptr<const PdfFontMetrics> PdfFontMetrics::CreateFromBuffer(const bufferview& view, unsigned faceIndex,
    const PdfFontMetrics* refMetrics, bool skipNormalization)
{
    charbuff buffer;
    unique_ptr<FT_FaceRec_, decltype(&FT_Done_Face)> face(FT::CreateFaceFromBuffer(view, faceIndex, buffer), FT_Done_Face);
    if (face == nullptr)
    {
        PoDoFo::LogMessage(PdfLogSeverity::Error, "Error when loading the face from buffer");
        return nullptr;
    }

    auto ret = CreateFromFace(face.get(), std::make_unique<charbuff>(std::move(buffer)), refMetrics, skipNormalization);
    if (ret != nullptr)
        ret->m_FaceIndex = faceIndex;

    (void)face.release();
    return ret;
}

unique_ptr<const PdfFontMetrics> PdfFontMetrics::CreateMergedMetrics(bool skipNormalization) const
{
    if (!skipNormalization)
    {
        auto fontType = GetFontFileType();
        if (fontType == PdfFontFileType::Type1)
        {
            // Unconditionally convert the Type1 font to CFF: this allow
            // the font file to be insterted in a CID font
            charbuff cffDest;
            PoDoFo::ConvertFontType1ToCFF(GetOrLoadFontFileData(), cffDest);
            unique_ptr<FT_FaceRec_, decltype(&FT_Done_Face)> face(FT::CreateFaceFromBuffer(cffDest), FT_Done_Face);
            auto ret = unique_ptr<PdfFontMetricsFreetype>(new PdfFontMetricsFreetype(
                face.get(), datahandle(std::move(cffDest)), this));
            (void)face.release();
            return ret;
        }
    }

    auto face = GetFaceHandle();
    auto ret = unique_ptr<PdfFontMetricsFreetype>(new PdfFontMetricsFreetype(face,
        GetFontFileDataHandle(), this));
    // Reference the face after having created a new PdfFontMetricsFreetype instance
    FT_Reference_Face(face);
    return ret;
}

unique_ptr<PdfFontMetrics> PdfFontMetrics::CreateFromFace(FT_Face face, unique_ptr<charbuff>&& buffer,
    const PdfFontMetrics* refMetrics, bool skipNormalization)
{
    PdfFontFileType fontType;
    if (!FT::TryGetFontFileFormat(face, fontType))
        return nullptr;

    if (!skipNormalization)
    {
        if (fontType == PdfFontFileType::Type1)
        {
            // Unconditionally convert the Type1 font to CFF: this allow
            // the font file to be insterted in a CID font
            charbuff cffDest;
            PoDoFo::ConvertFontType1ToCFF(*buffer, cffDest);
            unique_ptr<FT_FaceRec_, decltype(&FT_Done_Face)> newface(FT::CreateFaceFromBuffer(cffDest), FT_Done_Face);
            auto ret = unique_ptr<PdfFontMetricsFreetype>(new PdfFontMetricsFreetype(
                newface.get(), datahandle(std::move(cffDest)), refMetrics));
            (void)newface.release();
            return ret;
        }
    }

    return unique_ptr<PdfFontMetrics>(new PdfFontMetricsFreetype(face, datahandle(std::move(buffer)), refMetrics));
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

PdfCIDToGIDMapConstPtr PdfFontMetrics::GetBuiltinCIDToGIDMap() const
{
    // By default assume there's no map available
    return nullptr;
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

PdfFontDescriptorFlags PdfFontMetrics::GetFlags() const
{
    PdfFontDescriptorFlags ret;
    (void)TryGetFlags(ret);
    return ret;
}

Corners PdfFontMetrics::GetBoundingBox() const
{
    Corners ret;
    (void)TryGetBoundingBox(ret);
    return ret;
}

double PdfFontMetrics::GetItalicAngle() const
{
    double ret;
    (void)TryGetItalicAngle(ret);
    return ret;
}

double PdfFontMetrics::GetAscent() const
{
    double ret;
    (void)TryGetAscent(ret);
    return ret;
}

double PdfFontMetrics::GetDescent() const
{
    double ret;
    (void)TryGetDescent(ret);
    return ret;
}

double PdfFontMetrics::GetCapHeight() const
{
    double ret;
    (void)TryGetCapHeight(ret);
    return ret;
}

double PdfFontMetrics::GetStemV() const
{
    double ret;
    (void)TryGetStemV(ret);
    return ret;
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

PdfFontType PdfFontMetrics::GetFontType() const
{
    return PdfFontType::Unknown;
}

PdfEncodingMapConstPtr PdfFontMetrics::GetImplicitEncoding(PdfCIDToGIDMapConstPtr& cidToGidMap) const
{
    return getImplicitEncoding(true, cidToGidMap);
}

PdfEncodingMapConstPtr PdfFontMetrics::GetImplicitEncoding() const
{
    PdfCIDToGIDMapConstPtr discard;
    return getImplicitEncoding(false, discard);
}

PdfEncodingMapConstPtr PdfFontMetrics::getImplicitEncoding(bool tryFetchCidToGidMap, PdfCIDToGIDMapConstPtr& cidToGidMap) const
{
    PdfStandard14FontType std14Font;
    // Implicit base encoding can be :
    // 1) The implicit encoding of a standard 14 font
    if (IsStandard14FontMetrics(std14Font))
    {
        return PdfEncodingMapFactory::GetStandard14FontEncodingMap(std14Font);
    }
    else if (IsType1Kind())
    {
        // 2.1) An encoding stored in the font program (Type1)
        // ISO 32000-1:2008 9.6.6.2 "Encodings for Type 1 Fonts"
        auto face = GetFaceHandle();
        if (face != nullptr)
            return getFontType1ImplicitEncoding(face);
    }
    else if (IsTrueTypeKind() && tryFetchCidToGidMap)
    {
        // 2.2) An encoding stored in the font program (TrueType)
        // ISO 32000-1:2008 9.6.6.4 "Encodings for TrueType Fonts"
        // NOTE: We just take the inferred builtin CID to GID map and we create
        // a identity encoding of the maximum code size. It should always be 1
        // anyway
        cidToGidMap = GetBuiltinCIDToGIDMap();
        if (cidToGidMap != nullptr)
        {
            // Find the maximum CID code size
            unsigned maxCID = 0;
            for (auto& pair : *cidToGidMap)
            {
                if (pair.first > maxCID)
                    maxCID = pair.first;
            }

            return std::make_shared<PdfIdentityEncoding>(utls::GetCharCodeSize(maxCID));
        }
    }

    // As a last chance, try check if the font name is actually a Standard14
    if (PdfFont::IsStandard14Font(GetFontName(), std14Font))
        return PdfEncodingMapFactory::GetStandard14FontEncodingMap(std14Font);

    return nullptr;
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
        // NOTE: The data always represents a face, not a collection
        if (view.size() != 0)
            rthis.m_Face = FT::CreateFaceFromBuffer(view);

        rthis.m_faceInit = true;
    }

    return m_Face;
}


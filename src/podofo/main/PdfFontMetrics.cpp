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
#include "PdfDifferenceEncoding.h"
#include "PdfPredefinedEncoding.h"

using namespace std;
using namespace PoDoFo;

static PdfCIDToGIDMapConstPtr getIntrinsicCIDToGIDMapType1(FT_Face face, const PdfEncodingMap& baseEncodings,
    const PdfDifferenceMap* differences);
static PdfCIDToGIDMapConstPtr getIntrinsicCIDToGIDMapTrueType(FT_Face face, const PdfEncodingMap& baseEncodings,
    const PdfDifferenceMap* differences);

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

PdfCIDToGIDMapConstPtr PdfFontMetrics::GetTrueTypeBuiltinCIDToGIDMap() const
{
    PODOFO_ASSERT(GetFontFileType() == PdfFontFileType::TrueType);
    FT_Face face;
    if ((face = GetFaceHandle()) == nullptr
        || face->num_charmaps == 0)
    {
        return nullptr;
    }

    CIDToGIDMap map;

    // ISO 32000-2:2020 "9.6.5.4 Encodings for TrueType fonts"
    // "A TrueType font program’s built-in encoding maps directly
    // from character codes to glyph descriptions by means of an
    // internal data structure called a 'cmap' "
    FT_Error rc;
    FT_ULong code;
    FT_UInt index;

    if (FT_Select_Charmap(face, FT_ENCODING_MS_SYMBOL) == 0)
    {
        code = FT_Get_First_Char(face, &index);
        while (index != 0)
        {
            // "If the font contains a (3, 0) subtable, the range of character
            // codes shall be one of these: 0x0000 - 0x00FF,
            // 0xF000 - 0xF0FF, 0xF100 - 0xF1FF, or 0xF200 - 0xF2FF"
            // NOTE: we just take the first byte
            map.insert({ (unsigned)(code & 0xFF), index });
            code = FT_Get_Next_Char(face, code, &index);
        }
    }
    else
    {
        // "Otherwise, if the font contains a (1, 0) subtable, single bytes
        // from the string shall be used to look  up the associated glyph
        // descriptions from the subtable"
        if (FT_Select_Charmap(face, FT_ENCODING_APPLE_ROMAN) != 0)
        {
            // "If a character cannot be mapped in any of the ways described previously,
            // a PDF processor may supply a mapping of its choosing"
            // NOTE: We just pick the first cmap
            rc = FT_Set_Charmap(face, face->charmaps[0]);
            CHECK_FT_RC(rc, FT_Set_Charmap);
        }

        code = FT_Get_First_Char(face, &index);
        while (index != 0)
        {
            map.insert({ (unsigned)code, index });
            code = FT_Get_Next_Char(face, code, &index);
        }
    }

    return PdfCIDToGIDMapConstPtr(new PdfCIDToGIDMap(std::move(map)));
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

PdfEncodingMapConstPtr PdfFontMetrics::GetDefaultEncoding(PdfCIDToGIDMapConstPtr& cidToGidMap) const
{
    return getDefaultEncoding(true, cidToGidMap);
}

PdfEncodingMapConstPtr PdfFontMetrics::GetDefaultEncoding() const
{
    PdfCIDToGIDMapConstPtr discard;
    return getDefaultEncoding(false, discard);
}

PdfEncodingMapConstPtr PdfFontMetrics::getDefaultEncoding(bool tryFetchCidToGidMap, PdfCIDToGIDMapConstPtr& cidToGidMap) const
{
    PdfStandard14FontType std14Font;
    // Implicit base encoding can be :
    // 1) The implicit encoding of a standard 14 font
    if (IsStandard14FontMetrics(std14Font))
    {
        return PdfEncodingMapFactory::GetStandard14FontEncodingInstancePtr(std14Font);
    }
    else if (IsType1Kind())
    {
        // 2.1) An encoding stored in the font program (Type1)
        // ISO 32000-2:2020 9.6.5.2 "Encodings for Type 1 Fonts"
        auto face = GetFaceHandle();
        if (face != nullptr)
        {
            auto ret = getFontType1BuiltInEncoding(face);
            if (tryFetchCidToGidMap)
                cidToGidMap = getIntrinsicCIDToGIDMapType1(face, *ret, nullptr);

            return ret;
        }
    }
    else if (IsTrueTypeKind() && tryFetchCidToGidMap)
    {
        // 2.2) An encoding stored in the font program (TrueType)
        // ISO 32000-2:2020 9.6.5.4 Encodings for TrueType Fonts
        // "When the font has no Encoding entry..."
        cidToGidMap = GetTrueTypeBuiltinCIDToGIDMap();
        if (cidToGidMap != nullptr)
        {
            // NOTE: We just take the inferred builtin CID to GID map and we create
            // a identity encoding of the maximum code size. It should always be 1
            // anyway
            // CHECK-ME: Is this really correct?

            // Find the maximum CID code size
            unsigned maxCID = 0;
            for (auto& pair : *cidToGidMap)
            {
                if (pair.first > maxCID)
                    maxCID = pair.first;
            }

            return PdfEncodingMapConstPtr(new PdfIdentityEncoding(PdfEncodingMapType::Simple, utls::GetCharCodeSize(maxCID)));
        }
    }

    // As a last chance, try check if the font name is actually a Standard14
    if (PdfFont::IsStandard14Font(GetFontName(), std14Font))
        return PdfEncodingMapFactory::GetStandard14FontEncodingInstancePtr(std14Font);

    return nullptr;
}

unsigned PdfFontMetrics::GetGlyphCountFontProgram() const
{
    auto face = GetFaceHandle();
    if (face == nullptr)
        return 0;

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

void PdfFontMetrics::ExportType3GlyphData(PdfDictionary& fontDict, cspan<string_view> glyphs) const
{
    (void)fontDict;
    (void)glyphs;
    // Do nothing by default
}

bool PdfFontMetrics::HasParsedWidths() const
{
    return m_ParsedWidths != nullptr;
}

unsigned PdfFontMetrics::GetParsedWidthsCount() const
{
    if (m_ParsedWidths == nullptr)
        return 0;

    return (unsigned)m_ParsedWidths->size();
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

PdfCIDToGIDMapConstPtr PdfEncodingMapSimple::GetIntrinsicCIDToGIDMap(const PdfDictionary& fontDict, const PdfFontMetrics& metrics) const
{
    (void)fontDict;
    const PdfEncodingMap* baseEncoding;
    const PdfDifferenceMap* differences;
    switch (metrics.GetFontFileType())
    {
        case PdfFontFileType::Type1:
        case PdfFontFileType::Type1CFF:
        {
            auto face = metrics.GetFaceHandle();
            if (face == nullptr)
                return nullptr;

            GetBaseEncoding(baseEncoding, differences);
            return getIntrinsicCIDToGIDMapType1(face, *baseEncoding, differences);
        }
        case PdfFontFileType::TrueType:
        {
            auto face = metrics.GetFaceHandle();
            if (face == nullptr)
                return nullptr;

            GetBaseEncoding(baseEncoding, differences);
            // ISO 32000-2:2020 9.6.5.4 Encodings for TrueType fonts:
            // "If the font has a named Encoding entry of either MacRomanEncoding or
            // WinAnsiEncoding, or if the font descriptor’s Nonsymbolic flag is set,
            // the PDF processor shall create a table that maps from character codes
            // to glyph names"
            if (typeid(baseEncoding) == typeid(PdfWinAnsiEncoding) 
                || typeid(baseEncoding) == typeid(PdfMacRomanEncoding)
                || (metrics.GetFlags() & PdfFontDescriptorFlags::NonSymbolic) != PdfFontDescriptorFlags::None)
            {
                return getIntrinsicCIDToGIDMapTrueType(face, *baseEncoding, differences);
            }

            return nullptr;
        }
        case PdfFontFileType::Type3:
        {
            // CHECK-ME: ISO 32000-2:2020 "9.6.5.3 Encodings for Type 3 fonts"
            return PdfCIDToGIDMapConstPtr();
        }
        default:
        {
            // Nothing to do
            return nullptr;
        }
    }
}

void PdfEncodingMapSimple::GetBaseEncoding(const PdfEncodingMap*& baseEncoding, const PdfDifferenceMap*& differences) const
{
    baseEncoding = this;
    differences = nullptr;
}

// ISO 32000-2:2020 "9.6.5.2 Encodings for Type 1 fonts"
PdfCIDToGIDMapConstPtr getIntrinsicCIDToGIDMapType1(FT_Face face, const PdfEncodingMap& baseEncodings,
    const PdfDifferenceMap* differences)
{
    // Iterate all the codes of the encoding
    CIDToGIDMap map;
    const PdfName* name;
    CodePointSpan codePoints;
    FT_UInt index;
    // NOTE:  It's safe to assume the base encoding is a one byte encoding.
    // Iterate all the range, as the base econding may be narrow
    for (unsigned code = 0; code <= 0xFFU; code++)
    {
        // If there's a difference, use that instead
        if (differences == nullptr || !differences->TryGetMappedName((unsigned char)code, name, codePoints))
        {
            // NOTE: 9.6.5.2 does not mention about querying the AGL, but
            // all predefined encodings characater names are also present in the AGL
            if (!baseEncodings.TryGetCodePoints(PdfCharCode(code, 1), codePoints)
                || codePoints.GetSize() != 1
                || !PdfPredefinedEncoding::TryGetCharNameFromCodePoint(*codePoints, name))
            {
                // It may happen the code is not found even in the base encoding,
                // just add an identity mapping
            Identity:
                map[code] = code;
                continue;
            }
        }

        // "A Type 1 font program’s glyph descriptions are keyed by glyph names, not by character codes"
        index = FT_Get_Name_Index(face, name->GetString().data());
        if (index == 0)
            goto Identity;

        map[code] = index;
    }

    if (map.size() == 0)
        return nullptr;

    return PdfCIDToGIDMapConstPtr(new PdfCIDToGIDMap(std::move(map)));
}

// ISO 32000-2:2020 "9.6.5.4 Encodings for TrueType fonts"
PdfCIDToGIDMapConstPtr getIntrinsicCIDToGIDMapTrueType(FT_Face face, const PdfEncodingMap & baseEncodings,
    const PdfDifferenceMap* differences)
{
    // "If a (3, 1) 'cmap' subtable (Microsoft Unicode) is present:
    // A character code shall be first mapped to a glyph name using
    // the table described above"
    const PdfEncodingMap* inverseUnicodeMap = nullptr;
    if (FT_Select_Charmap(face, FT_ENCODING_UNICODE) != 0)
    {
        if (FT_Select_Charmap(face, FT_ENCODING_APPLE_ROMAN) == 0)
        {
            // If no (3, 1) subtable is present but a (1, 0) subtable
            // (Macintosh Roman) is present: A character code shall be
            // first mapped to a glyph name using the table described above.
            // The glyph name shall then be mapped back to a character code
            // according to the standard encoding used on Mac OS.
            // NOTE: the so called "standard Roman encoding" differs from
            // /MacRomanEncoding defining some more entries as specified in
            // "Table 113 — Additional entries in Mac OS Roman encoding not
            // in MacRomanEncoding". Our PdfMacRomanEncoding defines those
            // as well
            inverseUnicodeMap = &PdfEncodingMapFactory::GetMacRomanEncodingInstance();
        }
        else
        {
            return nullptr;
        }
    }

    // Iterate all the codes of the encoding
    CIDToGIDMap map;
    const PdfName* name = nullptr;
    CodePointSpan codePoints;
    unique_ptr<unordered_map<string_view, unsigned>> fontPostMap;
    unordered_map<string_view, unsigned>::const_iterator found;
    PdfCharCode charCode;
    FT_UInt index;
    auto& standardEncoding = PdfEncodingMapFactory::GetStandardEncodingInstance();
    // NOTE:  It's safe to assume the base encoding is a one byte encoding.
    // Iterate all the range, as the base econding may be narrow
    for (unsigned code = 0; code <= 0xFFU; code++)
    {
        // If there's a difference, use that instead
        if (differences == nullptr || !differences->TryGetMappedName((unsigned char)code, name, codePoints))
        {
            // "...the table shall be initialised with the entries from the
            // dictionary’s BaseEncoding entry. (...) Finally, any undefined
            // entries in the table shall be filled using StandardEncoding"
            charCode = PdfCharCode(code, 1);
            if (!(baseEncodings.TryGetCodePoints(charCode, codePoints)
                || standardEncoding.TryGetCodePoints(charCode, codePoints)))
            {
                // It may happen the code is not found even in the base encoding,
                // just add an identity mapping
            Identity:
                map[code] = code;
                continue;
            }
        }

        if (codePoints.GetSize() != 1)
            goto TryPost;

        // "Finally, the Unicode value shall be mapped to a glyph description according to the (x, y) subtable"
        if (inverseUnicodeMap == nullptr || !inverseUnicodeMap->TryGetCharCode(codePoints, charCode))
            index = FT_Get_Char_Index(face, *codePoints);
        else
            index = FT_Get_Char_Index(face, charCode.Code);

        if (index != 0)
        {
            map[code] = index;
            continue;
        }

    TryPost:
        if (name == nullptr)
        {
            if (codePoints.GetSize() != 1 || !PdfPredefinedEncoding::TryGetCharNameFromCodePoint(*codePoints, name))
                goto Identity;
        }

        // "In any of these cases, if the glyph name cannot be mapped as specified,
        // the glyph name shall be looked up in the font program’s "post" table
        // (if one is present) and the associated glyph description shall be used
        if (fontPostMap == nullptr)
            fontPostMap.reset(new unordered_map<string_view, unsigned>(FT::GetPostMap(face)));

        if ((found = fontPostMap->find(*name)) == fontPostMap->end())
            goto Identity;

        map[code] = found->second;
    }

    if (map.size() == 0)
        return nullptr;

    return PdfCIDToGIDMapConstPtr(new PdfCIDToGIDMap(std::move(map)));
}

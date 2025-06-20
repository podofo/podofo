/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFont.h"

#include <utf8cpp/utf8.h>

#include <podofo/private/PdfEncodingPrivate.h>
#include <podofo/private/PdfStandard14FontData.h>
#include <podofo/private/outstringstream.h>

#include "PdfArray.h"
#include "PdfEncoding.h"
#include "PdfEncodingFactory.h"
#include <podofo/auxiliary/InputStream.h>
#include "PdfObjectStream.h"
#include "PdfCharCodeMap.h"
#include "PdfFontMetrics.h"
#include "PdfPage.h"
#include "PdfFontMetricsStandard14.h"
#include "PdfFontManager.h"
#include "PdfFontMetricsFreetype.h"
#include "PdfDocument.h"
#include "PdfStringStream.h"

using namespace std;
using namespace cmn;
using namespace PoDoFo;

static double getGlyphLength(double glyphLength, const PdfTextState& state, bool ignoreCharSpacing);
static string_view toString(PdfFontStretch stretch);

PdfFont::PdfFont(PdfDocument& doc, PdfFontType type, PdfFontMetricsConstPtr&& metrics,
        const PdfEncoding& encoding) :
    PdfDictionaryElement(doc, "Font"_n),
    m_Type(type),
    m_WordSpacingLengthRaw(-1),
    m_SpaceCharLengthRaw(-1),
    m_Metrics(std::move(metrics))
{
    if (m_Metrics == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Metrics must me not null");

    this->initBase(encoding);
}

PdfFont::PdfFont(PdfObject& obj, PdfFontType type, PdfFontMetricsConstPtr&& metrics,
        const PdfEncoding& encoding) :
    PdfDictionaryElement(obj),
    m_Type(type),
    m_WordSpacingLengthRaw(-1),
    m_SpaceCharLengthRaw(-1),
    m_Metrics(std::move(metrics))
{
    if (m_Metrics == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Metrics must me not null");

    this->initBase(encoding);
}

PdfFont::~PdfFont() { }

bool PdfFont::TryCreateProxyFont(PdfFont*& proxyFont) const
{
    return TryCreateProxyFont(PdfFontCreateFlags::None, proxyFont);
}

bool PdfFont::TryCreateProxyFont(PdfFontCreateFlags initFlags, PdfFont*& proxyFont) const
{
    if (m_IsProxy)
    {
        // Don't substitute already proxied fonts
        proxyFont = nullptr;
        return false;
    }

    auto& metrics = GetMetrics();
    // No need to normalize the font if embedding is not enabled
    bool skipNormalization = (initFlags & PdfFontCreateFlags::DontEmbed) != PdfFontCreateFlags::None;
    PdfFontMetricsConstPtr proxyMetrics;
    PdfStandard14FontType std14Font = PdfStandard14FontType::Unknown;
    if (metrics.HasFontFileData() && !m_Metrics->IsStandard14FontMetrics(std14Font))
    {
        proxyMetrics = metrics.CreateMergedMetrics(skipNormalization);
    }
    else
    {
        // Early intercept Standard14 fonts
        if (std14Font != PdfStandard14FontType::Unknown ||
            PdfFont::IsStandard14Font(metrics.GetFontName(), false, std14Font))
        {
            auto parsedMetrics = metrics.GetParsedWidths();
            if (parsedMetrics == nullptr)
                proxyMetrics = PdfFontMetricsStandard14::GetInstance(std14Font);
            else
                proxyMetrics = PdfFontMetricsStandard14::Create(std14Font, std::move(parsedMetrics));
        }
        else
        {
            if (m_Metrics->GetFontFileType() == PdfFontFileType::Type3)
            {
                // We just re-use the same metrics
                proxyMetrics = m_Metrics;
            }
            else
            {
                PdfFontSearchParams params;
                params.Style = metrics.GetStyle();
                params.FontFamilyPattern = metrics.GeFontFamilyNameSafe();
                proxyMetrics = PdfFontManager::SearchFontMetrics(metrics.GetPostScriptNameRough(), params, metrics, skipNormalization);
                if (proxyMetrics == nullptr)
                {
                    proxyFont = nullptr;
                    return false;
                }
            }
        }
    }

    PdfFontCreateParams params;
    if (m_Encoding->HasValidToUnicodeMap())
    {
        params.Encoding = *m_Encoding;
    }
    else
    {
        shared_ptr<PdfCMapEncoding> toUnicode = proxyMetrics->CreateToUnicodeMap(m_Encoding->GetLimits());
        params.Encoding = PdfEncoding::Create(*m_Encoding, std::move(toUnicode));
    }

    params.Flags = initFlags;
    auto newFont = PdfFont::Create(GetDocument(), std::move(proxyMetrics), params, true);
    if (newFont == nullptr)
    {
        proxyFont = nullptr;
        return false;
    }

    proxyFont = GetDocument().GetFonts().AddImported(std::move(newFont));
    return true;
}

void PdfFont::initBase(const PdfEncoding& encoding)
{
    m_IsEmbedded = false;
    m_EmbeddingEnabled = false;
    m_SubsettingEnabled = false;
    m_IsProxy = false;

    if (encoding.IsNull())
    {
        m_DynamicCIDMap = std::make_shared<PdfCharCodeMap>();
        m_DynamicToUnicodeMap = std::make_shared<PdfCharCodeMap>();
        m_Encoding = PdfEncoding::CreateDynamicEncoding(m_DynamicCIDMap, m_DynamicToUnicodeMap, *this);
    }
    else
    {
        m_Encoding = PdfEncoding::CreateSchim(encoding, *this);
    }

    m_fontProgCIDToGIDMap = m_Encoding->GetCIDToGIDMap();

    // By default ensure the font has the /BaseFont name or /FontName
    // or, the name inferred from a font file
    m_Name = m_Metrics->GetFontName();
}

void PdfFont::WriteStringToStream(OutputStream& stream, const string_view& str) const
{
    // Optimize serialization for simple encodings
    auto encoded = m_Encoding->ConvertToEncoded(str);
    if (m_Encoding->IsSimpleEncoding())
        utls::SerializeEncodedString(stream, encoded, false);
    else
        utls::SerializeEncodedString(stream, encoded, true);
}

void PdfFont::InitImported(bool wantEmbed, bool wantSubset, bool isProxy)
{
    PODOFO_ASSERT(!IsObjectLoaded());

    // Init the subset maps
    m_subsetCIDMap.reset(new CIDSubsetMap());
    m_subsetGIDToCIDMap.reset(new unordered_map<unsigned, unsigned>());

    // No embedding implies no subsetting
    m_EmbeddingEnabled = wantEmbed;
    m_SubsettingEnabled = wantEmbed && wantSubset && SupportsSubsetting();
    m_IsProxy = isProxy;
    if (m_SubsettingEnabled && !isProxy)
    {
        // If it exist a glyph for the space character,
        // add it for subsetting. NOTE: Search the GID
        // in the font program
        unsigned gid;
        char32_t spaceCp = U' ';
        if (TryGetGID(spaceCp, PdfGlyphAccess::FontProgram, gid))
        {
            unicodeview codepoints(&spaceCp, 1);
            PdfCID cid;
            (void)tryAddSubsetGID(gid, codepoints, cid);
        }
    }

    unsigned char subsetPrefixLength = m_Metrics->GetSubsetPrefixLength();
    if (subsetPrefixLength == 0)
    {
        if (m_SubsettingEnabled)
        {
            m_SubsetPrefix = GetDocument().GetFonts().GenerateSubsetPrefix();
            m_Name = m_SubsetPrefix.append(m_Metrics->GetPostScriptNameRough());
        }
        else
        {
            m_Name = (string)m_Metrics->GetPostScriptNameRough();
        }
    }
    else
    {
        m_Name = m_Metrics->GetFontName();
        m_SubsetPrefix = m_Name.substr(0, subsetPrefixLength);
    }

    initImported();
}

void PdfFont::EmbedFont()
{
    if (m_IsEmbedded || !m_EmbeddingEnabled)
        return;

    if (m_SubsettingEnabled)
        embedFontSubset();
    else
        embedFont();

    m_IsEmbedded = true;
}

void PdfFont::embedFont()
{
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Embedding not implemented for this font type");
}

void PdfFont::embedFontSubset()
{
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Subsetting not implemented for this font type");
}

unsigned PdfFont::GetGID(char32_t codePoint, PdfGlyphAccess access) const
{
    unsigned gid;
    if (!TryGetGID(codePoint, access, gid))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Can't find a gid");

    return gid;
}

bool PdfFont::TryGetGID(char32_t codePoint, PdfGlyphAccess access, unsigned& gid) const
{
    if (m_Encoding->IsObjectLoaded() || !m_Metrics->HasUnicodeMapping())
    {
        PdfCharCode codeUnit;
        unsigned cid;
        if (!m_Encoding->GetToUnicodeMapSafe().TryGetCharCode(codePoint, codeUnit)
            || !m_Encoding->TryGetCIDId(codeUnit, cid))
        {
            gid = 0;
            return false;
        }

        return TryMapCIDToGID(cid, access, gid);
    }
    else
    {
        return m_Metrics->TryGetGID(codePoint, gid);
    }
}

double PdfFont::GetStringLength(const string_view& str, const PdfTextState& state) const
{
    // Ignore failures
    double length;
    (void)TryGetStringLength(str, state, length);
    return length;
}

bool PdfFont::TryGetStringLength(const string_view& str, const PdfTextState& state, double& length) const
{
    vector<unsigned> gids;
    bool success = tryConvertToGIDs(str, PdfGlyphAccess::ReadMetrics, gids);
    length = 0;
    for (unsigned i = 0; i < gids.size(); i++)
        length += getGlyphLength(m_Metrics->GetGlyphWidth(gids[i]), state, false);

    return success;
}

double PdfFont::GetEncodedStringLength(const PdfString& encodedStr, const PdfTextState& state) const
{
    // Ignore failures
    double length;
    (void)TryGetEncodedStringLength(encodedStr, state, length);
    return length;
}

bool PdfFont::TryGetEncodedStringLength(const PdfString& encodedStr, const PdfTextState& state, double& length) const
{
    vector<PdfCID> cids;
    bool success = true;
    if (!m_Encoding->TryConvertToCIDs(encodedStr, cids))
        success = false;

    length = getStringLength(cids, state);
    return success;
}

bool PdfFont::TryScanEncodedString(const PdfString& encodedStr, const PdfTextState& state, string& utf8str, vector<double>& lengths, vector<unsigned>& positions) const
{
    utf8str.clear();
    lengths.clear();
    positions.clear();

    if (encodedStr.IsEmpty())
        return true;

    auto context = m_Encoding->StartStringScan(encodedStr);
    CodePointSpan codepoints;
    PdfCID cid;
    bool success = true;
    double length;
    while (!context.IsEndOfString())
    {
        if (!context.TryScan(cid, utf8str, positions, codepoints))
            success = false;

        length = getGlyphLength(GetCIDWidth(cid.Id), state, false);
        for (unsigned i = 1; i < codepoints.GetSize(); i++)
        {
            // Arbitrarily prefix 0 length positions for ligatures,
            // for the code point span size minus one
            lengths.push_back(0);
        }

        lengths.push_back(length);
    }

    return success;
}

double PdfFont::GetWordSpacingLength(const PdfTextState& state) const
{
    const_cast<PdfFont&>(*this).initSpaceDescriptors();
    return getGlyphLength(m_WordSpacingLengthRaw, state, false);
}

double PdfFont::GetSpaceCharLength(const PdfTextState& state) const
{
    const_cast<PdfFont&>(*this).initSpaceDescriptors();
    return getGlyphLength(m_SpaceCharLengthRaw, state, false);
}

double PdfFont::GetCharLength(char32_t codePoint, const PdfTextState& state, bool ignoreCharSpacing) const
{
    // Ignore failures
    double length;
    if (!TryGetCharLength(codePoint, state, ignoreCharSpacing, length))
        return GetDefaultCharLength(state, ignoreCharSpacing);

    return length;
}

bool PdfFont::TryGetCharLength(char32_t codePoint, const PdfTextState& state, double& length) const
{
    return TryGetCharLength(codePoint, state, false, length);
}

bool PdfFont::TryGetCharLength(char32_t codePoint, const PdfTextState& state,
    bool ignoreCharSpacing, double& length) const
{
    unsigned gid;
    if (TryGetGID(codePoint, PdfGlyphAccess::ReadMetrics, gid))
    {
        length = getGlyphLength(m_Metrics->GetGlyphWidth(gid), state, ignoreCharSpacing);
        return true;
    }
    else
    {
        length = getGlyphLength(m_Metrics->GetDefaultWidth(), state, ignoreCharSpacing);
        return false;
    }
}

double PdfFont::GetDefaultCharLength(const PdfTextState& state, bool ignoreCharSpacing) const
{
    if (ignoreCharSpacing)
    {
        return m_Metrics->GetDefaultWidth() * state.FontSize
            * state.FontScale;
    }
    else
    {
        return (m_Metrics->GetDefaultWidth() * state.FontSize
            + state.CharSpacing) * state.FontScale;
    }
}

/*
vector<PdfSplittedString> PdfFont::SplitEncodedString(const PdfString& str) const
{
    (void)str;
    // TODO: retrieve space character codes with m_Encoding->GetToUnicodeMapSafe().TryGetCharCode(codePoint, codeUnit),
    // then iterate char codes and return splitted strings
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}
*/

double PdfFont::GetCIDWidth(unsigned cid) const
{
    unsigned gid;
    if (!TryMapCIDToGID(cid, PdfGlyphAccess::ReadMetrics, gid))
        return m_Metrics->GetDefaultWidth();

    return m_Metrics->GetGlyphWidth(gid);
}

void PdfFont::GetBoundingBox(PdfArray& arr) const
{
    auto& matrix = m_Metrics->GetMatrix();
    arr.Clear();
    arr.Reserve(4);
    auto bbox = m_Metrics->GetBoundingBox();
    arr.Add(PdfObject(bbox.X1 / matrix[0]));
    arr.Add(PdfObject(bbox.Y1 / matrix[3]));
    arr.Add(PdfObject(bbox.X2 / matrix[0]));
    arr.Add(PdfObject(bbox.Y2 / matrix[3]));
}

void PdfFont::WriteDescriptors(PdfDictionary& fontDict, PdfDictionary& descriptorDict) const
{
    // Optional values
    int weight;
    double xHeight;
    double stemH;
    string familyName;
    double leading;
    double avgWidth;
    double maxWidth;
    double defaultWidth;
    PdfFontStretch stretch;

    descriptorDict.AddKey("FontName"_n, PdfName(this->GetName()));
    if ((familyName = m_Metrics->GetFontFamilyName()).length() != 0)
        descriptorDict.AddKey("FontFamily"_n, PdfString(familyName));
    if ((stretch = m_Metrics->GetFontStretch()) != PdfFontStretch::Unknown)
        descriptorDict.AddKey("FontStretch"_n, PdfName(toString(stretch)));
    descriptorDict.AddKey("Flags"_n, static_cast<int64_t>(m_Metrics->GetFlags()));
    descriptorDict.AddKey("ItalicAngle"_n, static_cast<int64_t>(std::round(m_Metrics->GetItalicAngle())));

    auto& matrix = m_Metrics->GetMatrix();
    if (GetType() == PdfFontType::Type3)
    {
        // ISO 32000-1:2008 "should be used for Type 3 fonts in Tagged PDF documents"
        descriptorDict.AddKey("FontWeight"_n, static_cast<int64_t>(m_Metrics->GetWeight()));

        PdfArray arr;
        arr.Reserve(6);

        for (unsigned i = 0; i < 6; i++)
            arr.Add(PdfObject(matrix[i]));

        fontDict.AddKey("FontMatrix"_n, std::move(arr));

        GetBoundingBox(arr);
        fontDict.AddKey("FontBBox"_n, std::move(arr));
    }
    else
    {
        if ((weight = m_Metrics->GetWeightRaw()) > 0)
            descriptorDict.AddKey("FontWeight"_n, static_cast<int64_t>(weight));

        PdfArray bbox;
        GetBoundingBox(bbox);

        // The following entries are all optional in /Type3 fonts
        descriptorDict.AddKey("FontBBox"_n, std::move(bbox));
        descriptorDict.AddKey("Ascent"_n, static_cast<int64_t>(std::round(m_Metrics->GetAscent() / matrix[3])));
        descriptorDict.AddKey("Descent"_n, static_cast<int64_t>(std::round(m_Metrics->GetDescent() / matrix[3])));
        descriptorDict.AddKey("CapHeight"_n, static_cast<int64_t>(std::round(m_Metrics->GetCapHeight() / matrix[3])));
        // NOTE: StemV is measured horizontally
        descriptorDict.AddKey("StemV"_n, static_cast<int64_t>(std::round(m_Metrics->GetStemV() / matrix[0])));

        if ((xHeight = m_Metrics->GetXHeightRaw()) > 0)
            descriptorDict.AddKey("XHeight"_n, static_cast<int64_t>(std::round(xHeight / matrix[3])));

        if ((stemH = m_Metrics->GetStemHRaw()) > 0)
        {
            // NOTE: StemH is measured vertically
            descriptorDict.AddKey("StemH"_n, static_cast<int64_t>(std::round(stemH / matrix[3])));
        }

        if (!IsCIDFont())
        {
            // Default for /MissingWidth is 0
            // NOTE: We assume CID keyed fonts to use the /DW entry
            // in the CIDFont dictionary instead. See 9.7.4.3 Glyph
            // Metrics in CIDFonts in ISO 32000-1:2008
            if ((defaultWidth = m_Metrics->GetDefaultWidthRaw()) > 0)
                descriptorDict.AddKey("MissingWidth"_n, static_cast<int64_t>(std::round(defaultWidth / matrix[0])));
        }
    }

    if ((leading = m_Metrics->GetLeadingRaw()) > 0)
        descriptorDict.AddKey("Leading"_n, static_cast<int64_t>(std::round(leading / matrix[3])));
    if ((avgWidth = m_Metrics->GetAvgWidthRaw()) > 0)
        descriptorDict.AddKey("AvgWidth"_n, static_cast<int64_t>(std::round(avgWidth / matrix[0])));
    if ((maxWidth = m_Metrics->GetMaxWidthRaw()) > 0)
        descriptorDict.AddKey("MaxWidth"_n, static_cast<int64_t>(std::round(maxWidth / matrix[0])));
}

void PdfFont::EmbedFontFile(PdfDictionary& descriptor) const
{
    auto fontdata = m_Metrics->GetOrLoadFontFileData();
    if (fontdata.empty())
        PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);

    switch (m_Metrics->GetFontFileType())
    {
        case PdfFontFileType::Type1:
            EmbedFontFileType1(descriptor, fontdata, m_Metrics->GetFontFileLength1(), m_Metrics->GetFontFileLength2(), m_Metrics->GetFontFileLength3());
            break;
        case PdfFontFileType::Type1CFF:
            EmbedFontFileCFF(descriptor, fontdata, false);
            break;
        case PdfFontFileType::CIDKeyedCFF:
            EmbedFontFileCFF(descriptor, fontdata, true);
            break;
        case PdfFontFileType::TrueType:
            EmbedFontFileTrueType(descriptor, fontdata);
            break;
        case PdfFontFileType::OpenTypeCFF:
            EmbedFontFileOpenType(descriptor, fontdata);
            break;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Unsupported font type embedding");
    }
}

void PdfFont::EmbedFontFileType1(PdfDictionary& descriptorDict, const bufferview& data, unsigned length1, unsigned length2, unsigned length3) const
{
    embedFontFileData(descriptorDict, "FontFile"_n, [length1, length2, length3](PdfDictionary& dict)
    {
        dict.AddKey("Length1"_n, static_cast<int64_t>(length1));
        dict.AddKey("Length2"_n, static_cast<int64_t>(length2));
        dict.AddKey("Length3"_n, static_cast<int64_t>(length3));
    }, data);
}

void PdfFont::EmbedFontFileCFF(PdfDictionary& descriptorDict, const bufferview& data, bool cidKeyed) const
{
    embedFontFileData(descriptorDict, "FontFile3"_n, [&](PdfDictionary& dict)
    {
        PdfName subtype;
        if (cidKeyed)
            subtype = "CIDFontType0C"_n;
        else
            subtype = "Type1C"_n;

        dict.AddKey("Subtype"_n, subtype);
    }, data);
}

void PdfFont::EmbedFontFileTrueType(PdfDictionary& descriptor, const bufferview& data) const
{
    embedFontFileData(descriptor, "FontFile2"_n, [&data](PdfDictionary& dict)
    {
        dict.AddKey("Length1"_n, static_cast<int64_t>(data.size()));
    }, data);
}

void PdfFont::EmbedFontFileOpenType(PdfDictionary& descriptor, const bufferview& data) const
{
    embedFontFileData(descriptor, "FontFile3"_n, [](PdfDictionary& dict)
    {
        dict.AddKey("Subtype"_n, "OpenType"_n);
    }, data);
}

void PdfFont::embedFontFileData(PdfDictionary& descriptor, const PdfName& fontFileName,
    const function<void(PdfDictionary& dict)>& dictWriter, const bufferview& data) const
{
    auto& contents = GetDocument().GetObjects().CreateDictionaryObject();
    descriptor.AddKeyIndirect(fontFileName, contents);
    // NOTE: Access to directory is mediated by functor to not crash
    // operations when using PdfStreamedDocument. Do not remove it
    dictWriter(contents.GetDictionary());
    contents.GetOrCreateStream().SetData(data);
}

void PdfFont::initSpaceDescriptors()
{
    if (m_WordSpacingLengthRaw >= 0)
        return;

    // TODO: Maybe try looking up other characters if U' ' is missing?
    // https://docs.microsoft.com/it-it/dotnet/api/system.char.iswhitespace
    unsigned gid;
    if (!TryGetGID(U' ', PdfGlyphAccess::ReadMetrics, gid)
        || !m_Metrics->TryGetGlyphWidth(gid, m_SpaceCharLengthRaw)
        || m_SpaceCharLengthRaw <= 0)
    {
        double lengthsum = 0;
        unsigned nonZeroCount = 0;
        for (unsigned i = 0, count = m_Metrics->GetGlyphCount(PdfGlyphAccess::ReadMetrics); i < count; i++)
        {
            double length;
            m_Metrics->TryGetGlyphWidth(i, length);
            if (length > 0)
            {
                lengthsum += length;
                nonZeroCount++;
            }
        }

        m_SpaceCharLengthRaw = lengthsum / nonZeroCount;
    }

    // We arbitrarily take a fraction of the read or inferred
    // char space to determine the word spacing length. The
    // factor proved to work well with a consistent tests corpus
    // NOTE: This is very different from what Adobe Acrobat does,
    // but there's no reference heuristic to look at, every
    // implementation does something different
    // https://github.com/pdf-association/pdf-issues/issues/564
    constexpr double WORD_SPACING_FRACTIONAL_FACTOR = 5.3;
    m_WordSpacingLengthRaw = m_SpaceCharLengthRaw / WORD_SPACING_FRACTIONAL_FACTOR;
}

void PdfFont::pushSubsetInfo(unsigned cid, const PdfGID& gid, const PdfCharCode& code)
{
    auto& info = (*m_subsetCIDMap)[cid];
    info.Gid = gid;
    for (unsigned i = 0; i < info.Codes.size(); i++)
    {
        // Check if the code is already present and skip insertion in that case
        if (info.Codes[i] == code)
            goto Skip;
    }

    info.Codes.push_back(code);
Skip:
    (*m_subsetGIDToCIDMap)[gid.Id] = cid;
}

void PdfFont::initImported()
{
    // By default do nothing
}

double PdfFont::getStringLength(const vector<PdfCID>& cids, const PdfTextState& state) const
{
    double length = 0;
    for (auto& cid : cids)
        length += getGlyphLength(GetCIDWidth(cid.Id), state, false);

    return length;
}

double PdfFont::GetLineSpacing(const PdfTextState& state) const
{
    return m_Metrics->GetLineSpacing() * state.FontSize;
}

// CHECK-ME Should state.GetFontScale() be considered?
double PdfFont::GetUnderlineThickness(const PdfTextState& state) const
{
    return m_Metrics->GetUnderlineThickness() * state.FontSize;
}

// CHECK-ME Should state.GetFontScale() be considered?
double PdfFont::GetUnderlinePosition(const PdfTextState& state) const
{
    return m_Metrics->GetUnderlinePosition() * state.FontSize;
}

// CHECK-ME Should state.GetFontScale() be considered?
double PdfFont::GetStrikeThroughPosition(const PdfTextState& state) const
{
    return m_Metrics->GetStrikeThroughPosition() * state.FontSize;
}

// CHECK-ME Should state.GetFontScale() be considered?
double PdfFont::GetStrikeThroughThickness(const PdfTextState& state) const
{
    return m_Metrics->GetStrikeThroughThickness() * state.FontSize;
}

double PdfFont::GetAscent(const PdfTextState& state) const
{
    return m_Metrics->GetAscent() * state.FontSize;
}

double PdfFont::GetDescent(const PdfTextState& state) const
{
    return m_Metrics->GetDescent() * state.FontSize;
}

bool PdfFont::TryAddSubsetGID(unsigned gid, const unicodeview& codePoints, PdfCID& cid)
{
    PODOFO_ASSERT(m_SubsettingEnabled && !m_IsEmbedded && !m_IsProxy);
    auto found = m_subsetGIDToCIDMap->find(gid);
    if (found != m_subsetGIDToCIDMap->end())
    {
        // NOTE: Assume the subset CID map contains a single code
        cid = PdfCID(found->first, (*m_subsetCIDMap)[found->second].Codes[0]);
        return true;
    }

    return tryAddSubsetGID(gid, codePoints, cid);
}

PdfCharCode PdfFont::AddCharCodeSafe(unsigned gid, const unicodeview& codePoints)
{
    // NOTE: This method is supported only when doing fully embedding
    // of an imported font with valid unicode mapping
    PODOFO_ASSERT(!m_SubsettingEnabled
        && m_Encoding->IsDynamicEncoding()
        && !m_Encoding->IsObjectLoaded()
        && m_Metrics->HasUnicodeMapping());

    PdfCharCode code;
    if (m_DynamicToUnicodeMap->TryGetCharCode(codePoints, code))
        return code;

    // Encode the code point with FSS-UTF encoding so
    // it will be variable code size safe
    code = PdfCharCode(utls::FSSUTFEncode((unsigned)m_DynamicToUnicodeMap->GetMappings().size()));
    // NOTE: We assume in this context cid == gid identity
    m_DynamicCIDMap->PushMapping(code, gid);
    m_DynamicToUnicodeMap->PushMapping(code, codePoints);
    return code;
}

bool PdfFont::tryConvertToGIDs(const std::string_view& utf8Str, PdfGlyphAccess access, std::vector<unsigned>& gids) const
{
    bool success = true;
    if (m_Encoding->IsObjectLoaded() || !m_Metrics->HasUnicodeMapping())
    {
        // NOTE: This is a best effort strategy. It's not intended to
        // be accurate in loaded fonts
        auto it = utf8Str.begin();
        auto end = utf8Str.end();

        auto& toUnicode = m_Encoding->GetToUnicodeMapSafe();
        while (it != end)
        {
            char32_t cp = utf8::next(it, end);
            PdfCharCode codeUnit;
            unsigned cid;
            unsigned gid;
            if (toUnicode.TryGetCharCode(cp, codeUnit))
            {
                if (m_Encoding->TryGetCIDId(codeUnit, cid))
                {
                    if (!TryMapCIDToGID(cid, access, gid))
                    {
                        // Fallback
                        gid = cid;
                        success = false;
                    }
                }
                else
                {
                    // Fallback
                    gid = codeUnit.Code;
                    success = false;
                }
            }
            else
            {
                // Fallback
                gid = cp;
                success = false;
            }

            gids.push_back(gid);
        }
    }
    else
    {
        auto it = utf8Str.begin();
        auto end = utf8Str.end();
        while (it != end)
        {
            char32_t cp = utf8::next(it, end);
            unsigned gid;
            if (!m_Metrics->TryGetGID(cp, gid))
            {
                // Fallback
                gid = cp;
                success = false;
            }

            gids.push_back(gid);
        }

        // Try to subsistute GIDs for fonts that support
        // a glyph substitution mechanism
        vector<unsigned char> backwardMap;
        m_Metrics->SubstituteGIDs(gids, backwardMap);
    }

    return success;
}

bool PdfFont::tryAddSubsetGID(unsigned gid, const unicodeview& codePoints, PdfCID& cid)
{
    (void)codePoints;
    PODOFO_ASSERT(m_SubsettingEnabled && !m_IsProxy);
    if (m_Encoding->IsDynamicEncoding())
    {
        // We start numberings CIDs from 1 since CID 0
        // is reserved for fallbacks. Encode it with FSS-UTF
        // encoding so it will be variable code size safe

        cid = PdfCID((unsigned)m_subsetCIDMap->size() + 1, PdfCharCode(utls::FSSUTFEncode((unsigned)m_subsetCIDMap->size() + 1)));
        m_DynamicCIDMap->PushMapping(cid.Unit, cid.Id);
        m_DynamicToUnicodeMap->PushMapping(cid.Unit, codePoints);
    }
    else
    {
        PdfCharCode codeUnit;
        if (!m_Encoding->GetToUnicodeMapSafe().TryGetCharCode(codePoints, codeUnit))
        {
            cid = { };
            return false;
        }

        // We start numberings CIDs from 1 since CID 0
        // is reserved for fallbacks
        cid = PdfCID((unsigned)m_subsetCIDMap->size() + 1, codeUnit);
    }

    pushSubsetInfo(cid.Id, PdfGID(gid), cid.Unit);
    return true;
}

void PdfFont::AddSubsetCIDs(const PdfString& encodedStr)
{
    if (!m_IsProxy)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Can't add used CIDs to a non substitute font");

    if (m_IsEmbedded)
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic,
            "Can't add more subsetting glyphs on an already embedded font");
    }

    PODOFO_ASSERT(!m_Encoding->IsDynamicEncoding());

    vector<PdfCID> cids;
    PdfGID gid;
    (void)m_Encoding->TryConvertToCIDs(encodedStr, cids);
    unsigned glyphCount = m_Metrics->GetGlyphCount();
    for (unsigned i = 0; i < cids.size(); i++)
    {
        auto& cid = cids[i];
        if (!TryMapCIDToGID(cid.Id, gid) || gid.Id >= glyphCount)
        {
            // Assume the font will always contain at least one glyph
            // and add a mapping to CID 0 for the char code
            pushSubsetInfo(cid.Id, PdfGID(0), cid.Unit);
            continue;
        }

        // Ignore trying to replace existing mapping
        pushSubsetInfo(cid.Id, gid, cid.Unit);
    }
}

bool PdfFont::HasCIDSubset() const
{
    return m_subsetCIDMap != nullptr && m_subsetCIDMap->size() != 0;
}

bool PdfFont::SupportsSubsetting() const
{
    return false;
}

bool PdfFont::IsStandard14Font() const
{
    return m_Metrics->IsStandard14FontMetrics();
}

bool PdfFont::IsStandard14Font(PdfStandard14FontType& std14Font) const
{
    return m_Metrics->IsStandard14FontMetrics(std14Font);
}

PdfObject& PdfFont::GetDescendantFontObject()
{
    auto obj = getDescendantFontObject();
    if (obj == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Descendant font object must not be null");

    return *obj;
}

bool PdfFont::TryMapCIDToGID(unsigned cid, PdfGID& gid) const
{
    // Retrieve first the font program GID first
    bool normalLookup = false;
    if (m_fontProgCIDToGIDMap == nullptr)
    {
        if (!tryMapCIDToGIDNormal(cid, gid.Id))
            goto Fail;

        normalLookup = true;
    }
    else
    {
        if (!m_fontProgCIDToGIDMap->TryMapCIDToGID(cid, gid.Id))
            goto Fail;
    }

    // Secondly, retrieve PDF metrics Id
    if (!tryMapCIDToGIDLoadedMetrics(cid, gid.MetricsId))
    {
        if (normalLookup) // The normal lookup just happened, no need to repeat it
            gid.MetricsId = gid.Id;
        else if (!tryMapCIDToGIDNormal(cid, gid.MetricsId))
            goto Fail;
    }

    return true;

Fail:
    gid = { };
    return false;
}

bool PdfFont::TryMapCIDToGID(unsigned cid, PdfGlyphAccess access, unsigned& gid) const
{
    switch (access)
    {
        case PdfGlyphAccess::ReadMetrics:
        {
            if (tryMapCIDToGIDLoadedMetrics(cid, gid))
                return true;
            else
                return tryMapCIDToGIDNormal(cid, gid);
        }
        case PdfGlyphAccess::FontProgram:
        {
            if (m_fontProgCIDToGIDMap != nullptr)
                return m_fontProgCIDToGIDMap->TryMapCIDToGID(cid, gid);

            return tryMapCIDToGIDNormal(cid, gid);
        }
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

bool PdfFont::tryMapCIDToGIDLoadedMetrics(unsigned cid, unsigned& gid) const
{
    if (!m_Encoding->IsObjectLoaded() || !m_Metrics->HasParsedWidths())
        return false;

    if (m_Encoding->IsSimpleEncoding())
    {
        // We just convert to a GID using /FirstChar
        gid = cid - m_Encoding->GetFirstChar().Code;
    }
    else
    {
        // Else we assume identity
        gid = cid;
    }

    return true;
}

bool PdfFont::tryMapCIDToGIDNormal(unsigned cid, unsigned& gid) const
{
    if (m_Encoding->IsSimpleEncoding() && m_Metrics->HasUnicodeMapping())
    {
        // For simple fonts, try map CID to GID using the unicode
        // map from metrics, if available
        char32_t mappedCodePoint = m_Encoding->GetCodePoint(cid);
        if (mappedCodePoint == U'\0'
            || !m_Metrics->TryGetGID(mappedCodePoint, gid))
        {
            gid = 0;
            return false;
        }

        return true;
    }
    else
    {
        // We assume cid == gid identity.
        gid = cid;
        return true;
    }
}

vector<PdfCharGIDInfo> PdfFont::GetCharGIDInfos() const
{
    vector<PdfCharGIDInfo> ret;
    if (m_subsetCIDMap == nullptr)
    {
        PODOFO_ASSERT(!IsSubsettingEnabled());
        // Create an identity cid/gid map
        unsigned gidCount = GetMetrics().GetGlyphCount();
        ret.resize(gidCount);
        for (unsigned i = 0; i < gidCount; i++)
            ret[i] = { i, i, PdfGID(i) };
    }
    else
    {
        if (m_subsetCIDMap->size() == 0)
        {
            ret.push_back({ 0, 0, PdfGID(0)});
            return ret;
        }

        ret.resize(m_subsetCIDMap->size());
        unsigned i = 0;
        if (m_SubsettingEnabled)
        {
            for (auto& pair : *m_subsetCIDMap)
            {
                // Reserve CID 0 and start numbering CIDS from 1
                ret[i] = { i + 1, pair.first, pair.second.Gid };
                i++;
            }
        }
        else
        {
            for (auto& pair : *m_subsetCIDMap)
            {
                ret[i] = { pair.first, pair.first, pair.second.Gid };
                i++;
            }
        }
    }

    return ret;
}

bool PdfFont::TryGetSubstituteCIDEncoding(unique_ptr<PdfEncodingMap>& cidEncodingMap) const
{
    if (m_subsetCIDMap == nullptr || m_subsetCIDMap->size() == 0 || m_DynamicCIDMap != nullptr)
    {
        // Return if the subset map is non existing or invalid, or this font
        // is already defining a dynamic CID mapping
        cidEncodingMap.reset();
        return false;
    }

    PdfCharCodeMap map;
    if (m_SubsettingEnabled)
    {
        unsigned i = 0;
        for (auto& pair : *m_subsetCIDMap)
        {
            for (auto& code : pair.second.Codes)
                map.PushMapping(code, i + 1);

            i++;
        }
    }
    else
    {
        // The identifier for the new CID encoding
        // unconditionally becomes the found GID
        for (auto& pair : *m_subsetCIDMap)
        {
            for (auto& code : pair.second.Codes)
                map.PushMapping(code, pair.second.Gid.Id);
        }
    }

    cidEncodingMap.reset(new PdfCMapEncoding(std::move(map)));
    return true;
}

PdfCIDSystemInfo PdfFont::GetCIDSystemInfo() const
{
    PdfCIDSystemInfo ret;
    auto fontName = m_Name;
    if (IsSubsettingEnabled())
        fontName.append("-subset");

    ret.Registry = PdfString(CMAP_REGISTRY_NAME);
    ret.Ordering = PdfString(fontName);
    ret.Supplement = 0;
    return ret;
}

PdfObject* PdfFont::getDescendantFontObject()
{
    // By default return null
    return nullptr;
}

string_view PdfFont::GetStandard14FontName(PdfStandard14FontType stdFont)
{
    return ::GetStandard14FontName(stdFont);
}

bool PdfFont::IsStandard14Font(const string_view& fontName, PdfStandard14FontType& stdFont)
{
    return ::IsStandard14Font(fontName, true, stdFont);
}

bool PdfFont::IsStandard14Font(const string_view& fontName, bool useAltNames, PdfStandard14FontType& stdFont)
{
    return ::IsStandard14Font(fontName, useAltNames, stdFont);
}

bool PdfFont::IsCIDFont() const
{
    switch (GetType())
    {
        case PdfFontType::CIDTrueType:
        case PdfFontType::CIDCFF:
            return true;
        default:
            return false;
    }
}

bool PdfFont::IsObjectLoaded() const
{
    return false;
}

inline string_view PdfFont::GetSubsetPrefix() const
{
    return m_SubsetPrefix;
}

// TODO:
// Handle word spacing Tw
// 5.2.2 Word Spacing
// Note: Word spacing is applied to every occurrence of the single-byte character code
// 32 in a string when using a simple font or a composite font that defines code 32 as a
// single - byte code.It does not apply to occurrences of the byte value 32 in multiplebyte
// codes.
double getGlyphLength(double glyphLength, const PdfTextState& state, bool ignoreCharSpacing)
{
    if (ignoreCharSpacing)
        return glyphLength * state.FontSize * state.FontScale;
    else
        return (glyphLength * state.FontSize + state.CharSpacing) * state.FontScale;
}

string_view toString(PdfFontStretch stretch)
{
    switch (stretch)
    {
        case PdfFontStretch::UltraCondensed:
            return "UltraCondensed";
        case PdfFontStretch::ExtraCondensed:
            return "ExtraCondensed";
        case PdfFontStretch::Condensed:
            return "Condensed";
        case PdfFontStretch::SemiCondensed:
            return "SemiCondensed";
        case PdfFontStretch::Normal:
            return "Normal";
        case PdfFontStretch::SemiExpanded:
            return "SemiExpanded";
        case PdfFontStretch::Expanded:
            return "Expanded";
        case PdfFontStretch::ExtraExpanded:
            return "ExtraExpanded";
        case PdfFontStretch::UltraExpanded:
            return "UltraExpanded";
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

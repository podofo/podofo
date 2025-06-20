/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfEncoding.h"

#include <atomic>
#include <utf8cpp/utf8.h>

#include <podofo/private/PdfEncodingPrivate.h>

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfEncodingMapFactory.h"

using namespace std;
using namespace PoDoFo;

namespace PoDoFo
{
    class PdfDynamicEncodingMap : public PdfEncodingMapBase
    {
    public:
        PdfDynamicEncodingMap(shared_ptr<PdfCharCodeMap> map);

        void AppendCodeSpaceRange(OutputStream& stream, charbuff& temp) const override;
    };
}

static PdfCharCode fetchFallbackCharCode(string_view::iterator& it, const string_view::iterator& end, const PdfEncodingLimits& limits);
static void pushCodeRangeSize(vector<unsigned char>& codeRangeSizes, unsigned char codeRangeSize);

PdfEncoding::PdfEncoding()
    : PdfEncoding(NullEncodingId, shared_ptr(PdfEncodingMapFactory::GetNullEncodingInstancePtr()), nullptr)
{
}

PdfEncoding::PdfEncoding(PdfEncodingMapConstPtr encoding, PdfToUnicodeMapConstPtr toUnicode)
    : PdfEncoding(GetNextId(), std::move(encoding), std::move(toUnicode))
{
    if (m_ToUnicode != nullptr && m_ToUnicode->GetType() != PdfEncodingMapType::CMap)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "The encoding map must be CMap type");
}

PdfEncoding::PdfEncoding(unsigned id, PdfEncodingMapConstPtr&& encoding, PdfEncodingMapConstPtr&& toUnicode)
    : m_Id(id), m_IsObjectLoaded(false), m_Font(nullptr), m_Encoding(std::move(encoding)), m_ToUnicode(std::move(toUnicode))
{
    if (m_Encoding == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Main encoding must be not null");
}

PdfEncoding::PdfEncoding(unsigned id, bool isObjectLoaded, const PdfEncodingLimits& limits, PdfFont* font,
        PdfEncodingMapConstPtr&& encoding, PdfEncodingMapConstPtr&& toUnicode,
        PdfCIDToGIDMapConstPtr&& cidToGidMap) :
    m_Id(id), m_IsObjectLoaded(isObjectLoaded), m_ParsedLimits(limits), m_Font(font),
    m_Encoding(std::move(encoding)), m_ToUnicode(std::move(toUnicode)), m_CIDToGIDMap(std::move(cidToGidMap))
{
}

PdfEncoding PdfEncoding::Create(const PdfEncoding& ref, PdfToUnicodeMapConstPtr&& toUnicode)
{
    return PdfEncoding(GetNextId(), ref.IsObjectLoaded(), ref.GetLimits(),
        nullptr, ref.GetEncodingMapPtr(), std::move(toUnicode), nullptr);
}

PdfEncoding PdfEncoding::Create(const PdfEncodingLimits& parsedLimits, PdfEncodingMapConstPtr&& encoding,
    PdfEncodingMapConstPtr&& toUnicode, PdfCIDToGIDMapConstPtr&& cidToGidMap)
{
    return PdfEncoding(GetNextId(), true, parsedLimits, nullptr,
        std::move(encoding), std::move(toUnicode), std::move(cidToGidMap));
}

unique_ptr<PdfEncoding> PdfEncoding::CreateSchim(const PdfEncoding& encoding, PdfFont& font)
{
    unique_ptr<PdfEncoding> ret(new PdfEncoding(encoding));
    ret->m_Font = &font;
    return ret;
}

unique_ptr<PdfEncoding> PdfEncoding::CreateDynamicEncoding(const shared_ptr<PdfCharCodeMap>& cidMap,
    const shared_ptr<PdfCharCodeMap>& toUnicodeMap, PdfFont& font)
{
    unique_ptr<PdfEncoding> ret(new PdfEncoding(GetNextId(), PdfEncodingMapConstPtr(new PdfDynamicEncodingMap(cidMap)),
        PdfEncodingMapConstPtr(new PdfDynamicEncodingMap(toUnicodeMap))));
    ret->m_Font = &font;
    return ret;
}

string PdfEncoding::ConvertToUtf8(const PdfString& encodedStr) const
{
    // Just ignore failures
    string ret;
    (void)tryConvertEncodedToUtf8(encodedStr.GetRawData(), ret);
    return ret;
}

bool PdfEncoding::TryConvertToUtf8(const PdfString& encodedStr, string& str) const
{
    return tryConvertEncodedToUtf8(encodedStr.GetRawData(), str);
}

charbuff PdfEncoding::ConvertToEncoded(const string_view& str) const
{
    charbuff ret;
    if (!TryConvertToEncoded(str, ret))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "The provided string can't be converted to CID encoding");

    return ret;
}

bool PdfEncoding::TryConvertToEncoded(const string_view& str, charbuff& encoded) const
{
    encoded.clear();
    if (str.empty())
        return true;

    PODOFO_ASSERT(m_Font != nullptr);
    if (m_IsObjectLoaded || !m_Font->GetMetrics().HasUnicodeMapping())
    {
        // The font is loaded from object or substitute. We will attempt
        // to use the loaded map to perform the conversion
        const PdfEncodingMap* toUnicode;
        if (!GetToUnicodeMapSafe(toUnicode))
            return false;

        auto it = str.begin();
        auto end = str.end();
        while (it != end)
        {
            PdfCharCode code;
            if (!toUnicode->TryGetNextCharCode(it, end, code))
                return false;

            code.AppendTo(encoded);
        }

        return true;
    }
    else
    {
        // If the font is not loaded from object but created
        // from scratch, we will attempt first to infer GIDs
        // from Unicode code points using the font metrics
        auto& metrics = m_Font->GetMetrics();
        auto it = str.begin();
        auto end = str.end();
        vector<unsigned> gids;
        vector<char32_t> cps;   // Code points
        while (it != end)
        {
            char32_t cp = utf8::next(it, end);
            unsigned gid;
            if (!metrics.TryGetGID(cp, gid))
                return false;

            cps.push_back(cp);
            gids.push_back(gid);
        }

        // Try to subsistute GIDs for fonts that support
        // a glyph substitution mechanism
        vector<unsigned char> backwardMap;
        metrics.SubstituteGIDs(gids, backwardMap);

        // Add used gid to the font mapping afferent code points,
        // and append the returned code unit to encoded string
        unsigned cpOffset = 0;
        PdfCharCode codeUnit;
        for (unsigned i = 0; i < gids.size(); i++)
        {
            unsigned char cpsSpanSize = backwardMap[i];
            unicodeview span(cps.data() + cpOffset, cpsSpanSize);
            if (!tryGetCharCode(*m_Font, gids[i], span, codeUnit))
                return false;

            codeUnit.AppendTo(encoded);
            cpOffset += cpsSpanSize;
        }

        return true;
    }
}

bool PdfEncoding::tryGetCharCode(PdfFont& font, unsigned gid, const unicodeview& codePoints, PdfCharCode& codeUnit) const
{
    if (font.IsSubsettingEnabled() && !font.IsProxy())
    {
        PdfCID cid;
        if (font.TryAddSubsetGID(gid, codePoints, cid))
        {
            codeUnit = cid.Unit;
            return true;
        }

        codeUnit = { };
        return false;
    }
    else
    {
        if (IsDynamicEncoding())
        {
            codeUnit = font.AddCharCodeSafe(gid, codePoints);
            return true;
        }
        else
        {
            return GetToUnicodeMapSafe().TryGetCharCode(codePoints, codeUnit);
        }
    }
}

bool PdfEncoding::tryConvertEncodedToUtf8(const string_view& encoded, string& str) const
{
    str.clear();
    if (encoded.empty())
        return true;

    auto& map = GetToUnicodeMapSafe();
    auto& limits = map.GetLimits();
    bool success = true;
    auto it = encoded.begin();
    auto end = encoded.end();
    CodePointSpan codePoints;
    while (it != end)
    {
        if (!map.TryGetNextCodePoints(it, end, codePoints))
        {
            success = false;
            codePoints = CodePointSpan((char32_t)fetchFallbackCharCode(it, end, limits).Code);
        }

        auto view = codePoints.view();
        for (size_t i = 0; i < view.size(); i++)
        {
            char32_t codePoint = view[i];
            if (codePoint != U'\0' && utf8::internal::is_code_point_valid(codePoint))
            {
                // Validate codepoints to insert
                utf8::unchecked::append((uint32_t)view[i], std::back_inserter(str));
            }
        }
    }

    return success;
}

vector<PdfCID> PdfEncoding::ConvertToCIDs(const PdfString& encodedStr) const
{
    // Just ignore failures
    vector<PdfCID> cids;
    (void)tryConvertEncodedToCIDs(encodedStr.GetRawData(), cids);
    return cids;
}

bool PdfEncoding::TryGetCIDId(const PdfCharCode& codeUnit, unsigned& cid) const
{
    if (m_Encoding->GetType() == PdfEncodingMapType::CMap)
    {
        return m_Encoding->TryGetCIDId(codeUnit, cid);
    }
    else
    {
        PODOFO_INVARIANT(m_Encoding->IsSimpleEncoding());
        PODOFO_ASSERT(m_Font != nullptr);
        if (m_IsObjectLoaded || !m_Font->GetMetrics().HasUnicodeMapping())
        {
            // Assume cid == charcode
            cid = codeUnit.Code;
            return true;
        }
        else
        {
            // Retrieve the code point and get directly the
            // a GID from the metrics
            char32_t cp = GetCodePoint(codeUnit);
            unsigned gid;
            if (cp == U'\0' || !m_Font->GetMetrics().TryGetGID(cp, gid))
            {
                cid = 0;
                return false;
            }

            // We assume cid == gid identity
            cid = gid;
            return true;
        }
    }
}

bool PdfEncoding::TryConvertToCIDs(const PdfString& encodedStr, vector<PdfCID>& cids) const
{
    return tryConvertEncodedToCIDs(encodedStr.GetRawData(), cids);
}

bool PdfEncoding::tryConvertEncodedToCIDs(const string_view& encoded, vector<PdfCID>& cids) const
{
    cids.clear();
    if (encoded.empty())
        return true;

    bool success = true;
    auto it = encoded.begin();
    auto end = encoded.end();
    auto& limits = m_Encoding->GetLimits();
    PdfCID cid;
    while (it != end)
    {
        if (!m_Encoding->TryGetNextCID(it, end, cid))
        {
            success = false;
            PdfCharCode unit = fetchFallbackCharCode(it, end, limits);
            cid = PdfCID(unit);
        }

        cids.push_back(cid);
    }

    return success;
}

const PdfCharCode& PdfEncoding::GetFirstChar() const
{
    auto& limits = GetLimits();
    if (limits.FirstChar.Code > limits.LastChar.Code)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "FirstChar shall be smaller than LastChar");

    return limits.FirstChar;
}

const PdfCharCode& PdfEncoding::GetLastChar() const
{
    auto& limits = GetLimits();
    if (limits.FirstChar.Code > limits.LastChar.Code)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "FirstChar shall be smaller than LastChar");

    return limits.LastChar;
}

void PdfEncoding::ExportToFont(PdfFont& font) const
{
    exportToFont(font, nullptr);
}

void PdfEncoding::ExportToFont(PdfFont& font, const PdfCIDSystemInfo& cidInfo) const
{
    exportToFont(font, &cidInfo);
}

void PdfEncoding::exportToFont(PdfFont& font, const PdfCIDSystemInfo* cidInfo) const
{
    auto& fontDict = font.GetDictionary();
    if (font.IsCIDFont())
    {
        PODOFO_ASSERT(cidInfo != nullptr);

        // The CIDSystemInfo, should be an indirect object
        auto& cidInfoObj = font.GetDocument().GetObjects().CreateDictionaryObject();
        cidInfoObj.GetDictionary().AddKey("Registry"_n, cidInfo->Registry);
        cidInfoObj.GetDictionary().AddKey("Ordering"_n, cidInfo->Ordering);
        cidInfoObj.GetDictionary().AddKey("Supplement"_n, static_cast<int64_t>(cidInfo->Supplement));

        // NOTE: Setting the CIDSystemInfo params in the descendant font object is required
        font.GetDescendantFontObject().GetDictionary().AddKeyIndirect("CIDSystemInfo"_n, cidInfoObj);

        // Some CMap encodings has a name representation, such as
        // Identity-H/Identity-V. NOTE: Use a fixed representation only
        // if we are not subsetting. In that case we unconditionally want a CID mapping
        if (font.HasCIDSubset() || !tryExportEncodingTo(fontDict, true))
        {
            // If it doesn't have a name representation, try to export a CID CMap
            auto& cmapObj = fontDict.GetOwner()->GetDocument()->GetObjects().CreateDictionaryObject();

            // NOTE: Setting the CIDSystemInfo params in the CMap stream object is required
            cmapObj.GetDictionary().AddKeyIndirect("CIDSystemInfo"_n, cidInfoObj);

            writeCIDMapping(cmapObj, font, *cidInfo);
            fontDict.AddKeyIndirect("Encoding"_n, cmapObj);
        }
    }
    else // Simple font
    {
        if (!tryExportEncodingTo(fontDict, false))
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "The encoding should supply an export object");

        fontDict.AddKey("FirstChar"_n, PdfVariant(static_cast<int64_t>(GetFirstChar().Code)));
        fontDict.AddKey("LastChar"_n, PdfVariant(static_cast<int64_t>(GetLastChar().Code)));
    }

    auto& cmapObj = fontDict.GetOwner()->GetDocument()->GetObjects().CreateDictionaryObject();
    writeToUnicodeCMap(cmapObj, font);
    fontDict.AddKeyIndirect("ToUnicode"_n, cmapObj);
}

PdfStringScanContext PdfEncoding::StartStringScan(const PdfString& encodedStr)
{
    return PdfStringScanContext(encodedStr.GetRawData(), *this);
}

bool PdfEncoding::tryExportEncodingTo(PdfDictionary& dictionary, bool wantCIDMapping) const
{
    if (wantCIDMapping && !HasCIDMapping())
    {
        // If we want a CID mapping but we don't have
        // one, just return here
        return false;
    }

    auto& objects = dictionary.GetOwner()->GetDocument()->GetObjects();
    PdfName name;
    PdfObject* obj;

    if (!m_Encoding->TryGetExportObject(objects, name, obj))
        return false;

    if (obj == nullptr)
        dictionary.AddKey("Encoding"_n, name);
    else
        dictionary.AddKeyIndirect("Encoding"_n, *obj);

    return true;
}

bool PdfEncoding::IsNull() const
{
    return m_Id == NullEncodingId;
}

bool PdfEncoding::HasCIDMapping() const
{
    // The encoding of the font has a CID mapping when it's a
    // predefined CMap name, such as Identity-H/Identity-V, when
    // the main /Encoding is a CMap, or it exports a CMap anyway,
    // such in case of custom PdfIdentityEncoding
    return m_Encoding->GetType() == PdfEncodingMapType::CMap;
}

bool PdfEncoding::IsSimpleEncoding() const
{
    switch (m_Encoding->GetType())
    {
        case PdfEncodingMapType::Simple:
            return true;
        case PdfEncodingMapType::Indeterminate:
            // NOTE: See TrueType implicit encoding
            // CHECK-ME: Maybe we should check font type instead,
            // eg. /Type1 and /Type3 can use only simple encodings
            return m_ParsedLimits.AreValid();
        case PdfEncodingMapType::CMap:
            return false;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

bool PdfEncoding::HasParsedLimits() const
{
    return m_ParsedLimits.AreValid();
}

bool PdfEncoding::IsDynamicEncoding() const
{
    return m_Encoding != nullptr && typeid(*m_Encoding) == typeid(PdfDynamicEncodingMap);
}

char32_t PdfEncoding::GetCodePoint(const PdfCharCode& codeUnit) const
{
    auto& map = GetToUnicodeMapSafe();
    CodePointSpan codePoints;
    if (!map.TryGetCodePoints(codeUnit, codePoints)
        || codePoints.GetSize() != 1)
    {
        return U'\0';
    }

    return *codePoints;
}

char32_t PdfEncoding::GetCodePoint(unsigned charCode) const
{
    auto& map = GetToUnicodeMapSafe();
    auto& limits = map.GetLimits();
    CodePointSpan codePoints;
    for (unsigned char i = limits.MinCodeSize; i <= limits.MaxCodeSize; i++)
    {
        if (map.TryGetCodePoints({ charCode, i }, codePoints)
            && codePoints.GetSize() == 1)
        {
            return *codePoints;
        }
    }

    return U'\0';
}

const PdfEncodingLimits& PdfEncoding::GetLimits() const
{
    if (m_ParsedLimits.AreValid())
        return m_ParsedLimits;

    return m_Encoding->GetLimits();
}

bool PdfEncoding::HasValidToUnicodeMap() const
{
    const PdfEncodingMap* ret;
    return GetToUnicodeMapSafe(ret);
}

const PdfEncodingMap& PdfEncoding::GetToUnicodeMap() const
{
    const PdfEncodingMap* ret;
    if (!GetToUnicodeMapSafe(ret))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "No valid /ToUnicode map present");

    return *ret;
}

const PdfEncodingMap& PdfEncoding::GetToUnicodeMapSafe() const
{
    const PdfEncodingMap* ret;
    (void)GetToUnicodeMapSafe(ret);
    return *ret;
}

PdfEncodingMapConstPtr PdfEncoding::GetToUnicodeMapPtr() const
{
    if (m_ToUnicode != nullptr)
        return m_ToUnicode;

    if (m_Encoding->GetType() == PdfEncodingMapType::Simple)
        return m_Encoding;

    return nullptr;
}

bool PdfEncoding::GetToUnicodeMapSafe(const PdfEncodingMap*& toUnicode) const
{
    if (m_ToUnicode != nullptr)
    {
        toUnicode = m_ToUnicode.get();
        return true;
    }

    // Fallback to main /Encoding entry. It is a valid
    // ToUnicode map for simple encodings
    toUnicode = m_Encoding.get();
    return IsSimpleEncoding();
}

// Handle missing mapped code by just appending current
// extracted raw character of minimum code size. Increment the
// iterator since failure on previous call doesn't do it. This
// is similar to what Adobe reader does for 1 byte encodings
// TODO: See also Pdf Reference 1.7 "Handling Undefined Characters"
// and try to implement all the fallbacks rules that applies here
// properly. Note: CID 0 fallback selection doesn't apply here.
// that is needed only when rendering the glyph
PdfCharCode fetchFallbackCharCode(string_view::iterator& it, const string_view::iterator& end, const PdfEncodingLimits& limits)
{
    unsigned code = (unsigned char)*it;
    unsigned char i = 1;
    PODOFO_ASSERT(i <= limits.MinCodeSize);
    while (true)
    {
        it++;
        if (it == end || i == limits.MinCodeSize)
            break;

        code = code << 8 | (unsigned char)*it;
        i++;
    }

    return { code, i };
}

void PdfEncoding::writeCIDMapping(PdfObject& cmapObj, const PdfFont& font, const PdfCIDSystemInfo& cidInfo) const
{
    // CMap specification is in Adobe technical node #5014
    auto& cmapDict = cmapObj.GetDictionary();

    string cmapName = "CMap-";
    cmapName.append(cidInfo.Ordering.GetString());

    // Table 120: Additional entries in a CMap stream dictionary
    cmapDict.AddKey("Type"_n, "CMap"_n);
    cmapDict.AddKey("CMapName"_n, PdfName(cmapName));

    charbuff temp;
    auto& stream = cmapObj.GetOrCreateStream();
    auto output = stream.GetOutputStream();
    utls::FormatTo(temp,
        "/CIDInit /ProcSet findresource begin\n"
        "12 dict begin\n"
        "begincmap\n"
        "/CIDSystemInfo <<\n"
        "   /Registry ({})\n"
        "   /Ordering ({})\n"
        "   /Supplement {}\n"
        ">> def\n"
        "/CMapName /{} def\n"
        "/CMapType 1 def\n"     // As defined in Adobe Technical Notes #5099
        , cidInfo.Registry.GetString(), cidInfo.Ordering.GetString(),
        cidInfo.Supplement, cmapName);
    auto wmode = m_Encoding->GetWModeSafe();
    if (wmode != PdfWModeKind::Horizontal)
    {
        utls::FormatTo(temp, "/WMode {} def\n", (unsigned)wmode);
        output.Write(temp);
    }
    output.Write(temp);

    unique_ptr<PdfEncodingMap> replCIDEncodingMap;
    if (font.TryGetSubstituteCIDEncoding(replCIDEncodingMap))
    {
        replCIDEncodingMap->AppendCodeSpaceRange(output, temp);
        replCIDEncodingMap->AppendCIDMappingEntries(output, font, temp);
    }
    else
    {
        m_Encoding->AppendCodeSpaceRange(output, temp);
        m_Encoding->AppendCIDMappingEntries(output, font, temp);
    }

    output.Write(
        "endcmap\n"
        "CMapName currentdict /CMap defineresource pop\n"
        "end\n"
        "end");
}

void PdfEncoding::writeToUnicodeCMap(PdfObject& cmapObj, const PdfFont& font) const
{
    (void)font;
    // NOTE: We definitely want a valid Unicode map at this point
    charbuff temp;
    auto& toUnicode = GetToUnicodeMap();
    auto& stream = cmapObj.GetOrCreateStream();
    auto output = stream.GetOutputStream();

    // CMap specification is in Adobe technical node #5014
    // The /ToUnicode dictionary doesn't need /CMap type, /CIDSystemInfo or /CMapName
    output.Write(
        "/CIDInit /ProcSet findresource begin\n"
        "12 dict begin\n"
        "begincmap\n"
        "/CIDSystemInfo <<\n"
        "   /Registry (Adobe)\n"
        "   /Ordering (UCS)\n"
        "   /Supplement 0\n"
        ">> def\n"
        "/CMapName /Adobe-Identity-UCS def\n"
        "/CMapType 2 def\n");     // As defined in Adobe Technical Notes #5099
    toUnicode.AppendCodeSpaceRange(output, temp);

    toUnicode.AppendToUnicodeEntries(output, temp);
    output.Write(
        "endcmap\n"
        "CMapName currentdict /CMap defineresource pop\n"
        "end\n"
        "end");
}

unsigned PdfEncoding::GetNextId()
{
    static atomic<unsigned> s_nextid = CustomEncodingStartId;
    return s_nextid++;
}

PdfStringScanContext::PdfStringScanContext(const string_view& encodedstr, const PdfEncoding& encoding) :
    m_it(encodedstr.begin()),
    m_end(encodedstr.end()),
    m_encoding(&encoding.GetEncodingMap()),
    m_limits(m_encoding->GetLimits()),
    m_toUnicode(&encoding.GetToUnicodeMapSafe())
{
}

bool PdfStringScanContext::IsEndOfString() const
{
    return m_it == m_end;
}

bool PdfStringScanContext::TryScan(PdfCID& cid, string& utf8str, CodePointSpan& codepoints)
{
    vector<unsigned> positions;
    return TryScan(cid, utf8str, positions, codepoints);
}

bool PdfStringScanContext::TryScan(PdfCID & cid, string &utf8str, vector<unsigned>&positions, CodePointSpan & codepoints)
{
    bool success = true;
    if (!m_encoding->TryGetNextCID(m_it, m_end, cid))
    {
        PdfCharCode unit = fetchFallbackCharCode(m_it, m_end, m_limits);
        cid = PdfCID(unit);
        success = false;
    }

    if (m_toUnicode->TryGetCodePoints(cid, codepoints))
    {
        auto view = codepoints.view();
        unsigned prevPos = (unsigned)utf8str.length();
        for (size_t i = 0; i < view.size(); i++)
        {
            char32_t codePoint = view[i];
            if (codePoint != U'\0' && utf8::internal::is_code_point_valid(codePoint))
            {
                // Validate codepoints to insert
                utf8::unchecked::append((uint32_t)view[i], std::back_inserter(utf8str));
                positions.push_back(prevPos);
                prevPos = (unsigned)utf8str.length();
            }
        }
    }
    else
    {
        success = false;
    }

    return success;
}

PdfDynamicEncodingMap::PdfDynamicEncodingMap(shared_ptr<PdfCharCodeMap> map)
    : PdfEncodingMapBase(std::move(map), PdfEncodingMapType::CMap) {
}

void PdfDynamicEncodingMap::AppendCodeSpaceRange(OutputStream& stream, charbuff& temp) const
{
    vector<unsigned char> usedCodeSpaceSizes;
    for (auto& pair : m_charMap->GetMappings())
        pushCodeRangeSize(usedCodeSpaceSizes, pair.first.CodeSpaceSize);

    for (auto& range : m_charMap->GetRanges())
        pushCodeRangeSize(usedCodeSpaceSizes, range.SrcCodeLo.CodeSpaceSize);

    unsigned size = 0;
    for (auto& usedCodeSpaceSize : usedCodeSpaceSizes)
    {
        vector<utls::FSSUTFRange> ranges = utls::GetFSSUTFRanges(usedCodeSpaceSize);
        size += (unsigned)ranges.size();
    }

    stream.Write(std::to_string(size));
    stream.Write(" begincodespacerange\n");

    bool first = true;
    for (auto& usedCodeSpaceSize : usedCodeSpaceSizes)
    {
        vector<utls::FSSUTFRange> ranges = utls::GetFSSUTFRanges(usedCodeSpaceSize);

        for (auto& range : ranges)
        {
            if (first)
                first = false;
            else
                stream.Write("\n");

            PdfCharCode firstCode(range.FirstCode);
            PdfCharCode lastCode(range.LastCode);

            firstCode.WriteHexTo(temp);
            stream.Write(temp);
            lastCode.WriteHexTo(temp);
            stream.Write(temp);
        }
    }

    stream.Write("\nendcodespacerange\n");
}

void pushCodeRangeSize(vector<unsigned char>& codeRangeSizes, unsigned char codeRangeSize)
{
    auto found = std::find(codeRangeSizes.begin(), codeRangeSizes.end(), codeRangeSize);
    if (found != codeRangeSizes.end())
        return;

    codeRangeSizes.push_back(codeRangeSize);
}

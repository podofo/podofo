/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfEncoding.h"

#include <atomic>
#include <utfcpp/utf8.h>

#include <podofo/private/PdfEncodingPrivate.h>

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfEncodingMapFactory.h"

using namespace std;
using namespace PoDoFo;

static PdfCharCode fetchFallbackCharCode(string_view::iterator& it, const string_view::iterator& end, const PdfEncodingLimits& limits);

PdfEncoding::PdfEncoding()
    : PdfEncoding(NullEncodingId, PdfEncodingMapFactory::GetNullEncodingMap(), nullptr)
{
}

PdfEncoding::PdfEncoding(const PdfEncodingMapConstPtr& encoding, const PdfToUnicodeMapConstPtr& toUnicode)
    : PdfEncoding(GetNextId(), encoding, toUnicode)
{
    if (toUnicode != nullptr && toUnicode->GetType() != PdfEncodingMapType::CMap)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "The encoding map must be CMap type");
}

PdfEncoding::PdfEncoding(size_t id, const PdfEncodingMapConstPtr& encoding, const PdfEncodingMapConstPtr& toUnicode)
    : m_Id(id), m_Encoding(encoding), m_ToUnicode(toUnicode)
{
    if (encoding == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Main encoding must be not null");
}

PdfEncoding::PdfEncoding(const PdfObject& fontObj, const PdfEncodingMapConstPtr& encoding, const PdfEncodingMapConstPtr& toUnicode)
    : PdfEncoding(GetNextId(), encoding, toUnicode)
{
    auto firstCharObj = fontObj.GetDictionary().FindKey("FirstChar");
    if (firstCharObj != nullptr)
        m_ParsedLimits.FirstChar = PdfCharCode(static_cast<unsigned>(firstCharObj->GetNumber()));

    auto lastCharObj = fontObj.GetDictionary().FindKey("LastChar");
    if (lastCharObj != nullptr)
        m_ParsedLimits.LastChar = PdfCharCode(static_cast<unsigned>(lastCharObj->GetNumber()));

    if (m_ParsedLimits.LastChar.Code > m_ParsedLimits.FirstChar.Code)
    {
        // If found valid /FirstChar and /LastChar, valorize
        //  also the code size limits
        m_ParsedLimits.MinCodeSize = utls::GetCharCodeSize(m_ParsedLimits.FirstChar.Code);
        m_ParsedLimits.MaxCodeSize = utls::GetCharCodeSize(m_ParsedLimits.LastChar.Code);
    }
}

PdfEncoding::~PdfEncoding() { }

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

    auto& font = GetFont();
    if (font.IsObjectLoaded() || !font.GetMetrics().HasUnicodeMapping())
    {
        // The font is loaded from object. We will attempt to use
        // just the loaded map to perform the conversion
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
        auto& metrics = font.GetMetrics();
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
            if (!tryGetCharCode(font, gids[i], span, codeUnit))
                return false;

            codeUnit.AppendTo(encoded);
            cpOffset += cpsSpanSize;
        }

        return true;
    }
}

bool PdfEncoding::tryGetCharCode(PdfFont& font, unsigned gid, const unicodeview& codePoints, PdfCharCode& codeUnit) const
{
    if (font.IsSubsettingEnabled())
    {
        codeUnit = font.AddSubsetGIDSafe(gid, codePoints).Unit;
        return true;
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
            if (!GetToUnicodeMapSafe().TryGetCharCode(codePoints, codeUnit))
                return false;

            return true;
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
    vector<char32_t> codePoints;
    while (it != end)
    {
        if (!map.TryGetNextCodePoints(it, end, codePoints))
        {
            success = false;
            codePoints.clear();
            codePoints.push_back((char32_t)fetchFallbackCharCode(it, end, limits).Code);
        }

        for (size_t i = 0; i < codePoints.size(); i++)
        {
            char32_t codePoint = codePoints[i];
            if (codePoint != U'\0' && utf8::internal::is_code_point_valid(codePoint))
            {
                // Validate codepoints to insert
                utf8::unchecked::append((uint32_t)codePoints[i], std::back_inserter(str));
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
        auto& font = GetFont();
        if (font.IsObjectLoaded() || !font.GetMetrics().HasUnicodeMapping())
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
            if (cp == U'\0' || !font.GetMetrics().TryGetGID(cp, gid))
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

void PdfEncoding::ExportToFont(PdfFont& font, PdfEncodingExportFlags flags) const
{
    auto& fontDict = font.GetObject().GetDictionary();
    if (font.IsCIDKeyed())
    {
        auto fontName = font.GetName();

        // The CIDSystemInfo, should be an indirect object
        auto& cidSystemInfo = font.GetDocument().GetObjects().CreateDictionaryObject();
        cidSystemInfo.GetDictionary().AddKey("Registry", PdfString(CMAP_REGISTRY_NAME));
        cidSystemInfo.GetDictionary().AddKey("Ordering", PdfString(fontName));
        cidSystemInfo.GetDictionary().AddKey("Supplement", static_cast<int64_t>(0));

        // NOTE: Setting the CIDSystemInfo params in the descendant font object is required
        font.GetDescendantFontObject().GetDictionary().AddKeyIndirect("CIDSystemInfo", cidSystemInfo);

        // Some CMap encodings has a name representation, such as
        // Identity-H/Identity-V. NOTE: Use a fixed representation only
        // if we are not subsetting. In that case we want a CID mapping
        if (font.IsSubsettingEnabled() || !tryExportObjectTo(fontDict, true))
        {
            // If it doesn't have a name represenation, try to export a CID CMap
            auto& cmapObj = fontDict.GetOwner()->GetDocument()->GetObjects().CreateDictionaryObject();

            // NOTE: Setting the CIDSystemInfo params in the CMap stream object is required
            cmapObj.GetDictionary().AddKeyIndirect("CIDSystemInfo", cidSystemInfo);

            writeCIDMapping(cmapObj, GetFont(), fontName);
            fontDict.AddKeyIndirect("Encoding", cmapObj);
        }
    }
    else
    {
        if (!tryExportObjectTo(fontDict, false))
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "The encoding should supply an export object");
    }

    if ((flags & PdfEncodingExportFlags::SkipToUnicode) == PdfEncodingExportFlags::None)
    {
        auto& cmapObj = fontDict.GetOwner()->GetDocument()->GetObjects().CreateDictionaryObject();
        writeToUnicodeCMap(cmapObj);
        fontDict.AddKeyIndirect("ToUnicode", cmapObj);
    }
}

PdfStringScanContext PdfEncoding::StartStringScan(const PdfString& encodedStr)
{
    return PdfStringScanContext(encodedStr.GetRawData(), *this);
}

bool PdfEncoding::tryExportObjectTo(PdfDictionary& dictionary, bool wantCIDMapping) const
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
        dictionary.AddKey("Encoding", name);
    else
        dictionary.AddKeyIndirect("Encoding", *obj);

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
    return m_Encoding->GetType() == PdfEncodingMapType::Simple;
}

bool PdfEncoding::HasParsedLimits() const
{
    return m_ParsedLimits.AreValid();
}

bool PdfEncoding::IsDynamicEncoding() const
{
    return false;
}

PdfFont& PdfEncoding::GetFont() const
{
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "The encoding has not been binded to a font");
}

char32_t PdfEncoding::GetCodePoint(const PdfCharCode& codeUnit) const
{
    auto& map = GetToUnicodeMapSafe();
    vector<char32_t> codePoints;
    if (!map.TryGetCodePoints(codeUnit, codePoints)
        || codePoints.size() != 1)
    {
        return U'\0';
    }

    return codePoints[0];
}

char32_t PdfEncoding::GetCodePoint(unsigned charCode) const
{
    auto& map = GetToUnicodeMapSafe();
    auto& limits = map.GetLimits();
    vector<char32_t> codePoints;
    for (unsigned char i = limits.MinCodeSize; i <= limits.MaxCodeSize; i++)
    {
        if (map.TryGetCodePoints({ charCode, i }, codePoints)
            && codePoints.size() == 1)
        {
            return codePoints[0];
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

const PdfEncodingMapConstPtr PdfEncoding::GetToUnicodeMapPtr() const
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

void PdfEncoding::writeCIDMapping(PdfObject& cmapObj, const PdfFont& font, const string_view& fontName) const
{
    // CMap specification is in Adobe technical node #5014
    auto& cmapDict = cmapObj.GetDictionary();
    string cmapName = (string)fontName;
    if (font.IsSubsettingEnabled())
        cmapName.append("-subset");

    // Table 120: Additional entries in a CMap stream dictionary
    cmapDict.AddKey(PdfName::KeyType, PdfName("CMap"));
    cmapDict.AddKey("CMapName", PdfName(cmapName));

    charbuff temp;
    auto& stream = cmapObj.GetOrCreateStream();
    auto output = stream.GetOutputStream();
    utls::FormatTo(temp,
        "/CIDInit /ProcSet findresource begin\n"
        "12 dict begin\n"
        "begincmap\n"
        "/CIDSystemInfo <<\n"
        "   /Registry (" CMAP_REGISTRY_NAME  ")\n"
        "   /Ordering ({})\n"
        "   /Supplement 0\n"
        ">> def\n"
        "/CMapName /{} def\n"
        "/CMapType 1 def\n"     // As defined in Adobe Technical Notes #5099
        "1 begincodespacerange\n", fontName, cmapName);
    output.Write(temp);

    if (font.IsSubsettingEnabled())
    {
        struct Limit
        {
            PdfCharCode FirstCode;
            PdfCharCode LastCode;
        };

        unordered_map<unsigned char, Limit> ranges;
        auto& usedGids = font.GetUsedGIDs();
        for (auto& pair : usedGids)
        {
            auto& codeUnit = pair.second.Unit;
            auto found = ranges.find(codeUnit.CodeSpaceSize);
            if (found == ranges.end())
            {
                ranges[codeUnit.CodeSpaceSize] = Limit{ codeUnit, codeUnit };
            }
            else
            {
                auto& limit = found->second;
                if (codeUnit.Code < limit.FirstCode.Code)
                    limit.FirstCode = codeUnit;

                if (codeUnit.Code > limit.LastCode.Code)
                    limit.LastCode = codeUnit;
            }
        }

        bool first = true;
        for (auto& pair : ranges)
        {
            if (first)
                first = false;
            else
                output.Write("\n");

            auto& range = pair.second;
            range.FirstCode.WriteHexTo(temp);
            output.Write(temp);
            range.LastCode.WriteHexTo(temp);
            output.Write(temp);
        }
    }
    else
    {
        m_Encoding->AppendCodeSpaceRange(output, temp);
    }

    output.Write("\nendcodespacerange\n");

    if (font.IsSubsettingEnabled())
    {
        auto& usedGids = font.GetUsedGIDs();
        output.Write(std::to_string(usedGids.size()));
        output.Write(" begincidchar\n");
        string code;
        for (auto& pair : usedGids)
        {
            auto& cid = pair.second;
            cid.Unit.WriteHexTo(code);
            output.Write(code);
            output.Write(" ");
            output.Write(std::to_string(cid.Id));
            output.Write("\n");;
        }
        output.Write("endcidchar\n");
    }
    else
    {
        m_Encoding->AppendCIDMappingEntries(output, font, temp);
    }

    output.Write(
        "endcmap\n"
        "CMapName currentdict / CMap defineresource pop\n"
        "end\n"
        "end");
}

void PdfEncoding::writeToUnicodeCMap(PdfObject& cmapObj) const
{
    // NOTE: We definetely want a valid Unicode map at this point
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
        "/CMapType 2 def\n"     // As defined in Adobe Technical Notes #5099
        "1 begincodespacerange\n");
    toUnicode.AppendCodeSpaceRange(output, temp);
    output.Write("\nendcodespacerange\n");

    toUnicode.AppendToUnicodeEntries(output, temp);
    output.Write(
        "\nendcmap\n"
        "CMapName currentdict / CMap defineresource pop\n"
        "end\n"
        "end");
}

size_t PdfEncoding::GetNextId()
{
    static atomic<size_t> s_nextid = CustomEncodingStartId;
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

bool PdfStringScanContext::TryScan(PdfCID& cid, string& utf8str, vector<codepoint>& codepoints)
{
    bool success = true;
    if (!m_encoding->TryGetNextCID(m_it, m_end, cid))
    {
        PdfCharCode unit = fetchFallbackCharCode(m_it, m_end, m_limits);
        cid = PdfCID(unit);
        success = false;
    }

    if (m_toUnicode->TryGetCodePoints(cid.Unit, codepoints))
    {
        for (size_t i = 0; i < codepoints.size(); i++)
        {
            char32_t codePoint = codepoints[i];
            if (codePoint != U'\0' && utf8::internal::is_code_point_valid(codePoint))
            {
                // Validate codepoints to insert
                utf8::unchecked::append((uint32_t)codepoints[i], std::back_inserter(utf8str));
            }
        }
    }
    else
    {
        success = false;
    }

    return success;
}

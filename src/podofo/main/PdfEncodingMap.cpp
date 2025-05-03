/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfEncodingMap.h"

#include <utf8cpp/utf8.h>

#include <podofo/private/PdfEncodingPrivate.h>

#include "PdfDictionary.h"
#include "PdfCMapEncoding.h"
#include "PdfFont.h"

using namespace std;
using namespace PoDoFo;

PdfEncodingMap::PdfEncodingMap(PdfEncodingMapType type)
    : m_Type(type) { }

PdfEncodingMap::~PdfEncodingMap() { }

PdfCIDToGIDMapConstPtr PdfEncodingMap::GetIntrinsicCIDToGIDMap(const PdfDictionary& fontDict, const PdfFontMetrics& metrics) const
{
    (void)fontDict;
    (void)metrics;
    return nullptr;
}

bool PdfEncodingMap::TryGetExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const
{
    name = { };
    obj = nullptr;
    getExportObject(objects, name, obj);
    return !(obj == nullptr && name.IsNull());
}

void PdfEncodingMap::getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const
{
    (void)objects;
    (void)name;
    (void)obj;
}

bool PdfEncodingMap::TryGetNextCharCode(string_view::iterator& it, const string_view::iterator& end, PdfCharCode& code) const
{
    if (it == end)
    {
        code = { };
        return false;
    }

    if (HasLigaturesSupport())
    {
        auto temp = it;
        if (!tryGetNextCharCode(temp, end, code))
        {
            code = { };
            return false;
        }

        it = temp;
        return true;
    }
    else
    {
        char32_t cp = (char32_t)utf8::next(it, end);
        return tryGetCharCode(cp, code);
    }
}

bool PdfEncodingMap::tryGetNextCharCode(string_view::iterator& it, const string_view::iterator& end, PdfCharCode& codeUnit) const
{
    (void)it;
    (void)end;
    (void)codeUnit;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

bool PdfEncodingMap::tryGetCharCodeSpan(const unicodeview& ligature, PdfCharCode& codeUnit) const
{
    (void)ligature;
    (void)codeUnit;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

bool PdfEncodingMap::TryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const
{
    return tryGetCharCode(codePoint, codeUnit);
}

bool PdfEncodingMap::TryGetCharCode(const unicodeview& codePoints, PdfCharCode& codeUnit) const
{
    if (codePoints.size() == 1)
    {
        return tryGetCharCode(codePoints[0], codeUnit);
    }
    else if (codePoints.size() == 0 || !HasLigaturesSupport())
    {
        codeUnit = { };
        return false;
    }

    // Try to lookup the ligature
    PODOFO_INVARIANT(codePoints.size() > 1);
    return tryGetCharCodeSpan(codePoints, codeUnit);
}

bool PdfEncodingMap::TryGetCharCode(unsigned cid, PdfCharCode& codeUnit) const
{
    // NOTE: getting the char code from a cid on this map is the same operation
    // as getting it from an unicode code point
    return tryGetCharCode((char32_t)cid, codeUnit);
}

bool PdfEncodingMap::TryGetNextCID(string_view::iterator& it,
    const string_view::iterator& end, PdfCID& cid) const
{
    if (GetType() == PdfEncodingMapType::CMap)
    {
        CodePointSpan codePoints;
        bool success = tryGetNextCodePoints(it, end, cid.Unit, codePoints);
        if (!success || codePoints.GetSize() != 1)
        {
            // Return false on missing lookup or malformed multiple code points found
            cid = { };
            return false;
        }

        // char32_t is also unsigned
        cid.Id = (unsigned)*codePoints;
        return true;
    }
    else
    {
        // If there's no CID mapping, we just iterate character codes

        // Save current iterator in the case the search is unsuccessful
        string_view::iterator curr = it;

        unsigned code = 0;
        unsigned char i = 1;
        auto& limits = GetLimits();
        PODOFO_ASSERT(i <= limits.MaxCodeSize);
        while (curr != end)
        {
            // Iterate the string and accumulate a
            // code of the appropriate code size
            code <<= 8;
            code |= (unsigned char)*curr;
            curr++;
            if (i == limits.MaxCodeSize)
            {
                cid.Unit = { code, limits.MaxCodeSize };
                cid.Id = code; // We assume identity with CharCode
                it = curr;
                return true;
            }
            i++;
        }

        cid = { };
        return false;
    }
}

bool PdfEncodingMap::TryGetNextCodePoints(string_view::iterator& it,
    const string_view::iterator& end, CodePointSpan& codePoints) const
{
    PdfCharCode code;
    return tryGetNextCodePoints(it, end, code, codePoints);
}

bool PdfEncodingMap::TryGetCIDId(const PdfCharCode& codeUnit, unsigned& cid) const
{
    // NOTE: Here we assume the map will actually
    // contains cids, and not unicode codepoints
    CodePointSpan cids;
    bool success = tryGetCodePoints(codeUnit, nullptr, cids);
    if (!success || cids.GetSize() != 1)
    {
        // Return false on missing lookup or malformed multiple code points found
        return false;
    }

    cid = (unsigned)*cids;
    return true;
}

bool PdfEncodingMap::TryGetCodePoints(const PdfCharCode& codeUnit, CodePointSpan& codePoints) const
{
    return tryGetCodePoints(codeUnit, nullptr, codePoints);
}

bool PdfEncodingMap::TryGetCodePoints(const PdfCID& cid, CodePointSpan& codePoints) const
{
    return tryGetCodePoints(cid.Unit, &cid.Id, codePoints);
}

PdfPredefinedEncodingType PdfEncodingMap::GetPredefinedEncodingType() const
{
    return PdfPredefinedEncodingType::Indeterminate;
}

bool PdfEncodingMap::HasLigaturesSupport() const
{
    return false;
}

int PdfEncodingMap::GetWModeRaw() const
{
    return -1;
}

PdfWModeKind PdfEncodingMap::GetWModeSafe() const
{
    return GetWModeRaw() == 1 ? PdfWModeKind::Vertical : PdfWModeKind::Horizontal;
}

// NOTE: Don't clear the result on failure. It is done externally
bool PdfEncodingMap::tryGetNextCodePoints(string_view::iterator& it, const string_view::iterator& end,
    PdfCharCode& codeUnit, CodePointSpan& codePoints) const
{
    // Save current iterator in the case the search is unsuccessful
    string_view::iterator curr = it;
    unsigned code = 0;
    unsigned char i = 1;
    auto& limits = GetLimits();
    while (curr != end)
    {
        if (i > limits.MaxCodeSize)
            return false;

        // ISO 32000-1:2008 "9.7.6.2 CMap Mapping"
        // "A sequence of one or more bytes is extracted from the string and matched against
        // the codespace ranges in the CMap. That is, the first byte is matched against 1-byte
        // codespace ranges; if no match is found, a second byte is extracted, and the 2-byte
        // srcCode is matched against 2-byte codespace ranges. This process continues for
        // successively longer codes until a match is found or all codespace ranges have been
        // tested. There will be at most one match because codespace ranges do not overlap."

        code <<= 8;
        code |= (uint8_t)*curr;
        curr++;
        codeUnit = { code, i };
        if (i < limits.MinCodeSize || !tryGetCodePoints(codeUnit, nullptr, codePoints))
        {
            i++;
            continue;
        }

        it = curr;
        return true;
    }

    return false;
}

void PdfEncodingMap::AppendCodeSpaceRange(OutputStream& stream, charbuff& temp) const
{
    stream.Write("1 begincodespacerange\n");
    auto& limits = GetLimits();
    limits.FirstChar.WriteHexTo(temp);
    stream.Write(temp);
    limits.LastChar.WriteHexTo(temp);
    stream.Write(temp);
    stream.Write("\nendcodespacerange\n");
}

PdfEncodingMapBase::PdfEncodingMapBase(PdfCharCodeMap&& map, PdfEncodingMapType type)
    : PdfEncodingMap(type), m_charMap(std::make_shared<PdfCharCodeMap>(std::move(map)))
{
}

PdfEncodingMapBase::PdfEncodingMapBase(shared_ptr<PdfCharCodeMap>&& map, PdfEncodingMapType type)
    : PdfEncodingMap(type), m_charMap(std::move(map))
{
    if (m_charMap == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Map must be not null");
}

bool PdfEncodingMapBase::tryGetNextCharCode(string_view::iterator& it, const string_view::iterator& end,
    PdfCharCode& codeUnit) const
{
    return m_charMap->TryGetNextCharCode(it, end, codeUnit);
}

bool PdfEncodingMapBase::tryGetCharCodeSpan(const unicodeview& codePoints, PdfCharCode& codeUnit) const
{
    return m_charMap->TryGetCharCode(codePoints, codeUnit);
}

bool PdfEncodingMapBase::tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const
{
    return m_charMap->TryGetCharCode(codePoint, codeUnit);
}

bool PdfEncodingMapBase::tryGetCodePoints(const PdfCharCode& code, const unsigned* cidId, CodePointSpan& codePoints) const
{
    (void)cidId;
    return m_charMap->TryGetCodePoints(code, codePoints);
}

void PdfEncodingMapBase::AppendCodeSpaceRange(OutputStream& stream, charbuff& temp) const
{
    PoDoFo::AppendCodeSpaceRangeTo(stream, *m_charMap, temp);
}

void PdfEncodingMapBase::AppendToUnicodeEntries(OutputStream& stream, charbuff& temp) const
{
    PoDoFo::AppendToUnicodeEntriesTo(stream, *m_charMap, temp);
}

void PdfEncodingMapBase::AppendCIDMappingEntries(OutputStream& stream, const PdfFont& font, charbuff& temp) const
{
    (void)font;
    PoDoFo::AppendCIDMappingEntriesTo(stream, *m_charMap, temp);
}

const PdfEncodingLimits& PdfEncodingMapBase::GetLimits() const
{
    return m_charMap->GetLimits();
}

PdfEncodingMapSimple::PdfEncodingMapSimple(const PdfEncodingLimits& limits)
    : PdfEncodingMap(PdfEncodingMapType::Simple), m_Limits(limits) { }

void PdfEncodingMapSimple::AppendToUnicodeEntries(OutputStream& stream, charbuff& temp) const
{
    auto& limits = GetLimits();
    PODOFO_ASSERT(limits.MaxCodeSize == 1);
    CodePointSpan codePoints;
    unsigned code = limits.FirstChar.Code;
    unsigned lastCode = limits.LastChar.Code;
    stream.Write("1 beginbfrange\n");
    limits.FirstChar.WriteHexTo(temp);
    stream.Write(temp);
    stream.Write(" ");
    limits.LastChar.WriteHexTo(temp);
    stream.Write(temp);
    stream.Write(" [\n");
    u16string u16tmp;
    for (; code <= lastCode; code++)
    {
        if (!TryGetCodePoints(PdfCharCode(code), codePoints))
        {
            // If we don't find the code in the encoding/font
            // program, it's safe to continue
            continue;
        }

        PoDoFo::AppendUTF16CodeTo(stream, codePoints, u16tmp);
        stream.Write("\n");
    }
    stream.Write("]\n");
    stream.Write("endbfrange\n");
}

void PdfEncodingMapSimple::AppendCIDMappingEntries(OutputStream& stream, const PdfFont& font, charbuff& temp) const
{
    auto& limits = GetLimits();
    PODOFO_ASSERT(limits.MaxCodeSize == 1);
    unsigned code = limits.FirstChar.Code;
    unsigned lastCode = limits.LastChar.Code;
    CodePointSpan codePoints;
    unsigned gid;
    struct Mapping
    {
        PdfCharCode Code;
        unsigned CID;
    };

    vector<Mapping> mappings;
    for (; code <= lastCode; code++)
    {
        PdfCharCode charCode(code);
        if (!TryGetCodePoints(charCode, codePoints))
        {
            // If we don't find the code in the encoding/font
            // program, it's safe to continue
            continue;
        }

        // NOTE: CID mapping entries in a CMap also map CIDs to glyph
        // indices within the font program, unless a /CIDToGID map is
        // used. Here, we won't provide one, so we ensure to query
        // for the GID in the font program
        if (!font.TryGetGID(*codePoints, PdfGlyphAccess::FontProgram, gid))
            continue;

        // NOTE: We will map the char code directly to the gid, so
        // we assume cid == gid identity
        mappings.push_back({ charCode, gid });
    }
    utls::FormatTo(temp, mappings.size());
    stream.Write(temp);
    stream.Write(" begincidchar\n");
    for (auto& mapping : mappings)
        PoDoFo::WriteCIDMapping(stream, mapping.Code, mapping.CID, temp);

    stream.Write("endcidchar\n");
}

const PdfEncodingLimits& PdfEncodingMapSimple::GetLimits() const
{
    return m_Limits;
}

// NOTE: We assume PdfNullEncodingMap will be used in the default
// constructed PdfEncoding that ends being replaced with a dynamic
// encoding in PdfFont. See PdfFont implementation
PdfNullEncodingMap::PdfNullEncodingMap()
    : PdfEncodingMap(PdfEncodingMapType::CMap) { }

const PdfEncodingLimits& PdfNullEncodingMap::GetLimits() const
{
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "The null encoding must be bound to a PdfFont");
}

bool PdfNullEncodingMap::tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const
{
    (void)codePoint;
    (void)codeUnit;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "The null encoding must be bound to a PdfFont");
}

bool PdfNullEncodingMap::tryGetCodePoints(const PdfCharCode& codeUnit, const unsigned* cidId, CodePointSpan& codePoints) const
{
    (void)codeUnit;
    (void)codePoints;
    (void)cidId;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "The null encoding must be bound to a PdfFont");
}

void PdfNullEncodingMap::AppendToUnicodeEntries(OutputStream& stream, charbuff& temp) const
{
    (void)stream;
    (void)temp;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "The null encoding must be bound to a PdfFont");
}

void PdfNullEncodingMap::AppendCIDMappingEntries(OutputStream& stream, const PdfFont& font, charbuff& temp) const
{
    (void)stream;
    (void)font;
    (void)temp;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "The null encoding must be bound to a PdfFont");
}

PdfBuiltInEncoding::PdfBuiltInEncoding(const PdfName& name)
    : PdfEncodingMapSimple({ 1, 1, PdfCharCode(0), PdfCharCode(0xFF) }), m_Name(name)
{
}

// Create an unicode to GID map, filtering on available GIDs
// specified in the input char code to GID map
void PdfBuiltInEncoding::CreateUnicodeToGIDMap(const unordered_map<unsigned, unsigned>& codeToGidMap,
    unordered_map<uint32_t, unsigned>& unicodeMap) const
{
    const char32_t* cpUnicodeTable = this->GetToUnicodeTable();
    for (unsigned i = 0; i < 256; i++)
    {
        char32_t unicodeCp = cpUnicodeTable[i];
        if (unicodeCp == U'\0')
            continue;

        // Verify if the gid is actually available in the code to GID map
        auto found = codeToGidMap.find(i);
        if (found == codeToGidMap.end())
            continue;

        // Set the found Unicode code point -> GID mapping
        unicodeMap[unicodeCp] = found->second;
    }
}

void PdfBuiltInEncoding::initEncodingTable()
{
    if (!m_EncodingTable.empty())
        return;

    const char32_t* cpUnicodeTable = this->GetToUnicodeTable();
    for (unsigned i = 0; i < 256; i++)
    {
        // fill the table with data
        m_EncodingTable[cpUnicodeTable[i]] =
            static_cast<unsigned char>(i);
    }
}

bool PdfBuiltInEncoding::tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const
{
    const_cast<PdfBuiltInEncoding*>(this)->initEncodingTable();
    auto found = m_EncodingTable.find(codePoint);
    if (found == m_EncodingTable.end())
    {
        codeUnit = { };
        return false;
    }

    codeUnit = { (unsigned char)found->second, 1 };
    return true;
}

bool PdfBuiltInEncoding::tryGetCodePoints(const PdfCharCode& codeUnit, const unsigned* cidId, CodePointSpan& codePoints) const
{
    (void)cidId;
    if (codeUnit.Code >= 256)
        return false;

    const char32_t* cpUnicodeTable = this->GetToUnicodeTable();
    codePoints = CodePointSpan(cpUnicodeTable[codeUnit.Code]);
    return true;
}

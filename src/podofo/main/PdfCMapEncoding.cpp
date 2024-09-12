/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfCMapEncoding.h"

#include <utf8cpp/utf8.h>

#include "PdfDictionary.h"
#include "PdfObjectStream.h"
#include "PdfPostScriptTokenizer.h"
#include "PdfArray.h"
#include "PdfIdentityEncoding.h"
#include "PdfEncodingMapFactory.h"
#include <podofo/auxiliary/StreamDevice.h>

using namespace std;
using namespace PoDoFo;

struct CodeLimits
{
    unsigned char MinCodeSize = numeric_limits<unsigned char>::max();
    unsigned char MaxCodeSize = 0;
};

static void readNextVariantSequence(PdfPostScriptTokenizer& tokenizer, InputStreamDevice& device,
    PdfVariant& variant, const string_view& endSequenceKeyword, bool& endOfSequence);
static uint32_t getCodeFromVariant(const PdfVariant& var, CodeLimits& limits);
static uint32_t getCodeFromVariant(const PdfVariant& var, CodeLimits& limits, unsigned char& codeSize);
static vector<char32_t> handleNameMapping(const PdfName& name);
static vector<char32_t> handleStringMapping(const PdfString& str);
static void handleRangeMapping(PdfCharCodeMap& map,
    uint32_t srcCodeLo, const vector<char32_t>& dstCodeLo,
    unsigned char codeSize, unsigned rangeSize);
static vector<char32_t> handleUtf8String(const string_view& str);
static void pushMapping(PdfCharCodeMap& map, const PdfCharCode& codeUnit, const std::vector<char32_t>& codePoints);
static PdfCharCodeMap parseCMapObject(InputStreamDevice& stream, PdfName& name, PdfCIDSystemInfo& info, int& wMode, PdfEncodingLimits& limits);

PdfCMapEncoding::PdfCMapEncoding(PdfCharCodeMap&& map) :
    PdfEncodingMapBase(std::move(map), PdfEncodingMapType::CMap),
    m_WMode(0),
    m_Limits(GetCharMap().GetLimits()) { }

PdfCMapEncoding::PdfCMapEncoding(PdfCharCodeMap&& map, const PdfName& name, const PdfCIDSystemInfo& info, PdfWModeKind wMode) :
    PdfEncodingMapBase(std::move(map), PdfEncodingMapType::CMap),
    m_Name(name),
    m_CIDSystemInfo(info),
    m_WMode((int)wMode),
    m_Limits(GetCharMap().GetLimits()) { }

PdfCMapEncoding PdfCMapEncoding::Parse(const string_view& filepath)
{
    FileStreamDevice device(filepath);
    return Parse(device);
}

PdfCMapEncoding::PdfCMapEncoding(PdfCharCodeMap&& map, const PdfName& name,
        const PdfCIDSystemInfo& info, int wmode, const PdfEncodingLimits& limits) :
    PdfEncodingMapBase(std::move(map), PdfEncodingMapType::CMap),
    m_Name(name),
    m_CIDSystemInfo(info),
    m_WMode(wmode),
    m_Limits(limits) { }

PdfCMapEncoding PdfCMapEncoding::Parse(InputStreamDevice& device)
{
    PdfEncodingLimits mapLimits;
    int wMode = 0;
    PdfCIDSystemInfo info;
    PdfName name;
    auto map = parseCMapObject(device, name, info, wMode, mapLimits);
    return PdfCMapEncoding(std::move(map), name, info, wMode, mapLimits);
}

unique_ptr<PdfEncodingMap> PdfEncodingMapFactory::ParseCMapEncoding(const PdfObject& cmapObj)
{
    unique_ptr<PdfEncodingMap> ret;
    if (!TryParseCMapEncoding(cmapObj, ret))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Unable to parse a valid CMap");

    return ret;
}

bool PdfEncodingMapFactory::TryParseCMapEncoding(const PdfObject& cmapObj, unique_ptr<PdfEncodingMap>& encoding)
{
    const PdfDictionary* dict;
    const PdfObjectStream* stream;
    if (!cmapObj.TryGetDictionary(dict) || (stream = cmapObj.GetStream()) == nullptr)
    {
        encoding.reset();
        return false;
    }

    charbuff streamBuffer;
    stream->CopyTo(streamBuffer);
    SpanStreamDevice device(streamBuffer);
    PdfEncodingLimits mapLimits;
    PdfName cmapName;
    int wMode = 0;
    PdfCIDSystemInfo info;
    auto map = parseCMapObject(device, cmapName, info, wMode, mapLimits);
    if (map.GetSize() != 0
        && mapLimits.MinCodeSize == mapLimits.MaxCodeSize)
    {
        // Try to determine if the encoding is actually
        // an identity encoding
        auto it = map.begin();
        auto end = map.end();
        unsigned prev = it->first.Code - 1;
        bool identity = true;
        do
        {
            if (it->second.size() > 1
                || it->first.Code != it->second[0]
                || it->first.Code > (prev + 1))
            {
                identity = false;
                break;
            }

            prev = it->first.Code;
            it++;
        } while (it != end);

        if (identity)
        {
            encoding.reset(new PdfIdentityEncoding(
                PdfEncodingMapType::CMap, mapLimits, PdfIdentityOrientation::Unkwnown));
            return true;
        }
    }

    // Properties in the CMap stream dictionary get priority
    wMode = (int)dict->FindKeyAsSafe<int64_t>("WMode", wMode);
    const PdfString* str;
    const PdfName* name;
    const PdfDictionary* cidInfoDict;
    if (dict->TryFindKeyAs("CIDSystemInfo", cidInfoDict))
    {
        if (cidInfoDict->TryFindKeyAs("Registry", str))
            info.Registry = *str;

        if (cidInfoDict->TryFindKeyAs("Ordering", str))
            info.Ordering = *str;

        info.Supplement = (int)cidInfoDict->FindKeyAs<int64_t>("Supplement", 0);
    }
    if (dict->TryFindKeyAs("CMapName", name))
        cmapName = *name;

    encoding.reset(new PdfCMapEncoding(std::move(map), cmapName, info, wMode, mapLimits));

    return true;
}

const PdfEncodingLimits& PdfCMapEncoding::GetLimits() const
{
    return m_Limits;
}

int PdfCMapEncoding::GetWModeRaw() const
{
    return m_WMode;
}

PdfWModeKind PdfCMapEncoding::GetWMode() const
{
    return m_WMode == 1 ? PdfWModeKind::Vertical : PdfWModeKind::Horizontal;
}

bool PdfCMapEncoding::HasLigaturesSupport() const
{
    // CMap encodings may have ligatures
    return true;
}

PdfCharCodeMap parseCMapObject(InputStreamDevice& device, PdfName& cmapName,
    PdfCIDSystemInfo& info, int& wMode, PdfEncodingLimits& mapLimits)
{
    PdfCharCodeMap ret;

    // NOTE: Found a CMap like this
    // /CIDSystemInfo
    // <<
    //   /Registry (Adobe) def
    //   /Ordering (UCS) def
    //   /Supplement 0 def
    // >> def
    // which should be invalid Postscript (any language level). Adobe
    // doesn't crash with such CMap(s), but crashes if such syntax is
    // used elsewhere. Assuming the CMap(s) uses only PS Level 1, which
    // doesn't, support << syntax, is a workaround to read these CMap(s)
    // without crashing.
    PdfPostScriptTokenizer tokenizer(PdfPostScriptLanguageLevel::L1);
    CodeLimits codeLimits;
    deque<unique_ptr<PdfVariant>> tokens;
    const PdfString* str;
    int64_t num;
    const PdfName* name;
    auto var = make_unique<PdfVariant>();
    PdfPostScriptTokenType tokenType;
    string_view token;
    bool endOfSequence;
    vector<char32_t> mappedCodes;
    while (tokenizer.TryReadNext(device, tokenType, token, *var))
    {
        switch (tokenType)
        {
            case PdfPostScriptTokenType::Keyword:
            {
                if (token == "begincodespacerange")
                {
                    while (true)
                    {
                        readNextVariantSequence(tokenizer, device, *var, "endcodespacerange", endOfSequence);
                        if (endOfSequence)
                            break;

                        unsigned char codeSize;
                        (void)getCodeFromVariant(*var, codeLimits, codeSize);
                        tokenizer.ReadNextVariant(device, *var);
                        (void)getCodeFromVariant(*var, codeLimits, codeSize);
                    }
                }
                // NOTE: "bf" in "beginbfrange" stands for Base Font
                // see Adobe tecnichal notes #5014
                else if (token == "beginbfrange")
                {
                    while (true)
                    {
                        readNextVariantSequence(tokenizer, device, *var, "endbfrange", endOfSequence);
                        if (endOfSequence)
                            break;

                        unsigned char codeSize;
                        uint32_t srcCodeLo = getCodeFromVariant(*var, codeLimits, codeSize);
                        tokenizer.ReadNextVariant(device, *var);
                        uint32_t srcCodeHi = getCodeFromVariant(*var, codeLimits);
                        unsigned rangeSize = srcCodeHi - srcCodeLo + 1;
                        tokenizer.ReadNextVariant(device, *var);
                        if (var->IsArray())
                        {
                            PdfArray& arr = var->GetArray();
                            for (unsigned i = 0; i < rangeSize; i++)
                            {
                                auto& dst = arr[i];
                                if (dst.TryGetString(str) && str->IsHex()) // pp. 475 PdfReference 1.7
                                    pushMapping(ret, { srcCodeLo + i, codeSize }, handleStringMapping(*str));
                                else if (dst.IsName()) // Not mentioned in tecnincal document #5014 but seems safe
                                    pushMapping(ret, { srcCodeLo + i, codeSize }, handleNameMapping(dst.GetName()));
                                else
                                    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "beginbfrange: expected string or name inside array");
                            }
                        }
                        else if (var->TryGetString(str) && str->IsHex())
                        {
                            // pp. 474 PdfReference 1.7
                            auto dstCodeLo = handleStringMapping(*str);
                            if (dstCodeLo.size() != 0)
                                handleRangeMapping(ret, srcCodeLo, dstCodeLo, codeSize, rangeSize);
                        }
                        else if (var->IsName())
                        {
                            // As found in tecnincal document #5014
                            handleRangeMapping(ret, srcCodeLo, handleNameMapping(var->GetName()), codeSize, rangeSize);
                        }
                        else
                            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "beginbfrange: expected array, string or array");
                    }
                }
                // NOTE: "bf" in "beginbfchar" stands for Base Font
                // see Adobe tecnichal notes #5014
                else if (token == "beginbfchar")
                {
                    while (true)
                    {
                        readNextVariantSequence(tokenizer, device, *var, "endbfchar", endOfSequence);
                        if (endOfSequence)
                            break;

                        unsigned char codeSize;
                        uint32_t srcCode = getCodeFromVariant(*var, codeLimits, codeSize);
                        tokenizer.ReadNextVariant(device, *var);
                        if (var->IsNumber())
                        {
                            char32_t dstCode = (char32_t)getCodeFromVariant(*var, codeLimits);
                            mappedCodes.clear();
                            mappedCodes.push_back(dstCode);
                        }
                        else if (var->TryGetString(str) && str->IsHex())
                        {
                            // pp. 474 PdfReference 1.7
                            mappedCodes = handleStringMapping(*str);
                        }
                        else if (var->IsName())
                        {
                            // As found in tecnincal document #5014
                            mappedCodes = handleNameMapping(var->GetName());
                        }
                        else
                            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "beginbfchar: expected number or name");

                        pushMapping(ret, { srcCode, codeSize }, mappedCodes);
                    }
                }
                else if (token == "begincidrange")
                {
                    while (true)
                    {
                        readNextVariantSequence(tokenizer, device, *var, "endcidrange", endOfSequence);
                        if (endOfSequence)
                            break;

                        unsigned char codeSize;
                        uint32_t srcCodeLo = getCodeFromVariant(*var, codeLimits, codeSize);
                        tokenizer.ReadNextVariant(device, *var);
                        uint32_t srcCodeHi = getCodeFromVariant(*var, codeLimits);
                        tokenizer.ReadNextVariant(device, *var);
                        char32_t dstCIDLo = (char32_t)getCodeFromVariant(*var, codeLimits);

                        unsigned rangeSize = srcCodeHi - srcCodeLo + 1;
                        for (unsigned i = 0; i < rangeSize; i++)
                        {
                            char32_t newbackchar = dstCIDLo + i;
                            mappedCodes.clear();
                            mappedCodes.push_back(newbackchar);
                            pushMapping(ret, { srcCodeLo + i, codeSize }, mappedCodes);
                        }
                    }
                }
                else if (token == "begincidchar")
                {
                    if (tokens.size() != 1)
                        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidStream, "CMap missing object number before begincidchar");

                    int charCount = (int)tokens.front()->GetNumber();

                    for (int i = 0; i < charCount; i++)
                    {
                        tokenizer.TryReadNext(device, tokenType, token, *var);
                        unsigned char codeSize;
                        uint32_t srcCode = getCodeFromVariant(*var, codeLimits, codeSize);
                        tokenizer.ReadNextVariant(device, *var);
                        char32_t dstCode = (char32_t)getCodeFromVariant(*var, codeLimits);
                        mappedCodes.clear();
                        mappedCodes.push_back(dstCode);
                        pushMapping(ret, { srcCode, codeSize }, mappedCodes);
                    }
                }

                tokens.clear();
                break;
            }
            case PdfPostScriptTokenType::Variant:
            {
                if (var->TryGetName(name))
                {
                    tokens.push_front(std::move(var));
                    var.reset(new PdfVariant());

                    if (tokenizer.TryReadNextVariant(device, *var))
                    {
                        if (*name == "CMapName" && var->TryGetName(name))
                            cmapName = *name;
                        if (*name == "Registry" && var->TryGetString(str))
                            info.Registry = *str;
                        else if (*name == "Ordering" && var->TryGetString(str))
                            info.Ordering = *str;
                        else if (*name == "Supplement" && var->TryGetNumber(num))
                            info.Supplement = (int)num;
                        else if (*name == "WMode" && var->TryGetNumber(num))
                            wMode = (int)num;

                        tokens.push_front(std::move(var));
                        var.reset(new PdfVariant());
                    }
                }
                else
                {
                    tokens.push_front(std::move(var));
                    var.reset(new PdfVariant());
                }

                break;
            }
            default:
                PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
        }
    }

    // NOTE: In some cases the encoding is degenerate and has no code
    // entries at all, but the CMap may still encode the code size
    // in "begincodespacerange"
    mapLimits = ret.GetLimits();
    if (codeLimits.MinCodeSize < mapLimits.MinCodeSize)
        mapLimits.MinCodeSize = codeLimits.MinCodeSize;
    if (codeLimits.MaxCodeSize > mapLimits.MaxCodeSize)
        mapLimits.MaxCodeSize = codeLimits.MaxCodeSize;

    return ret;
}

// Base Font 3 type CMap interprets strings as found in
// beginbfchar and beginbfrange as UTF-16BE, see PdfReference 1.7
// page 472. NOTE: Before UTF-16BE there was UCS-2 but UTF-16
// is backward compatible with UCS-2
vector<char32_t> handleStringMapping(const PdfString& str)
{
    string utf8;
    utls::ReadUtf16BEString(str.GetRawData(), utf8);
    return handleUtf8String(utf8);
}

// Handle a range in begingbfrage "srcCodeLo srcCodeHi dstCodeLo" clause
void handleRangeMapping(PdfCharCodeMap& map,
    uint32_t srcCodeLo, const vector<char32_t>& dstCodeLo,
    unsigned char codeSize, unsigned rangeSize)
{
    auto it = dstCodeLo.begin();
    auto end = dstCodeLo.end();
    PODOFO_INVARIANT(it != end);
    char32_t back = dstCodeLo.back();
    vector<char32_t> newdstbase;
    // Compute a destination string that has all code points except the last one
    if (dstCodeLo.size() != 1)
    {
        do
        {
            newdstbase.push_back(*it);
            it++;
        } while (it != end);
    }

    // Compute new destination string with last character/code point incremented by one
    vector<char32_t> newdst;
    for (unsigned i = 0; i < rangeSize; i++)
    {
        newdst = newdstbase;
        char32_t newbackchar = back + i;
        newdst.push_back(newbackchar);
        pushMapping(map, { srcCodeLo + i, codeSize }, newdst);
    }
}

// codeSize is the number of the octets in the string or the minimum number
// of bytes to represent the number, example <cd> -> 1, <00cd> -> 2
static uint32_t getCodeFromVariant(const PdfVariant& var, unsigned char& codeSize)
{
    if (var.IsNumber())
    {
        int64_t num = var.GetNumber();
        uint32_t ret = (uint32_t)num;
        if (num == 0)
        {
            codeSize = 1;
        }
        else
        {
            codeSize = 0;
            do
            {
                codeSize++;
                num >>= 8;
            } while (num != 0);
        }

        return ret;
    }

    const PdfString& str = var.GetString();
    uint32_t ret = 0;
    auto rawstr = str.GetRawData();
    unsigned len = (unsigned)rawstr.length();
    for (unsigned i = 0; i < len; i++)
    {
        uint8_t code = (uint8_t)rawstr[len - 1 - i];
        ret += code << i * 8;
    }

    codeSize = (unsigned char)len;
    return ret;
}

void pushMapping(PdfCharCodeMap& map, const PdfCharCode& codeUnit, const vector<char32_t>& codePoints)
{
    if (codePoints.size() == 0)
        return;

    map.PushMapping(codeUnit, codePoints);
}

uint32_t getCodeFromVariant(const PdfVariant& var, CodeLimits& limits, unsigned char& codeSize)
{
    uint32_t ret = getCodeFromVariant(var, codeSize);
    if (codeSize < limits.MinCodeSize)
        limits.MinCodeSize = codeSize;
    if (codeSize > limits.MaxCodeSize)
        limits.MaxCodeSize = codeSize;

    return ret;
}

uint32_t getCodeFromVariant(const PdfVariant& var, CodeLimits& limits)
{
    unsigned char codeSize;
    return getCodeFromVariant(var, limits, codeSize);
}

vector<char32_t> handleNameMapping(const PdfName& name)
{
    return handleUtf8String(name.GetString());
}

vector<char32_t> handleUtf8String(const string_view& str)
{
    vector<char32_t> ret;
    auto it = str.begin();
    auto end = str.end();
    while (it != end)
        ret.push_back(utf8::next(it, end));

    return ret;
}

// Read variant from a sequence, unless it's the end of it
// We found Pdf(s) that have mismatching sequence length and
// end of sequence marker, and Acrobat preflight treats them as valid,
// so we must determine end of sequence only on the end of
// sequence keyword
void readNextVariantSequence(PdfPostScriptTokenizer& tokenizer, InputStreamDevice& device,
    PdfVariant& variant, const string_view& endSequenceKeyword, bool& endOfSequence)
{
    PdfPostScriptTokenType tokenType;
    string_view token;

    if (!tokenizer.TryReadNext(device, tokenType, token, variant))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidStream, "CMap unable to read a token");

    switch (tokenType)
    {
        case PdfPostScriptTokenType::Keyword:
        {
            if (token == endSequenceKeyword)
            {
                endOfSequence = true;
                break;
            }

            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidStream,
                "CMap unable to read an end of sequence keyword {}", endSequenceKeyword);
        }
        case PdfPostScriptTokenType::Variant:
        {
            endOfSequence = false;
            break;
        }
        default:
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Unexpected token type");
        }
    }
}

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
static uint32_t getCodeFromVariant(const PdfVariant& var);
static uint32_t getCodeFromVariant(const PdfVariant& var, CodeLimits& limits);
static uint32_t getCodeFromVariant(const PdfVariant& var, CodeLimits& limits, unsigned char& codeSize);
static void handleNameMapping(const PdfName& name, vector<char32_t>& codePoints);
static void handleStringMapping(const PdfString& str, vector<char32_t>& codePoints);
static void pushRangeMapping(PdfCharCodeMap& map, uint32_t srcCodeLo, unsigned rangeSize,
    const cspan<char32_t>& dstCodeLo, unsigned char codeSize);
static void handleUtf8String(const string_view& str, vector<char32_t>& copdePoints);
static void pushMapping(PdfCharCodeMap& map, uint32_t srcCode, unsigned char codeSize, const std::vector<char32_t>& codePoints);
static PdfCharCodeMap parseCMapObject(InputStreamDevice& stream, PdfName& name, PdfCIDSystemInfo& info, int& wMode, PdfEncodingLimits& limits);

PdfCMapEncoding::PdfCMapEncoding(PdfCharCodeMap&& map) :
    PdfEncodingMapBase(std::move(map), PdfEncodingMapType::CMap),
    m_isPredefined(false),
    m_WMode(0),
    m_Limits(GetCharMap().GetLimits()) { }

PdfCMapEncoding::PdfCMapEncoding(PdfCharCodeMap&& map, const PdfName& name, const PdfCIDSystemInfo& info, PdfWModeKind wMode) :
    PdfEncodingMapBase(std::move(map), PdfEncodingMapType::CMap),
    m_isPredefined(false),
    m_Name(name),
    m_CIDSystemInfo(info),
    m_WMode((int)wMode),
    m_Limits(GetCharMap().GetLimits()) { }

PdfCMapEncoding PdfCMapEncoding::Parse(const string_view& filepath)
{
    FileStreamDevice device(filepath);
    return Parse(device);
}

PdfCMapEncoding::PdfCMapEncoding(PdfCharCodeMap&& map, bool isPredefined, const PdfName& name,
        const PdfCIDSystemInfo& info, int wmode, const PdfEncodingLimits& limits) :
    PdfEncodingMapBase(std::move(map), PdfEncodingMapType::CMap),
    m_isPredefined(isPredefined),
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
    return PdfCMapEncoding(std::move(map), false, name, info, wMode, mapLimits);
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
    if (!map.IsEmpty() != 0 && mapLimits.MinCodeSize == mapLimits.MaxCodeSize && map.IsTrivialIdentity())
    {
        encoding.reset(new PdfIdentityEncoding(
            PdfEncodingMapType::CMap, mapLimits, PdfIdentityOrientation::Unkwnown));
        return true;
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

        info.Supplement = (int)cidInfoDict->FindKeyAsSafe<int64_t>("Supplement", 0);
    }
    if (dict->TryFindKeyAs("CMapName", name))
        cmapName = *name;

    encoding.reset(new PdfCMapEncoding(std::move(map), false, cmapName, info, wMode, mapLimits));

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

PdfPredefinedEncodingType PdfCMapEncoding::GetPredefinedEncodingType() const
{
    return m_isPredefined ? PdfPredefinedEncodingType::PredefinedCMap : PdfPredefinedEncodingType::Indeterminate;
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
                        tokenizer.ReadNextVariant(device, *var);
                        if (srcCodeHi < srcCodeLo)
                        {
                            PoDoFo::LogMessage(PdfLogSeverity::Warning, "begincidrange: Found range with srcCodeHi {} < srcCodeLo {}", srcCodeHi, srcCodeLo);
                            continue;
                        }

                        unsigned rangeSize = srcCodeHi - srcCodeLo + 1;
                        if (var->IsArray())
                        {
                            PdfArray& arr = var->GetArray();
                            for (unsigned i = 0; i < rangeSize; i++)
                            {
                                auto& dst = arr[i];
                                if (dst.TryGetString(str) && str->IsHex()) // pp. 475 PdfReference 1.7
                                {
                                    handleStringMapping(*str, mappedCodes);
                                    pushMapping(ret, srcCodeLo + i, codeSize, mappedCodes);
                                }
                                else if (dst.IsName()) // Not mentioned in tecnincal document #5014 but seems safe
                                {
                                    handleNameMapping(dst.GetName(), mappedCodes);
                                    pushMapping(ret, srcCodeLo + i, codeSize, mappedCodes);
                                }
                                else
                                {
                                    PoDoFo::LogMessage(PdfLogSeverity::Warning, "beginbfrange: expected string or name inside array");
                                    break;
                                }
                            }
                        }
                        else if (var->TryGetString(str) && str->IsHex())
                        {
                            // pp. 474 PdfReference 1.7
                            handleStringMapping(*str, mappedCodes);
                            pushRangeMapping(ret, srcCodeLo, rangeSize, mappedCodes, codeSize);
                        }
                        else if (var->IsName())
                        {
                            // As found in tecnincal document #5014
                            handleNameMapping(var->GetName(), mappedCodes);
                            pushRangeMapping(ret, srcCodeLo, rangeSize, mappedCodes, codeSize);
                        }
                        else
                        {
                            PoDoFo::LogMessage(PdfLogSeverity::Warning, "beginbfrange: expected array, string or array");
                        }
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
                            char32_t dstCode = (char32_t)getCodeFromVariant(*var);
                            mappedCodes.clear();
                            mappedCodes.push_back(dstCode);
                        }
                        else if (var->TryGetString(str) && str->IsHex())
                        {
                            // pp. 474 PdfReference 1.7
                            handleStringMapping(*str, mappedCodes);
                        }
                        else if (var->IsName())
                        {
                            // As found in tecnincal document #5014
                            handleNameMapping(var->GetName(), mappedCodes);
                        }
                        else
                        {
                            PoDoFo::LogMessage(PdfLogSeverity::Warning, "beginbfchar: expected number or name");
                            continue;
                        }

                        pushMapping(ret, srcCode, codeSize, mappedCodes);
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
                        char32_t dstCIDLo = (char32_t)getCodeFromVariant(*var);
                        if (srcCodeHi < srcCodeLo)
                        {
                            PoDoFo::LogMessage(PdfLogSeverity::Warning, "begincidrange: Found range with srcCodeHi {} < srcCodeLo {}", srcCodeHi, srcCodeLo);
                            continue;
                        }
                        unsigned rangeSize = srcCodeHi - srcCodeLo + 1;
                        pushRangeMapping(ret, srcCodeLo, rangeSize, { &dstCIDLo, 1 }, codeSize);
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
                        char32_t dstCode = (char32_t)getCodeFromVariant(*var);
                        mappedCodes.clear();
                        mappedCodes.push_back(dstCode);
                        pushMapping(ret, srcCode, codeSize, mappedCodes);
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
                        if (*name == "CMapName")
                        {
                            // /CMapName may be a string as well (https://github.com/podofo/podofo/issues/249)
                            // NOTE: String charset in theory may be  wider than names,
                            // as a fail-safe strategy let's create the name with an
                            // unevaluated raw buffer
                            if (var->TryGetName(name))
                                cmapName = *name;
                            else if (var->TryGetString(str))
                                cmapName = PdfName(charbuff(str->GetString()));
                        }
                        else if (*name == "Registry" && var->TryGetString(str))
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
void handleStringMapping(const PdfString& str, vector<char32_t>& codePoints)
{
    string utf8;
    utls::ReadUtf16BEString(str.GetRawData(), utf8);
    return handleUtf8String(utf8, codePoints);
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

void pushMapping(PdfCharCodeMap& map, uint32_t srcCode, unsigned char codeSize, const vector<char32_t>& codePoints)
{
    map.PushMapping({ srcCode, codeSize }, codePoints);
}

// Handle a range in begingbfrage "srcCodeLo srcCodeHi dstCodeLo" clause
void pushRangeMapping(PdfCharCodeMap& map, uint32_t srcCodeLo, unsigned rangeSize,
    const cspan<char32_t>& dstCodeLo, unsigned char codeSize)
{
    map.PushRange({ srcCodeLo, codeSize }, rangeSize, dstCodeLo);
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

uint32_t getCodeFromVariant(const PdfVariant& var)
{
    unsigned char codeSize;
    return getCodeFromVariant(var, codeSize);
}

void handleNameMapping(const PdfName& name, vector<char32_t>& copdePoints)
{
    return handleUtf8String(name.GetString(), copdePoints);
}

void handleUtf8String(const string_view& str, vector<char32_t>& codePoints)
{
    codePoints.clear();
    auto it = str.begin();
    auto end = str.end();
    while (it != end)
        codePoints.push_back(utf8::next(it, end));
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

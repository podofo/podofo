/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include "PdfDeclarationsPrivate.h"
#include "PdfEncodingPrivate.h"

#include <utf8cpp/utf8.h>

#include <podofo/main/PdfCharCodeMap.h>

using namespace std;
using namespace PoDoFo;

static const unordered_map<char32_t, char>& getUTF8ToPdfEncodingMap();
static CodePointMapNode* findOrAddNode(CodePointMapNode*& node, codepoint codePoint);
static const CodePointMapNode* findNode(const CodePointMapNode* node, codepoint codePoint);

static const char32_t s_cEncoding[] = {
    0x0000,
    0x0001,
    0x0002,
    0x0003,
    0x0004,
    0x0005,
    0x0006,
    0x0007,
    0x0008,
    0x0009,
    0x000A,
    0x000B,
    0x000C,
    0x000D,
    0x000E,
    0x000F,
    0x0010,
    0x0011,
    0x0012,
    0x0013,
    0x0014,
    0x0015,
    0x0017,
    0x0017,
    0x02D8,
    0x02C7, // dec 25
    0x02C6,
    0x02D9,
    0x02DD,
    0x02DB,
    0x02DA,
    0x02DC,
    0x0020,
    0x0021,
    0x0022,
    0x0023,
    0x0024,
    0x0025,
    0x0026,
    0x0027,
    0x0028,
    0x0029,
    0x002A,
    0x002B,
    0x002C,
    0x002D,
    0x002E,
    0x002F,
    0x0030,
    0x0031,
    0x0032,
    0x0033,
    0x0034,
    0x0035,
    0x0036,
    0x0037,
    0x0038,
    0x0039, // dec 57
    0x003A,
    0x003B,
    0x003C,
    0x003D,
    0x003E,
    0x003F,
    0x0040,
    0x0041,
    0x0042,
    0x0043,
    0x0044,
    0x0045,
    0x0046,
    0x0047,
    0x0048,
    0x0049,
    0x004A,
    0x004B,
    0x004C,
    0x004D,
    0x004E,
    0x004F,
    0x0050,
    0x0051,
    0x0052,
    0x0053,
    0x0054,
    0x0055,
    0x0056,
    0x0057,
    0x0058,
    0x0059, // 89
    0x005A,
    0x005B,
    0x005C,
    0x005D,
    0x005E,
    0x005F,
    0x0060,
    0x0061,
    0x0062,
    0x0063,
    0x0064,
    0x0065,
    0x0066,
    0x0067,
    0x0068,
    0x0069,
    0x006A,
    0x006B,
    0x006C,
    0x006D,
    0x006E,
    0x006F,
    0x0070,
    0x0071,
    0x0072,
    0x0073,
    0x0074,
    0x0075,
    0x0076,
    0x0077,
    0x0078,
    0x0079, //121
    0x007A,
    0x007B,
    0x007C,
    0x007D,
    0x007E,
    0x0000, // Undefined
    0x2022,
    0x2020,
    0x2021,
    0x2026,
    0x2014,
    0x2013,
    0x0192,
    0x2044,
    0x2039,
    0x203A,
    0x2212,
    0x2030,
    0x201E,
    0x201C,
    0x201D,
    0x2018,
    0x2019,
    0x201A,
    0x2122,
    0xFB01, // dec147
    0xFB02,
    0x0141,
    0x0152,
    0x0160,
    0x0178,
    0x017D,
    0x0131,
    0x0142,
    0x0153,
    0x0161,
    0x017E,
    0x0000, // Undefined
    0x20AC, // Euro
    0x00A1,
    0x00A2,
    0x00A3,
    0x00A4,
    0x00A5,
    0x00A6,
    0x00A7,
    0x00A8,
    0x00A9,
    0x00AA,
    0x00AB,
    0x00AC,
    0x0000, // Undefined
    0x00AE,
    0x00AF,
    0x00B0,
    0x00B1,
    0x00B2,
    0x00B3,
    0x00B4,
    0x00B5,
    0x00B6,
    0x00B7,
    0x00B8,
    0x00B9,
    0x00BA,
    0x00BB,
    0x00BC,
    0x00BD,
    0x00BE,
    0x00BF,
    0x00C0,
    0x00C1,
    0x00C2,
    0x00C3,
    0x00C4,
    0x00C5,
    0x00C6,
    0x00C7,
    0x00C8,
    0x00C9,
    0x00CA,
    0x00CB,
    0x00CC,
    0x00CD,
    0x00CE,
    0x00CF,
    0x00D0,
    0x00D1,
    0x00D2,
    0x00D3,
    0x00D4,
    0x00D5,
    0x00D6,
    0x00D7,
    0x00D8,
    0x00D9,
    0x00DA,
    0x00DB,
    0x00DC,
    0x00DD,
    0x00DE,
    0x00DF,
    0x00E0,
    0x00E1,
    0x00E2,
    0x00E3,
    0x00E4,
    0x00E5,
    0x00E6,
    0x00E7,
    0x00E8,
    0x00E9,
    0x00EA,
    0x00EB,
    0x00EC,
    0x00ED,
    0x00EE,
    0x00EF,
    0x00F0,
    0x00F1,
    0x00F2,
    0x00F3,
    0x00F4,
    0x00F5,
    0x00F6,
    0x00F7,
    0x00F8,
    0x00F9,
    0x00FA,
    0x00FB,
    0x00FC,
    0x00FD,
    0x00FE,
    0x00FF
};

bool PoDoFo::CheckValidUTF8ToPdfDocEcondingChars(const string_view& view, bool& isAsciiEqual)
{
    auto& map = getUTF8ToPdfEncodingMap();

    isAsciiEqual = true;
    char32_t cp = 0;
    auto it = view.begin();
    auto end = view.end();
    while (it != end)
    {
        cp = utf8::next(it, end);
        unordered_map<char32_t, char>::const_iterator found;
        if (cp > 0xFFFF || (found = map.find(cp)) == map.end())
        {
            // Code point out of range or not present in the map
            isAsciiEqual = false;
            return false;
        }

        if (cp >= 0x80 || found->second != (char)cp) // >= 128 or different mapped code
        {
            // The utf-8 char is not coincident to PdfDocEncoding representation
            isAsciiEqual = false;
        }
    }

    return true;
}

bool PoDoFo::IsPdfDocEncodingCoincidentToUTF8(string_view view)
{
    for (size_t i = 0; i < view.length(); i++)
    {
        unsigned char ch = view[i];
        if (ch != s_cEncoding[i])
            return false;
    }

    return true;
}

string PoDoFo::ConvertUTF8ToPdfDocEncoding(const string_view& view)
{
    string ret;
    if (!TryConvertUTF8ToPdfDocEncoding(view, ret))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncoding, "Unsupported chars in converting utf-8 string to PdfDocEncoding");

    return ret;
}

bool PoDoFo::TryConvertUTF8ToPdfDocEncoding(const string_view& view, string& pdfdocencstr)
{
    auto& map = getUTF8ToPdfEncodingMap();

    pdfdocencstr.clear();

    char32_t cp = 0;
    auto it = view.begin();
    auto end = view.end();
    while (it != end)
    {
        cp = utf8::next(it, end);
        unordered_map<char32_t, char>::const_iterator found;
        if (cp > 0xFFFF || (found = map.find(cp)) == map.end())
        {
            // Code point out of range or not present in the map
            pdfdocencstr.clear();
            return false;
        }

        pdfdocencstr.push_back(found->second);
    }

    return true;
}

string PoDoFo::ConvertPdfDocEncodingToUTF8(const string_view& view, bool& isAsciiEqual)
{
    string ret;
    ConvertPdfDocEncodingToUTF8(view, ret, isAsciiEqual);
    return ret;
}

void PoDoFo::ConvertPdfDocEncodingToUTF8(string_view view, string& u8str, bool& isAsciiEqual)
{
    u8str.clear();
    isAsciiEqual = true;
    for (size_t i = 0; i < view.length(); i++)
    {
        unsigned char ch = (unsigned char)view[i];
        char32_t mappedCode = s_cEncoding[ch];
        if (mappedCode >= 0x80 || ch != (char)mappedCode) // >= 128 or different mapped code
            isAsciiEqual = false;

        utf8::append(mappedCode, u8str);
    }
}

void PoDoFo::AppendCIDMappingEntriesTo(OutputStream& stream, const PdfCharCodeMap& charMap, charbuff& temp)
{
    auto& mappings = charMap.GetMappings();
    if (mappings.size() != 0)
    {
        // Sort the keys, so the output will be deterministic
        set<PdfCharCode> ordered;
        std::for_each(mappings.begin(), mappings.end(), [&ordered](auto& pair) {
            ordered.insert(pair.first);
            });

        utls::FormatTo(temp, mappings.size());
        stream.Write(temp);
        stream.Write(" begincidchar\n");
        for (auto& code : ordered)
        {
            // We assume the cid to be in the single element
            PoDoFo::WriteCIDMapping(stream, code, *mappings.at(code), temp);
        }
        stream.Write("endcidchar\n");
    }

    auto& ranges = charMap.GetRanges();
    if (ranges.size() != 0)
    {
        utls::FormatTo(temp, ranges.size());
        stream.Write(temp);
        stream.Write(" begincidrange\n");
        for (auto& range : ranges)
        {
            // We assume the cid to be in the single element
            PoDoFo::WriteCIDRange(stream, range.SrcCodeLo, range.GetSrcCodeHi(),
                *range.DstCodeLo, temp);
        }
        stream.Write("endcidrange\n");
    }
}

void PoDoFo::AppendCodeSpaceRangeTo(OutputStream& stream, const PdfCharCodeMap& charMap, charbuff& temp)
{
    // Iterate mappings to create ranges of different code sizes
    auto ranges = charMap.GetCodeSpaceRanges();
    stream.Write(std::to_string(ranges.size()));
    stream.Write(" begincodespacerange\n");

    bool first = true;
    for (auto& range : ranges)
    {
        if (first)
            first = false;
        else
            stream.Write("\n");

        range.GetSrcCodeLo().WriteHexTo(temp);
        stream.Write(temp);
        range.GetSrcCodeHi().WriteHexTo(temp);
        stream.Write(temp);
    }

    stream.Write("\nendcodespacerange\n");
}

void PoDoFo::AppendToUnicodeEntriesTo(OutputStream& stream, const PdfCharCodeMap& charMap, charbuff& temp)
{
    u16string u16temp;

    auto& mappings = charMap.GetMappings();
    if (mappings.size() != 0)
    {
        // Sort the keys, so the output will be deterministic
        set<PdfCharCode> ordered;
        std::for_each(mappings.begin(), mappings.end(), [&ordered](auto& pair) {
            ordered.insert(pair.first);
            });

        utls::FormatTo(temp, mappings.size());
        stream.Write(temp);
        stream.Write(" beginbfchar\n");

        for (auto& code : ordered)
        {
            code.WriteHexTo(temp);
            stream.Write(temp);
            stream.Write(" ");
            PoDoFo::AppendUTF16CodeTo(stream, mappings.at(code), u16temp);
            stream.Write("\n");
        }
        stream.Write("endbfchar\n");
    }

    auto& ranges = charMap.GetRanges();
    if (ranges.size() != 0)
    {
        utls::FormatTo(temp, ranges.size());
        stream.Write(temp);
        stream.Write(" beginbfrange\n");
        for (auto& range : ranges)
        {
            range.SrcCodeLo.WriteHexTo(temp);
            stream.Write(temp);
            range.GetSrcCodeHi().WriteHexTo(temp);
            stream.Write(temp);
            stream.Write(" ");
            PoDoFo::AppendUTF16CodeTo(stream, range.DstCodeLo, u16temp);
            stream.Write("\n");
        }
        stream.Write("endbfrange\n");
    }
}

bool PoDoFo::TryGetCodeReverseMap(const CodePointMapNode* node, const codepointview& codePoints, PdfCharCode& codeUnit)
{
    auto it = codePoints.begin();
    auto end = codePoints.end();
    if (it == end)
        goto NotFound;

    while (true)
    {
        // All the sequence must match
        node = findNode(node, *it);
        if (node == nullptr)
            goto NotFound;

        it++;
        if (it == end)
            break;

        node = node->Ligatures;
    }

    if (node->CodeUnit.CodeSpaceSize == 0)
    {
        // Undefined char code
        goto NotFound;
    }
    else
    {
        codeUnit = node->CodeUnit;
        return true;
    }

NotFound:
    codeUnit = { };
    return false;
}

bool PoDoFo::TryGetCodeReverseMap(const CodePointMapNode* node, codepoint codePoint, PdfCharCode& code)
{
    node = findNode(node, codePoint);
    if (node == nullptr)
    {
        code = { };
        return false;
    }

    code = node->CodeUnit;
    return true;
}

bool PoDoFo::TryGetCodeReverseMap(const CodePointMapNode* node,
    string_view::iterator& it, const string_view::iterator& end, PdfCharCode& codeUnit)
{
    PODOFO_ASSERT(it != end);
    string_view::iterator curr;
    codepoint codePoint = (codepoint)utf8::next(it, end);
    node = findNode(node, codePoint);
    if (node == nullptr)
        goto NotFound;

    if (it != end)
    {
        // Try to find ligatures, save a temporary iterator
        // in case the search in unsuccessful
        curr = it;
        if (PoDoFo::TryGetCodeReverseMap(node->Ligatures, curr, end, codeUnit))
        {
            it = curr;
            return true;
        }
    }

    if (node->CodeUnit.CodeSpaceSize == 0)
    {
        // Undefined char code
        goto NotFound;
    }
    else
    {
        codeUnit = node->CodeUnit;
        return true;
    }

NotFound:
    codeUnit = { };
    return false;
}

void PoDoFo::PushMappingReverseMap(CodePointMapNode*& root, const codepointview& codePoints, const PdfCharCode& codeUnit)
{
    CodePointMapNode** curr = &root;
    CodePointMapNode* found;                            // Last found node
    auto it = codePoints.begin();
    auto end = codePoints.end();
    PODOFO_ASSERT(it != end);
    while (true)
    {
        found = findOrAddNode(*curr, *it);
        it++;
        if (it == end)
            break;

        // We add subsequent codepoints to ligatures
        curr = &found->Ligatures;
    }

    // Finally set the char code on the last found/added node
    found->CodeUnit = codeUnit;
}

void PoDoFo::DeleteNodeReverseMap(CodePointMapNode* node)
{
    if (node == nullptr)
        return;

    DeleteNodeReverseMap(node->Ligatures);
    DeleteNodeReverseMap(node->Left);
    DeleteNodeReverseMap(node->Right);
    delete node;
}

void PoDoFo::AppendUTF16CodeTo(OutputStream& stream, char32_t codePoint, u16string& u16tmp)
{
    return AppendUTF16CodeTo(stream, unicodeview(&codePoint, 1), u16tmp);
}

void PoDoFo::AppendUTF16CodeTo(OutputStream& stream, const unicodeview& codePoints, u16string& u16tmp)
{
    char hexbuf[2];

    stream.Write("<");
    bool first = true;
    for (unsigned i = 0; i < codePoints.size(); i++)
    {
        if (first)
            first = false;
        else
            stream.Write(" "); // Separate each character in the ligatures

        char32_t cp = codePoints[i];
        utls::WriteUtf16BETo(u16tmp, cp);

        auto data = (const char*)u16tmp.data();
        size_t size = u16tmp.size() * sizeof(char16_t);
        for (unsigned l = 0; l < size; l++)
        {
            // Append hex codes of the converted utf16 string
            utls::WriteCharHexTo(hexbuf, data[l]);
            stream.Write(hexbuf, std::size(hexbuf));
        }
    }
    stream.Write(">");
}

void PoDoFo::WriteCIDMapping(OutputStream& stream, const PdfCharCode& unit, unsigned cid, charbuff& temp)
{
    unit.WriteHexTo(temp);
    stream.Write(temp);
    stream.Write(" ");
    utls::FormatTo(temp, cid);
    stream.Write(temp);
    stream.Write("\n");
}

void PoDoFo::WriteCIDRange(OutputStream& stream, const PdfCharCode& srcCodeLo, const PdfCharCode& srcCodeHi, unsigned dstCidLo, charbuff& temp)
{
    srcCodeLo.WriteHexTo(temp);
    stream.Write(temp);
    srcCodeHi.WriteHexTo(temp);
    stream.Write(temp);
    stream.Write(" ");
    utls::FormatTo(temp, dstCidLo);
    stream.Write(temp);
    stream.Write("\n");
}

CodePointMapNode* findOrAddNode(CodePointMapNode*& node, codepoint codePoint)
{
    if (node == nullptr)
    {
        node = new CodePointMapNode{ };
        node->CodePoint = codePoint;
        return node;
    }

    if (node->CodePoint == codePoint)
        return node;
    else if (node->CodePoint > codePoint)
        return findOrAddNode(node->Left, codePoint);
    else
        return findOrAddNode(node->Right, codePoint);
}

const CodePointMapNode* findNode(const CodePointMapNode* node, codepoint codePoint)
{
    if (node == nullptr)
        return nullptr;

    if (node->CodePoint == codePoint)
        return node;
    else if (node->CodePoint > codePoint)
        return findNode(node->Left, codePoint);
    else
        return findNode(node->Right, codePoint);
}

const unordered_map<char32_t, char>& getUTF8ToPdfEncodingMap()
{
    struct Map : public unordered_map<char32_t, char>
    {
        Map()
        {
            // Prepare UTF8 to PdfDocEncoding map
            for (int i = 0; i < 256; i++)
            {
                char32_t mapped = s_cEncoding[i];
                if (mapped == 0x0000 && i != 0)   // Undefined, skip this
                    continue;

                (*this)[mapped] = (char)i;
            }
        }
    };

    static Map map;
    return map;
}

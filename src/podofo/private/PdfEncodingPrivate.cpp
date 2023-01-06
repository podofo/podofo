/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PdfDeclarationsPrivate.h"
#include "PdfEncodingPrivate.h"

#include <utfcpp/utf8.h>

using namespace std;
using namespace PoDoFo;

static const unordered_map<char32_t, char>& getUTF8ToPdfEncodingMap();

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

bool PoDoFo::IsPdfDocEncodingCoincidentToUTF8(const string_view& view)
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
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Unsupported chars in converting utf-8 string to PdfDocEncoding");

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

void PoDoFo::ConvertPdfDocEncodingToUTF8(const string_view& view, string& u8str, bool& isAsciiEqual)
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

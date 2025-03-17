/**
 * SPDX-FileCopyrightText: (C) 2025 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "SASLprep.h"

#include <cassert>
#include <vector>

#include <utf8proc/utf8proc.h>
#include <utf8cpp/utf8.h>

#include "SASLprepPrivate.h"

using namespace std;
using namespace sprep;

namespace
{
    struct Categories
    {
        Categories();

        CharCategoryMap map;
    };
}

static bool tryNormalizeNFKC(const vector<char32_t>& codePoints, vector<char32_t>& normalized);

bool sprep::TrySASLprep(const string_view& str, string& prepd)
{
    // Ported from https://github.com/reklatsmasters/saslprep/blob/master/index.js

    prepd.clear();
    if (str.length() == 0)
        return true;

    static Categories s_categories;

    auto it = str.begin();
    auto end = str.end();
    vector<char32_t> codePoints;
    char32_t cp;
    CharCategory category;

    do
    {
        // 1. Map
        cp = utf8::next(it, end);
        if (!s_categories.map.TryGetValue(cp, category))
        {
        InsertAndContinue:
            // Just insert it and continue
            codePoints.push_back(cp);
            continue;
        }

        switch (category)
        {
            case CharCategory::CommonlyMappedToNothing:
            {
                // 1.2 mapping to nothing
                break;
            }
            case CharCategory::NonASCIISpaceCharacters:
            {
                // 1.1 mapping to space
                codePoints.push_back(U' ');
                break;
            }
            default:
            {
                goto InsertAndContinue;
            }
        }
    } while (it != end);

    // 2. Normalize
    // Perform a NFKC normalization on the code points

    vector<char32_t> normalized;
    if (!tryNormalizeNFKC(codePoints, normalized))
        return false;

    if (normalized.size() == 0)
        return true;

    bool hasBidiRAL = false;
    bool hasBidiL = false;
    bool isFirstBidiRAL = false;
    bool isLastBidiRAL = false;
    for (size_t i = 0; i < normalized.size(); i++)
    {
        cp = normalized[i];
        (void)s_categories.map.TryGetValue(cp, category);
        switch (category)
        {
            // 3. Prohibit
            case sprep::CharCategory::UnassignedCodePoints:
                // Unassigned code point, see https://tools.ietf.org/html/rfc4013#section-2.5
                prepd.clear();
                return false;
            case sprep::CharCategory::ProhibitedCharacters:
                // Prohibited character, see https://tools.ietf.org/html/rfc4013#section-2.3
                prepd.clear();
                return false;
            // 4. check bidi
            case sprep::CharCategory::Bidirectional_R_AL:
                hasBidiRAL = true;
                if (i == 0)
                    isFirstBidiRAL = true;
                else if (i == normalized.size() - 1)
                    isLastBidiRAL = true;
                utf8::unchecked::append((uint32_t)cp, std::back_inserter(prepd));
                break;
            case sprep::CharCategory::Bidirectional_L:
                hasBidiL = true;
                utf8::unchecked::append((uint32_t)cp, std::back_inserter(prepd));
                break;
            default:
                // CHECK-ME: Unknown category
                utf8::unchecked::append((uint32_t)cp, std::back_inserter(prepd));
                break;
        }
    }

    if (hasBidiRAL && hasBidiL)
    {
        // 4.1 If a string contains any RandALCat character, the string MUST NOT
        // contain any LCat character.
        // see https://tools.ietf.org/html/rfc3454#section-6
        prepd.clear();
        return false;
    }

    if (hasBidiRAL && !(isFirstBidiRAL && isLastBidiRAL))
    {
        // 4.2 If a string contains any RandALCat character, a RandALCat
        // character MUST be the first character of the string, and a
        // RandALCat character MUST be the last character of the string.
        //  see https://tools.ietf.org/html/rfc3454#section-6
        prepd.clear();
        return false;
    }

    return true;
}

CharCategoryMap::CharCategoryMap() { }

CharCategoryMap::CharCategoryMap(HashMap&& mappings, RangeMap&& ranges)
    : m_Mappings(std::move(mappings)), m_Ranges(std::move(ranges)) { }

void CharCategoryMap::PushMapping(char32_t key, CharCategory category)
{
    auto inserted = m_Mappings.emplace(key, category);
#ifdef CHECK_OVERLAPS
    assert(inserted.second);
#endif // CHECK_OVERLAPS
    (void)inserted;
}

void CharCategoryMap::PushRange(char32_t rangeLo, char32_t rangeHi, CharCategory category)
{
#ifdef CHECK_OVERLAPS
    for (char32_t i = rangeLo; i <= rangeHi; i++)
        PushMapping(i, category);
#else // CHECK_OVERLAPS
    // NOTE: This is not checked for valid ranges
    (void)m_Ranges.emplace(CharCategoryRange{ rangeLo, rangeHi - rangeLo + 1, category });
#endif // CHECK_OVERLAPS
}

bool CharCategoryMap::TryGetValue(char32_t key, CharCategory& category) const
{
    // Try to find direct mapppings first
    auto found = m_Mappings.find(key);
    if (found != m_Mappings.end())
    {
        category = found->second;
        return true;
    }

    // If not match on the direct mappings, try to find in the
    // ranges. Find the range with lower bound <= of the searched
    // key and verify if the range includes it
    auto foundRange = m_Ranges.upper_bound(key);
    if (foundRange == m_Ranges.begin() || key >= ((--foundRange)->RangeLo + foundRange->Size))
    {
        category = CharCategory::Unknown;
        return false;
    }

    category = foundRange->Value;
    return true;
}

// TODO: Optimize it so it will read the codes from a compressed stream
Categories::Categories()
{
    // Ported from https://github.com/reklatsmasters/saslprep/blob/master/lib/code-points.js

    // A.1 Unassigned code points in Unicode 3.2
    // @link https://tools.ietf.org/html/rfc3454#appendix-A.1

    map.PushMapping(0x0221, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0234, 0x024f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x02ae, 0x02af, CharCategory::UnassignedCodePoints);
    map.PushRange(0x02ef, 0x02ff, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0350, 0x035f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0370, 0x0373, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0376, 0x0379, CharCategory::UnassignedCodePoints);
    map.PushRange(0x037b, 0x037d, CharCategory::UnassignedCodePoints);
    map.PushRange(0x037f, 0x0383, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x038b, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x038d, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x03a2, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x03cf, CharCategory::UnassignedCodePoints);
    map.PushRange(0x03f7, 0x03ff, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0487, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x04cf, CharCategory::UnassignedCodePoints);
    map.PushRange(0x04f6, 0x04f7, CharCategory::UnassignedCodePoints);
    map.PushRange(0x04fa, 0x04ff, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0510, 0x0530, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0557, 0x0558, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0560, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0588, CharCategory::UnassignedCodePoints);
    map.PushRange(0x058b, 0x0590, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x05a2, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x05ba, CharCategory::UnassignedCodePoints);
    map.PushRange(0x05c5, 0x05cf, CharCategory::UnassignedCodePoints);
    map.PushRange(0x05eb, 0x05ef, CharCategory::UnassignedCodePoints);
    map.PushRange(0x05f5, 0x060b, CharCategory::UnassignedCodePoints);
    map.PushRange(0x060d, 0x061a, CharCategory::UnassignedCodePoints);
    map.PushRange(0x061c, 0x061e, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0620, CharCategory::UnassignedCodePoints);
    map.PushRange(0x063b, 0x063f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0656, 0x065f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x06ee, 0x06ef, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x06ff, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x070e, CharCategory::UnassignedCodePoints);
    map.PushRange(0x072d, 0x072f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x074b, 0x077f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x07b2, 0x0900, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0904, CharCategory::UnassignedCodePoints);
    map.PushRange(0x093a, 0x093b, CharCategory::UnassignedCodePoints);
    map.PushRange(0x094e, 0x094f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0955, 0x0957, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0971, 0x0980, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0984, CharCategory::UnassignedCodePoints);
    map.PushRange(0x098d, 0x098e, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0991, 0x0992, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x09a9, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x09b1, CharCategory::UnassignedCodePoints);
    map.PushRange(0x09b3, 0x09b5, CharCategory::UnassignedCodePoints);
    map.PushRange(0x09ba, 0x09bb, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x09bd, CharCategory::UnassignedCodePoints);
    map.PushRange(0x09c5, 0x09c6, CharCategory::UnassignedCodePoints);
    map.PushRange(0x09c9, 0x09ca, CharCategory::UnassignedCodePoints);
    map.PushRange(0x09ce, 0x09d6, CharCategory::UnassignedCodePoints);
    map.PushRange(0x09d8, 0x09db, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x09de, CharCategory::UnassignedCodePoints);
    map.PushRange(0x09e4, 0x09e5, CharCategory::UnassignedCodePoints);
    map.PushRange(0x09fb, 0x0a01, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0a03, 0x0a04, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0a0b, 0x0a0e, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0a11, 0x0a12, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0a29, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0a31, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0a34, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0a37, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0a3a, 0x0a3b, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0a3d, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0a43, 0x0a46, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0a49, 0x0a4a, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0a4e, 0x0a58, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0a5d, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0a5f, 0x0a65, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0a75, 0x0a80, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0a84, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0a8c, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0a8e, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0a92, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0aa9, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0ab1, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0ab4, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0aba, 0x0abb, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0ac6, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0aca, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0ace, 0x0acf, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0ad1, 0x0adf, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0ae1, 0x0ae5, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0af0, 0x0b00, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0b04, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0b0d, 0x0b0e, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0b11, 0x0b12, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0b29, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0b31, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0b34, 0x0b35, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0b3a, 0x0b3b, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0b44, 0x0b46, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0b49, 0x0b4a, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0b4e, 0x0b55, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0b58, 0x0b5b, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0b5e, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0b62, 0x0b65, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0b71, 0x0b81, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0b84, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0b8b, 0x0b8d, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0b91, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0b96, 0x0b98, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0b9b, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0b9d, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0ba0, 0x0ba2, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0ba5, 0x0ba7, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0bab, 0x0bad, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0bb6, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0bba, 0x0bbd, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0bc3, 0x0bc5, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0bc9, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0bce, 0x0bd6, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0bd8, 0x0be6, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0bf3, 0x0c00, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0c04, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0c0d, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0c11, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0c29, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0c34, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0c3a, 0x0c3d, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0c45, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0c49, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0c4e, 0x0c54, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0c57, 0x0c5f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0c62, 0x0c65, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0c70, 0x0c81, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0c84, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0c8d, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0c91, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0ca9, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0cb4, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0cba, 0x0cbd, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0cc5, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0cc9, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0cce, 0x0cd4, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0cd7, 0x0cdd, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0cdf, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0ce2, 0x0ce5, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0cf0, 0x0d01, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0d04, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0d0d, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0d11, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0d29, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0d3a, 0x0d3d, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0d44, 0x0d45, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0d49, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0d4e, 0x0d56, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0d58, 0x0d5f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0d62, 0x0d65, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0d70, 0x0d81, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0d84, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0d97, 0x0d99, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0db2, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0dbc, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0dbe, 0x0dbf, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0dc7, 0x0dc9, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0dcb, 0x0dce, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0dd5, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0dd7, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0de0, 0x0df1, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0df5, 0x0e00, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0e3b, 0x0e3e, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0e5c, 0x0e80, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0e83, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0e85, 0x0e86, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0e89, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0e8b, 0x0e8c, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0e8e, 0x0e93, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0e98, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0ea0, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0ea4, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0ea6, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0ea8, 0x0ea9, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0eac, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0eba, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0ebe, 0x0ebf, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0ec5, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0ec7, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0ece, 0x0ecf, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0eda, 0x0edb, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0ede, 0x0eff, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0f48, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0f6b, 0x0f70, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0f8c, 0x0f8f, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0f98, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x0fbd, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0fcd, 0x0fce, CharCategory::UnassignedCodePoints);
    map.PushRange(0x0fd0, 0x0fff, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1022, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1028, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x102b, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1033, 0x1035, CharCategory::UnassignedCodePoints);
    map.PushRange(0x103a, 0x103f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x105a, 0x109f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x10c6, 0x10cf, CharCategory::UnassignedCodePoints);
    map.PushRange(0x10f9, 0x10fa, CharCategory::UnassignedCodePoints);
    map.PushRange(0x10fc, 0x10ff, CharCategory::UnassignedCodePoints);
    map.PushRange(0x115a, 0x115e, CharCategory::UnassignedCodePoints);
    map.PushRange(0x11a3, 0x11a7, CharCategory::UnassignedCodePoints);
    map.PushRange(0x11fa, 0x11ff, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1207, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1247, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1249, CharCategory::UnassignedCodePoints);
    map.PushRange(0x124e, 0x124f, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1257, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1259, CharCategory::UnassignedCodePoints);
    map.PushRange(0x125e, 0x125f, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1287, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1289, CharCategory::UnassignedCodePoints);
    map.PushRange(0x128e, 0x128f, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x12af, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x12b1, CharCategory::UnassignedCodePoints);
    map.PushRange(0x12b6, 0x12b7, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x12bf, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x12c1, CharCategory::UnassignedCodePoints);
    map.PushRange(0x12c6, 0x12c7, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x12cf, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x12d7, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x12ef, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x130f, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1311, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1316, 0x1317, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x131f, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1347, CharCategory::UnassignedCodePoints);
    map.PushRange(0x135b, 0x1360, CharCategory::UnassignedCodePoints);
    map.PushRange(0x137d, 0x139f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x13f5, 0x1400, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1677, 0x167f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x169d, 0x169f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x16f1, 0x16ff, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x170d, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1715, 0x171f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1737, 0x173f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1754, 0x175f, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x176d, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1771, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1774, 0x177f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x17dd, 0x17df, CharCategory::UnassignedCodePoints);
    map.PushRange(0x17ea, 0x17ff, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x180f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x181a, 0x181f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1878, 0x187f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x18aa, 0x1dff, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1e9c, 0x1e9f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1efa, 0x1eff, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1f16, 0x1f17, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1f1e, 0x1f1f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1f46, 0x1f47, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1f4e, 0x1f4f, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1f58, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1f5a, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1f5c, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1f5e, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1f7e, 0x1f7f, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1fb5, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1fc5, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1fd4, 0x1fd5, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1fdc, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1ff0, 0x1ff1, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1ff5, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1fff, CharCategory::UnassignedCodePoints);
    map.PushRange(0x2053, 0x2056, CharCategory::UnassignedCodePoints);
    map.PushRange(0x2058, 0x205e, CharCategory::UnassignedCodePoints);
    map.PushRange(0x2064, 0x2069, CharCategory::UnassignedCodePoints);
    map.PushRange(0x2072, 0x2073, CharCategory::UnassignedCodePoints);
    map.PushRange(0x208f, 0x209f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x20b2, 0x20cf, CharCategory::UnassignedCodePoints);
    map.PushRange(0x20eb, 0x20ff, CharCategory::UnassignedCodePoints);
    map.PushRange(0x213b, 0x213c, CharCategory::UnassignedCodePoints);
    map.PushRange(0x214c, 0x2152, CharCategory::UnassignedCodePoints);
    map.PushRange(0x2184, 0x218f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x23cf, 0x23ff, CharCategory::UnassignedCodePoints);
    map.PushRange(0x2427, 0x243f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x244b, 0x245f, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x24ff, CharCategory::UnassignedCodePoints);
    map.PushRange(0x2614, 0x2615, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x2618, CharCategory::UnassignedCodePoints);
    map.PushRange(0x267e, 0x267f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x268a, 0x2700, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x2705, CharCategory::UnassignedCodePoints);
    map.PushRange(0x270a, 0x270b, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x2728, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x274c, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x274e, CharCategory::UnassignedCodePoints);
    map.PushRange(0x2753, 0x2755, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x2757, CharCategory::UnassignedCodePoints);
    map.PushRange(0x275f, 0x2760, CharCategory::UnassignedCodePoints);
    map.PushRange(0x2795, 0x2797, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x27b0, CharCategory::UnassignedCodePoints);
    map.PushRange(0x27bf, 0x27cf, CharCategory::UnassignedCodePoints);
    map.PushRange(0x27ec, 0x27ef, CharCategory::UnassignedCodePoints);
    map.PushRange(0x2b00, 0x2e7f, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x2e9a, CharCategory::UnassignedCodePoints);
    map.PushRange(0x2ef4, 0x2eff, CharCategory::UnassignedCodePoints);
    map.PushRange(0x2fd6, 0x2fef, CharCategory::UnassignedCodePoints);
    map.PushRange(0x2ffc, 0x2fff, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x3040, CharCategory::UnassignedCodePoints);
    map.PushRange(0x3097, 0x3098, CharCategory::UnassignedCodePoints);
    map.PushRange(0x3100, 0x3104, CharCategory::UnassignedCodePoints);
    map.PushRange(0x312d, 0x3130, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x318f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x31b8, 0x31ef, CharCategory::UnassignedCodePoints);
    map.PushRange(0x321d, 0x321f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x3244, 0x3250, CharCategory::UnassignedCodePoints);
    map.PushRange(0x327c, 0x327e, CharCategory::UnassignedCodePoints);
    map.PushRange(0x32cc, 0x32cf, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x32ff, CharCategory::UnassignedCodePoints);
    map.PushRange(0x3377, 0x337a, CharCategory::UnassignedCodePoints);
    map.PushRange(0x33de, 0x33df, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x33ff, CharCategory::UnassignedCodePoints);
    map.PushRange(0x4db6, 0x4dff, CharCategory::UnassignedCodePoints);
    map.PushRange(0x9fa6, 0x9fff, CharCategory::UnassignedCodePoints);
    map.PushRange(0xa48d, 0xa48f, CharCategory::UnassignedCodePoints);
    map.PushRange(0xa4c7, 0xabff, CharCategory::UnassignedCodePoints);
    map.PushRange(0xd7a4, 0xd7ff, CharCategory::UnassignedCodePoints);
    map.PushRange(0xfa2e, 0xfa2f, CharCategory::UnassignedCodePoints);
    map.PushRange(0xfa6b, 0xfaff, CharCategory::UnassignedCodePoints);
    map.PushRange(0xfb07, 0xfb12, CharCategory::UnassignedCodePoints);
    map.PushRange(0xfb18, 0xfb1c, CharCategory::UnassignedCodePoints);
    map.PushMapping(0xfb37, CharCategory::UnassignedCodePoints);
    map.PushMapping(0xfb3d, CharCategory::UnassignedCodePoints);
    map.PushMapping(0xfb3f, CharCategory::UnassignedCodePoints);
    map.PushMapping(0xfb42, CharCategory::UnassignedCodePoints);
    map.PushMapping(0xfb45, CharCategory::UnassignedCodePoints);
    map.PushRange(0xfbb2, 0xfbd2, CharCategory::UnassignedCodePoints);
    map.PushRange(0xfd40, 0xfd4f, CharCategory::UnassignedCodePoints);
    map.PushRange(0xfd90, 0xfd91, CharCategory::UnassignedCodePoints);
    map.PushRange(0xfdc8, 0xfdcf, CharCategory::UnassignedCodePoints);
    map.PushRange(0xfdfd, 0xfdff, CharCategory::UnassignedCodePoints);
    map.PushRange(0xfe10, 0xfe1f, CharCategory::UnassignedCodePoints);
    map.PushRange(0xfe24, 0xfe2f, CharCategory::UnassignedCodePoints);
    map.PushRange(0xfe47, 0xfe48, CharCategory::UnassignedCodePoints);
    map.PushMapping(0xfe53, CharCategory::UnassignedCodePoints);
    map.PushMapping(0xfe67, CharCategory::UnassignedCodePoints);
    map.PushRange(0xfe6c, 0xfe6f, CharCategory::UnassignedCodePoints);
    map.PushMapping(0xfe75, CharCategory::UnassignedCodePoints);
    map.PushRange(0xfefd, 0xfefe, CharCategory::UnassignedCodePoints);
    map.PushMapping(0xff00, CharCategory::UnassignedCodePoints);
    map.PushRange(0xffbf, 0xffc1, CharCategory::UnassignedCodePoints);
    map.PushRange(0xffc8, 0xffc9, CharCategory::UnassignedCodePoints);
    map.PushRange(0xffd0, 0xffd1, CharCategory::UnassignedCodePoints);
    map.PushRange(0xffd8, 0xffd9, CharCategory::UnassignedCodePoints);
    map.PushRange(0xffdd, 0xffdf, CharCategory::UnassignedCodePoints);
    map.PushMapping(0xffe7, CharCategory::UnassignedCodePoints);
    map.PushRange(0xffef, 0xfff8, CharCategory::UnassignedCodePoints);
    map.PushRange(0x10000, 0x102ff, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1031f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x10324, 0x1032f, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1034b, 0x103ff, CharCategory::UnassignedCodePoints);
    map.PushRange(0x10426, 0x10427, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1044e, 0x1cfff, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1d0f6, 0x1d0ff, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1d127, 0x1d129, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1d1de, 0x1d3ff, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1d455, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1d49d, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1d4a0, 0x1d4a1, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1d4a3, 0x1d4a4, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1d4a7, 0x1d4a8, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1d4ad, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1d4ba, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1d4bc, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1d4c1, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1d4c4, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1d506, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1d50b, 0x1d50c, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1d515, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1d51d, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1d53a, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1d53f, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1d545, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1d547, 0x1d549, CharCategory::UnassignedCodePoints);
    map.PushMapping(0x1d551, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1d6a4, 0x1d6a7, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1d7ca, 0x1d7cd, CharCategory::UnassignedCodePoints);
    map.PushRange(0x1d800, 0x1fffd, CharCategory::UnassignedCodePoints);
    map.PushRange(0x2a6d7, 0x2f7ff, CharCategory::UnassignedCodePoints);
    map.PushRange(0x2fa1e, 0x2fffd, CharCategory::UnassignedCodePoints);
    map.PushRange(0x30000, 0x3fffd, CharCategory::UnassignedCodePoints);
    map.PushRange(0x40000, 0x4fffd, CharCategory::UnassignedCodePoints);
    map.PushRange(0x50000, 0x5fffd, CharCategory::UnassignedCodePoints);
    map.PushRange(0x60000, 0x6fffd, CharCategory::UnassignedCodePoints);
    map.PushRange(0x70000, 0x7fffd, CharCategory::UnassignedCodePoints);
    map.PushRange(0x80000, 0x8fffd, CharCategory::UnassignedCodePoints);
    map.PushRange(0x90000, 0x9fffd, CharCategory::UnassignedCodePoints);
    map.PushRange(0xa0000, 0xafffd, CharCategory::UnassignedCodePoints);
    map.PushRange(0xb0000, 0xbfffd, CharCategory::UnassignedCodePoints);
    map.PushRange(0xc0000, 0xcfffd, CharCategory::UnassignedCodePoints);
    map.PushRange(0xd0000, 0xdfffd, CharCategory::UnassignedCodePoints);
    map.PushMapping(0xe0000, CharCategory::UnassignedCodePoints);
    map.PushRange(0xe0002, 0xe001f, CharCategory::UnassignedCodePoints);
    map.PushRange(0xe0080, 0xefffd, CharCategory::UnassignedCodePoints);

    // B.1 Commonly mapped to nothing
    // https://tools.ietf.org/html/rfc3454#appendix-B.1

    map.PushMapping(0x00ad, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0x034f, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0x1806, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0x180b, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0x180c, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0x180d, CharCategory::CommonlyMappedToNothing);
    // map.PushMapping(0x200b, CharCategory::CommonlyMappedToNothing);      // NOTE: These are also non ASCII space characters
    map.PushMapping(0x200c, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0x200d, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0x2060, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0xfe00, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0xfe01, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0xfe02, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0xfe03, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0xfe04, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0xfe05, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0xfe06, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0xfe07, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0xfe08, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0xfe09, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0xfe0a, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0xfe0b, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0xfe0c, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0xfe0d, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0xfe0e, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0xfe0f, CharCategory::CommonlyMappedToNothing);
    map.PushMapping(0xfeff, CharCategory::CommonlyMappedToNothing);

    // C.1.2 Non-ASCII space characters
    // https://tools.ietf.org/html/rfc3454#appendix-C.1.2
    // NOTE: They are also prohibited

    map.PushMapping(0x00a0, CharCategory::NonASCIISpaceCharacters); // NO-BREAK SPACE
    map.PushMapping(0x1680, CharCategory::NonASCIISpaceCharacters); // OGHAM SPACE MARK
    map.PushMapping(0x2000, CharCategory::NonASCIISpaceCharacters); // EN QUAD
    map.PushMapping(0x2001, CharCategory::NonASCIISpaceCharacters); // EM QUAD
    map.PushMapping(0x2002, CharCategory::NonASCIISpaceCharacters); // EN SPACE
    map.PushMapping(0x2003, CharCategory::NonASCIISpaceCharacters); // EM SPACE
    map.PushMapping(0x2004, CharCategory::NonASCIISpaceCharacters); // THREE-PER-EM SPACE
    map.PushMapping(0x2005, CharCategory::NonASCIISpaceCharacters); // FOUR-PER-EM SPACE
    map.PushMapping(0x2006, CharCategory::NonASCIISpaceCharacters); // SIX-PER-EM SPACE
    map.PushMapping(0x2007, CharCategory::NonASCIISpaceCharacters); // FIGURE SPACE
    map.PushMapping(0x2008, CharCategory::NonASCIISpaceCharacters); // PUNCTUATION SPACE
    map.PushMapping(0x2009, CharCategory::NonASCIISpaceCharacters); // THIN SPACE
    map.PushMapping(0x200a, CharCategory::NonASCIISpaceCharacters); // HAIR SPACE
    map.PushMapping(0x200b, CharCategory::NonASCIISpaceCharacters); // ZERO WIDTH SPACE
    map.PushMapping(0x202f, CharCategory::NonASCIISpaceCharacters); // NARROW NO-BREAK SPACE
    map.PushMapping(0x205f, CharCategory::NonASCIISpaceCharacters); // MEDIUM MATHEMATICAL SPACE
    map.PushMapping(0x3000, CharCategory::NonASCIISpaceCharacters); // IDEOGRAPHIC SPACE

    // C.2.1 ASCII control characters
    // https://tools.ietf.org/html/rfc3454#appendix-C.2.1

    map.PushRange(0, 0x001f, CharCategory::ProhibitedCharacters); // [CONTROL CHARACTERS]
    map.PushMapping(0x007f, CharCategory::ProhibitedCharacters); // DELETE

    // C.2.2 Non-ASCII control characters
    // https://tools.ietf.org/html/rfc3454#appendix-C.2.2

    map.PushRange(0x0080, 0x009f, CharCategory::ProhibitedCharacters); // [CONTROL CHARACTERS]
    map.PushMapping(0x06dd, CharCategory::ProhibitedCharacters); // ARABIC END OF AYAH
    map.PushMapping(0x070f, CharCategory::ProhibitedCharacters); // SYRIAC ABBREVIATION MARK
    map.PushMapping(0x180e, CharCategory::ProhibitedCharacters); // MONGOLIAN VOWEL SEPARATOR
    //map.PushMapping(0x200c, CharCategory::ProhibitedCharacters); // ZERO WIDTH NON-JOINER      // NOTE: These are also non ASCII space characters
    //map.PushMapping(0x200d, CharCategory::ProhibitedCharacters); // ZERO WIDTH JOINER          // NOTE: These are also non ASCII space characters
    map.PushMapping(0x2028, CharCategory::ProhibitedCharacters); // LINE SEPARATOR
    map.PushMapping(0x2029, CharCategory::ProhibitedCharacters); // PARAGRAPH SEPARATOR
    //map.PushMapping(0x2060, CharCategory::ProhibitedCharacters); // WORD JOINER                // NOTE: These are also non ASCII space characters
    map.PushMapping(0x2061, CharCategory::ProhibitedCharacters); // FUNCTION APPLICATION
    map.PushMapping(0x2062, CharCategory::ProhibitedCharacters); // INVISIBLE TIMES
    map.PushMapping(0x2063, CharCategory::ProhibitedCharacters); // INVISIBLE SEPARATOR
    //map.PushMapping(0xfeff, CharCategory::ProhibitedCharacters); // ZERO WIDTH NO-BREAK SPACE  // NOTE: These are also non ASCII space characters
    map.PushRange(0x1d173, 0x1d17a, CharCategory::ProhibitedCharacters); // [MUSICAL CONTROL CHARACTERS]

    // C.3 Private use
    // https://tools.ietf.org/html/rfc3454#appendix-C.3

    map.PushRange(0xe000, 0xf8ff, CharCategory::ProhibitedCharacters); // [PRIVATE USE, PLANE 0]
    map.PushRange(0xf0000, 0xffffd, CharCategory::ProhibitedCharacters); // [PRIVATE USE, PLANE 15]
    map.PushRange(0x100000, 0x10fffd, CharCategory::ProhibitedCharacters); // [PRIVATE USE, PLANE 16]

    // C.4 Non-character code points
    // https://tools.ietf.org/html/rfc3454#appendix-C.4

    map.PushRange(0xfdd0, 0xfdef, CharCategory::ProhibitedCharacters); // [NONCHARACTER CODE POINTS]
    map.PushRange(0xfffe, 0xffff, CharCategory::ProhibitedCharacters); // [NONCHARACTER CODE POINTS]
    map.PushRange(0x1fffe, 0x1ffff, CharCategory::ProhibitedCharacters); // [NONCHARACTER CODE POINTS]
    map.PushRange(0x2fffe, 0x2ffff, CharCategory::ProhibitedCharacters); // [NONCHARACTER CODE POINTS]
    map.PushRange(0x3fffe, 0x3ffff, CharCategory::ProhibitedCharacters); // [NONCHARACTER CODE POINTS]
    map.PushRange(0x4fffe, 0x4ffff, CharCategory::ProhibitedCharacters); // [NONCHARACTER CODE POINTS]
    map.PushRange(0x5fffe, 0x5ffff, CharCategory::ProhibitedCharacters); // [NONCHARACTER CODE POINTS]
    map.PushRange(0x6fffe, 0x6ffff, CharCategory::ProhibitedCharacters); // [NONCHARACTER CODE POINTS]
    map.PushRange(0x7fffe, 0x7ffff, CharCategory::ProhibitedCharacters); // [NONCHARACTER CODE POINTS]
    map.PushRange(0x8fffe, 0x8ffff, CharCategory::ProhibitedCharacters); // [NONCHARACTER CODE POINTS]
    map.PushRange(0x9fffe, 0x9ffff, CharCategory::ProhibitedCharacters); // [NONCHARACTER CODE POINTS]
    map.PushRange(0xafffe, 0xaffff, CharCategory::ProhibitedCharacters); // [NONCHARACTER CODE POINTS]
    map.PushRange(0xbfffe, 0xbffff, CharCategory::ProhibitedCharacters); // [NONCHARACTER CODE POINTS]
    map.PushRange(0xcfffe, 0xcffff, CharCategory::ProhibitedCharacters); // [NONCHARACTER CODE POINTS]
    map.PushRange(0xdfffe, 0xdffff, CharCategory::ProhibitedCharacters); // [NONCHARACTER CODE POINTS]
    map.PushRange(0xefffe, 0xeffff, CharCategory::ProhibitedCharacters); // [NONCHARACTER CODE POINTS]
    map.PushRange(0x10fffe, 0x10ffff, CharCategory::ProhibitedCharacters); // [NONCHARACTER CODE POINTS]

    // C.5 Surrogate codes
    // @link https://tools.ietf.org/html/rfc3454#appendix-C.5

    map.PushRange(0xd800, 0xdfff, CharCategory::ProhibitedCharacters);

    // C.6 Inappropriate for plain text
    // https://tools.ietf.org/html/rfc3454#appendix-C.6

    map.PushMapping(0xfff9, CharCategory::ProhibitedCharacters); // INTERLINEAR ANNOTATION ANCHOR
    map.PushMapping(0xfffa, CharCategory::ProhibitedCharacters); // INTERLINEAR ANNOTATION SEPARATOR
    map.PushMapping(0xfffb, CharCategory::ProhibitedCharacters); // INTERLINEAR ANNOTATION TERMINATOR
    map.PushMapping(0xfffc, CharCategory::ProhibitedCharacters); // OBJECT REPLACEMENT CHARACTER
    map.PushMapping(0xfffd, CharCategory::ProhibitedCharacters); // REPLACEMENT CHARACTER

    // C.7 Inappropriate for canonical representation
    // https://tools.ietf.org/html/rfc3454#appendix-C.7

    map.PushRange(0x2ff0, 0x2ffb, CharCategory::ProhibitedCharacters); // [IDEOGRAPHIC DESCRIPTION CHARACTERS]

    // C.8 Change display properties or are deprecated
    // https://tools.ietf.org/html/rfc3454#appendix-C.8

    map.PushMapping(0x0340, CharCategory::ProhibitedCharacters); // COMBINING GRAVE TONE MARK
    map.PushMapping(0x0341, CharCategory::ProhibitedCharacters); // COMBINING ACUTE TONE MARK
    map.PushMapping(0x200e, CharCategory::ProhibitedCharacters); // LEFT-TO-RIGHT MARK
    map.PushMapping(0x200f, CharCategory::ProhibitedCharacters); // RIGHT-TO-LEFT MARK
    map.PushMapping(0x202a, CharCategory::ProhibitedCharacters); // LEFT-TO-RIGHT EMBEDDING
    map.PushMapping(0x202b, CharCategory::ProhibitedCharacters); // RIGHT-TO-LEFT EMBEDDING
    map.PushMapping(0x202c, CharCategory::ProhibitedCharacters); // POP DIRECTIONAL FORMATTING
    map.PushMapping(0x202d, CharCategory::ProhibitedCharacters); // LEFT-TO-RIGHT OVERRIDE
    map.PushMapping(0x202e, CharCategory::ProhibitedCharacters); // RIGHT-TO-LEFT OVERRIDE
    map.PushMapping(0x206a, CharCategory::ProhibitedCharacters); // INHIBIT SYMMETRIC SWAPPING
    map.PushMapping(0x206b, CharCategory::ProhibitedCharacters); // ACTIVATE SYMMETRIC SWAPPING
    map.PushMapping(0x206c, CharCategory::ProhibitedCharacters); // INHIBIT ARABIC FORM SHAPING
    map.PushMapping(0x206d, CharCategory::ProhibitedCharacters); // ACTIVATE ARABIC FORM SHAPING
    map.PushMapping(0x206e, CharCategory::ProhibitedCharacters); // NATIONAL DIGIT SHAPES
    map.PushMapping(0x206f, CharCategory::ProhibitedCharacters); // NOMINAL DIGIT SHAPES

    // C.9 Tagging characters
    // https://tools.ietf.org/html/rfc3454#appendix-C.9

    map.PushMapping(0xe0001, CharCategory::ProhibitedCharacters); // LANGUAGE TAG
    map.PushRange(0xe0020, 0xe007f, CharCategory::ProhibitedCharacters); // [TAGGING CHARACTERS]

    // D.1 Characters with bidirectional property "R" or "AL"
    // https://tools.ietf.org/html/rfc3454#appendix-D.1

    map.PushMapping(0x05be, CharCategory::Bidirectional_R_AL);
    map.PushMapping(0x05c0, CharCategory::Bidirectional_R_AL);
    map.PushMapping(0x05c3, CharCategory::Bidirectional_R_AL);
    map.PushRange(0x05d0, 0x05ea, CharCategory::Bidirectional_R_AL);
    map.PushRange(0x05f0, 0x05f4, CharCategory::Bidirectional_R_AL);
    map.PushMapping(0x061b, CharCategory::Bidirectional_R_AL);
    map.PushMapping(0x061f, CharCategory::Bidirectional_R_AL);
    map.PushRange(0x0621, 0x063a, CharCategory::Bidirectional_R_AL);
    map.PushRange(0x0640, 0x064a, CharCategory::Bidirectional_R_AL);
    map.PushRange(0x066d, 0x066f, CharCategory::Bidirectional_R_AL);
    map.PushRange(0x0671, 0x06d5, CharCategory::Bidirectional_R_AL);
    //map.PushMapping(0x06dd, CharCategory::Bidirectional_R_AL);        // NOTE: These are also prohibited
    map.PushRange(0x06e5, 0x06e6, CharCategory::Bidirectional_R_AL);
    map.PushRange(0x06fa, 0x06fe, CharCategory::Bidirectional_R_AL);
    map.PushRange(0x0700, 0x070d, CharCategory::Bidirectional_R_AL);
    map.PushMapping(0x0710, CharCategory::Bidirectional_R_AL);
    map.PushRange(0x0712, 0x072c, CharCategory::Bidirectional_R_AL);
    map.PushRange(0x0780, 0x07a5, CharCategory::Bidirectional_R_AL);
    map.PushMapping(0x07b1, CharCategory::Bidirectional_R_AL);
    //map.PushMapping(0x200f, CharCategory::Bidirectional_R_AL);        // NOTE: These are also prohibited
    map.PushMapping(0xfb1d, CharCategory::Bidirectional_R_AL);
    map.PushRange(0xfb1f, 0xfb28, CharCategory::Bidirectional_R_AL);
    map.PushRange(0xfb2a, 0xfb36, CharCategory::Bidirectional_R_AL);
    map.PushRange(0xfb38, 0xfb3c, CharCategory::Bidirectional_R_AL);
    map.PushMapping(0xfb3e, CharCategory::Bidirectional_R_AL);
    map.PushRange(0xfb40, 0xfb41, CharCategory::Bidirectional_R_AL);
    map.PushRange(0xfb43, 0xfb44, CharCategory::Bidirectional_R_AL);
    map.PushRange(0xfb46, 0xfbb1, CharCategory::Bidirectional_R_AL);
    map.PushRange(0xfbd3, 0xfd3d, CharCategory::Bidirectional_R_AL);
    map.PushRange(0xfd50, 0xfd8f, CharCategory::Bidirectional_R_AL);
    map.PushRange(0xfd92, 0xfdc7, CharCategory::Bidirectional_R_AL);
    map.PushRange(0xfdf0, 0xfdfc, CharCategory::Bidirectional_R_AL);
    map.PushRange(0xfe70, 0xfe74, CharCategory::Bidirectional_R_AL);
    map.PushRange(0xfe76, 0xfefc, CharCategory::Bidirectional_R_AL);

    // D.2 Characters with bidirectional property "L"
    // https://tools.ietf.org/html/rfc3454#appendix-D.2

    map.PushRange(0x0041, 0x005a, CharCategory::Bidirectional_L);
    map.PushRange(0x0061, 0x007a, CharCategory::Bidirectional_L);
    map.PushMapping(0x00aa, CharCategory::Bidirectional_L);
    map.PushMapping(0x00b5, CharCategory::Bidirectional_L);
    map.PushMapping(0x00ba, CharCategory::Bidirectional_L);
    map.PushRange(0x00c0, 0x00d6, CharCategory::Bidirectional_L);
    map.PushRange(0x00d8, 0x00f6, CharCategory::Bidirectional_L);
    map.PushRange(0x00f8, 0x0220, CharCategory::Bidirectional_L);
    map.PushRange(0x0222, 0x0233, CharCategory::Bidirectional_L);
    map.PushRange(0x0250, 0x02ad, CharCategory::Bidirectional_L);
    map.PushRange(0x02b0, 0x02b8, CharCategory::Bidirectional_L);
    map.PushRange(0x02bb, 0x02c1, CharCategory::Bidirectional_L);
    map.PushRange(0x02d0, 0x02d1, CharCategory::Bidirectional_L);
    map.PushRange(0x02e0, 0x02e4, CharCategory::Bidirectional_L);
    map.PushMapping(0x02ee, CharCategory::Bidirectional_L);
    map.PushMapping(0x037a, CharCategory::Bidirectional_L);
    map.PushMapping(0x0386, CharCategory::Bidirectional_L);
    map.PushRange(0x0388, 0x038a, CharCategory::Bidirectional_L);
    map.PushMapping(0x038c, CharCategory::Bidirectional_L);
    map.PushRange(0x038e, 0x03a1, CharCategory::Bidirectional_L);
    map.PushRange(0x03a3, 0x03ce, CharCategory::Bidirectional_L);
    map.PushRange(0x03d0, 0x03f5, CharCategory::Bidirectional_L);
    map.PushRange(0x0400, 0x0482, CharCategory::Bidirectional_L);
    map.PushRange(0x048a, 0x04ce, CharCategory::Bidirectional_L);
    map.PushRange(0x04d0, 0x04f5, CharCategory::Bidirectional_L);
    map.PushRange(0x04f8, 0x04f9, CharCategory::Bidirectional_L);
    map.PushRange(0x0500, 0x050f, CharCategory::Bidirectional_L);
    map.PushRange(0x0531, 0x0556, CharCategory::Bidirectional_L);
    map.PushRange(0x0559, 0x055f, CharCategory::Bidirectional_L);
    map.PushRange(0x0561, 0x0587, CharCategory::Bidirectional_L);
    map.PushMapping(0x0589, CharCategory::Bidirectional_L);
    map.PushMapping(0x0903, CharCategory::Bidirectional_L);
    map.PushRange(0x0905, 0x0939, CharCategory::Bidirectional_L);
    map.PushRange(0x093d, 0x0940, CharCategory::Bidirectional_L);
    map.PushRange(0x0949, 0x094c, CharCategory::Bidirectional_L);
    map.PushMapping(0x0950, CharCategory::Bidirectional_L);
    map.PushRange(0x0958, 0x0961, CharCategory::Bidirectional_L);
    map.PushRange(0x0964, 0x0970, CharCategory::Bidirectional_L);
    map.PushRange(0x0982, 0x0983, CharCategory::Bidirectional_L);
    map.PushRange(0x0985, 0x098c, CharCategory::Bidirectional_L);
    map.PushRange(0x098f, 0x0990, CharCategory::Bidirectional_L);
    map.PushRange(0x0993, 0x09a8, CharCategory::Bidirectional_L);
    map.PushRange(0x09aa, 0x09b0, CharCategory::Bidirectional_L);
    map.PushMapping(0x09b2, CharCategory::Bidirectional_L);
    map.PushRange(0x09b6, 0x09b9, CharCategory::Bidirectional_L);
    map.PushRange(0x09be, 0x09c0, CharCategory::Bidirectional_L);
    map.PushRange(0x09c7, 0x09c8, CharCategory::Bidirectional_L);
    map.PushRange(0x09cb, 0x09cc, CharCategory::Bidirectional_L);
    map.PushMapping(0x09d7, CharCategory::Bidirectional_L);
    map.PushRange(0x09dc, 0x09dd, CharCategory::Bidirectional_L);
    map.PushRange(0x09df, 0x09e1, CharCategory::Bidirectional_L);
    map.PushRange(0x09e6, 0x09f1, CharCategory::Bidirectional_L);
    map.PushRange(0x09f4, 0x09fa, CharCategory::Bidirectional_L);
    map.PushRange(0x0a05, 0x0a0a, CharCategory::Bidirectional_L);
    map.PushRange(0x0a0f, 0x0a10, CharCategory::Bidirectional_L);
    map.PushRange(0x0a13, 0x0a28, CharCategory::Bidirectional_L);
    map.PushRange(0x0a2a, 0x0a30, CharCategory::Bidirectional_L);
    map.PushRange(0x0a32, 0x0a33, CharCategory::Bidirectional_L);
    map.PushRange(0x0a35, 0x0a36, CharCategory::Bidirectional_L);
    map.PushRange(0x0a38, 0x0a39, CharCategory::Bidirectional_L);
    map.PushRange(0x0a3e, 0x0a40, CharCategory::Bidirectional_L);
    map.PushRange(0x0a59, 0x0a5c, CharCategory::Bidirectional_L);
    map.PushMapping(0x0a5e, CharCategory::Bidirectional_L);
    map.PushRange(0x0a66, 0x0a6f, CharCategory::Bidirectional_L);
    map.PushRange(0x0a72, 0x0a74, CharCategory::Bidirectional_L);
    map.PushMapping(0x0a83, CharCategory::Bidirectional_L);
    map.PushRange(0x0a85, 0x0a8b, CharCategory::Bidirectional_L);
    map.PushMapping(0x0a8d, CharCategory::Bidirectional_L);
    map.PushRange(0x0a8f, 0x0a91, CharCategory::Bidirectional_L);
    map.PushRange(0x0a93, 0x0aa8, CharCategory::Bidirectional_L);
    map.PushRange(0x0aaa, 0x0ab0, CharCategory::Bidirectional_L);
    map.PushRange(0x0ab2, 0x0ab3, CharCategory::Bidirectional_L);
    map.PushRange(0x0ab5, 0x0ab9, CharCategory::Bidirectional_L);
    map.PushRange(0x0abd, 0x0ac0, CharCategory::Bidirectional_L);
    map.PushMapping(0x0ac9, CharCategory::Bidirectional_L);
    map.PushRange(0x0acb, 0x0acc, CharCategory::Bidirectional_L);
    map.PushMapping(0x0ad0, CharCategory::Bidirectional_L);
    map.PushMapping(0x0ae0, CharCategory::Bidirectional_L);
    map.PushRange(0x0ae6, 0x0aef, CharCategory::Bidirectional_L);
    map.PushRange(0x0b02, 0x0b03, CharCategory::Bidirectional_L);
    map.PushRange(0x0b05, 0x0b0c, CharCategory::Bidirectional_L);
    map.PushRange(0x0b0f, 0x0b10, CharCategory::Bidirectional_L);
    map.PushRange(0x0b13, 0x0b28, CharCategory::Bidirectional_L);
    map.PushRange(0x0b2a, 0x0b30, CharCategory::Bidirectional_L);
    map.PushRange(0x0b32, 0x0b33, CharCategory::Bidirectional_L);
    map.PushRange(0x0b36, 0x0b39, CharCategory::Bidirectional_L);
    map.PushRange(0x0b3d, 0x0b3e, CharCategory::Bidirectional_L);
    map.PushMapping(0x0b40, CharCategory::Bidirectional_L);
    map.PushRange(0x0b47, 0x0b48, CharCategory::Bidirectional_L);
    map.PushRange(0x0b4b, 0x0b4c, CharCategory::Bidirectional_L);
    map.PushMapping(0x0b57, CharCategory::Bidirectional_L);
    map.PushRange(0x0b5c, 0x0b5d, CharCategory::Bidirectional_L);
    map.PushRange(0x0b5f, 0x0b61, CharCategory::Bidirectional_L);
    map.PushRange(0x0b66, 0x0b70, CharCategory::Bidirectional_L);
    map.PushMapping(0x0b83, CharCategory::Bidirectional_L);
    map.PushRange(0x0b85, 0x0b8a, CharCategory::Bidirectional_L);
    map.PushRange(0x0b8e, 0x0b90, CharCategory::Bidirectional_L);
    map.PushRange(0x0b92, 0x0b95, CharCategory::Bidirectional_L);
    map.PushRange(0x0b99, 0x0b9a, CharCategory::Bidirectional_L);
    map.PushMapping(0x0b9c, CharCategory::Bidirectional_L);
    map.PushRange(0x0b9e, 0x0b9f, CharCategory::Bidirectional_L);
    map.PushRange(0x0ba3, 0x0ba4, CharCategory::Bidirectional_L);
    map.PushRange(0x0ba8, 0x0baa, CharCategory::Bidirectional_L);
    map.PushRange(0x0bae, 0x0bb5, CharCategory::Bidirectional_L);
    map.PushRange(0x0bb7, 0x0bb9, CharCategory::Bidirectional_L);
    map.PushRange(0x0bbe, 0x0bbf, CharCategory::Bidirectional_L);
    map.PushRange(0x0bc1, 0x0bc2, CharCategory::Bidirectional_L);
    map.PushRange(0x0bc6, 0x0bc8, CharCategory::Bidirectional_L);
    map.PushRange(0x0bca, 0x0bcc, CharCategory::Bidirectional_L);
    map.PushMapping(0x0bd7, CharCategory::Bidirectional_L);
    map.PushRange(0x0be7, 0x0bf2, CharCategory::Bidirectional_L);
    map.PushRange(0x0c01, 0x0c03, CharCategory::Bidirectional_L);
    map.PushRange(0x0c05, 0x0c0c, CharCategory::Bidirectional_L);
    map.PushRange(0x0c0e, 0x0c10, CharCategory::Bidirectional_L);
    map.PushRange(0x0c12, 0x0c28, CharCategory::Bidirectional_L);
    map.PushRange(0x0c2a, 0x0c33, CharCategory::Bidirectional_L);
    map.PushRange(0x0c35, 0x0c39, CharCategory::Bidirectional_L);
    map.PushRange(0x0c41, 0x0c44, CharCategory::Bidirectional_L);
    map.PushRange(0x0c60, 0x0c61, CharCategory::Bidirectional_L);
    map.PushRange(0x0c66, 0x0c6f, CharCategory::Bidirectional_L);
    map.PushRange(0x0c82, 0x0c83, CharCategory::Bidirectional_L);
    map.PushRange(0x0c85, 0x0c8c, CharCategory::Bidirectional_L);
    map.PushRange(0x0c8e, 0x0c90, CharCategory::Bidirectional_L);
    map.PushRange(0x0c92, 0x0ca8, CharCategory::Bidirectional_L);
    map.PushRange(0x0caa, 0x0cb3, CharCategory::Bidirectional_L);
    map.PushRange(0x0cb5, 0x0cb9, CharCategory::Bidirectional_L);
    map.PushMapping(0x0cbe, CharCategory::Bidirectional_L);
    map.PushRange(0x0cc0, 0x0cc4, CharCategory::Bidirectional_L);
    map.PushRange(0x0cc7, 0x0cc8, CharCategory::Bidirectional_L);
    map.PushRange(0x0cca, 0x0ccb, CharCategory::Bidirectional_L);
    map.PushRange(0x0cd5, 0x0cd6, CharCategory::Bidirectional_L);
    map.PushMapping(0x0cde, CharCategory::Bidirectional_L);
    map.PushRange(0x0ce0, 0x0ce1, CharCategory::Bidirectional_L);
    map.PushRange(0x0ce6, 0x0cef, CharCategory::Bidirectional_L);
    map.PushRange(0x0d02, 0x0d03, CharCategory::Bidirectional_L);
    map.PushRange(0x0d05, 0x0d0c, CharCategory::Bidirectional_L);
    map.PushRange(0x0d0e, 0x0d10, CharCategory::Bidirectional_L);
    map.PushRange(0x0d12, 0x0d28, CharCategory::Bidirectional_L);
    map.PushRange(0x0d2a, 0x0d39, CharCategory::Bidirectional_L);
    map.PushRange(0x0d3e, 0x0d40, CharCategory::Bidirectional_L);
    map.PushRange(0x0d46, 0x0d48, CharCategory::Bidirectional_L);
    map.PushRange(0x0d4a, 0x0d4c, CharCategory::Bidirectional_L);
    map.PushMapping(0x0d57, CharCategory::Bidirectional_L);
    map.PushRange(0x0d60, 0x0d61, CharCategory::Bidirectional_L);
    map.PushRange(0x0d66, 0x0d6f, CharCategory::Bidirectional_L);
    map.PushRange(0x0d82, 0x0d83, CharCategory::Bidirectional_L);
    map.PushRange(0x0d85, 0x0d96, CharCategory::Bidirectional_L);
    map.PushRange(0x0d9a, 0x0db1, CharCategory::Bidirectional_L);
    map.PushRange(0x0db3, 0x0dbb, CharCategory::Bidirectional_L);
    map.PushMapping(0x0dbd, CharCategory::Bidirectional_L);
    map.PushRange(0x0dc0, 0x0dc6, CharCategory::Bidirectional_L);
    map.PushRange(0x0dcf, 0x0dd1, CharCategory::Bidirectional_L);
    map.PushRange(0x0dd8, 0x0ddf, CharCategory::Bidirectional_L);
    map.PushRange(0x0df2, 0x0df4, CharCategory::Bidirectional_L);
    map.PushRange(0x0e01, 0x0e30, CharCategory::Bidirectional_L);
    map.PushRange(0x0e32, 0x0e33, CharCategory::Bidirectional_L);
    map.PushRange(0x0e40, 0x0e46, CharCategory::Bidirectional_L);
    map.PushRange(0x0e4f, 0x0e5b, CharCategory::Bidirectional_L);
    map.PushRange(0x0e81, 0x0e82, CharCategory::Bidirectional_L);
    map.PushMapping(0x0e84, CharCategory::Bidirectional_L);
    map.PushRange(0x0e87, 0x0e88, CharCategory::Bidirectional_L);
    map.PushMapping(0x0e8a, CharCategory::Bidirectional_L);
    map.PushMapping(0x0e8d, CharCategory::Bidirectional_L);
    map.PushRange(0x0e94, 0x0e97, CharCategory::Bidirectional_L);
    map.PushRange(0x0e99, 0x0e9f, CharCategory::Bidirectional_L);
    map.PushRange(0x0ea1, 0x0ea3, CharCategory::Bidirectional_L);
    map.PushMapping(0x0ea5, CharCategory::Bidirectional_L);
    map.PushMapping(0x0ea7, CharCategory::Bidirectional_L);
    map.PushRange(0x0eaa, 0x0eab, CharCategory::Bidirectional_L);
    map.PushRange(0x0ead, 0x0eb0, CharCategory::Bidirectional_L);
    map.PushRange(0x0eb2, 0x0eb3, CharCategory::Bidirectional_L);
    map.PushMapping(0x0ebd, CharCategory::Bidirectional_L);
    map.PushRange(0x0ec0, 0x0ec4, CharCategory::Bidirectional_L);
    map.PushMapping(0x0ec6, CharCategory::Bidirectional_L);
    map.PushRange(0x0ed0, 0x0ed9, CharCategory::Bidirectional_L);
    map.PushRange(0x0edc, 0x0edd, CharCategory::Bidirectional_L);
    map.PushRange(0x0f00, 0x0f17, CharCategory::Bidirectional_L);
    map.PushRange(0x0f1a, 0x0f34, CharCategory::Bidirectional_L);
    map.PushMapping(0x0f36, CharCategory::Bidirectional_L);
    map.PushMapping(0x0f38, CharCategory::Bidirectional_L);
    map.PushRange(0x0f3e, 0x0f47, CharCategory::Bidirectional_L);
    map.PushRange(0x0f49, 0x0f6a, CharCategory::Bidirectional_L);
    map.PushMapping(0x0f7f, CharCategory::Bidirectional_L);
    map.PushMapping(0x0f85, CharCategory::Bidirectional_L);
    map.PushRange(0x0f88, 0x0f8b, CharCategory::Bidirectional_L);
    map.PushRange(0x0fbe, 0x0fc5, CharCategory::Bidirectional_L);
    map.PushRange(0x0fc7, 0x0fcc, CharCategory::Bidirectional_L);
    map.PushMapping(0x0fcf, CharCategory::Bidirectional_L);
    map.PushRange(0x1000, 0x1021, CharCategory::Bidirectional_L);
    map.PushRange(0x1023, 0x1027, CharCategory::Bidirectional_L);
    map.PushRange(0x1029, 0x102a, CharCategory::Bidirectional_L);
    map.PushMapping(0x102c, CharCategory::Bidirectional_L);
    map.PushMapping(0x1031, CharCategory::Bidirectional_L);
    map.PushMapping(0x1038, CharCategory::Bidirectional_L);
    map.PushRange(0x1040, 0x1057, CharCategory::Bidirectional_L);
    map.PushRange(0x10a0, 0x10c5, CharCategory::Bidirectional_L);
    map.PushRange(0x10d0, 0x10f8, CharCategory::Bidirectional_L);
    map.PushMapping(0x10fb, CharCategory::Bidirectional_L);
    map.PushRange(0x1100, 0x1159, CharCategory::Bidirectional_L);
    map.PushRange(0x115f, 0x11a2, CharCategory::Bidirectional_L);
    map.PushRange(0x11a8, 0x11f9, CharCategory::Bidirectional_L);
    map.PushRange(0x1200, 0x1206, CharCategory::Bidirectional_L);
    map.PushRange(0x1208, 0x1246, CharCategory::Bidirectional_L);
    map.PushMapping(0x1248, CharCategory::Bidirectional_L);
    map.PushRange(0x124a, 0x124d, CharCategory::Bidirectional_L);
    map.PushRange(0x1250, 0x1256, CharCategory::Bidirectional_L);
    map.PushMapping(0x1258, CharCategory::Bidirectional_L);
    map.PushRange(0x125a, 0x125d, CharCategory::Bidirectional_L);
    map.PushRange(0x1260, 0x1286, CharCategory::Bidirectional_L);
    map.PushMapping(0x1288, CharCategory::Bidirectional_L);
    map.PushRange(0x128a, 0x128d, CharCategory::Bidirectional_L);
    map.PushRange(0x1290, 0x12ae, CharCategory::Bidirectional_L);
    map.PushMapping(0x12b0, CharCategory::Bidirectional_L);
    map.PushRange(0x12b2, 0x12b5, CharCategory::Bidirectional_L);
    map.PushRange(0x12b8, 0x12be, CharCategory::Bidirectional_L);
    map.PushMapping(0x12c0, CharCategory::Bidirectional_L);
    map.PushRange(0x12c2, 0x12c5, CharCategory::Bidirectional_L);
    map.PushRange(0x12c8, 0x12ce, CharCategory::Bidirectional_L);
    map.PushRange(0x12d0, 0x12d6, CharCategory::Bidirectional_L);
    map.PushRange(0x12d8, 0x12ee, CharCategory::Bidirectional_L);
    map.PushRange(0x12f0, 0x130e, CharCategory::Bidirectional_L);
    map.PushMapping(0x1310, CharCategory::Bidirectional_L);
    map.PushRange(0x1312, 0x1315, CharCategory::Bidirectional_L);
    map.PushRange(0x1318, 0x131e, CharCategory::Bidirectional_L);
    map.PushRange(0x1320, 0x1346, CharCategory::Bidirectional_L);
    map.PushRange(0x1348, 0x135a, CharCategory::Bidirectional_L);
    map.PushRange(0x1361, 0x137c, CharCategory::Bidirectional_L);
    map.PushRange(0x13a0, 0x13f4, CharCategory::Bidirectional_L);
    map.PushRange(0x1401, 0x1676, CharCategory::Bidirectional_L);
    map.PushRange(0x1681, 0x169a, CharCategory::Bidirectional_L);
    map.PushRange(0x16a0, 0x16f0, CharCategory::Bidirectional_L);
    map.PushRange(0x1700, 0x170c, CharCategory::Bidirectional_L);
    map.PushRange(0x170e, 0x1711, CharCategory::Bidirectional_L);
    map.PushRange(0x1720, 0x1731, CharCategory::Bidirectional_L);
    map.PushRange(0x1735, 0x1736, CharCategory::Bidirectional_L);
    map.PushRange(0x1740, 0x1751, CharCategory::Bidirectional_L);
    map.PushRange(0x1760, 0x176c, CharCategory::Bidirectional_L);
    map.PushRange(0x176e, 0x1770, CharCategory::Bidirectional_L);
    map.PushRange(0x1780, 0x17b6, CharCategory::Bidirectional_L);
    map.PushRange(0x17be, 0x17c5, CharCategory::Bidirectional_L);
    map.PushRange(0x17c7, 0x17c8, CharCategory::Bidirectional_L);
    map.PushRange(0x17d4, 0x17da, CharCategory::Bidirectional_L);
    map.PushMapping(0x17dc, CharCategory::Bidirectional_L);
    map.PushRange(0x17e0, 0x17e9, CharCategory::Bidirectional_L);
    map.PushRange(0x1810, 0x1819, CharCategory::Bidirectional_L);
    map.PushRange(0x1820, 0x1877, CharCategory::Bidirectional_L);
    map.PushRange(0x1880, 0x18a8, CharCategory::Bidirectional_L);
    map.PushRange(0x1e00, 0x1e9b, CharCategory::Bidirectional_L);
    map.PushRange(0x1ea0, 0x1ef9, CharCategory::Bidirectional_L);
    map.PushRange(0x1f00, 0x1f15, CharCategory::Bidirectional_L);
    map.PushRange(0x1f18, 0x1f1d, CharCategory::Bidirectional_L);
    map.PushRange(0x1f20, 0x1f45, CharCategory::Bidirectional_L);
    map.PushRange(0x1f48, 0x1f4d, CharCategory::Bidirectional_L);
    map.PushRange(0x1f50, 0x1f57, CharCategory::Bidirectional_L);
    map.PushMapping(0x1f59, CharCategory::Bidirectional_L);
    map.PushMapping(0x1f5b, CharCategory::Bidirectional_L);
    map.PushMapping(0x1f5d, CharCategory::Bidirectional_L);
    map.PushRange(0x1f5f, 0x1f7d, CharCategory::Bidirectional_L);
    map.PushRange(0x1f80, 0x1fb4, CharCategory::Bidirectional_L);
    map.PushRange(0x1fb6, 0x1fbc, CharCategory::Bidirectional_L);
    map.PushMapping(0x1fbe, CharCategory::Bidirectional_L);
    map.PushRange(0x1fc2, 0x1fc4, CharCategory::Bidirectional_L);
    map.PushRange(0x1fc6, 0x1fcc, CharCategory::Bidirectional_L);
    map.PushRange(0x1fd0, 0x1fd3, CharCategory::Bidirectional_L);
    map.PushRange(0x1fd6, 0x1fdb, CharCategory::Bidirectional_L);
    map.PushRange(0x1fe0, 0x1fec, CharCategory::Bidirectional_L);
    map.PushRange(0x1ff2, 0x1ff4, CharCategory::Bidirectional_L);
    map.PushRange(0x1ff6, 0x1ffc, CharCategory::Bidirectional_L);
    //map.PushMapping(0x200e, CharCategory::Bidirectional_L);           // NOTE: These are also prohibited
    map.PushMapping(0x2071, CharCategory::Bidirectional_L);
    map.PushMapping(0x207f, CharCategory::Bidirectional_L);
    map.PushMapping(0x2102, CharCategory::Bidirectional_L);
    map.PushMapping(0x2107, CharCategory::Bidirectional_L);
    map.PushRange(0x210a, 0x2113, CharCategory::Bidirectional_L);
    map.PushMapping(0x2115, CharCategory::Bidirectional_L);
    map.PushRange(0x2119, 0x211d, CharCategory::Bidirectional_L);
    map.PushMapping(0x2124, CharCategory::Bidirectional_L);
    map.PushMapping(0x2126, CharCategory::Bidirectional_L);
    map.PushMapping(0x2128, CharCategory::Bidirectional_L);
    map.PushRange(0x212a, 0x212d, CharCategory::Bidirectional_L);
    map.PushRange(0x212f, 0x2131, CharCategory::Bidirectional_L);
    map.PushRange(0x2133, 0x2139, CharCategory::Bidirectional_L);
    map.PushRange(0x213d, 0x213f, CharCategory::Bidirectional_L);
    map.PushRange(0x2145, 0x2149, CharCategory::Bidirectional_L);
    map.PushRange(0x2160, 0x2183, CharCategory::Bidirectional_L);
    map.PushRange(0x2336, 0x237a, CharCategory::Bidirectional_L);
    map.PushMapping(0x2395, CharCategory::Bidirectional_L);
    map.PushRange(0x249c, 0x24e9, CharCategory::Bidirectional_L);
    map.PushRange(0x3005, 0x3007, CharCategory::Bidirectional_L);
    map.PushRange(0x3021, 0x3029, CharCategory::Bidirectional_L);
    map.PushRange(0x3031, 0x3035, CharCategory::Bidirectional_L);
    map.PushRange(0x3038, 0x303c, CharCategory::Bidirectional_L);
    map.PushRange(0x3041, 0x3096, CharCategory::Bidirectional_L);
    map.PushRange(0x309d, 0x309f, CharCategory::Bidirectional_L);
    map.PushRange(0x30a1, 0x30fa, CharCategory::Bidirectional_L);
    map.PushRange(0x30fc, 0x30ff, CharCategory::Bidirectional_L);
    map.PushRange(0x3105, 0x312c, CharCategory::Bidirectional_L);
    map.PushRange(0x3131, 0x318e, CharCategory::Bidirectional_L);
    map.PushRange(0x3190, 0x31b7, CharCategory::Bidirectional_L);
    map.PushRange(0x31f0, 0x321c, CharCategory::Bidirectional_L);
    map.PushRange(0x3220, 0x3243, CharCategory::Bidirectional_L);
    map.PushRange(0x3260, 0x327b, CharCategory::Bidirectional_L);
    map.PushRange(0x327f, 0x32b0, CharCategory::Bidirectional_L);
    map.PushRange(0x32c0, 0x32cb, CharCategory::Bidirectional_L);
    map.PushRange(0x32d0, 0x32fe, CharCategory::Bidirectional_L);
    map.PushRange(0x3300, 0x3376, CharCategory::Bidirectional_L);
    map.PushRange(0x337b, 0x33dd, CharCategory::Bidirectional_L);
    map.PushRange(0x33e0, 0x33fe, CharCategory::Bidirectional_L);
    map.PushRange(0x3400, 0x4db5, CharCategory::Bidirectional_L);
    map.PushRange(0x4e00, 0x9fa5, CharCategory::Bidirectional_L);
    map.PushRange(0xa000, 0xa48c, CharCategory::Bidirectional_L);
    map.PushRange(0xac00, 0xd7a3, CharCategory::Bidirectional_L);
    //map.PushRange(0xe000, 0xf8ff, CharCategory::Bidirectional_L);     // NOTE: These are also prohibited. Also Don't include surrogates
    map.PushRange(0xf900, 0xfa2d, CharCategory::Bidirectional_L);
    map.PushRange(0xfa30, 0xfa6a, CharCategory::Bidirectional_L);
    map.PushRange(0xfb00, 0xfb06, CharCategory::Bidirectional_L);
    map.PushRange(0xfb13, 0xfb17, CharCategory::Bidirectional_L);
    map.PushRange(0xff21, 0xff3a, CharCategory::Bidirectional_L);
    map.PushRange(0xff41, 0xff5a, CharCategory::Bidirectional_L);
    map.PushRange(0xff66, 0xffbe, CharCategory::Bidirectional_L);
    map.PushRange(0xffc2, 0xffc7, CharCategory::Bidirectional_L);
    map.PushRange(0xffca, 0xffcf, CharCategory::Bidirectional_L);
    map.PushRange(0xffd2, 0xffd7, CharCategory::Bidirectional_L);
    map.PushRange(0xffda, 0xffdc, CharCategory::Bidirectional_L);
    map.PushRange(0x10300, 0x1031e, CharCategory::Bidirectional_L);
    map.PushRange(0x10320, 0x10323, CharCategory::Bidirectional_L);
    map.PushRange(0x10330, 0x1034a, CharCategory::Bidirectional_L);
    map.PushRange(0x10400, 0x10425, CharCategory::Bidirectional_L);
    map.PushRange(0x10428, 0x1044d, CharCategory::Bidirectional_L);
    map.PushRange(0x1d000, 0x1d0f5, CharCategory::Bidirectional_L);
    map.PushRange(0x1d100, 0x1d126, CharCategory::Bidirectional_L);
    map.PushRange(0x1d12a, 0x1d166, CharCategory::Bidirectional_L);
    map.PushRange(0x1d16a, 0x1d172, CharCategory::Bidirectional_L);
    map.PushRange(0x1d183, 0x1d184, CharCategory::Bidirectional_L);
    map.PushRange(0x1d18c, 0x1d1a9, CharCategory::Bidirectional_L);
    map.PushRange(0x1d1ae, 0x1d1dd, CharCategory::Bidirectional_L);
    map.PushRange(0x1d400, 0x1d454, CharCategory::Bidirectional_L);
    map.PushRange(0x1d456, 0x1d49c, CharCategory::Bidirectional_L);
    map.PushRange(0x1d49e, 0x1d49f, CharCategory::Bidirectional_L);
    map.PushMapping(0x1d4a2, CharCategory::Bidirectional_L);
    map.PushRange(0x1d4a5, 0x1d4a6, CharCategory::Bidirectional_L);
    map.PushRange(0x1d4a9, 0x1d4ac, CharCategory::Bidirectional_L);
    map.PushRange(0x1d4ae, 0x1d4b9, CharCategory::Bidirectional_L);
    map.PushMapping(0x1d4bb, CharCategory::Bidirectional_L);
    map.PushRange(0x1d4bd, 0x1d4c0, CharCategory::Bidirectional_L);
    map.PushRange(0x1d4c2, 0x1d4c3, CharCategory::Bidirectional_L);
    map.PushRange(0x1d4c5, 0x1d505, CharCategory::Bidirectional_L);
    map.PushRange(0x1d507, 0x1d50a, CharCategory::Bidirectional_L);
    map.PushRange(0x1d50d, 0x1d514, CharCategory::Bidirectional_L);
    map.PushRange(0x1d516, 0x1d51c, CharCategory::Bidirectional_L);
    map.PushRange(0x1d51e, 0x1d539, CharCategory::Bidirectional_L);
    map.PushRange(0x1d53b, 0x1d53e, CharCategory::Bidirectional_L);
    map.PushRange(0x1d540, 0x1d544, CharCategory::Bidirectional_L);
    map.PushMapping(0x1d546, CharCategory::Bidirectional_L);
    map.PushRange(0x1d54a, 0x1d550, CharCategory::Bidirectional_L);
    map.PushRange(0x1d552, 0x1d6a3, CharCategory::Bidirectional_L);
    map.PushRange(0x1d6a8, 0x1d7c9, CharCategory::Bidirectional_L);
    map.PushRange(0x20000, 0x2a6d6, CharCategory::Bidirectional_L);
    map.PushRange(0x2f800, 0x2fa1d, CharCategory::Bidirectional_L);
    //map.PushRange(0xf0000, 0xffffd, CharCategory::Bidirectional_L);   // NOTE: These are also prohibited
    //map.PushRange(0x100000, 0x10fffd, CharCategory::Bidirectional_L); // NOTE: These are also prohibited

}

bool tryNormalizeNFKC(const vector<char32_t>& codePoints, vector<char32_t>& normalized)
{
    normalized.clear();
    normalized.reserve(codePoints.size());

    char32_t buff[8];
    utf8proc_ssize_t rc;
    int lastBoundClass;
    for (size_t i = 0; i < codePoints.size(); i++)
    {
        // NOTE: UTF8PROC_DECOMPOSE is undocumented for utf8proc_decompose_char but it's necessary
        rc = utf8proc_decompose_char(codePoints[i], (utf8proc_int32_t*)buff, std::size(buff),
            (utf8proc_option_t)(UTF8PROC_DECOMPOSE | UTF8PROC_COMPAT), &lastBoundClass);
        if (rc < 0 || (size_t)rc > std::size(buff))
            goto Fail;

        normalized.insert(normalized.end(), buff, buff + rc);
    }

    rc = utf8proc_normalize_utf32((utf8proc_int32_t*)normalized.data(),
        (utf8proc_ssize_t)normalized.size(), (utf8proc_option_t)(UTF8PROC_COMPOSE | UTF8PROC_STABLE));

    if (rc < 0)
        goto Fail;

    normalized.resize((size_t)rc);
    return true;

Fail:
    normalized.clear();
    return false;
}

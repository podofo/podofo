/**
 * SPDX-FileCopyrightText: (C) 2025 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <type_traits>
#include <set>
#include <unordered_map>

namespace sprep
{
    enum class CharCategory : uint8_t
    {
        Unknown = 0,
        UnassignedCodePoints,
        CommonlyMappedToNothing,
        NonASCIISpaceCharacters,
        ProhibitedCharacters,
        Bidirectional_R_AL,
        Bidirectional_L,
    };

    struct CharCategoryRange final
    {
        char32_t RangeLo{ };
        unsigned Size = 0;
        CharCategory Value{ };

        char32_t GetRangeHi() const
        {
            return RangeLo + Size;
        }
    };

    struct CharCategoryRangeInequality
    {
        using is_transparent = std::true_type;

        bool operator()(const CharCategoryRange& lhs, const char32_t rhs) const
        {
            return lhs.RangeLo < rhs;
        }
        bool operator()(const char32_t lhs, const CharCategoryRange& rhs) const
        {
            return lhs < rhs.RangeLo;
        }
        bool operator()(const CharCategoryRange& lhs, const CharCategoryRange& rhs) const
        {
            return lhs.RangeLo < rhs.RangeLo;
        }
    };

    class CharCategoryMap final
    {
    public:
        using HashMap = std::unordered_map<char32_t, CharCategory>;
        using RangeMap = std::set<CharCategoryRange, CharCategoryRangeInequality>;

    public:
        CharCategoryMap();

        CharCategoryMap(HashMap&& mappings, RangeMap&& ranges);

    public:
        void PushMapping(char32_t key, CharCategory category);

        void PushRange(char32_t rangeLo, char32_t rangeHi, CharCategory category);

        bool TryGetValue(char32_t key, CharCategory& category) const;

    public:
        const HashMap& GetMappings() const { return m_Mappings; }
        const RangeMap& GetRanges() const { return m_Ranges; }

    private:
        HashMap m_Mappings;
        RangeMap m_Ranges;
    };
}

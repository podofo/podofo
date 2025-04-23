/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfCharCodeMap.h"
#include <random>
#include <algorithm>

#include <podofo/private/PdfEncodingPrivate.h>

using namespace std;
using namespace PoDoFo;

namespace
{
    /// <summary>
    /// A temporary structure used to compute CodeSpaceRange(s)
    /// </summary>
    struct MappingRange
    {
        PdfCharCode SrcCodeLo;
        unsigned Size;

        MappingRange(PdfCharCode srcCodeLo, unsigned size);

        PdfCharCode GetSrcCodeHi() const;

        bool operator<(const MappingRange& rhs) const;
    };
}

static void appendRangesTo(vector<pair<PdfCharCode, CodePointSpan>>& mapppings, const CodeUnitMap& mappings, const CodeUnitRanges ranges);
static void fetchCodePoints(vector<codepoint>& codePoints, const PdfCharCode& code, const CodeUnitRange& range);
static void fetchCodePoints(CodePointSpan& codePoints, const PdfCharCode& code, const CodeUnitRange& range);
static void updateCodeSpaceRangeLoHi(unsigned refCodeLo, unsigned refCodeHi, unsigned char codeSpaceSize,
    unsigned& codeLo, unsigned& codeHi);

PdfCharCodeMap::PdfCharCodeMap()
    : m_MapDirty(false), m_codePointMapHead(nullptr) { }

PdfCharCodeMap::PdfCharCodeMap(PdfCharCodeMap&& map) noexcept
{
    move(map);
}

PdfCharCodeMap::~PdfCharCodeMap()
{
    PoDoFo::DeleteNodeReverseMap(m_codePointMapHead);
}

PdfCharCodeMap& PdfCharCodeMap::operator=(PdfCharCodeMap&& map) noexcept
{
    move(map);
    return *this;
}

PdfCharCodeMap::PdfCharCodeMap(CodeUnitMap&& mappings, CodeUnitRanges&& ranges, const PdfEncodingLimits& limits)
    : m_Limits(limits), m_Mappings(std::move(mappings)), m_Ranges(std::move(ranges)), m_MapDirty(true), m_codePointMapHead(nullptr)
{
}

bool PdfCharCodeMap::IsEmpty() const
{
    return m_Mappings.empty() && m_Ranges.empty();
}

bool PdfCharCodeMap::IsTrivialIdentity() const
{
    // CHECK-ME: Should we do it this way? Maybe we should support
    // only full code ranges identities. Like <00><FF>, or <0000><FFFF>

    // We look first if we can look just at straight mappings
    if (m_Mappings.size() != 0)
    {
        // If we also have ranges, then it's definetely not trivial
        if (m_Ranges.size() != 0)
            return false;

        // Determine the range of the current mappings
        unsigned rangeSize = m_Limits.LastChar.Code - m_Limits.FirstChar.Code + 1;
        if (m_Mappings.size() != rangeSize)
            return false;

        // Ensure the mappings are an identity
        auto it = m_Mappings.begin();
        auto end = m_Mappings.end();
        unsigned prev = it->first.Code - 1;
        do
        {
            if (it->second.GetSize() > 1
                || it->first.Code != *it->second
                || it->first.Code > (prev + 1))
            {
                return false;
            }

            prev = it->first.Code;
            it++;
        } while (it != end);

        // If there are no discontinuities then it's an identity
        return true;
    }

    if (m_Ranges.size() != 0)
    {
        unsigned rangeUpper = numeric_limits<unsigned>::max();
        auto it = m_Ranges.begin();
        do
        {
            if (rangeUpper < it->SrcCodeLo.Code)
            {
                // If the ranges are not continuous
                // then it's not an identity
                return false;
            }

            rangeUpper = it->SrcCodeLo.Code + it->Size;
        } while (++it != m_Ranges.end());

        // If there are no discontinuities then it's an identity
        return true;
    }

    // If the map is empty, treat it as not an identity
    return false;
}

vector<CodeSpaceRange> PdfCharCodeMap::GetCodeSpaceRanges() const
{
    set<MappingRange> ranges;
    for (auto& pair : m_Mappings)
        ranges.emplace(pair.first, 1);

    for (auto& range : m_Ranges)
        ranges.emplace(range.SrcCodeLo, range.Size);

    vector<CodeSpaceRange> ret;
    auto it = ranges.begin();
    auto end = ranges.end();
    if (it == end)
        return ret;

    PdfCharCode prevCodeHi = it->GetSrcCodeHi();
    while (++it != end)
    {
        auto& range = *it;
        if (range.SrcCodeLo.CodeSpaceSize != prevCodeHi.CodeSpaceSize && range.SrcCodeLo.GetByteCode(0) <= prevCodeHi.GetByteCode(0))
        {
            // TODO1 Fix overlapping ranges by splitting existing 2-byte, 3-byte, 4-byte ranges
            PoDoFo::LogMessage(PdfLogSeverity::Warning, "Overlapping CodeSpaceRange");
        }
        prevCodeHi = range.GetSrcCodeHi();
    }

    // Merge all smae code space size ranges
    it = ranges.begin();
    end = ranges.end();
    ret.push_back(CodeSpaceRange{ it->SrcCodeLo.Code, it->GetSrcCodeHi().Code, it->SrcCodeLo.CodeSpaceSize });
    auto prevCodeSpaceRange = &ret.back();
    while (++it != end)
    {
        auto& range = *it;
        if (range.SrcCodeLo.CodeSpaceSize == prevCodeSpaceRange->CodeSpaceSize)
        {
            updateCodeSpaceRangeLoHi(range.SrcCodeLo.Code, range.GetSrcCodeHi().Code, range.SrcCodeLo.CodeSpaceSize,
                prevCodeSpaceRange->CodeLo, prevCodeSpaceRange->CodeHi);
        }
        else
        {
            ret.push_back(CodeSpaceRange{ range.SrcCodeLo.Code, range.GetSrcCodeHi().Code, range.SrcCodeLo.CodeSpaceSize });
            prevCodeSpaceRange = &ret.back();
        }
    }

    // TODO2: Fix byte1, byte2, byte3 possible overlappings. This
    // time we can just restrict ranges on subsequent bytes
    return ret;
}

void PdfCharCodeMap::move(PdfCharCodeMap& map) noexcept
{
    m_Mappings = std::move(map.m_Mappings);
    m_Ranges = std::move(map.m_Ranges);
    utls::move(map.m_Limits, m_Limits);
    utls::move(map.m_MapDirty, m_MapDirty);
    utls::move(map.m_codePointMapHead, m_codePointMapHead);
}

void PdfCharCodeMap::PushMapping(const PdfCharCode& codeUnit, const codepointview& codePoints)
{
    if (codePoints.size() == 0)
        return;

    pushMapping(codeUnit, codePoints);
}

void PdfCharCodeMap::PushMapping(const PdfCharCode& codeUnit, codepoint codePoint)
{
    codepointview codePoints = { &codePoint, 1 };
    pushMapping(codeUnit, codePoints);
}

void PdfCharCodeMap::PushRange(const PdfCharCode& srcCodeLo, unsigned size, codepoint dstCodeLo)
{
    PushRange(srcCodeLo, size, { &dstCodeLo, 1 });
}

void PdfCharCodeMap::PushRange(const PdfCharCode& srcCodeLo, unsigned rangeSize, const codepointview& dstCodeLo)
{
    if (rangeSize == 0 || dstCodeLo.size() == 0)
        return;

    if (rangeSize == 1)
    {
        // Avoid pushing a proper range if it's size 1. Push it at a single mapping
        pushMapping(srcCodeLo, vector<codepoint>(dstCodeLo.begin(), dstCodeLo.end()));
        return;
    }

    auto inserted = m_Ranges.emplace(srcCodeLo, rangeSize, CodePointSpan(dstCodeLo));
    // Try fix invalid ranges: the inserted range
    // always overrides previous ones
    bool invalidRanges = false;
    if (inserted.second)
    {
        auto it = inserted.first;
        if (it != m_Ranges.begin() && ((--it)->SrcCodeLo.Code + it->Size) > srcCodeLo.Code)
        {
            // Previous range overlaps new one
            invalidRanges = true;
            unsigned newSize = srcCodeLo.Code - it->SrcCodeLo.Code;
            auto node = m_Ranges.extract(it);
            // If the new size of the previous node
            // is valid update it and reinsert the node
            if (newSize != 0)
            {
                node.value().Size = newSize;
                m_Ranges.insert(inserted.first, std::move(node));
            }
        }

        invalidRanges |= tryFixNextRanges(inserted.first, srcCodeLo.Code + rangeSize);
    }
    else
    {
        if (inserted.first->Size < rangeSize)
        {
            // Prepare an hint for re-insertion
            auto it = std::prev(inserted.first);
            // If the current range with same srcCodeLo has a
            // size lesser than the one being inserted update it
            invalidRanges = true;
            auto node = m_Ranges.extract(inserted.first);
            node.value().Size = rangeSize;
            it = m_Ranges.insert(it, std::move(node));
            (void)tryFixNextRanges(it, srcCodeLo.Code + rangeSize);
        }
    }

    if (invalidRanges)
        PoDoFo::LogMessage(PdfLogSeverity::Warning, "Overlapping code unit ranges found");

    updateLimits(srcCodeLo);
    auto srcCodeHi = PdfCharCode(srcCodeLo.Code + rangeSize - 1, srcCodeLo.CodeSpaceSize);
    if (srcCodeHi.Code < m_Limits.LastChar.Code)
        m_Limits.LastChar = srcCodeHi;
    if (srcCodeHi.Code > m_Limits.LastChar.Code)
        m_Limits.LastChar = srcCodeHi;

    m_MapDirty = true;
}

bool PdfCharCodeMap::TryGetCodePoints(const PdfCharCode& codeUnit, CodePointSpan& codePoints) const
{
    // Try to find direct mapppings first
    auto found = m_Mappings.find(codeUnit);
    if (found != m_Mappings.end())
    {
        codePoints = found->second;
        return true;
    }

    // If not match on the direct mappings, try to find in the
    // ranges. Find the range with lower code <= of the searched
    // unit and verify if the range includes it
    auto foundRange = m_Ranges.upper_bound(codeUnit);
    if (foundRange == m_Ranges.begin() || codeUnit.Code >= ((--foundRange)->SrcCodeLo.Code + foundRange->Size))
    {
        codePoints = { };
        return false;
    }

    fetchCodePoints(codePoints, codeUnit, *foundRange);
    return true;
}

bool PdfCharCodeMap::TryGetNextCharCode(string_view::iterator& it, const string_view::iterator& end, PdfCharCode& code) const
{
    const_cast<PdfCharCodeMap&>(*this).reviseCodePointMap();
    return PoDoFo::TryGetCodeReverseMap(m_codePointMapHead, it, end, code);
}

bool PdfCharCodeMap::TryGetCharCode(const codepointview& codePoints, PdfCharCode& code) const
{
    const_cast<PdfCharCodeMap&>(*this).reviseCodePointMap();
    return PoDoFo::TryGetCodeReverseMap(m_codePointMapHead, codePoints, code);
}

bool PdfCharCodeMap::TryGetCharCode(codepoint codePoint, PdfCharCode& code) const
{
    const_cast<PdfCharCodeMap&>(*this).reviseCodePointMap();
    return PoDoFo::TryGetCodeReverseMap(m_codePointMapHead, codePoint, code);
}

void PdfCharCodeMap::pushMapping(const PdfCharCode& codeUnit, const codepointview& codePoints)
{
    if (codeUnit.CodeSpaceSize == 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Code unit must be valid");

    m_Mappings[codeUnit] = CodePointSpan(codePoints);

    // Update limits
    updateLimits(codeUnit);
    m_MapDirty = true;
}

void PdfCharCodeMap::updateLimits(const PdfCharCode& codeUnit)
{
    if (codeUnit.CodeSpaceSize < m_Limits.MinCodeSize)
        m_Limits.MinCodeSize = codeUnit.CodeSpaceSize;
    if (codeUnit.CodeSpaceSize > m_Limits.MaxCodeSize)
        m_Limits.MaxCodeSize = codeUnit.CodeSpaceSize;
    if (codeUnit.Code < m_Limits.FirstChar.Code)
        m_Limits.FirstChar = codeUnit;
    if (codeUnit.Code > m_Limits.LastChar.Code)
        m_Limits.LastChar = codeUnit;
}

// Try to rebuild the inverse code point -> char code
void PdfCharCodeMap::reviseCodePointMap()
{
    if (!m_MapDirty)
        return;

    if (m_codePointMapHead != nullptr)
    {
        PoDoFo::DeleteNodeReverseMap(m_codePointMapHead);
        m_codePointMapHead = nullptr;
    }

    vector<pair<PdfCharCode, CodePointSpan>> mappings;
    mappings.reserve(m_Mappings.size());
    std::copy(m_Mappings.begin(), m_Mappings.end(), std::back_inserter(mappings));
    appendRangesTo(mappings, m_Mappings, m_Ranges);

    // Randomize items in the map in a separate list
    // so BST creation will be more balanced
    // https://en.wikipedia.org/wiki/Random_binary_tree
    // TODO: Create a perfectly balanced BST
    std::mt19937 e(random_device{}());
    std::shuffle(mappings.begin(), mappings.end(), e);

    for (auto& pair : mappings)
        PoDoFo::PushMappingReverseMap(m_codePointMapHead, pair.second.view(), pair.first);

    m_MapDirty = false;
}

// Returns true if there are invalid ranges
bool PdfCharCodeMap::tryFixNextRanges(const CodeUnitRanges::iterator& it, unsigned prevRangeCodeUpper)
{
    auto prev = it;
    auto curr = std::next(prev);
    bool hasInvalidRanges = false;
    while (curr != m_Ranges.end())
    {
        // Try to find nodes with overlapping range
        if (prevRangeCodeUpper > curr->SrcCodeLo.Code)
        {
            // The current range is invalid, extract it
            hasInvalidRanges = true;
            auto node = m_Ranges.extract(curr);
            auto currRangeCodeLower = node.value().SrcCodeLo;
            unsigned currRangeLo = currRangeCodeLower.Code + node.value().Size;
            if (prevRangeCodeUpper <= currRangeLo)
            {
                // Only current node needs fixing, evaluate if
                // will be still valid after fixing
                unsigned newsize = currRangeLo - prevRangeCodeUpper;
                if (newsize != 0)
                {
                    // The fixed range is valid, reinsert the node
                    node.value().SrcCodeLo = PdfCharCode(prevRangeCodeUpper, currRangeCodeLower.CodeSpaceSize);
                    node.value().Size = newsize;
                    m_Ranges.insert(prev, std::move(node));
                }

                // We either fixed or removed the current
                // invalid range, we can quit
                break;
            }
        }
        else
        {
            // Else stop search
            break;
        }

        // Increment previous iterator, which didn't change
        curr = std::next(prev);
    }

    return hasInvalidRanges;
}

// Append mappings coming from ranges, excluding the ones
// that are already directly mapped
void appendRangesTo(vector<pair<PdfCharCode, CodePointSpan>>& allMapppings,
    const CodeUnitMap& mappings, const CodeUnitRanges ranges)
{
    PdfCharCode code;
    vector<codepoint> codePoints;
    for (auto& range : ranges)
    {
        for (unsigned i = 0; i < range.Size; i++)
        {
            code = PdfCharCode(range.SrcCodeLo.Code + i, range.SrcCodeLo.CodeSpaceSize);
            // Skip the mapping if it's already mapped by the straight map
            if (mappings.find(code) != mappings.end())
                continue;

            fetchCodePoints(codePoints, code, range);
            allMapppings.push_back({ code, CodePointSpan(codePoints) });
        }
    }
}

// Fetch codepoints from range for the given code
void fetchCodePoints(vector<codepoint>& codePoints, const PdfCharCode& code, const CodeUnitRange& range)
{
    range.DstCodeLo.CopyTo(codePoints);
    unsigned codeDiff = code.Code - range.SrcCodeLo.Code;
    if (codeDiff > 0)
    {
        PODOFO_INVARIANT(codePoints.size() != 0);
        auto newcode = (codepoint)((unsigned)codePoints.back() + codeDiff);
        codePoints[codePoints.size() - 1] = newcode;
    }
}

void fetchCodePoints(CodePointSpan& codePoints, const PdfCharCode& code, const CodeUnitRange& range)
{
    unsigned codeDiff = code.Code - range.SrcCodeLo.Code;
    if (codeDiff > 0)
    {
        auto dstCodeLo = range.DstCodeLo.view();
        PODOFO_INVARIANT(dstCodeLo.size() != 0);
        auto newcode = (codepoint)((unsigned)dstCodeLo.back() + codeDiff);
        codePoints = CodePointSpan(dstCodeLo.subspan(0, dstCodeLo.size() - 1), newcode);
    }
    else
    {
        codePoints = range.DstCodeLo;
    }
}

CodeUnitRange::CodeUnitRange()
    : Size(0) { }

CodeUnitRange::CodeUnitRange(PdfCharCode srcCodeLo, unsigned size, CodePointSpan dstCodeLo)
    : SrcCodeLo(srcCodeLo), Size(size), DstCodeLo(dstCodeLo) { }

PdfCharCode CodeUnitRange::GetSrcCodeHi() const
{
    return PdfCharCode(SrcCodeLo.Code + Size - 1, SrcCodeLo.CodeSpaceSize);
}

CodeSpaceRange::CodeSpaceRange()
    : CodeLo(std::numeric_limits<unsigned>::max()), CodeHi(0), CodeSpaceSize(0) { }

CodeSpaceRange::CodeSpaceRange(unsigned codeLo, unsigned codeHi, unsigned char codeSpaceSize)
    : CodeLo(codeLo), CodeHi(codeHi), CodeSpaceSize(codeSpaceSize) { }

PdfCharCode CodeSpaceRange::GetSrcCodeLo() const
{
    return PdfCharCode(CodeLo, CodeSpaceSize);
}

PdfCharCode CodeSpaceRange::GetSrcCodeHi() const
{
    return PdfCharCode(CodeHi, CodeSpaceSize);
}

MappingRange::MappingRange(PdfCharCode srcCodeLo, unsigned size)
    : SrcCodeLo(srcCodeLo), Size(size) { }

PdfCharCode MappingRange::GetSrcCodeHi() const
{
    return PdfCharCode(SrcCodeLo.Code + Size - 1);
}

bool MappingRange::operator<(const MappingRange& rhs) const
{
    // Order ranges basing on subsequent bytes of the lower code.
    // If current code byte is same for both ranges, we analyze
    // the next one. If all bytes were equal, try to compare
    // code space sizes
    unsigned char minCodeSpaceSize = std::min(SrcCodeLo.CodeSpaceSize, rhs.SrcCodeLo.CodeSpaceSize);
    for (unsigned char i = 0; i < minCodeSpaceSize; i++)
    {
        unsigned lhsByteCode = SrcCodeLo.GetByteCode(i);
        unsigned rhsByteCode = rhs.SrcCodeLo.GetByteCode(i);
        if (lhsByteCode == rhsByteCode)
            continue;

        return lhsByteCode < rhsByteCode;
    }

    return SrcCodeLo.CodeSpaceSize < rhs.SrcCodeLo.CodeSpaceSize;
}

// Iterate all the bytes of the codes and pick the minimum/maximum of each byte
void updateCodeSpaceRangeLoHi(unsigned refCodeLo, unsigned refCodeHi, unsigned char codeSpaceSize,
    unsigned& codeLo, unsigned& codeHi)
{
    unsigned currCodeLo = codeLo;
    unsigned currCodeHi = codeHi;
    unsigned currByteCode;
    unsigned refByteCode;
    unsigned mask;
    for (unsigned char i = 0; i < codeSpaceSize; i++)
    {
        // Create a mask to clear the target byte
        mask = 0xFF << i * CHAR_BIT;

        currByteCode = (currCodeLo >> i * CHAR_BIT) & 0xFFU;
        refByteCode = (refCodeLo >> i * CHAR_BIT) & 0xFFU;
        if (refByteCode < currByteCode)
            codeLo = (codeLo & ~mask) | (refByteCode << i * CHAR_BIT);

        currByteCode = (currCodeHi >> i * CHAR_BIT) & 0xFFU;
        refByteCode = (refCodeHi >> i * CHAR_BIT) & 0xFFU;
        if (refByteCode > currByteCode)
            codeHi = (codeHi & ~mask) | (refByteCode << i * CHAR_BIT);
    }
}

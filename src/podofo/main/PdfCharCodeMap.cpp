/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfCharCodeMap.h"
#include <random>
#include <algorithm>
#include <utf8cpp/utf8.h>

using namespace std;
using namespace PoDoFo;

static void appendRangesTo(vector<pair<PdfCharCode, CodePointSpan>>& mapppings, const CodeUnitMap& mappings, const CodeUnitRanges ranges);
static void fetchCodePoints(vector<codepoint>& codePoints, const PdfCharCode& code, const CodeUnitRange& range);
static void fetchCodePoints(CodePointSpan& codePoints, const PdfCharCode& code, const CodeUnitRange& range);
static void pushCodeRangeSize(vector<unsigned char>& codeRangeSizes, unsigned char codeRangeSize);

PdfCharCodeMap::PdfCharCodeMap()
    : m_MapDirty(false), m_codePointMapHead(nullptr) { }

PdfCharCodeMap::PdfCharCodeMap(PdfCharCodeMap&& map) noexcept
{
    move(map);
}

PdfCharCodeMap::~PdfCharCodeMap()
{
    deleteNode(m_codePointMapHead);
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

vector<unsigned char> PdfCharCodeMap::GetCodeRangeSizes() const
{
    vector<unsigned char> ret;
    for (auto& pair : m_Mappings)
        pushCodeRangeSize(ret, pair.first.CodeSpaceSize);

    for (auto& range : m_Ranges)
        pushCodeRangeSize(ret, range.SrcCodeLo.CodeSpaceSize);

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

    auto inserted = m_Ranges.emplace(CodeUnitRange{ srcCodeLo, rangeSize, CodePointSpan(dstCodeLo) });
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
        auto it = inserted.first;
        if (it->Size < rangeSize)
        {
            // If the current range with same srcCodeLo has a
            // size lesser than the one being inserted update it
            invalidRanges = true;
            auto node = m_Ranges.extract(it);
            node.value().Size = rangeSize;
            m_Ranges.insert(inserted.first, std::move(node));
            (void)tryFixNextRanges(inserted.first, srcCodeLo.Code + rangeSize);
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
    return tryFindNextCharacterId(m_codePointMapHead, it, end, code);
}

bool PdfCharCodeMap::TryGetCharCode(const codepointview& codePoints, PdfCharCode& codeUnit) const
{
    const_cast<PdfCharCodeMap&>(*this).reviseCodePointMap();
    auto it = codePoints.begin();
    auto end = codePoints.end();
    const CodePointMapNode* node = m_codePointMapHead;
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

bool PdfCharCodeMap::TryGetCharCode(codepoint codePoint, PdfCharCode& code) const
{
    const_cast<PdfCharCodeMap&>(*this).reviseCodePointMap();
    auto node = findNode(m_codePointMapHead, codePoint);
    if (node == nullptr)
    {
        code = { };
        return false;
    }

    code = node->CodeUnit;
    return true;
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

bool PdfCharCodeMap::tryFindNextCharacterId(const CodePointMapNode* node, string_view::iterator& it,
    const string_view::iterator& end, PdfCharCode& codeUnit)
{
    PODOFO_INVARIANT(it != end);
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
        if (tryFindNextCharacterId(node->Ligatures, curr, end, codeUnit))
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

const PdfCharCodeMap::CodePointMapNode* PdfCharCodeMap::findNode(const CodePointMapNode* node, codepoint codePoint)
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
        deleteNode(m_codePointMapHead);
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
    {
        CodePointMapNode** curr = &m_codePointMapHead;      // Node root being searched
        CodePointMapNode* found;                            // Last found node
        auto codepoints = pair.second.view();
        auto it = codepoints.begin();
        auto end = codepoints.end();
        PODOFO_INVARIANT(it != end);
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
        found->CodeUnit = pair.first;
    }

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

PdfCharCodeMap::CodePointMapNode* PdfCharCodeMap::findOrAddNode(CodePointMapNode*& node, codepoint codePoint)
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

void PdfCharCodeMap::deleteNode(CodePointMapNode* node)
{
    if (node == nullptr)
        return;

    deleteNode(node->Ligatures);
    deleteNode(node->Left);
    deleteNode(node->Right);
    delete node;
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

void pushCodeRangeSize(vector<unsigned char>& codeRangeSizes, unsigned char codeRangeSize)
{
    auto found = std::find(codeRangeSizes.begin(), codeRangeSizes.end(), codeRangeSize);
    if (found != codeRangeSizes.end())
        return;

    codeRangeSizes.push_back(codeRangeSize);
}

PdfCharCode CodeUnitRange::GetSrcCodeHi() const
{
    return PdfCharCode(SrcCodeLo.Code + Size - 1, SrcCodeLo.CodeSpaceSize);
}

CodePointSpan::CodePointSpan()
    : m_Block{ 0, { U'\0', U'\0', U'\0' } }
{
}

CodePointSpan::~CodePointSpan()
{
    unsigned size = *(const uint32_t*)this;
    if (size > std::size(m_Block.Data))
        m_Array.Data.~unique_ptr();
}

CodePointSpan::CodePointSpan(codepoint cp)
    : m_Block{ 1, { cp, U'\0', U'\0' } }
{
}

CodePointSpan::CodePointSpan(const codepointview& view)
{
    if (view.size() > std::size(m_Block.Data))
    {
        auto data = new codepoint[view.size()];
        std::memcpy(data, view.data(), view.size() * sizeof(char32_t));
        new(&m_Array.Data)unique_ptr<codepoint[]>(data);
        m_Array.Size = (unsigned)view.size();
    }
    else
    {
        new(&m_Block.Data)array<codepoint, 3>{ };
        std::memcpy(m_Block.Data.data(), view.data(), view.size() * sizeof(char32_t));
        m_Block.Size = (unsigned)view.size();
    }
}

CodePointSpan::CodePointSpan(const codepointview& view, codepoint cp)
{
    if (view.size() > std::size(m_Block.Data))
    {
        auto data = new codepoint[view.size() + 1];
        std::memcpy(data, view.data(), view.size() * sizeof(char32_t));
        data[view.size()] = cp;
        new(&m_Array.Data)unique_ptr<codepoint[]>(data);
        m_Array.Size = (unsigned)(view.size() + 1);
    }
    else
    {
        new(&m_Block.Data)array<codepoint, 3>{ };
        std::memcpy(m_Block.Data.data(), view.data(), view.size() * sizeof(char32_t));
        m_Block.Data[view.size()] = cp;
        m_Block.Size = (unsigned)view.size() + 1;
    }
}

CodePointSpan::CodePointSpan(const CodePointSpan& rhs)
    : CodePointSpan(rhs.view()) { }

void CodePointSpan::CopyTo(vector<codepoint>& codePoints) const
{
    auto span = view();
    codePoints.resize(span.size());
    std::memcpy(codePoints.data(), span.data(), span.size() * sizeof(char32_t));
}

unsigned CodePointSpan::GetSize() const
{
    return *(const uint32_t*)this;
}

CodePointSpan& CodePointSpan::operator=(const CodePointSpan& rhs)
{
    this->~CodePointSpan();
    auto view = rhs.view();
    if (view.size() > std::size(m_Block.Data))
    {
        auto data = new codepoint[view.size()];
        std::memcpy(data, view.data(), view.size() * sizeof(char32_t));
        new(&m_Array.Data)unique_ptr<codepoint[]>(data);
        m_Array.Size = (unsigned)view.size();
    }
    else
    {
        new(&m_Block.Data)array<codepoint, 3>{ };
        std::memcpy(m_Block.Data.data(), view.data(), view.size() * sizeof(char32_t));
        m_Block.Size = (unsigned)view.size();
    }
    return *this;
}

codepointview CodePointSpan::view() const
{
    unsigned size = *(const uint32_t*)this;
    if (size > std::size(m_Block.Data))
        return codepointview(m_Array.Data.get(), size);
    else
        return codepointview(m_Block.Data.data(), size);
}

CodePointSpan::operator codepointview() const
{
    unsigned size = *(const uint32_t*)this;
    if (size > std::size(m_Block.Data))
        return codepointview(m_Array.Data.get(), size);
    else
        return codepointview(m_Block.Data.data(), size);
}

codepoint CodePointSpan::operator*() const
{
    unsigned size = *(const uint32_t*)this;
    if (size > std::size(m_Block.Data))
        return m_Array.Data[0];
    else
        return m_Block.Data[0];
}

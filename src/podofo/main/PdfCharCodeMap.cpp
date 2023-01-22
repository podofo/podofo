/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfCharCodeMap.h"
#include <random>
#include <algorithm>
#include <utfcpp/utf8.h>

using namespace std;
using namespace PoDoFo;

PdfCharCodeMap::PdfCharCodeMap()
    : m_MapDirty(false), m_codePointMapHead(nullptr), m_depth(0) { }

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

unsigned PdfCharCodeMap::GetSize() const
{
    return (unsigned)m_CodeUnitMap.size();
}

const PdfEncodingLimits& PdfCharCodeMap::GetLimits() const
{
    return m_Limits;
}

void PdfCharCodeMap::move(PdfCharCodeMap& map) noexcept
{
    m_CodeUnitMap = std::move(map.m_CodeUnitMap);
    utls::move(map.m_Limits, m_Limits);
    utls::move(map.m_MapDirty, m_MapDirty);
    utls::move(map.m_codePointMapHead, m_codePointMapHead);
    utls::move(map.m_depth, m_depth);
}

void PdfCharCodeMap::PushMapping(const PdfCharCode& codeUnit, const codepointview& codePoints)
{
    if (codePoints.size() == 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "CodePoints must not be empty");

    vector<codepoint> copy(codePoints.begin(), codePoints.end());
    pushMapping(codeUnit, std::move(copy));
}

void PdfCharCodeMap::PushMapping(const PdfCharCode& codeUnit, codepoint codePoint)
{
    vector<codepoint> codePoints = { codePoint };
    pushMapping(codeUnit, std::move(codePoints));
}

bool PdfCharCodeMap::TryGetCodePoints(const PdfCharCode& codeUnit, vector<codepoint>& codePoints) const
{
    auto found = m_CodeUnitMap.find(codeUnit);
    if (found == m_CodeUnitMap.end())
    {
        codePoints.clear();
        return false;
    }

    codePoints = found->second;
    return true;
}

bool PdfCharCodeMap::TryGetNextCharCode(string_view::iterator& it, const string_view::iterator& end, PdfCharCode& code) const
{
    const_cast<PdfCharCodeMap&>(*this).reviseCPMap();
    return tryFindNextCharacterId(m_codePointMapHead, it, end, code);
}

bool PdfCharCodeMap::TryGetCharCode(const codepointview& codePoints, PdfCharCode& codeUnit) const
{
    const_cast<PdfCharCodeMap&>(*this).reviseCPMap();
    auto it = codePoints.begin();
    auto end = codePoints.end();
    const CPMapNode* node = m_codePointMapHead;
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
    const_cast<PdfCharCodeMap&>(*this).reviseCPMap();
    auto node = findNode(m_codePointMapHead, codePoint);
    if (node == nullptr)
    {
        code = { };
        return false;
    }

    code = node->CodeUnit;
    return true;
}

void PdfCharCodeMap::pushMapping(const PdfCharCode& codeUnit, vector<codepoint>&& codePoints)
{
    if (codeUnit.CodeSpaceSize == 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Code unit must be valid");

    m_CodeUnitMap[codeUnit] = std::move(codePoints);

    // Update limits
    if (codeUnit.CodeSpaceSize < m_Limits.MinCodeSize)
        m_Limits.MinCodeSize = codeUnit.CodeSpaceSize;
    if (codeUnit.CodeSpaceSize > m_Limits.MaxCodeSize)
        m_Limits.MaxCodeSize = codeUnit.CodeSpaceSize;
    if (codeUnit.Code < m_Limits.FirstChar.Code)
        m_Limits.FirstChar = codeUnit;
    if (codeUnit.Code > m_Limits.LastChar.Code)
        m_Limits.LastChar = codeUnit;

    m_MapDirty = true;
}

bool PdfCharCodeMap::tryFindNextCharacterId(const CPMapNode* node, string_view::iterator& it,
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

const PdfCharCodeMap::CPMapNode* PdfCharCodeMap::findNode(const CPMapNode* node, codepoint codePoint)
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

void PdfCharCodeMap::reviseCPMap()
{
    if (!m_MapDirty)
        return;

    if (m_codePointMapHead != nullptr)
    {
        deleteNode(m_codePointMapHead);
        m_codePointMapHead = nullptr;
    }

    // Randomize items in the map in a separate list
    // so BST creation will be more balanced
    // https://en.wikipedia.org/wiki/Random_binary_tree
    // TODO: Create a perfectly balanced BST
    vector<pair<PdfCharCode, vector<codepoint>>> pairs;
    pairs.reserve(m_CodeUnitMap.size());
    std::copy(m_CodeUnitMap.begin(), m_CodeUnitMap.end(), std::back_inserter(pairs));
    std::mt19937 e(random_device{}());
    std::shuffle(pairs.begin(), pairs.end(), e);

    for (auto& pair : pairs)
    {
        CPMapNode** curr = &m_codePointMapHead;      // Node root being searched
        CPMapNode* found;                     // Last found node
        auto it = pair.second.begin();
        auto end = pair.second.end();
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

PdfCharCodeMap::CPMapNode* PdfCharCodeMap::findOrAddNode(CPMapNode*& node, codepoint codePoint)
{
    if (node == nullptr)
    {
        node = new CPMapNode{ };
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

PdfCharCodeMap::iterator PdfCharCodeMap::begin() const
{
    return m_CodeUnitMap.begin();
}

PdfCharCodeMap::iterator PdfCharCodeMap::end() const
{
    return m_CodeUnitMap.end();
}

void PdfCharCodeMap::deleteNode(CPMapNode* node)
{
    if (node == nullptr)
        return;

    deleteNode(node->Ligatures);
    deleteNode(node->Left);
    deleteNode(node->Right);
    delete node;
}

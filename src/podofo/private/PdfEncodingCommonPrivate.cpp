/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include "PdfDeclarationsPrivate.h"
#include "PdfEncodingCommonPrivate.h"

#include <utf8cpp/utf8.h>

using namespace std;
using namespace PoDoFo;

static CodePointMapNode* findOrAddNode(CodePointMapNode*& node, codepoint codePoint);
static const CodePointMapNode* findNode(const CodePointMapNode* node, codepoint codePoint);

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

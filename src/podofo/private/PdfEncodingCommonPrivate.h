/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_ENCODING_COMMON_PRIVATE_H
#define PDF_ENCODING_COMMON_PRIVATE_H

#include <podofo/main/PdfEncodingCommon.h>

namespace PoDoFo
{
    // Map code point(s) -> code units
    struct CodePointMapNode
    {
        codepoint CodePoint;
        PdfCharCode CodeUnit;
        CodePointMapNode* Ligatures;
        CodePointMapNode* Left;
        CodePointMapNode* Right;
    };

    /**
     * Methods to maintain and query a reverse map from code points to code units
     */

    bool TryGetCodeReverseMap(const CodePointMapNode* root, const codepointview& codePoints, PdfCharCode& codeUnit);
    bool TryGetCodeReverseMap(const CodePointMapNode* root, codepoint codePoint, PdfCharCode& codeUnit);
    bool TryGetCodeReverseMap(const CodePointMapNode* root, std::string_view::iterator& it,
        const std::string_view::iterator& end, PdfCharCode& codeUnit);
    void PushMappingReverseMap(CodePointMapNode*& root, const codepointview& codePoints, const PdfCharCode& codeUnit);
    void DeleteNodeReverseMap(CodePointMapNode* node);
}

#endif // PDF_ENCODING_COMMON_PRIVATE_H

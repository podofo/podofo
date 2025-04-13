/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_CHAR_CODE_MAP_H
#define PDF_CHAR_CODE_MAP_H

#include "PdfDeclarations.h"
#include "PdfEncodingCommon.h"

namespace PoDoFo
{
    struct CodePointMapNode;

    struct PODOFO_API CodeUnitRange final
    {
        PdfCharCode SrcCodeLo;
        unsigned Size = 0;
        CodePointSpan DstCodeLo;

        CodeUnitRange();

        CodeUnitRange(PdfCharCode srcCodeLo, unsigned size, CodePointSpan dstCodeLo);

        PdfCharCode GetSrcCodeHi() const;
    };

    struct PODOFO_API CodeUnitRangeInequality
    {
        using is_transparent = std::true_type;

        bool operator()(const CodeUnitRange& lhs, const PdfCharCode& rhs) const
        {
            return lhs.SrcCodeLo < rhs;
        }
        bool operator()(const PdfCharCode& lhs, const CodeUnitRange& rhs) const
        {
            return lhs < rhs.SrcCodeLo;
        }
        bool operator()(const CodeUnitRange& lhs, const CodeUnitRange& rhs) const
        {
            return lhs.SrcCodeLo < rhs.SrcCodeLo;
        }
    };

    using CodeUnitRanges = std::set<CodeUnitRange, CodeUnitRangeInequality>;

    /**
     * Represent a range in the "begincodespacerange" section
     * \remarks Lo/Hi codes for different ranges can't be compared linearly, unless
     * they are 1-byte codes. See Adobe CMap specification, pages 48-50:
     * https://adobe-type-tools.github.io/font-tech-notes/pdfs/5014.CIDFont_Spec.pdf
     */
    struct PODOFO_API CodeSpaceRange final
    {
        CodeSpaceRange();
        CodeSpaceRange(unsigned codeLo, unsigned codeHi, unsigned char codeSpaceSize);

        unsigned CodeLo;
        unsigned CodeHi;
        unsigned char CodeSpaceSize;

        PdfCharCode GetSrcCodeLo() const;
        PdfCharCode GetSrcCodeHi() const;
    };

    /**
     * A bidirectional map from character code units to unspecified code points
     *
     * \remarks The actual code point nature is unspecified, but
     * it can either be unicode code points or CID(s) as used
     * in CID keyed fonts. For generic terminology see
     * https://en.wikipedia.org/wiki/Character_encoding#Terminology
     * See also 5014.CIDFont_Spec, 2.1 Terminology
     */
    class PODOFO_API PdfCharCodeMap final
    {
        PODOFO_PRIVATE_FRIEND(class PdfCMapEncodingFactory);

    public:
        PdfCharCodeMap();

        PdfCharCodeMap(PdfCharCodeMap&& map) noexcept;

        ~PdfCharCodeMap();

    private:
        PdfCharCodeMap(CodeUnitMap&& mapping, CodeUnitRanges&& ranges, const PdfEncodingLimits& limits);

    public:
        /** Method to push a mapping.
         * Given string can be a ligature, es "ffi"
         * \remarks The mapping is ignored if codePoints is empty
         */
        void PushMapping(const PdfCharCode& codeUnit, const codepointview& codePoints);

        /** Convenience method to push a single code point mapping
         */
        void PushMapping(const PdfCharCode& codeUnit, codepoint codePoint);

        /** Push a range mapping in the form "srcCodeLo srcCodeHi dstCodeLo".
         * See 5014.CIDFont_Spec, 7.2 Operator summary for begincidrange specifications
         * \remarks The range is ignored if srcCodeHi < srcCodeLo
         */
        void PushRange(const PdfCharCode& srcCodeLo, unsigned size, codepoint dstCodeLo);

        /** Push a range mapping in the form "srcCodeLo srcCodeHi dstCodeLo".
         * See 5014.CIDFont_Spec, 7.2 Operator summary for beginbfrange specifications
         * \remarks The range is ignored if srcCodeHi < srcCodeLo or dstCodeLo is empty
         */
        void PushRange(const PdfCharCode& srcCodeLo, unsigned size, const codepointview& dstCodeLo);

        /** Returns false when no mapped identifiers are not found in the map
         */
        bool TryGetCodePoints(const PdfCharCode& codeUnit, CodePointSpan& codePoints) const;

        /** Try get char code from utf8 encoded range
         * \remarks It assumes it != and it will consumes the iterator
         * also when returning false
         */
        bool TryGetNextCharCode(std::string_view::iterator& it,
            const std::string_view::iterator& end, PdfCharCode& code) const;

        /** Try get char code from unicode code points
         * \param codePoints sequence of unicode code points. All the sequence must match
         */
        bool TryGetCharCode(const codepointview& codePoints, PdfCharCode& code) const;

        /** Try get char code from unicode code point
         */
        bool TryGetCharCode(codepoint codePoint, PdfCharCode& code) const;

        PdfCharCodeMap& operator=(PdfCharCodeMap&& map) noexcept;

        const PdfEncodingLimits& GetLimits() const { return m_Limits; }

        bool IsEmpty() const;

        /** Determines if the map is a trivial identity
         */
        bool IsTrivialIdentity() const;

        std::vector<CodeSpaceRange> GetCodeSpaceRanges() const;

    public:
        /** Provides direct mappings
         */
        const CodeUnitMap& GetMappings() const { return m_Mappings; }

        /** Provides range mappings
         */
        const CodeUnitRanges& GetRanges() const { return m_Ranges; }

    private:
        void move(PdfCharCodeMap& map) noexcept;
        void pushMapping(const PdfCharCode& codeUnit, const codepointview& codePoints);

    private:
        PdfCharCodeMap(const PdfCharCodeMap&) = delete;
        PdfCharCodeMap& operator=(const PdfCharCodeMap&) = delete;

    private:
        void updateLimits(const PdfCharCode& codeUnit);
        void reviseCodePointMap();
        bool tryFixNextRanges(const CodeUnitRanges::iterator& it, unsigned prevRangeCodeUpper);

    private:
        PdfEncodingLimits m_Limits;
        CodeUnitMap m_Mappings;
        CodeUnitRanges m_Ranges;
        bool m_MapDirty;
        CodePointMapNode* m_codePointMapHead;           // Head of a BST to lookup code points
    };
}

#endif // PDF_CHAR_CODE_MAP_H

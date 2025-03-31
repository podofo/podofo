/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_ENCODING_COMMON_H
#define PDF_ENCODING_COMMON_H

#include "PdfString.h"

namespace PoDoFo
{
    /** A character code unit
     *
     * For generic terminology see https://en.wikipedia.org/wiki/Character_encoding#Terminology
     * See also 5014.CIDFont_Spec, 2.1 Terminology
     */
    struct PODOFO_API PdfCharCode final
    {
        unsigned Code;

        // RangeSize example <cd> -> 1, <00cd> -> 2
        unsigned char CodeSpaceSize;

        PdfCharCode();

        /** Create a code of minimum size
         */
        explicit PdfCharCode(unsigned code);

        PdfCharCode(unsigned code, unsigned char codeSpaceSize);

        bool operator<(const PdfCharCode& rhs) const;

        bool operator==(const PdfCharCode& rhs) const;

        bool operator!=(const PdfCharCode& rhs) const;

    public:
        unsigned GetByteCode(unsigned char byteIdx) const;
        void AppendTo(std::string& str) const;
        void WriteHexTo(std::string& str, bool wrap = true) const;
    };

    // TODO: Optimize me
    using PdfCharCodeList = std::vector<PdfCharCode>;

    /** Represent a CID (Character ID) with full code unit information
     */
    struct PODOFO_API PdfCID final
    {
        unsigned Id;
        PdfCharCode Unit;

        PdfCID();

        /**
         * Create a CID that has an identical code unit of minimum size
         */
        explicit PdfCID(unsigned id);

        PdfCID(unsigned id, const PdfCharCode& unit);

        /**
         * Create a CID that has an identical code as a code unit representation
         */
        PdfCID(const PdfCharCode& unit);
    };

    /** Represents a GID (Glyph ID) with PDF metrics identifier
     */
    struct PODOFO_API PdfGID final
    {
        unsigned Id = 0;            ///< The id of the glyph in the font program
        unsigned MetricsId = 0;     ///< The id of the glyph in the PDF metrics (/Widths, /W arrays). In case of Type 0 CIDFonts this effectively corresponds to the CID

        PdfGID();

        PdfGID(unsigned id);

        PdfGID(unsigned id, unsigned metricsId);
    };

    /** Represents a bundle of a CID and GID information
     */
    struct PODOFO_API PdfCharGIDInfo
    {
        unsigned Cid = 0;           ///< The identifier of the character
        PdfGID Gid;                 ///< The identifier of the glyph in font program and PDF metrics
    };

    struct PODOFO_API PdfEncodingLimits final
    {
    public:
        PdfEncodingLimits(unsigned char minCodeSize, unsigned char maxCodeSize,
            const PdfCharCode& firstCharCode, const PdfCharCode& lastCharCode);

        /** Create invalid limits
         */
        PdfEncodingLimits();

        /** Determines if the limits are valid
         * This happens when FirstChar is <= LastChar and MinCodeSize <= MaxCodeSize
         */
        bool AreValid() const;

        /** Determines if the limits code size range is valid
         * This happens when MinCodeSize <= MaxCodeSize
         */
        bool HaveValidCodeSizeRange() const;

        PdfCharCode FirstChar;     // The first defined character code
        PdfCharCode LastChar;      // The last defined character code
        unsigned char MinCodeSize;
        unsigned char MaxCodeSize;
    };

    struct PODOFO_API PdfCIDSystemInfo final
    {
        PdfString Registry;
        PdfString Ordering;
        int Supplement = 0;
    };
}

namespace std
{
    /** Overload hasher for PdfCharCode
     */
    template<>
    struct hash<PoDoFo::PdfCharCode>
    {
        size_t operator()(const PoDoFo::PdfCharCode& code) const noexcept
        {
            return code.CodeSpaceSize << 24 | code.Code;
        }
    };
}

#endif // PDF_ENCODING_COMMON_H

/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_ENCODING_COMMON_H
#define PDF_ENCODING_COMMON_H

#include "PdfDeclarations.h"

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

    public:
        void AppendTo(std::string& str) const;
        void WriteHexTo(std::string& str, bool wrap = true) const;
    };

    /** Represent a CID (Character ID) with full code unit information
     */
    struct PdfCID final
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

    struct PODOFO_API PdfEncodingLimits final
    {
    public:
        PdfEncodingLimits(unsigned char minCodeSize, unsigned char maxCodeSize,
            const PdfCharCode& firstCharCode, const PdfCharCode& lastCharCode);
        PdfEncodingLimits();

        /** Determines if the limits are valid
         * This happens when FirstChar is <= LastChar and MinCodeSize <= MaxCodeSize
         */
        bool AreValid() const;

        /** Determines if the limits code size range is valid
         * This happens when MinCodeSize <= MaxCodeSize
         */
        bool HaveValidCodeSizeRange() const;

        unsigned char MinCodeSize;
        unsigned char MaxCodeSize;
        PdfCharCode FirstChar;     // The first defined character code
        PdfCharCode LastChar;      // The last defined character code
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

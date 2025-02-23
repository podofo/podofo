/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_UTILS_H
#define PDF_UTILS_H

#include <podofo/main/PdfDeclarations.h>

namespace PoDoFo
{
    inline bool IsCharWhitespace(char ch)
    {
        switch (ch)
        {
            case '\0': // NULL
                return true;
            case '\t': // TAB
                return true;
            case '\n': // Line Feed
                return true;
            case '\f': // Form Feed
                return true;
            case '\r': // Carriage Return
                return true;
            case ' ': // White space
                return true;
            default:
                return false;
        }
    }

    inline bool IsCharDelimiter(char ch)
    {
        switch (ch)
        {
            case '(':
                return true;
            case ')':
                return true;
            case '<':
                return true;
            case '>':
                return true;
            case '[':
                return true;
            case ']':
                return true;
            case '{':
                return true;
            case '}':
                return true;
            case '/':
                return true;
            case '%':
                return true;
            default:
                return false;
        }
    }

    inline bool IsCharTokenDelimiter(char ch, PdfTokenType& tokenType)
    {
        switch (ch)
        {
            case '(':
                tokenType = PdfTokenType::ParenthesisLeft;
                return true;
            case ')':
                tokenType = PdfTokenType::ParenthesisRight;
                return true;
            case '[':
                tokenType = PdfTokenType::SquareBracketLeft;
                return true;
            case ']':
                tokenType = PdfTokenType::SquareBracketRight;
                return true;
            case '{':
                tokenType = PdfTokenType::BraceLeft;
                return true;
            case '}':
                tokenType = PdfTokenType::BraceRight;
                return true;
            case '/':
                tokenType = PdfTokenType::Slash;
                return true;
            default:
                tokenType = PdfTokenType::Unknown;
                return false;
        }
    }

    /** Checks if a character is neither whitespace or delimiter
     */
    inline bool IsCharRegular(char ch)
    {
        return !(IsCharWhitespace(ch) || IsCharDelimiter(ch));
    }

    /** Check if the character is within the range of
     * non control code ASCII characters
     */
    inline bool IsCharASCIIPrintable(char ch)
    {
        return ch > 32 && ch < 127;
    }
}

#endif // PDF_UTILS_H

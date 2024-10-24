/**
 * SPDX-FileCopyrightText: (C) 2024 Kira Backes <kira.backes@nrwsoft.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfTextState.h"

#include <utf8cpp/utf8.h>

#include "PdfFont.h"

using namespace std;
using namespace PoDoFo;

vector<string> PdfTextState::SplitTextAsLines(const string_view& str, double width, bool preserveTrailingSpaces) const
{
    if (width <= 0) // nonsense arguments
        return {};

    if (str.empty()) // empty string
        return {{""}};

    auto& font = *Font;

    bool startOfWord = true;
    double curWidthOfLine = 0;
    vector<string> lines;

    // do simple word wrapping
    auto it = str.begin();
    auto end = str.end();
    auto lineBegin = it;
    auto prevIt = it;
    auto startOfCurrentWord = it;
    while (it != end)
    {
        char32_t ch = (char32_t)utf8::next(it, end);
        if (utls::IsNewLineLikeChar(ch)) // hard-break!
        {
            lines.push_back((string)str.substr(lineBegin - str.begin(), prevIt - lineBegin));

            lineBegin = it; // skip the line feed
            startOfWord = true;
            curWidthOfLine = 0;
        }
        else if (utls::IsSpaceLikeChar(ch))
        {
            if (curWidthOfLine > width)
            {
                // The previous word does not fit in the current line.
                // -> Move it to the next one.
                if (startOfCurrentWord > lineBegin)
                {
                    lines.push_back((string)str.substr(lineBegin - str.begin(), startOfCurrentWord - lineBegin));
                }
                else
                {
                    lines.push_back((string)str.substr(lineBegin - str.begin(), prevIt - lineBegin));
                    if (!preserveTrailingSpaces)
                    {
                        // Skip all spaces at the end of the line
                        while (it != end)
                        {
                            auto nextIt = it;
                            ch = (char32_t)utf8::next(nextIt, end);
                            if (!utls::IsSpaceLikeChar(ch))
                                break;
                            it = nextIt;
                        }

                        startOfCurrentWord = it;
                    }
                    else
                    {
                        startOfCurrentWord = prevIt;
                    }
                    startOfWord = true;
                }
                lineBegin = startOfCurrentWord;

                if (!startOfWord)
                {
                    curWidthOfLine = font.GetStringLength(
                        str.substr(startOfCurrentWord - str.begin(), prevIt - startOfCurrentWord),
                        *this);
                }
                else
                {
                    curWidthOfLine = 0;
                }
            }
            else if ((curWidthOfLine + font.GetCharLength(ch, *this)) > width)
            {
                lines.push_back((string)str.substr(lineBegin - str.begin(), prevIt - lineBegin));
                if (!preserveTrailingSpaces)
                {
                    // Skip all spaces at the end of the line
                    while (it != end)
                    {
                        auto nextIt = it;
                        ch = (char32_t)utf8::next(nextIt, end);
                        if (!utls::IsSpaceLikeChar(ch))
                            break;
                        it = nextIt;
                    }

                    startOfCurrentWord = it;
                }
                else
                {
                    startOfCurrentWord = prevIt;
                }
                lineBegin = startOfCurrentWord;
                startOfWord = true;
                curWidthOfLine = 0;
            }
            else
            {
                curWidthOfLine += font.GetCharLength(ch, *this);
            }

            startOfWord = true;
        }
        else
        {
            if (startOfWord)
            {
                startOfCurrentWord = prevIt;
                startOfWord = false;
            }
            //else do nothing

            if ((curWidthOfLine + font.GetCharLength(ch, *this)) > width)
            {
                if (lineBegin == startOfCurrentWord)
                {
                    // This word takes up the whole line.
                    // Put as much as possible on this line.
                    if (lineBegin == prevIt)
                    {
                        lines.push_back((string)str.substr(prevIt - str.begin(), it - prevIt));
                        lineBegin = it;
                        startOfCurrentWord = it;
                        curWidthOfLine = 0;
                    }
                    else
                    {
                        lines.push_back((string)str.substr(lineBegin - str.begin(), prevIt - lineBegin));
                        lineBegin = prevIt;
                        startOfCurrentWord = prevIt;
                        curWidthOfLine = font.GetCharLength(ch, *this);
                    }
                }
                else
                {
                    // The current word does not fit in the current line.
                    // -> Move it to the next one.
                    lines.push_back((string)str.substr(lineBegin - str.begin(), startOfCurrentWord - lineBegin));
                    lineBegin = startOfCurrentWord;
                    curWidthOfLine = font.GetStringLength((string)str.substr(startOfCurrentWord - str.begin(), it - startOfCurrentWord), *this);
                }
            }
            else
            {
                curWidthOfLine += font.GetCharLength(ch, *this);
            }
        }

        prevIt = it;
    }

    if ((prevIt - lineBegin) > 0)
    {
        if (curWidthOfLine > width && startOfCurrentWord > lineBegin)
        {
            // The previous word does not fit in the current line.
            // -> Move it to the next one.
            lines.push_back((string)str.substr(lineBegin - str.begin(), startOfCurrentWord - lineBegin));
            lineBegin = startOfCurrentWord;
        }
        //else do nothing

        if (prevIt - lineBegin > 0)
        {
            lines.push_back((string)str.substr(lineBegin - str.begin(), prevIt - lineBegin));
        }
        //else do nothing
    }

    return lines;
}

/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfEncodingCommon.h"

using namespace std;
using namespace PoDoFo;

PdfCharCode::PdfCharCode()
    : Code(0), CodeSpaceSize(0)
{
}

PdfCharCode::PdfCharCode(unsigned code)
    : Code(code), CodeSpaceSize(utls::GetCharCodeSize(code))
{
}

PdfCharCode::PdfCharCode(unsigned code, unsigned char codeSpaceSize)
    : Code(code), CodeSpaceSize(codeSpaceSize)
{
}

bool PdfCharCode::operator<(const PdfCharCode& rhs) const
{
    return Code < rhs.Code;
}

bool PdfCharCode::operator==(const PdfCharCode& rhs) const
{
    return CodeSpaceSize == rhs.CodeSpaceSize && Code == rhs.Code;
}

void PdfCharCode::AppendTo(string& str) const
{
    for (unsigned i = CodeSpaceSize; i >= 1; i--)
        str.append(1, (char)((Code >> (i - 1) * CHAR_BIT) & 0xFF));
}

void PdfCharCode::WriteHexTo(string& str, bool wrap) const
{
    str.clear();
    const char* pattern;
    if (wrap)
    {
        switch (CodeSpaceSize)
        {
            case 1:
                pattern = "<{:02X}>";
                break;
            case 2:
                pattern = "<{:04X}>";
                break;
            case 3:
                pattern = "<{:06X}>";
                break;
            case 4:
                pattern = "<{:08X}>";
                break;
            default:
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Code space must be [1,4]");
        }
    }
    else
    {
        switch (CodeSpaceSize)
        {
            case 1:
                pattern = "{:02X}";
                break;
            case 2:
                pattern = "{:04X}";
                break;
            case 3:
                pattern = "{:06X}";
                break;
            case 4:
                pattern = "{:08X}";
                break;
            default:
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Code space must be [1,4]");
        }
    }

    utls::FormatTo(str, pattern, Code);
}

PdfCID::PdfCID()
    : Id(0)
{
}

PdfCID::PdfCID(unsigned id)
    : Id(id), Unit(id)
{
}

PdfCID::PdfCID(unsigned id, const PdfCharCode& unit)
    : Id(id), Unit(unit)
{
}

PdfCID::PdfCID(const PdfCharCode& unit)
    : Id(unit.Code), Unit(unit)
{
}

PdfEncodingLimits::PdfEncodingLimits(unsigned char minCodeSize, unsigned char maxCodeSize,
    const PdfCharCode& firstChar, const PdfCharCode& lastChar) :
    MinCodeSize(minCodeSize),
    MaxCodeSize(maxCodeSize),
    FirstChar(firstChar),
    LastChar(lastChar)
{
}

PdfEncodingLimits::PdfEncodingLimits() :
    PdfEncodingLimits(numeric_limits<unsigned char>::max(), 0, PdfCharCode(numeric_limits<unsigned>::max()), PdfCharCode(0))
{
}

bool PdfEncodingLimits::AreValid() const
{
    return FirstChar.Code <= LastChar.Code &&
        MinCodeSize <= MaxCodeSize;
}

bool PdfEncodingLimits::HaveValidCodeSizeRange() const
{
    return MinCodeSize <= MaxCodeSize;
}

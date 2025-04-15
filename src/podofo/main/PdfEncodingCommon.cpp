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

bool PdfCharCode::operator!=(const PdfCharCode& rhs) const
{
    return CodeSpaceSize != rhs.CodeSpaceSize || Code != rhs.Code;
}

unsigned PdfCharCode::GetByteCode(unsigned char byteIdx) const
{
    return (Code >> (CodeSpaceSize - (byteIdx + 1)) * CHAR_BIT) & 0xFFU;
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
    FirstChar(firstChar),
    LastChar(lastChar),
    MinCodeSize(minCodeSize),
    MaxCodeSize(maxCodeSize)
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

PdfGID::PdfGID()
    : Id(0), MetricsId(0) { }

PdfGID::PdfGID(unsigned id)
    : Id(id), MetricsId(id) { }

PdfGID::PdfGID(unsigned id, unsigned metricsId)
    : Id(id), MetricsId(metricsId) { }


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

CodePointSpan::CodePointSpan(initializer_list<codepoint> initializer)
{
    if (initializer.size() > std::size(m_Block.Data))
    {
        auto data = new codepoint[initializer.size()];
        std::memcpy(data, initializer.begin(), initializer.size() * sizeof(char32_t));
        new(&m_Array.Data)unique_ptr<codepoint[]>(data);
        m_Array.Size = (unsigned)initializer.size();
    }
    else
    {
        new(&m_Block.Data)array<codepoint, 3>{ };
        std::memcpy(m_Block.Data.data(), initializer.begin(), initializer.size() * sizeof(char32_t));
        m_Block.Size = (unsigned)initializer.size();
    }
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
    : CodePointSpan(rhs.view()) {
}

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
    if (this == &rhs)
        return *this;
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

size_t CodePointSpan::size() const
{
    return *(const uint32_t*)this;
}

const codepoint* CodePointSpan::data() const
{
    unsigned size = *(const uint32_t*)this;
    if (size > std::size(m_Block.Data))
        return m_Array.Data.get();
    else
        return m_Block.Data.data();
}

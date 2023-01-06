/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfStringStream.h"

#include <podofo/private/outstringstream.h>

using namespace std;
using namespace cmn;
using namespace PoDoFo;

PdfStringStream::PdfStringStream()
    : m_stream(new outstringstream())
{
    m_stream->imbue(utls::GetInvariantLocale());
}

PdfStringStream& PdfStringStream::operator<<(float val)
{
    utls::FormatTo(m_temp, val, (unsigned short)m_stream->precision());
    (*m_stream) << m_temp;
    return *this;
}

PdfStringStream& PdfStringStream::operator<<(double val)
{
    utls::FormatTo(m_temp, val, (unsigned char)m_stream->precision());
    (*m_stream) << m_temp;
    return *this;
}

PdfStringStream& PdfStringStream::operator<<(
    std::ostream& (*pfn)(std::ostream&))
{
    pfn(*m_stream);
    return *this;
}

string_view PdfStringStream::GetString() const
{
    return static_cast<const outstringstream&>(*m_stream).str();
}

string PdfStringStream::TakeString()
{
    return static_cast<outstringstream&>(*m_stream).take_str();
}

void PdfStringStream::Clear()
{
    static_cast<outstringstream&>(*m_stream).clear();
    m_temp.clear();
}

void PdfStringStream::SetPrecision(unsigned short value)
{
    (void)m_stream->precision(value);
}

unsigned short PdfStringStream::GetPrecision() const
{
    return (unsigned short)m_stream->precision();
}

unsigned PdfStringStream::GetSize() const
{
    return (unsigned)static_cast<const outstringstream&>(*m_stream).size();
}

void PdfStringStream::writeBuffer(const char* buffer, size_t size)
{
    m_stream->write(buffer, size);
}

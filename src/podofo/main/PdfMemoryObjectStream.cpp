/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfMemoryObjectStream.h"

#include <podofo/auxiliary/StreamDevice.h>
#include "PdfStatefulEncrypt.h"

using namespace std;
using namespace PoDoFo;

PdfMemoryObjectStream::PdfMemoryObjectStream()
{
}

void PdfMemoryObjectStream::Init(PdfObject& obj)
{
    (void)obj;
}

void PdfMemoryObjectStream::Clear()
{
    m_buffer.clear();
}

bool PdfMemoryObjectStream::TryCopyFrom(const PdfObjectStreamProvider& rhs)
{
    const PdfMemoryObjectStream* memstream = dynamic_cast<const PdfMemoryObjectStream*>(&rhs);
    if (memstream == nullptr)
        return false;

    m_buffer = memstream->m_buffer;
    return true;
}

bool PdfMemoryObjectStream::TryMoveFrom(PdfObjectStreamProvider&& rhs)
{
    PdfMemoryObjectStream* memstream = dynamic_cast<PdfMemoryObjectStream*>(&rhs);
    if (memstream == nullptr)
        return false;

    m_buffer = std::move(memstream->m_buffer);
    return true;
}

unique_ptr<InputStream> PdfMemoryObjectStream::GetInputStream(PdfObject& obj)
{
    (void)obj;
    return unique_ptr<InputStream>(new SpanStreamDevice(m_buffer));
}

unique_ptr<OutputStream> PdfMemoryObjectStream::GetOutputStream(PdfObject& obj)
{
    (void)obj;
    m_buffer.clear();
    return unique_ptr<OutputStream>(new StringStreamDevice(m_buffer));
}

void PdfMemoryObjectStream::Write(OutputStream& stream, const PdfStatefulEncrypt* encrypt)
{
    stream.Write("stream\n");
    if (encrypt != nullptr)
    {
        charbuff encrypted;
        encrypt->EncryptTo(encrypted, { m_buffer.data(), m_buffer.size() });
        stream.Write(encrypted);
    }
    else
    {
        stream.Write(string_view(m_buffer.data(), m_buffer.size()));
    }

    stream.Write("\nendstream\n");
    stream.Flush();
}

size_t PdfMemoryObjectStream::GetLength() const
{
    return m_buffer.size();
}

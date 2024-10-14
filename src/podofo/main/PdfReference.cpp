/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfReference.h"

#include <podofo/auxiliary/OutputDevice.h>

using namespace std;
using namespace PoDoFo;

PdfReference::PdfReference()
    : PdfDataMember(PdfDataType::Reference), m_GenerationNo(0), m_ObjectNo(0)
{
}

PdfReference::PdfReference(const uint32_t objectNo, const uint16_t generationNo)
    : PdfDataMember(PdfDataType::Reference), m_GenerationNo(generationNo), m_ObjectNo(objectNo)
{
}

void PdfReference::Write(OutputStream& device, PdfWriteFlags flags, const PdfStatefulEncrypt* encrypt, charbuff& buffer) const
{
    (void)encrypt;
    if ((flags & PdfWriteFlags::NoInlineLiteral) == PdfWriteFlags::None)
        device.Write(' '); // Write space before the reference

    utls::FormatTo(buffer, "{} {} R", m_ObjectNo, m_GenerationNo);
    device.Write(buffer);
}

bool PdfReference::operator<(const PdfReference& rhs) const
{
    return m_ObjectNo == rhs.m_ObjectNo ? m_GenerationNo < rhs.m_GenerationNo : m_ObjectNo < rhs.m_ObjectNo;
}

bool PdfReference::operator==(const PdfReference& rhs) const
{
    return m_ObjectNo == rhs.m_ObjectNo && m_GenerationNo == rhs.m_GenerationNo;
}

bool PdfReference::operator!=(const PdfReference& rhs) const
{
    return m_ObjectNo != rhs.m_ObjectNo || m_GenerationNo != rhs.m_GenerationNo;
}

bool PdfReference::IsIndirect() const
{
    return m_ObjectNo != 0 || m_GenerationNo != 0;
}

/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfData.h"

#include "PdfOutputDevice.h"

using namespace std;
using namespace PoDoFo;

PdfData::PdfData() { }

PdfData::PdfData(charbuff&& data, const shared_ptr<size_t>& writeBeacon)
    : m_data(std::move(data)), m_writeBeacon(writeBeacon)
{
}

PdfData::PdfData(const bufferview& data, const shared_ptr<size_t>& writeBeacon)
    : m_data(charbuff(data)), m_writeBeacon(writeBeacon)
{
}

PdfData& PdfData::operator=(const bufferview& data)
{
    m_data = data;
    return *this;
}

void PdfData::Write(OutputStreamDevice& device, PdfWriteFlags,
    const PdfStatefulEncrypt& encrypt, charbuff& buffer) const
{
    (void)encrypt;
    (void)buffer;
    if (m_writeBeacon != nullptr)
        *m_writeBeacon = device.GetPosition();

    device.Write(m_data);
}

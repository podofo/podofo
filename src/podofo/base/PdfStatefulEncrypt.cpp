/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfStatefulEncrypt.h"
#include "PdfEncrypt.h"

using namespace std;
using namespace PoDoFo;

PdfStatefulEncrypt::PdfStatefulEncrypt() : m_encrypt(nullptr) { }

PdfStatefulEncrypt::PdfStatefulEncrypt(const PdfEncrypt& encrypt, const PdfReference& objref)
    : m_encrypt(&encrypt), m_currReference(objref) { }

void PdfStatefulEncrypt::EncryptTo(charbuff& out, const bufferview& view) const
{
    PODOFO_INVARIANT(m_encrypt != nullptr);
    m_encrypt->EncryptTo(out, view, m_currReference);
}

void PdfStatefulEncrypt::DecryptTo(charbuff& out, const bufferview& view) const
{
    PODOFO_INVARIANT(m_encrypt != nullptr);
    m_encrypt->DecryptTo(out, view, m_currReference);
}

size_t PdfStatefulEncrypt::CalculateStreamLength(size_t length) const
{
    PODOFO_INVARIANT(m_encrypt != nullptr);
    return m_encrypt->CalculateStreamLength(length);
}

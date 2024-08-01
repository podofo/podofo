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


PdfStatefulEncrypt::PdfStatefulEncrypt(const PdfEncrypt& encrypt, PdfEncryptContext& context, const PdfReference& objref)
    : m_encrypt(&encrypt), m_context(&context), m_currReference(objref) { }

void PdfStatefulEncrypt::EncryptTo(charbuff& out, const bufferview& view) const
{
    m_encrypt->EncryptTo(out, view, *m_context, m_currReference);
}

void PdfStatefulEncrypt::DecryptTo(charbuff& out, const bufferview& view) const
{
    m_encrypt->DecryptTo(out, view, *m_context, m_currReference);
}

size_t PdfStatefulEncrypt::CalculateStreamLength(size_t length) const
{
    return m_encrypt->CalculateStreamLength(length);
}

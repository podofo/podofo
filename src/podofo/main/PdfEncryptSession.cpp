/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfEncryptSession.h"

using namespace std;
using namespace PoDoFo;

PdfEncryptSession::PdfEncryptSession(const PdfEncrypt& encrypt, const PdfEncryptContext& context)
    : m_Encrypt(PdfEncrypt::CreateFromEncrypt(encrypt)), m_Context(context)
{
}

PdfEncryptSession::PdfEncryptSession(shared_ptr<PdfEncrypt>&& encrypt)
    : m_Encrypt(std::move(encrypt))
{
    PODOFO_ASSERT(m_Encrypt != nullptr);
}

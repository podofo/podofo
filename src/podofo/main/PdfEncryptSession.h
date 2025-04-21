/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_ENCRYPT_SESSION
#define PDF_ENCRYPT_SESSION

#include "PdfEncrypt.h"

namespace PoDoFo {
    
/**
 * A bundle of the encrypt object together a context
 */
class PODOFO_API PdfEncryptSession final
{
    friend class PdfMemDocument;
    PODOFO_PRIVATE_FRIEND(class PdfParser);
    PODOFO_PRIVATE_FRIEND(class PdfImmediateWriter);

private:
    /** A copy constructor that does deep copy of PdfEncrypt as well
     */
    PdfEncryptSession(const PdfEncrypt& encrypt, const PdfEncryptContext& context);
    PdfEncryptSession(std::shared_ptr<PdfEncrypt>&& encrypt);
    PdfEncryptSession(const PdfEncryptSession&) = default;

public:
    PdfEncrypt& GetEncrypt() { return *m_Encrypt; }
    PdfEncryptContext& GetContext() { return m_Context; }

private:
    PdfEncryptSession& operator=(const PdfEncryptSession&) = delete;

private:
    std::shared_ptr<PdfEncrypt> m_Encrypt;
    PdfEncryptContext m_Context;
};

}

#endif // PDF_ENCRYPT_SESSION

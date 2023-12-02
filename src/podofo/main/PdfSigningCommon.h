/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_SIGNING_COMMON_H
#define PDF_SIGNING_COMMON_H

#include <podofo/main/PdfDeclarations.h>

namespace PoDoFo
{
    enum class PdfSignatureType
    {
        Unknown = 0,
        PAdES_B = 1,
        Pkcs7 = 2,
    };

    enum class PdfEncryptionAlgorithm
    {
        Unknown = 0,
        RSA,
    };

    enum class PdfHashingAlgorithm
    {
        Unknown = 0,
        SHA256,
        SHA384,
        SHA512,
    };

    using PdfSigningService = std::function<void(bufferview, bool, charbuff&)>;
}

#endif // PDF_SIGNING_COMMON_H

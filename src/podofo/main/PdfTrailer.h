/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_TRAILER
#define PDF_TRAILER

#include "PdfElement.h"

namespace PoDoFo
{
    class PODOFO_API PdfTrailer final : public PdfDictionaryElement
    {
        friend class PdfDocument;
    private:
        PdfTrailer(PdfObject& obj);
    public:
    };
}

#endif // PDF_TRAILER

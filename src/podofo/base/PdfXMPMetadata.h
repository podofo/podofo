/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_XMP_METADATA
#define PDF_XMP_METADATA

#include "PdfString.h"
#include "PdfDate.h"

namespace PoDoFo
{
    struct PODOFO_API PdfXMPMetadata
    {
        PdfXMPMetadata();
        nullable<PdfString> Title;
        nullable<PdfString> Author;
        nullable<PdfString> Subject;
        nullable<PdfString> Keywords;
        nullable<PdfString> Creator;
        nullable<PdfString> Producer;
        nullable<PdfDate> CreationDate;
        nullable<PdfDate> ModDate;
        PdfALevel PdfaLevel;
    };
}

#endif // PDF_XMP_METADATA

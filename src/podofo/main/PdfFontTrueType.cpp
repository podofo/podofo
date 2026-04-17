// SPDX-FileCopyrightText: 2005 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontTrueType.h"

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfObjectStream.h"

using namespace PoDoFo;

// NOTE: TrueType fonts subsetting is supported only
// in PdfFontCIDTrueType through conversion

PdfFontTrueType::PdfFontTrueType(PdfDocument& doc, PdfFontMetricsConstPtr&& metrics,
        const PdfEncoding& encoding) :
    PdfFontSimple(doc, PdfFontType::TrueType, std::move(metrics), encoding)
{
}

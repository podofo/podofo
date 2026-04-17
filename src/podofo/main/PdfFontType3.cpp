// SPDX-FileCopyrightText: 2005 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontType3.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfObjectStream.h"

using namespace std;
using namespace PoDoFo;

PdfFontType3::PdfFontType3(PdfDocument& doc, PdfFontMetricsConstPtr&& metrics,
        const PdfEncoding& encoding)
    : PdfFontSimple(doc, PdfFontType::Type3, std::move(metrics), encoding)
{
}

bool PdfFontType3::SupportsSubsetting() const
{
    return true;
}

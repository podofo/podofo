/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontType1.h"

#include <utf8cpp/utf8.h>

#include <podofo/auxiliary/InputDevice.h>
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfObjectStream.h"
#include "PdfDifferenceEncoding.h"
#include "PdfDocument.h"

using namespace std;
using namespace PoDoFo;

// NOTE: Type 1 fonts subsetting is supported only
// in PdfFontCIDCFF through conversion
PdfFontType1::PdfFontType1(PdfDocument& doc, PdfFontMetricsConstPtr&& metrics,
    const PdfEncoding& encoding) :
    PdfFontSimple(doc, PdfFontType::Type1, std::move(metrics), encoding)
{
}

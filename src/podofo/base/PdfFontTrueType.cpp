/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontTrueType.h"

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfObjectStream.h"

using namespace PoDoFo;

// TODO: Implement subsetting using PdfFontTrueTypeSubset
// similarly to to PdfFontCIDTrueType

PdfFontTrueType::PdfFontTrueType(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
    const PdfEncoding& encoding) :
    PdfFontSimple(doc, metrics, encoding)
{
}

PdfFontType PdfFontTrueType::GetType() const
{
    return PdfFontType::TrueType;
}

/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontCIDType1.h"

using namespace std;
using namespace PoDoFo;

PdfFontCIDType1::PdfFontCIDType1(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
    const PdfEncoding& encoding) : PdfFontCID(doc, metrics, encoding) { }

bool PdfFontCIDType1::SupportsSubsetting() const
{
    // Not yet supported
    return false;
}

PdfFontType PdfFontCIDType1::GetType() const
{
    return PdfFontType::CIDType1;
}

void PdfFontCIDType1::embedFontSubset()
{
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

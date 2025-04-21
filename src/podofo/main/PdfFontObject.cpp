/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontObject.h"
#include "PdfDictionary.h"

using namespace std;
using namespace PoDoFo;

PdfFontObject::PdfFontObject(PdfObject& obj, PdfFontMetricsConstPtr&& metrics,
        const PdfEncoding& encoding) :
    PdfFont(obj, metrics->GetFontType(), std::move(metrics), encoding) { }

unique_ptr<PdfFontObject> PdfFontObject::Create(PdfObject& obj, PdfObject& descendantObj,
    PdfFontMetricsConstPtr&& metrics, const PdfEncoding& encoding)
{
    (void)descendantObj;
    // TODO: MAke a virtual GetDescendantFontObject()
    return unique_ptr<PdfFontObject>(new PdfFontObject(obj, std::move(metrics), encoding));
}

unique_ptr<PdfFontObject> PdfFontObject::Create(PdfObject& obj, PdfFontMetricsConstPtr&& metrics,
    const PdfEncoding& encoding)
{
    return unique_ptr<PdfFontObject>(new PdfFontObject(obj, std::move(metrics), encoding));
}

bool PdfFontObject::IsObjectLoaded() const
{
    return true;
}

/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfColorSpace.h"
#include "PdfDocument.h"

using namespace std;
using namespace PoDoFo;

PdfColorSpace::PdfColorSpace(PdfDocument& doc, PdfColorSpaceFilterPtr&& filter)
    : PdfElement(getExportObject(doc, filter.get())), m_Filter(std::move(filter))
{
}

PdfObject& PdfColorSpace::getExportObject(PdfDocument& doc, const PdfColorSpaceFilter* filter)
{
    if (filter == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Filter must not be null");

    return doc.GetObjects().CreateObject(filter->GetExportObject(doc.GetObjects()));
}

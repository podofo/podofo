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

PdfColorSpace::PdfColorSpace(PdfDocument& doc, const PdfColorSpaceFilterPtr& filter)
    : PdfDictionaryElement(doc), m_Filter(filter)
{
    GetObject() = filter->GetExportObject(doc.GetObjects());
}

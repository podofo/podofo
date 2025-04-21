/**
 * SPDX-FileCopyrightText: (C) 2025 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFunction.h"

#include "PdfDocument.h"

using namespace std;
using namespace PoDoFo;

PdfFunction::PdfFunction(PdfDocument& doc, PdfFunctionDefinitionPtr&& definition)
    : PdfDictionaryElement(doc), m_Definition(std::move(definition))
{
    m_Definition->FillExportDictionary(GetDictionary());
}

/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfExtGState.h"

#include "PdfDocument.h"

using namespace std;
using namespace PoDoFo;

PdfExtGState::PdfExtGState(PdfDocument& doc, const PdfExtGStateDefinitionPtr& definition)
    : PdfDictionaryElement(doc, "ExtGState"_n), m_Definition(definition)
{
    if (definition == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The definition must be non null");

    if (definition->NonStrokingAlpha != nullptr)
        GetDictionary().AddKey("ca"_n, *definition->NonStrokingAlpha);

    if (definition->StrokingAlpha != nullptr)
        GetDictionary().AddKey("CA"_n, *definition->StrokingAlpha);

    if (definition->BlendMode != nullptr)
        GetDictionary().AddKey("BM"_n, PdfName(PoDoFo::ToString(*definition->BlendMode)));

    if (definition->RenderingIntent != nullptr)
        GetDictionary().AddKey("RI"_n, PdfName(PoDoFo::ToString(*definition->RenderingIntent)));

    if ((definition->OverprintControl & PdfOverprintEnablement::NonStroking) != PdfOverprintEnablement::None)
        GetDictionary().AddKey("op"_n, true);

    if ((definition->OverprintControl & PdfOverprintEnablement::Stroking) != PdfOverprintEnablement::None)
        GetDictionary().AddKey("OP"_n, true);

    if (definition->NonZeroOverprintMode != nullptr)
        GetDictionary().AddKey("OPM"_n, PdfVariant(static_cast<int64_t>(*definition->NonZeroOverprintMode ? 1 : 0)));
}

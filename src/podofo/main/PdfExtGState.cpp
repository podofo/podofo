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

PdfExtGState::PdfExtGState(PdfDocument& doc, PdfExtGStateDefinitionPtr&& definition)
    : PdfDictionaryElement(doc, "ExtGState"_n), m_Definition(std::move(definition))
{
    if (m_Definition == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The definition must be non null");

    if (m_Definition->NonStrokingAlpha != nullptr)
        GetDictionary().AddKey("ca"_n, *m_Definition->NonStrokingAlpha);

    if (m_Definition->StrokingAlpha != nullptr)
        GetDictionary().AddKey("CA"_n, *m_Definition->StrokingAlpha);

    if (m_Definition->BlendMode != nullptr)
        GetDictionary().AddKey("BM"_n, PdfName(PoDoFo::ToString(*m_Definition->BlendMode)));

    if (m_Definition->RenderingIntent != nullptr)
        GetDictionary().AddKey("RI"_n, PdfName(PoDoFo::ToString(*m_Definition->RenderingIntent)));

    if ((m_Definition->OverprintControl & PdfOverprintEnablement::NonStroking) != PdfOverprintEnablement::None)
        GetDictionary().AddKey("op"_n, true);

    if ((m_Definition->OverprintControl & PdfOverprintEnablement::Stroking) != PdfOverprintEnablement::None)
        GetDictionary().AddKey("OP"_n, true);

    if (m_Definition->NonZeroOverprintMode != nullptr)
        GetDictionary().AddKey("OPM"_n, PdfVariant(static_cast<int64_t>(*m_Definition->NonZeroOverprintMode ? 1 : 0)));
}

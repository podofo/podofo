/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfExtGState.h"

#include "PdfDictionary.h"
#include "PdfStringStream.h"
#include "PdfPage.h"

using namespace std;
using namespace PoDoFo;

PdfExtGState::PdfExtGState(PdfDocument& doc)
    : PdfDictionaryElement(doc, "ExtGState") { }

void PdfExtGState::SetFillOpacity(nullable<double> opacity)
{
    if (opacity == nullptr)
        GetDictionary().RemoveKey("ca");
    else
        GetDictionary().AddKey("ca", PdfVariant(*opacity));
}

void PdfExtGState::SetStrokeOpacity(nullable<double> opacity)
{
    if (opacity == nullptr)
        GetDictionary().RemoveKey("CA");
    else
        GetDictionary().AddKey("CA", PdfVariant(*opacity));
}

void PdfExtGState::SetBlendMode(nullable<PdfBlendMode> blendMode)
{
    if (blendMode == nullptr)
        GetDictionary().RemoveKey("BM");
    else
        GetDictionary().AddKey("BM", PdfName(PoDoFo::ToString(*blendMode)));
}

void PdfExtGState::SetOverprintEnabled(nullable<bool> enabled)
{
    if (enabled == nullptr)
    {
        GetDictionary().RemoveKey("OP");
        GetDictionary().RemoveKey("op");
    }
    else
    {
        GetDictionary().AddKey("OP", PdfVariant(*enabled));
        GetDictionary().RemoveKey("op");
    }
}

void PdfExtGState::SetFillOverprintEnabled(nullable<bool> enabled)
{
    if (enabled == nullptr)
        GetDictionary().RemoveKey("op");
    else
        GetDictionary().AddKey("op", PdfVariant(*enabled));
}

void PdfExtGState::SetStrokeOverprintEnabled(nullable<bool> enabled)
{
    if (enabled == nullptr)
        GetDictionary().RemoveKey("OP");
    else
        GetDictionary().AddKey("OP", PdfVariant(*enabled));
}

void PdfExtGState::SetNonZeroOverprintEnabled(nullable<bool> enabled)
{
    if (enabled == nullptr)
        GetDictionary().RemoveKey("OPM");
    else
        GetDictionary().AddKey("OPM", PdfVariant(static_cast<int64_t>(*enabled ? 1 : 0)));
}

void PdfExtGState::SetRenderingIntent(nullable<PdfRenderingIntent> intent)
{
    if (intent == nullptr)
        GetDictionary().RemoveKey("RI");
    else
        GetDictionary().AddKey("RI", PdfName(PoDoFo::ToString(*intent)));
}

void PdfExtGState::SetFrequency(double frequency)
{
    PdfDictionary halftoneDict;
    halftoneDict.AddKey("HalftoneType", PdfVariant(static_cast<int64_t>(1)));
    halftoneDict.AddKey("Frequency", PdfVariant(frequency));
    halftoneDict.AddKey("Angle", PdfVariant(45.0));
    halftoneDict.AddKey("SpotFunction", PdfName("SimpleDot"));

    GetDictionary().AddKey("HT", halftoneDict);
}

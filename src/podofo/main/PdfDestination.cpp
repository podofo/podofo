// SPDX-FileCopyrightText: 2006 Dominik Seichter <domseichter@web.de>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfDestination.h"

#include "PdfDictionary.h"
#include "PdfAction.h"
#include "PdfMemDocument.h"

using namespace std;
using namespace PoDoFo;

PdfDestination::PdfDestination(PdfDocument& doc) :
    PdfArrayElement(doc)
{
}

PdfDestination::PdfDestination(PdfObject& obj) :
    PdfArrayElement(obj)
{
}

void PdfDestination::SetDestination(const PdfPage& page, PdfDestinationFit fit)
{
    PdfName type;
    if (fit == PdfDestinationFit::Fit)
        type = "Fit"_n;
    else if (fit == PdfDestinationFit::FitB)
        type = "FitB"_n;

    auto& arr = GetArray();
    arr.Add(page.GetObject().GetIndirectReference());
    arr.Add(type);
}

void PdfDestination::SetDestination(const PdfPage& page, const Rect& rect)
{
    PdfArray rectArr;
    rect.ToArray(rectArr);

    auto& arr = GetArray();
    arr.Add(page.GetObject().GetIndirectReference());
    arr.Add("FitR"_n);
    arr.insert(arr.end(), rectArr.begin(), rectArr.end());
}

void PdfDestination::SetDestination(const PdfPage& page, double left, double top, double zoom)
{
    auto& arr = GetArray();
    arr.Add(page.GetObject().GetIndirectReference());
    arr.Add("XYZ"_n);
    arr.Add(left);
    arr.Add(top);
    arr.Add(zoom);
}

void PdfDestination::SetDestination(const PdfPage& page, PdfDestinationFit fit, double value)
{
    PdfName type;
    if (fit == PdfDestinationFit::FitH)
        type = "FitH"_n;
    else if (fit == PdfDestinationFit::FitV)
        type = "FitV"_n;
    else if (fit == PdfDestinationFit::FitBH)
        type = "FitBH"_n;
    else if (fit == PdfDestinationFit::FitBV)
        type = "FitBV"_n;
    else
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidKey);

    auto& arr = GetArray();
    arr.Add(page.GetObject().GetIndirectReference());
    arr.Add(type);
    arr.Add(value);
}

bool PdfDestination::TryCreateFromObject(PdfObject& obj, unique_ptr<PdfDestination>& dest)
{
    PdfObject* destObj;
    auto doc = obj.GetDocument();
    if (doc == nullptr || (destObj = FindDestinationObject(*doc, obj)) == nullptr)
    {
        dest.reset();
        return false;
    }

    dest.reset(new PdfDestination(*destObj));
    return true;
}

PdfObject* PdfDestination::FindDestinationObject(PdfDocument& doc, PdfObject& obj)
{
    if (obj.GetDataType() == PdfDataType::Array)
        return &obj;

    PdfObject* value;
    if (obj.GetDataType() == PdfDataType::String)
    {
        auto names = doc.GetNames();
        if (names == nullptr)
            return nullptr;

        value = names->GetValue(PdfKnownNameTree::Dests, obj.GetString());
    }
    else if (obj.GetDataType() == PdfDataType::Name)
    {
        PdfDictionary* dests;
        if (!doc.GetCatalog().GetDictionary().TryFindKeyAs("Dests", dests))
            return nullptr;

        value = dests->FindKey(obj.GetName());
    }
    else
    {
        return nullptr;
    }

    if (value == nullptr)
        return nullptr;

    if (value->IsDictionary())
        value = value->GetDictionary().FindKey("D");

    if (value == nullptr || !value->IsArray())
        return nullptr;

    return value;
}

void PdfDestination::AddToDictionary(PdfDictionary& dictionary) const
{
    if (GetArray().size() == 0)
        // Do not add empty destinations
        dictionary.RemoveKey("Dest");
    else
        dictionary.AddKey("Dest"_n, GetObject());
}

PdfPage* PdfDestination::GetPage()
{
    auto& arr = GetArray();
    if (arr.size() == 0)
        return nullptr;

    // first entry in the array is the page - so just make a new page from it!
    return &GetDocument().GetPages().GetPage(arr[0].GetReference());
}

PdfDestinationType PdfDestination::GetType() const
{
    auto& arr = GetArray();
    if (arr.size() == 0)
        return PdfDestinationType::Unknown;

    PdfName tp = arr[1].GetName();
    if (tp == "XYZ")
        return PdfDestinationType::XYZ;
    else if (tp == "Fit")
        return PdfDestinationType::Fit;
    else if (tp == "FitH")
        return PdfDestinationType::FitH;
    else if (tp == "FitV")
        return PdfDestinationType::FitV;
    else if (tp == "FitR")
        return PdfDestinationType::FitR;
    else if (tp == "FitB")
        return PdfDestinationType::FitB;
    else if (tp == "FitBH")
        return PdfDestinationType::FitBH;
    else if (tp == "FitBV")
        return PdfDestinationType::FitBV;
    else
        return PdfDestinationType::Unknown;
}

double PdfDestination::GetDValue() const
{
    PdfDestinationType tp = GetType();
    if (tp != PdfDestinationType::FitH
        && tp != PdfDestinationType::FitV
        && tp != PdfDestinationType::FitBH)
    {
        PODOFO_RAISE_ERROR(PdfErrorCode::WrongDestinationType);
    }

    return GetArray()[2].GetReal();
}

double PdfDestination::GetLeft() const
{
    PdfDestinationType tp = GetType();
    if (tp != PdfDestinationType::FitV
        && tp != PdfDestinationType::XYZ
        && tp != PdfDestinationType::FitR)
    {
        PODOFO_RAISE_ERROR(PdfErrorCode::WrongDestinationType);
    }

    return GetArray()[2].GetReal();
}

Rect PdfDestination::GetRect() const
{
    if (GetType() != PdfDestinationType::FitR)
        PODOFO_RAISE_ERROR(PdfErrorCode::WrongDestinationType);

    auto& arr = GetArray();
    return Rect(arr[2].GetReal(), arr[3].GetReal(),
        arr[4].GetReal(), arr[5].GetReal());
}

double PdfDestination::GetTop() const
{
    auto& arr = GetArray();
    PdfDestinationType tp = GetType();
    switch (tp)
    {
        case PdfDestinationType::XYZ:
            return arr[3].GetReal();
        case PdfDestinationType::FitH:
        case PdfDestinationType::FitBH:
            return arr[2].GetReal();
        case PdfDestinationType::FitR:
            return arr[5].GetReal();
        case PdfDestinationType::Fit:
        case PdfDestinationType::FitV:
        case PdfDestinationType::FitB:
        case PdfDestinationType::FitBV:
        case PdfDestinationType::Unknown:
        default:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::WrongDestinationType);
        }
    };
}

double PdfDestination::GetZoom() const
{
    if (GetType() != PdfDestinationType::XYZ)
        PODOFO_RAISE_ERROR(PdfErrorCode::WrongDestinationType);

    return GetArray()[4].GetReal();
}

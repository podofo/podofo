/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfDestination.h"

#include "PdfDictionary.h"
#include "PdfAction.h"
#include "PdfMemDocument.h"
#include "PdfNameTree.h"
#include "PdfPage.h"
#include "PdfPageCollection.h"

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

PdfDestination::PdfDestination(const PdfPage& page, PdfDestinationFit fit)
    : PdfDestination(page.GetDocument())
{
    PdfName type;
    if (fit == PdfDestinationFit::Fit)
        type = PdfName("Fit");
    else if (fit == PdfDestinationFit::FitB)
        type = PdfName("FitB");

    auto& arr = GetArray();
    arr.Add(page.GetObject().GetIndirectReference());
    arr.Add(type);
}

PdfDestination::PdfDestination(const PdfPage& page, const PdfRect& rect)
    : PdfDestination(page.GetDocument())
{
    PdfArray rectArr;
    rect.ToArray(rectArr);

    auto& arr = GetArray();
    arr.Add(page.GetObject().GetIndirectReference());
    arr.Add(PdfName("FitR"));
    arr.insert(arr.end(), rectArr.begin(), rectArr.end());
}

PdfDestination::PdfDestination(const PdfPage& page, double left, double top, double zoom)
    : PdfDestination(page.GetDocument())
{
    auto& arr = GetArray();
    arr.Add(page.GetObject().GetIndirectReference());
    arr.Add(PdfName("XYZ"));
    arr.Add(left);
    arr.Add(top);
    arr.Add(zoom);
}

PdfDestination::PdfDestination(const PdfPage& page, PdfDestinationFit fit, double value)
    : PdfDestination(page.GetDocument())
{
    PdfName type;
    if (fit == PdfDestinationFit::FitH)
        type = PdfName("FitH");
    else if (fit == PdfDestinationFit::FitV)
        type = PdfName("FitV");
    else if (fit == PdfDestinationFit::FitBH)
        type = PdfName("FitBH");
    else if (fit == PdfDestinationFit::FitBV)
        type = PdfName("FitBV");
    else
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidKey);

    auto& arr = GetArray();
    arr.Add(page.GetObject().GetIndirectReference());
    arr.Add(type);
    arr.Add(value);
}

unique_ptr<PdfDestination> PdfDestination::Create(PdfObject& obj)
{
    auto& doc = obj.MustGetDocument();
    PdfObject* value = nullptr;

    if (obj.GetDataType() == PdfDataType::Array)
    {
        return std::make_unique<PdfDestination>(obj);
    }
    else if (obj.GetDataType() == PdfDataType::String)
    {
        auto names = doc.GetNames();
        if (names == nullptr)
            PODOFO_RAISE_ERROR(PdfErrorCode::NoObject);

        value = names->GetValue("Dests", obj.GetString());
    }
    else if (obj.GetDataType() == PdfDataType::Name)
    {
        auto memDoc = dynamic_cast<PdfMemDocument*>(&doc);
        if (memDoc == nullptr)
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle,
                "For reading from a document, only use PdfMemDocument");
        }

        auto dests = memDoc->GetCatalog().GetDictionary().FindKey("Dests");
        if (dests == nullptr)
        {
            // The error code has been chosen for its distinguishability.
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidKey,
                "No PDF-1.1-compatible destination dictionary found");
        }
        value = dests->GetDictionary().FindKey(obj.GetName());
    }
    else
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Unsupported object given to "
            "PdfDestination::Init of type {}", obj.GetDataTypeString());
    }

    if (value == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidName);

    if (value->IsArray())
    {
        return std::make_unique<PdfDestination>(*value);
    }
    else if (value->IsDictionary())
    {
        return std::make_unique<PdfDestination>(value->GetDictionary().MustFindKey("D"));
    }
    else
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Unsupported object given to "
            "PdfDestination::Init of type {}", value->GetDataTypeString());
    }
}

void PdfDestination::AddToDictionary(PdfDictionary& dictionary) const
{
    // Do not add empty destinations
    if (GetArray().size() == 0)
        return;

    // since we can only have EITHER a Dest OR an Action
    // we check for an Action, and if already present, we throw
    if (dictionary.HasKey("A"))
        PODOFO_RAISE_ERROR(PdfErrorCode::ActionAlreadyPresent);

    dictionary.AddKey("Dest", GetObject());
}

PdfPage* PdfDestination::GetPage()
{
    auto& arr = GetArray();
    if (arr.size() == 0)
        return nullptr;

    // first entry in the array is the page - so just make a new page from it!
    return &GetObject().GetDocument()->GetPages().GetPage(arr[0].GetReference());
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

PdfRect PdfDestination::GetRect() const
{
    if (GetType() != PdfDestinationType::FitR)
        PODOFO_RAISE_ERROR(PdfErrorCode::WrongDestinationType);

    auto& arr = GetArray();
    return PdfRect(arr[2].GetReal(), arr[3].GetReal(),
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

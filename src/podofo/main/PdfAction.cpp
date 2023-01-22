/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfAction.h"
#include "PdfDictionary.h"

using namespace std;
using namespace PoDoFo;

static const char* s_names[] = {
    nullptr,
    "GoTo",
    "GoToR",
    "GoToE",
    "Launch",
    "Thread",
    "URI",
    "Sound",
    "Movie",
    "Hide",
    "Named",
    "SubmitForm",
    "ResetForm",
    "ImportData",
    "JavaScript",
    "SetOCGState",
    "Rendition",
    "Trans",
    "GoTo3DView",
};

PdfAction::PdfAction(PdfDocument& doc, PdfActionType action)
    : PdfDictionaryElement(doc, "Action"), m_Type(action)
{
    PdfName type(utls::TypeNameForIndex((unsigned)action, s_names, (unsigned)std::size(s_names)));
    if (type.IsNull())
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    this->GetObject().GetDictionary().AddKey("S", type);
}

PdfAction::PdfAction(PdfObject& obj)
// The typename /Action is optional for PdfActions
    : PdfDictionaryElement(obj)
{
    m_Type = static_cast<PdfActionType>(utls::TypeNameToIndex(
        this->GetObject().GetDictionary().FindKeyAs<PdfName>("S").GetString().c_str(),
        s_names, (unsigned)std::size(s_names), (int)PdfActionType::Unknown));
}

void PdfAction::SetURI(const PdfString& uri)
{
    this->GetObject().GetDictionary().AddKey("URI", uri);
}

PdfString PdfAction::GetURI() const
{
    return this->GetObject().GetDictionary().MustFindKey("URI").GetString();
}

bool PdfAction::HasURI() const
{
    return this->GetObject().GetDictionary().FindKey("URI") != nullptr;
}

void PdfAction::SetScript(const PdfString& script)
{
    this->GetObject().GetDictionary().AddKey("JS", script);
}

PdfString PdfAction::GetScript() const
{
    return this->GetObject().GetDictionary().MustFindKey("JS").GetString();
}

bool PdfAction::HasScript() const
{
    return this->GetObject().GetDictionary().HasKey("JS");
}

void PdfAction::AddToDictionary(PdfDictionary& dictionary) const
{
    // since we can only have EITHER a Dest OR an Action
    // we check for an Action, and if already present, we throw
    if (dictionary.HasKey("Dest"))
        PODOFO_RAISE_ERROR(PdfErrorCode::ActionAlreadyPresent);

    dictionary.AddKey("A", this->GetObject());
}

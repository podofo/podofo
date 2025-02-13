/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfCheckBox.h"
#include "PdfDictionary.h"

using namespace std;
using namespace PoDoFo;

PdfCheckBox::PdfCheckBox(PdfAcroForm& acroform, const shared_ptr<PdfField>& parent)
    : PdfToggleButton(acroform, PdfFieldType::CheckBox, parent)
{
}

PdfCheckBox::PdfCheckBox(PdfAnnotationWidget& widget, const shared_ptr<PdfField>& parent)
    : PdfToggleButton(widget, PdfFieldType::CheckBox, parent)
{
}

PdfCheckBox::PdfCheckBox(PdfObject& obj, PdfAcroForm* acroform)
    : PdfToggleButton(obj, acroform, PdfFieldType::CheckBox)
{
}

void PdfCheckBox::AddAppearanceStream(const PdfName& name, const PdfReference& reference)
{
    if (!GetObject().GetDictionary().HasKey("AP"))
        GetObject().GetDictionary().AddKey("AP", PdfDictionary());

    if (!GetObject().GetDictionary().MustFindKey("AP").GetDictionary().HasKey("N"))
        GetObject().GetDictionary().MustFindKey("AP").GetDictionary().AddKey("N", PdfDictionary());

    GetObject().GetDictionary().MustFindKey("AP").
        GetDictionary().MustFindKey("N").GetDictionary().AddKey(name, reference);
}

void PdfCheckBox::SetAppearanceChecked(const PdfXObject& xobj)
{
    this->AddAppearanceStream("Yes", xobj.GetObject().GetIndirectReference());
}

void PdfCheckBox::SetAppearanceUnchecked(const PdfXObject& xobj)
{
    this->AddAppearanceStream("Off", xobj.GetObject().GetIndirectReference());
}

void PdfCheckBox::SetChecked(bool isChecked)
{
    // FIXME: This is incorrect, and should handle toggle buttons export values
    GetObject().GetDictionary().AddKey("V", (isChecked ? PdfName("Yes") : PdfName("Off")));
    GetObject().GetDictionary().AddKey("AS", (isChecked ? PdfName("Yes") : PdfName("Off")));
}

bool PdfCheckBox::IsChecked() const
{
    // ISO 32000-2:2020 12.7.5.2.3 "Check boxes":
    // "The appearance for the off state is optional but,
    // if present, shall be stored in the appearance dictionary
    // under the name Off"
    // 12.7.5.2.4 "Radio buttons": "The parent field’s V entry holds
    // a name object corresponding to the appearance state of whichever
    // child field is currently in the on state; the default value for
    // this entry is Off"
    auto& dict = GetDictionary();
    const PdfName* name;
    if (dict.TryFindKeyAs("V", name))
        return *name != "Off";
    else if (dict.TryFindKeyAs("AS", name))
        return *name != "Off";

    return false;
}

PdfCheckBox* PdfCheckBox::GetParent()
{
    return GetParentTyped<PdfCheckBox>(PdfFieldType::CheckBox);
}

const PdfCheckBox* PdfCheckBox::GetParent() const
{
    return GetParentTyped<PdfCheckBox>(PdfFieldType::CheckBox);
}

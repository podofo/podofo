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
    if (!GetDictionary().HasKey("AP"))
        GetDictionary().AddKey("AP"_n, PdfDictionary());

    if (!GetDictionary().MustFindKey("AP").GetDictionary().HasKey("N"))
        GetDictionary().MustFindKey("AP").GetDictionary().AddKey("N"_n, PdfDictionary());

    GetDictionary().MustFindKey("AP").
        GetDictionary().MustFindKey("N").GetDictionary().AddKey(name, reference);
}

void PdfCheckBox::SetAppearanceChecked(const PdfXObject& xobj)
{
    this->AddAppearanceStream("Yes"_n, xobj.GetObject().GetIndirectReference());
}

void PdfCheckBox::SetAppearanceUnchecked(const PdfXObject& xobj)
{
    this->AddAppearanceStream("Off"_n, xobj.GetObject().GetIndirectReference());
}

void PdfCheckBox::SetChecked(bool isChecked)
{
    GetDictionary().AddKey("V"_n, (isChecked ? "Yes"_n : "Off"_n));
    GetDictionary().AddKey("AS"_n, (isChecked ? "Yes"_n : "Off"_n));
}

bool PdfCheckBox::IsChecked() const
{
    auto& dict = GetDictionary();
    if (dict.HasKey("V"))
    {
        auto& name = dict.MustFindKey("V").GetName();
        return (name == "Yes" || name == "On");
    }
    else if (dict.HasKey("AS"))
    {
        auto& name = dict.MustFindKey("AS").GetName();
        return (name == "Yes" || name == "On");
    }

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

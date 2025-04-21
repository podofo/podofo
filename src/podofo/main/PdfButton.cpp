/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfButton.h"
#include "PdfDictionary.h"

using namespace std;
using namespace PoDoFo;

PdfButton::PdfButton(PdfAcroForm& acroform, PdfFieldType fieldType,
        shared_ptr<PdfField>&& parent)
    : PdfField(acroform, fieldType, std::move(parent))
{
}

PdfButton::PdfButton(PdfAnnotationWidget& widget, PdfFieldType fieldType,
        shared_ptr<PdfField>&& parent)
    : PdfField(widget, fieldType, std::move(parent))
{
}

PdfButton::PdfButton(PdfObject& obj, PdfAcroForm* acroform, PdfFieldType fieldType)
    : PdfField(obj, acroform, fieldType)
{
}

bool PdfButton::IsPushButton() const
{
    return this->GetFieldFlag(static_cast<int>(PdfButton_PushButton), false);
}

bool PdfButton::IsCheckBox() const
{
    return (!this->GetFieldFlag(static_cast<int>(PdfButton_Radio), false) &&
        !this->GetFieldFlag(static_cast<int>(PdfButton_PushButton), false));
}

bool PdfButton::IsRadioButton() const
{
    return this->GetFieldFlag(static_cast<int>(PdfButton_Radio), false);
}

void PdfButton::SetCaption(nullable<const PdfString&> text)
{
    if (text.has_value())
    {
        GetWidget()->GetOrCreateAppearanceCharacteristics().SetCaption(*text);
    }
    else
    {
        auto apChars = GetWidget()->GetAppearanceCharacteristics();
        if (apChars != nullptr)
            apChars->SetCaption(nullptr);
    }
}

nullable<const PdfString&> PdfButton::GetCaption() const
{
    auto apChars = GetWidget()->GetAppearanceCharacteristics();
    if (apChars == nullptr)
        return { };

    return apChars->GetCaption();
}

PdfToggleButton::PdfToggleButton(PdfAcroForm& acroform, PdfFieldType fieldType,
        shared_ptr<PdfField>&& parent)
    : PdfButton(acroform, fieldType, std::move(parent))
{
}

PdfToggleButton::PdfToggleButton(PdfAnnotationWidget& widget, PdfFieldType fieldType,
        shared_ptr<PdfField>&& parent)
    : PdfButton(widget, fieldType, std::move(parent))
{
}

PdfToggleButton::PdfToggleButton(PdfObject& obj, PdfAcroForm* acroform, PdfFieldType fieldType)
    : PdfButton(obj, acroform, fieldType)
{
}

void PdfToggleButton::SetChecked(bool isChecked)
{
    // FIXME: This is incorrect, and should handle toggle buttons export values
    GetDictionary().AddKey("V"_n, (isChecked ? "Yes"_n : "Off"_n));
    GetDictionary().AddKey("AS"_n, (isChecked ? "Yes"_n : "Off"_n));
}

bool PdfToggleButton::IsChecked() const
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

void PdfToggleButton::SetAppearanceChecked(const PdfXObject& xobj)
{
    // TODO: Correct export value support
    MustGetWidget().PushAppearanceStream(xobj, PdfAppearanceType::Normal, "Yes"_n, false);
}

void PdfToggleButton::SetAppearanceUnchecked(const PdfXObject& xobj)
{
    MustGetWidget().PushAppearanceStream(xobj, PdfAppearanceType::Normal, "Off"_n, false);
}

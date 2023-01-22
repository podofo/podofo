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
        const shared_ptr<PdfField>& parent)
    : PdfField(acroform, fieldType, parent)
{
}

PdfButton::PdfButton(PdfAnnotationWidget& widget, PdfFieldType fieldType,
        const shared_ptr<PdfField>& parent)
    : PdfField(widget, fieldType, parent)
{
}

PdfButton::PdfButton(PdfObject& obj, PdfAcroForm* acroform, PdfFieldType fieldType)
    : PdfField(obj, acroform, fieldType)
{
}

bool PdfButton::IsPushButton() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfButton_PushButton), false);
}

bool PdfButton::IsCheckBox() const
{
    return (!this->GetFieldFlag(static_cast<int>(ePdfButton_Radio), false) &&
        !this->GetFieldFlag(static_cast<int>(ePdfButton_PushButton), false));
}

bool PdfButton::IsRadioButton() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfButton_Radio), false);
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
    const shared_ptr<PdfField>& parent)
    : PdfButton(acroform, fieldType, parent)
{
}

PdfToggleButton::PdfToggleButton(PdfAnnotationWidget& widget, PdfFieldType fieldType,
    const shared_ptr<PdfField>& parent)
    : PdfButton(widget, fieldType, parent)
{
}

PdfToggleButton::PdfToggleButton(PdfObject& obj, PdfAcroForm* acroform, PdfFieldType fieldType)
    : PdfButton(obj, acroform, fieldType)
{
}

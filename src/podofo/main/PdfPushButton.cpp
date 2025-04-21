/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfPushButton.h"
#include "PdfDictionary.h"

using namespace std;
using namespace PoDoFo;

PdfPushButton::PdfPushButton(PdfAcroForm& acroform, shared_ptr<PdfField>&& parent)
    : PdfButton(acroform, PdfFieldType::PushButton, std::move(parent))
{
    init();
}

PdfPushButton::PdfPushButton(PdfAnnotationWidget& widget, shared_ptr<PdfField>&& parent)
    : PdfButton(widget, PdfFieldType::PushButton, std::move(parent))
{
    init();
}

PdfPushButton::PdfPushButton(PdfObject& obj, PdfAcroForm* acroform)
    : PdfButton(obj, acroform, PdfFieldType::PushButton)
{
    // NOTE: Do not call init() here
}

void PdfPushButton::init()
{
    // make a push button
    this->SetFieldFlag(static_cast<int>(PdfButton_PushButton), true);
}

void PdfPushButton::SetRolloverCaption(nullable<const PdfString&> text)
{
    if (text.has_value())
    {
        GetWidget()->GetOrCreateAppearanceCharacteristics().SetRolloverCaption(*text);
    }
    else
    {
        auto apChars = GetWidget()->GetAppearanceCharacteristics();
        if (apChars != nullptr)
            apChars->SetRolloverCaption(nullptr);
    }
}

nullable<const PdfString&>  PdfPushButton::GetRolloverCaption() const
{
    auto apChars = GetWidget()->GetAppearanceCharacteristics();
    if (apChars == nullptr)
        return { };

    return apChars->GetRolloverCaption();
}

void PdfPushButton::SetAlternateCaption(nullable<const PdfString&> text)
{
    if (text.has_value())
    {
        GetWidget()->GetOrCreateAppearanceCharacteristics().SetAlternateCaption(*text);
    }
    else
    {
        auto apChars = GetWidget()->GetAppearanceCharacteristics();
        if (apChars != nullptr)
            apChars->SetAlternateCaption(nullptr);
    }

}

nullable<const PdfString&> PdfPushButton::GetAlternateCaption() const
{
    auto apChars = GetWidget()->GetAppearanceCharacteristics();
    if (apChars == nullptr)
        return { };

    return apChars->GetAlternateCaption();
}

PdfPushButton* PdfPushButton::GetParent()
{
    return GetParentTyped<PdfPushButton>(PdfFieldType::PushButton);
}

const PdfPushButton* PdfPushButton::GetParent() const
{
    return GetParentTyped<PdfPushButton>(PdfFieldType::PushButton);
}

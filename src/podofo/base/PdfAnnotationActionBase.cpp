/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfAnnotationActionBase.h"

#include "PdfDictionary.h"

using namespace std;
using namespace PoDoFo;

PdfAnnotationActionBase::PdfAnnotationActionBase(PdfPage& page, PdfAnnotationType annotType, const PdfRect& rect)
    : PdfAnnotation(page, annotType, rect)
{
}

PdfAnnotationActionBase::PdfAnnotationActionBase(PdfObject& obj, PdfAnnotationType annotType)
    : PdfAnnotation(obj, annotType)
{
}

void PdfAnnotationActionBase::SetAction(const shared_ptr<PdfAction>& action)
{
    GetDictionary().AddKey("A", action->GetObject().GetIndirectReference());
    m_Action = action;
}

shared_ptr<PdfAction> PdfAnnotationActionBase::GetAction() const
{
    return const_cast<PdfAnnotationActionBase&>(*this).getAction();
}

shared_ptr<PdfAction> PdfAnnotationActionBase::getAction()
{
    if (m_Action == nullptr)
    {
        auto obj = GetDictionary().FindKey("A");
        if (obj == nullptr)
            return nullptr;

        m_Action.reset(new PdfAction(*obj));
    }

    return m_Action;
}

PdfAppearanceCharacteristics::PdfAppearanceCharacteristics(PdfDocument& parent)
    : PdfDictionaryElement(parent)
{
}

PdfAppearanceCharacteristics::PdfAppearanceCharacteristics(PdfObject& obj)
    : PdfDictionaryElement(obj)
{
}

void PdfAppearanceCharacteristics::SetBorderColor(nullable<const PdfColor&> color)
{
    if (color.has_value())
        GetDictionary().AddKey("BC", color->ToArray());
    else
        GetDictionary().RemoveKey("BC");
}

PdfColor PdfAppearanceCharacteristics::GetBorderColor() const
{
    PdfColor color;
    auto colorObj = GetDictionary().FindKeyParent("BC");
    if (colorObj == nullptr
        || !PdfColor::TryCreateFromObject(*colorObj, color))
    {
        return { };
    }

    return color;
}

void PdfAppearanceCharacteristics::SetBackgroundColor(nullable<const PdfColor&> color)
{
    if (color.has_value())
        GetDictionary().AddKey("BG", color->ToArray());
    else
        GetDictionary().RemoveKey("BG");
}

PdfColor PdfAppearanceCharacteristics::GetBackgroundColor() const
{
    PdfColor color;
    auto colorObj = GetDictionary().FindKeyParent("BG");
    if (colorObj == nullptr
        || !PdfColor::TryCreateFromObject(*colorObj, color))
    {
        return { };
    }

    return color;
}

void PdfAppearanceCharacteristics::SetRolloverCaption(nullable<const PdfString&> text)
{
    if (text.has_value())
        GetDictionary().AddKey("RC", *text);
    else
        GetDictionary().RemoveKey("RC");
}

nullable<const PdfString&> PdfAppearanceCharacteristics::GetRolloverCaption() const
{
    auto obj = GetDictionary().FindKeyParent("RC");
    const PdfString* str;
    if (obj == nullptr || !obj->TryGetString(str))
        return { };

    return *str;
}

void PdfAppearanceCharacteristics::SetAlternateCaption(nullable<const PdfString&> text)
{
    if (text.has_value())
        GetDictionary().AddKey("AC", *text);
    else
        GetDictionary().RemoveKey("AC");
}

nullable<const PdfString&> PdfAppearanceCharacteristics::GetAlternateCaption() const
{
    auto obj = GetDictionary().FindKeyParent("AC");
    const PdfString* str;
    if (obj == nullptr || !obj->TryGetString(str))
        return { };

    return *str;
}

void PdfAppearanceCharacteristics::SetCaption(nullable<const PdfString&> text)
{
    if (text.has_value())
        GetDictionary().AddKey("CA", *text);
    else
        GetDictionary().RemoveKey("CA");
}

nullable<const PdfString&> PdfAppearanceCharacteristics::GetCaption() const
{
    auto obj = GetDictionary().FindKeyParent("CA");
    const PdfString* str;
    if (obj == nullptr || !obj->TryGetString(str))
        return { };

    return *str;
}

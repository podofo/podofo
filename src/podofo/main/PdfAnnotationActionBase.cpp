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

PdfAnnotationActionBase::PdfAnnotationActionBase(PdfPage& page, PdfAnnotationType annotType, const Rect& rect)
    : PdfAnnotation(page, annotType, rect)
{
}

PdfAnnotationActionBase::PdfAnnotationActionBase(PdfObject& obj, PdfAnnotationType annotType)
    : PdfAnnotation(obj, annotType)
{
}

void PdfAnnotationActionBase::SetAction(nullable<const PdfAction&> action)
{
    auto& dict = GetDictionary();
    if (action == nullptr)
    {
        dict.RemoveKey("A");
        m_Action *= nullptr;
    }
    else
    {
        m_Action = PdfAction::Create(*action);
        onActionSet();
        dict.AddKeyIndirect("A"_n, action->GetObject());
    }
}

nullable<PdfAction&> PdfAnnotationActionBase::GetAction()
{
    return getAction();
}

nullable<const PdfAction&> PdfAnnotationActionBase::GetAction() const
{
    return const_cast<PdfAnnotationActionBase&>(*this).getAction();
}

void PdfAnnotationActionBase::onActionSet()
{
    // Do nothing
}

void PdfAnnotationActionBase::ResetAction()
{
    m_Action *= nullptr;
    GetDictionary().RemoveKey("A");
}

nullable<PdfAction&> PdfAnnotationActionBase::getAction()
{
    if (!m_Action.has_value())
    {
        auto obj = GetDictionary().FindKey("A");
        if (obj == nullptr)
        {
            m_Action *= nullptr;
        }
        else
        {
            unique_ptr<PdfAction> action;
            if (PdfAction::TryCreateFromObject(*obj, action))
                m_Action = std::move(action);
        }
    }

    if (*m_Action == nullptr)
        return nullptr;
    else
        return **m_Action;
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
        GetDictionary().AddKey("BC"_n, color->ToArray());
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
        GetDictionary().AddKey("BG"_n, color->ToArray());
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
        GetDictionary().AddKey("RC"_n, *text);
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
        GetDictionary().AddKey("AC"_n, *text);
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
        GetDictionary().AddKey("CA"_n, *text);
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

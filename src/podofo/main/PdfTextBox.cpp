/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfTextBox.h"
#include "PdfDictionary.h"

using namespace std;
using namespace PoDoFo;

PdfTextBox::PdfTextBox(PdfAcroForm& acroform, shared_ptr<PdfField>&& parent)
    : PdfField(acroform, PdfFieldType::TextBox, std::move(parent))
{
    init();
}

PdfTextBox::PdfTextBox(PdfAnnotationWidget& widget, shared_ptr<PdfField>&& parent)
    : PdfField(widget, PdfFieldType::TextBox, std::move(parent))
{
    init();
}

PdfTextBox::PdfTextBox(PdfObject& obj, PdfAcroForm* acroform)
    : PdfField(obj, acroform, PdfFieldType::TextBox)
{
    // NOTE: Do not call init() here
}

void PdfTextBox::init()
{
    if (!GetDictionary().HasKey("DS"))
        GetDictionary().AddKey("DS"_n, PdfString("font: 12pt Helvetica"));
}

void PdfTextBox::SetText(nullable<const PdfString&> text)
{
    AssertTerminalField();
    auto key = this->IsRichText() ? "RV"_n : "V"_n;
    if (text.has_value())
    {

        // if text is longer than maxlen, truncate it
        int64_t maxLength = this->GetMaxLen();
        if (maxLength != -1 && text->GetString().length() > (unsigned)maxLength)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Unable to set text larger MaxLen");

        GetDictionary().AddKey(key, *text);
    }
    else
    {
        GetDictionary().RemoveKey(key);
    }
}

nullable<const PdfString&> PdfTextBox::GetText() const
{
    AssertTerminalField();
    auto key = this->IsRichText() ? "RV"_n : "V"_n;
    auto obj = GetDictionary().FindKeyParent(key);
    const PdfString* str;
    if (obj == nullptr || !obj->TryGetString(str))
        return { };

    return *str;
}

void PdfTextBox::SetMaxLen(int64_t maxLen)
{
    GetDictionary().AddKey("MaxLen"_n, maxLen);
}

int64_t PdfTextBox::GetMaxLen() const
{
    int64_t ret;
    auto found = GetDictionary().FindKeyParent("MaxLen");
    if (found == nullptr || found->TryGetNumber(ret))
        return -1;

    return ret;
}

void PdfTextBox::SetMultiLine(bool multiLine)
{
    this->SetFieldFlag(static_cast<int>(PdfTextBox_MultiLine), multiLine);
}

bool PdfTextBox::IsMultiLine() const
{
    return this->GetFieldFlag(static_cast<int>(PdfTextBox_MultiLine), false);
}

void PdfTextBox::SetPasswordField(bool password)
{
    this->SetFieldFlag(static_cast<int>(PdfTextBox_Password), password);
}

bool PdfTextBox::IsPasswordField() const
{
    return this->GetFieldFlag(static_cast<int>(PdfTextBox_Password), false);
}

void PdfTextBox::SetFileField(bool file)
{
    this->SetFieldFlag(static_cast<int>(PdfTextBox_FileSelect), file);
}

bool PdfTextBox::IsFileField() const
{
    return this->GetFieldFlag(static_cast<int>(PdfTextBox_FileSelect), false);
}

void PdfTextBox::SetSpellCheckingEnabled(bool spellcheck)
{
    this->SetFieldFlag(static_cast<int>(PdfTextBox_NoSpellcheck), !spellcheck);
}

bool PdfTextBox::IsSpellCheckingEnabled() const
{
    return this->GetFieldFlag(static_cast<int>(PdfTextBox_NoSpellcheck), true);
}

void PdfTextBox::SetScrollBarsEnabled(bool scroll)
{
    this->SetFieldFlag(static_cast<int>(PdfTextBox_NoScroll), !scroll);
}

bool PdfTextBox::IsScrollBarsEnabled() const
{
    return this->GetFieldFlag(static_cast<int>(PdfTextBox_NoScroll), true);
}

void PdfTextBox::SetCombs(bool combs)
{
    this->SetFieldFlag(static_cast<int>(PdfTextBox_Comb), combs);
}

bool PdfTextBox::IsCombs() const
{
    return this->GetFieldFlag(static_cast<int>(PdfTextBox_Comb), false);
}

void PdfTextBox::SetRichText(bool richText)
{
    this->SetFieldFlag(static_cast<int>(PdfTextBox_RichText), richText);
}

bool PdfTextBox::IsRichText() const
{
    return this->GetFieldFlag(static_cast<int>(PdfTextBox_RichText), false);
}

PdfTextBox* PdfTextBox::GetParent()
{
    return GetParentTyped<PdfTextBox>(PdfFieldType::TextBox);
}

const PdfTextBox* PdfTextBox::GetParent() const
{
    return GetParentTyped<PdfTextBox>(PdfFieldType::TextBox);
}

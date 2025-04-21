/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfChoiceField.h"

#include "PdfArray.h"
#include "PdfDictionary.h"

using namespace std;
using namespace PoDoFo;

PdChoiceField::PdChoiceField(PdfAcroForm& acroform, PdfFieldType fieldType,
        shared_ptr<PdfField>&& parent)
    : PdfField(acroform, fieldType, std::move(parent))
{
}

PdChoiceField::PdChoiceField(PdfAnnotationWidget& widget, PdfFieldType fieldType,
        shared_ptr<PdfField>&& parent)
    : PdfField(widget, fieldType, std::move(parent))
{
}

PdChoiceField::PdChoiceField(PdfObject& obj, PdfAcroForm* acroform, PdfFieldType fieldType)
    : PdfField(obj, acroform, fieldType)
{
}

void PdChoiceField::InsertItem(const PdfString& value, nullable<const PdfString&> displayName)
{
    PdfObject objToAdd;
    if (displayName.has_value())
    {
        PdfArray array;
        array.Add(value);
        array.Add(*displayName);
        objToAdd = array;
    }
    else
    {
        objToAdd = value;
    }

    auto optObj = GetDictionary().FindKey("Opt");
    if (optObj == nullptr)
        optObj = &GetDictionary().AddKey("Opt"_n, PdfArray());

    // TODO: Sorting
    optObj->GetArray().Add(objToAdd);

    // CHECK-ME: Should I set /V /I here?
}

void PdChoiceField::RemoveItem(unsigned index)
{
    auto optObj = GetDictionary().FindKey("Opt");
    if (optObj == nullptr)
        return;

    auto& arr = optObj->GetArray();
    if (index >= arr.size())
        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

    arr.RemoveAt(index);
}

PdfString PdChoiceField::GetItem(unsigned index) const
{
    auto opt = GetDictionary().FindKey("Opt");
    if (opt == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::ObjectNotFound);

    auto& optArray = opt->GetArray();
    if (index >= optArray.GetSize())
        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

    auto& item = optArray[index];
    if (item.IsArray())
    {
        auto& itemArray = item.GetArray();
        if (itemArray.size() < 2)
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);
        }
        else
            return itemArray.MustFindAt(0).GetString();
    }

    return item.GetString();
}

nullable<const PdfString&> PdChoiceField::GetItemDisplayText(int index) const
{
    auto* opt = GetDictionary().FindKey("Opt");
    if (opt == nullptr)
        return { };

    auto& optArray = opt->GetArray();
    if (index < 0 || index >= static_cast<int>(optArray.size()))
    {
        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);
    }

    auto& item = optArray[index];
    if (item.IsArray())
    {
        auto& itemArray = item.GetArray();
        if (itemArray.size() < 2)
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);
        }
        else
            return itemArray.MustFindAt(1).GetString();
    }

    return item.GetString();
}

unsigned PdChoiceField::GetItemCount() const
{
    auto* opt = GetDictionary().FindKey("Opt");
    if (opt == nullptr)
        return 0;

    return opt->GetArray().GetSize();
}

void PdChoiceField::SetSelectedIndex(int index)
{
    AssertTerminalField();
    PdfString selected = this->GetItem(index);
    GetDictionary().AddKey("V"_n, selected);
}

int PdChoiceField::GetSelectedIndex() const
{
    AssertTerminalField();
    auto* valueObj = GetDictionary().FindKey("V");
    if (valueObj == nullptr || !valueObj->IsString())
        return -1;

    auto& value = valueObj->GetString();
    auto* opt = GetDictionary().FindKey("Opt");
    if (opt == nullptr)
        return -1;

    auto& optArray = opt->GetArray();
    for (unsigned i = 0; i < optArray.GetSize(); i++)
    {
        auto& found = optArray.MustFindAt(i);
        if (found.IsString())
        {
            if (found.GetString() == value)
                return i;
        }
        else if (found.IsArray())
        {
            auto& arr = found.GetArray();
            if (arr.MustFindAt(0).GetString() == value)
                return i;
        }
        else
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Choice field item has invalid data type");
        }
    }

    return -1;
}

bool PdChoiceField::IsComboBox() const
{
    return this->GetFieldFlag(static_cast<int>(PdfListField_Combo), false);
}

void PdChoiceField::SetSpellCheckingEnabled(bool spellCheck)
{
    this->SetFieldFlag(static_cast<int>(PdfListField_NoSpellcheck), !spellCheck);
}

bool PdChoiceField::IsSpellCheckingEnabled() const
{
    return this->GetFieldFlag(static_cast<int>(PdfListField_NoSpellcheck), true);
}

void PdChoiceField::SetSorted(bool sorted)
{
    this->SetFieldFlag(static_cast<int>(PdfListField_Sort), sorted);
}

bool PdChoiceField::IsSorted() const
{
    return this->GetFieldFlag(static_cast<int>(PdfListField_Sort), false);
}

void PdChoiceField::SetMultiSelect(bool multi)
{
    this->SetFieldFlag(static_cast<int>(PdfListField_MultiSelect), multi);
}

bool PdChoiceField::IsMultiSelect() const
{
    return this->GetFieldFlag(static_cast<int>(PdfListField_MultiSelect), false);
}

void PdChoiceField::SetCommitOnSelectionChange(bool commit)
{
    this->SetFieldFlag(static_cast<int>(PdfListField_CommitOnSelChange), commit);
}

bool PdChoiceField::IsCommitOnSelectionChange() const
{
    return this->GetFieldFlag(static_cast<int>(PdfListField_CommitOnSelChange), false);
}

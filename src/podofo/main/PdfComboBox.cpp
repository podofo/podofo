/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfComboBox.h"

using namespace std;
using namespace PoDoFo;


PdfComboBox::PdfComboBox(PdfAcroForm& acroform, shared_ptr<PdfField>&& parent)
    : PdChoiceField(acroform, PdfFieldType::ComboBox, std::move(parent))
{
    this->SetFieldFlag(static_cast<int>(PdfListField_Combo), true);
}

PdfComboBox::PdfComboBox(PdfAnnotationWidget& widget, shared_ptr<PdfField>&& parent)
    : PdChoiceField(widget, PdfFieldType::ComboBox, std::move(parent))
{
    this->SetFieldFlag(static_cast<int>(PdfListField_Combo), true);
}

PdfComboBox::PdfComboBox(PdfObject& obj, PdfAcroForm* acroform)
    : PdChoiceField(obj, acroform, PdfFieldType::ComboBox)
{
    // NOTE: Do not do other initializations here
}

void PdfComboBox::SetEditable(bool edit)
{
    this->SetFieldFlag(static_cast<int>(PdfListField_Edit), edit);
}

bool PdfComboBox::IsEditable() const
{
    return this->GetFieldFlag(static_cast<int>(PdfListField_Edit), false);
}

PdfComboBox* PdfComboBox::GetParent()
{
    return GetParentTyped<PdfComboBox>(PdfFieldType::ComboBox);
}

const PdfComboBox* PdfComboBox::GetParent() const
{
    return GetParentTyped<PdfComboBox>(PdfFieldType::ComboBox);
}

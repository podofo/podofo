/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfListBox.h"

using namespace std;
using namespace PoDoFo;

PdfListBox::PdfListBox(PdfAcroForm& acroform, shared_ptr<PdfField>&& parent)
    : PdChoiceField(acroform, PdfFieldType::ListBox, std::move(parent))
{
    this->SetFieldFlag(static_cast<int>(PdfListField_Combo), false);
}

PdfListBox::PdfListBox(PdfAnnotationWidget& widget, shared_ptr<PdfField>&& parent)
    : PdChoiceField(widget, PdfFieldType::ListBox, std::move(parent))
{
    this->SetFieldFlag(static_cast<int>(PdfListField_Combo), false);
}

PdfListBox::PdfListBox(PdfObject& obj, PdfAcroForm* acroform)
    : PdChoiceField(obj, acroform, PdfFieldType::ListBox)
{
    // NOTE: Do not do other initializations here
}

PdfListBox* PdfListBox::GetParent()
{
    return GetParentTyped<PdfListBox>(PdfFieldType::ListBox);
}

const PdfListBox* PdfListBox::GetParent() const
{
    return GetParentTyped<PdfListBox>(PdfFieldType::ListBox);
}

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

PdfCheckBox::PdfCheckBox(PdfAcroForm& acroform, shared_ptr<PdfField>&& parent)
    : PdfToggleButton(acroform, PdfFieldType::CheckBox, std::move(parent))
{
}

PdfCheckBox::PdfCheckBox(PdfAnnotationWidget& widget, shared_ptr<PdfField>&& parent)
    : PdfToggleButton(widget, PdfFieldType::CheckBox, std::move(parent))
{
}

PdfCheckBox::PdfCheckBox(PdfObject& obj, PdfAcroForm* acroform)
    : PdfToggleButton(obj, acroform, PdfFieldType::CheckBox)
{
}

PdfCheckBox* PdfCheckBox::GetParent()
{
    return GetParentTyped<PdfCheckBox>(PdfFieldType::CheckBox);
}

const PdfCheckBox* PdfCheckBox::GetParent() const
{
    return GetParentTyped<PdfCheckBox>(PdfFieldType::CheckBox);
}

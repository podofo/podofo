/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfRadioButton.h"

using namespace std;
using namespace PoDoFo;

PdfRadioButton::PdfRadioButton(PdfAcroForm& acroform, const shared_ptr<PdfField>& parent)
    : PdfToggleButton(acroform, PdfFieldType::RadioButton, parent)
{
}

PdfRadioButton::PdfRadioButton(PdfAnnotationWidget& widget, const shared_ptr<PdfField>& parent)
    : PdfToggleButton(widget, PdfFieldType::RadioButton, parent)
{
}

PdfRadioButton::PdfRadioButton(PdfObject& obj, PdfAcroForm* acroform)
    : PdfToggleButton(obj, acroform, PdfFieldType::RadioButton)
{
}

PdfRadioButton* PdfRadioButton::GetParent()
{
    return GetParentTyped<PdfRadioButton>(PdfFieldType::RadioButton);
}

const PdfRadioButton* PdfRadioButton::GetParent() const
{
    return GetParentTyped<PdfRadioButton>(PdfFieldType::RadioButton);
}

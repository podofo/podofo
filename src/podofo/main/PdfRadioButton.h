/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_RADIO_BUTTON_H
#define PDF_RADIO_BUTTON_H

#include "PdfButton.h"

namespace PoDoFo
{
    /** A radio button
     * TODO: This is just a stub
     */
    class PODOFO_API PdfRadioButton final : public PdfToggleButton
    {
        friend class PdfField;

    private:
        PdfRadioButton(PdfAcroForm& acroform, std::shared_ptr<PdfField>&& parent);

        PdfRadioButton(PdfAnnotationWidget& widget, std::shared_ptr<PdfField>&& parent);

        PdfRadioButton(PdfObject& obj, PdfAcroForm* acroform);

    public:
        PdfRadioButton* GetParent();
        const PdfRadioButton* GetParent() const;
    };
}

#endif // PDF_RADIO_BUTTON_H

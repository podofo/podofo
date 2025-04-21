/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_CHECKBOX_H
#define PDF_CHECKBOX_H

#include "PdfButton.h"
#include "PdfXObject.h"

namespace PoDoFo
{
    /** A checkbox can be checked or unchecked by the user
     */
    class PODOFO_API PdfCheckBox final : public PdfToggleButton
    {
        friend class PdfField;

    private:
        PdfCheckBox(PdfAcroForm& acroform, std::shared_ptr<PdfField>&& parent);

        PdfCheckBox(PdfAnnotationWidget& widget, std::shared_ptr<PdfField>&& parent);

        PdfCheckBox(PdfObject& obj, PdfAcroForm* acroform);

    public:
        PdfCheckBox* GetParent();
        const PdfCheckBox* GetParent() const;
    };
}

#endif // PDF_CHECKBOX_H

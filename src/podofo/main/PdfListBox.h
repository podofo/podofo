/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_LISTBOX_H
#define PDF_LISTBOX_H

#include "PdfChoiceField.h"

namespace PoDoFo
{
    /** A list box
     */
    class PODOFO_API PdfListBox final : public PdChoiceField
    {
        friend class PdfField;

    private:
        PdfListBox(PdfAcroForm& acroform, std::shared_ptr<PdfField>&& parent);

        PdfListBox(PdfAnnotationWidget& widget, std::shared_ptr<PdfField>&& parent);

        PdfListBox(PdfObject& obj, PdfAcroForm* acroform);

    public:
        PdfListBox* GetParent();
        const PdfListBox* GetParent() const;
    };
}

#endif // PDF_LISTBOX_H

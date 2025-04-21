/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_COMBOBOX_H
#define PDF_COMBOBOX_H

#include "PdfChoiceField.h"

namespace PoDoFo
{
    /** A combo box with a drop down list of items.
     */
    class PODOFO_API PdfComboBox final : public PdChoiceField
    {
        friend class PdfField;

    private:
        PdfComboBox(PdfAcroForm& acroform, std::shared_ptr<PdfField>&& parent);

        PdfComboBox(PdfAnnotationWidget& widget, std::shared_ptr<PdfField>&& parent);

        PdfComboBox(PdfObject& obj, PdfAcroForm* acroform);

    public:
        /**
         * Sets the combobox to be editable
         *
         * \param edit if true the combobox can be edited by the user
         *
         * By default a combobox is not editable
         */
        void SetEditable(bool edit);

        /**
         *  \returns true if this is an editable combobox
         */
        bool IsEditable() const;

        PdfComboBox* GetParent();
        const PdfComboBox* GetParent() const;
    };
}

#endif // PDF_COMBOBOX_H

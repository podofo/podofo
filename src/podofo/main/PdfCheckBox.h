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
    class PODOFO_API PdfCheckBox : public PdfToggleButton
    {
        friend class PdfField;

    private:
        PdfCheckBox(PdfAcroForm& acroform, const std::shared_ptr<PdfField>& parent);

        PdfCheckBox(PdfAnnotationWidget& widget, const std::shared_ptr<PdfField>& parent);

        PdfCheckBox(PdfObject& obj, PdfAcroForm* acroform);

    public:
        /** Set the appearance stream which is displayed when the checkbox
         *  is checked.
         *
         *  \param rXObject an xobject which contains the drawing commands for a checked checkbox
         */
        void SetAppearanceChecked(const PdfXObject& xobj);

        /** Set the appearance stream which is displayed when the checkbox
         *  is unchecked.
         *
         *  \param rXObject an xobject which contains the drawing commands for an unchecked checkbox
         */
        void SetAppearanceUnchecked(const PdfXObject& xobj);

        /** Sets the state of this checkbox
         *
         *  \param isChecked if true the checkbox will be checked
         */
        void SetChecked(bool isChecked);

        /**
         * \returns true if the checkbox is checked
         */
        bool IsChecked() const;

        PdfCheckBox* GetParent();
        const PdfCheckBox* GetParent() const;

    private:

        /** Add a appearance stream to this checkbox
         *
         *  \param name name of the appearance stream
         *  \param reference reference to the XObject containing the appearance stream
         */
        void AddAppearanceStream(const PdfName& name, const PdfReference& reference);
    };
}

#endif // PDF_CHECKBOX_H

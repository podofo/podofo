/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_BUTTON_H
#define PDF_BUTTON_H

#include "PdfField.h"

namespace PoDoFo
{
    class PODOFO_API PdfButton : public PdfField
    {
        friend class PdfPushButton;
        friend class PdfToggleButton;

    private:
        PdfButton(PdfAcroForm& acroform, PdfFieldType fieldType,
            const std::shared_ptr<PdfField>& parent);

        PdfButton(PdfAnnotationWidget& widget, PdfFieldType fieldType,
            const std::shared_ptr<PdfField>& parent);

        PdfButton(PdfObject& obj, PdfAcroForm* acroform, PdfFieldType fieldType);

    public:
        /**
         * \returns true if this is a pushbutton
         */
        bool IsPushButton() const;

        /**
         * \returns true if this is a checkbox
         */
        bool IsCheckBox() const;

        /**
         * \returns true if this is a radiobutton
         */
        bool IsRadioButton() const;

        /** Set the normal caption of this button
         *
         *  \param text the caption
         */
        void SetCaption(nullable<const PdfString&> text);

        /**
         *  \returns the caption of this button
         */
        nullable<const PdfString&> GetCaption() const;
    };

    class PODOFO_API PdfToggleButton : public PdfButton
    {
        friend class PdfCheckBox;
        friend class PdfRadioButton;

    private:
        PdfToggleButton(PdfAcroForm& acroform, PdfFieldType fieldType,
            const std::shared_ptr<PdfField>& parent);

        PdfToggleButton(PdfAnnotationWidget& widget, PdfFieldType fieldType,
            const std::shared_ptr<PdfField>& parent);

        PdfToggleButton(PdfObject& obj, PdfAcroForm* acroform, PdfFieldType fieldType);
    };
}

#endif // PDF_BUTTON_H

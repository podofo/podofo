/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_PUSH_BUTTON_H
#define PDF_PUSH_BUTTON_H

#include "PdfButton.h"

namespace PoDoFo
{
    /** A push button is a button which has no state and value
     *  but can toggle actions.
     */
    class PODOFO_API PdfPushButton final : public PdfButton
    {
        friend class PdfField;

    private:
        PdfPushButton(PdfAcroForm& acroform, std::shared_ptr<PdfField>&& parent);

        PdfPushButton(PdfAnnotationWidget& widget, std::shared_ptr<PdfField>&& parent);

        PdfPushButton(PdfObject& obj, PdfAcroForm* acroform);

    public:
        /** Set the rollover caption of this button
         *  which is displayed when the cursor enters the field
         *  without the mouse button being pressed
         *
         *  \param text the caption
         */
        void SetRolloverCaption(nullable<const PdfString&> text);

        /**
         *  \returns the rollover caption of this button
         */
        nullable<const PdfString&> GetRolloverCaption() const;

        /** Set the alternate caption of this button
         *  which is displayed when the button is pressed.
         *
         *  \param text the caption
         */
        void SetAlternateCaption(nullable<const PdfString&> text);

        /**
         *  \returns the rollover caption of this button
         */
        nullable<const PdfString&> GetAlternateCaption() const;

        PdfPushButton* GetParent();
        const PdfPushButton* GetParent() const;

    private:
        void init();
    };
}

#endif // PDF_PUSH_BUTTON_H

/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_ANNOTATION_ACTION_BASE_H
#define PDF_ANNOTATION_ACTION_BASE_H

#include "PdfAnnotation.h"
#include "PdfAction.h"
#include "PdfDictionary.h"

namespace PoDoFo {

    class PODOFO_API PdfAppearanceCharacteristics final : public PdfDictionaryElement
    {
        template<typename T>
        friend class PdfAppearanceCharacteristicsProvider;

    private:
        PdfAppearanceCharacteristics(PdfDocument& parent);
        PdfAppearanceCharacteristics(PdfObject& obj);

    public:
        void SetBorderColor(nullable<const PdfColor&> color);

        PdfColor GetBorderColor() const;

        void SetBackgroundColor(nullable<const PdfColor&> color);

        PdfColor GetBackgroundColor() const;

        void SetRolloverCaption(nullable<const PdfString&> text);

        nullable<const PdfString&> GetRolloverCaption() const;

        void SetAlternateCaption(nullable<const PdfString&> text);

        nullable<const PdfString&> GetAlternateCaption() const;

        void SetCaption(nullable<const PdfString&> text);

        nullable<const PdfString&> GetCaption() const;
    };

    template <typename T>
    class PdfAppearanceCharacteristicsProvider
    {
        friend class PdfAnnotationWidget;
        friend class PdfAnnotationScreen;

    public:
        PdfAppearanceCharacteristicsProvider()
        {
            auto& dict = static_cast<T&>(*this).GetDictionary();
            auto mkObj = dict.FindKey("MK");
            if (mkObj != nullptr)
                m_AppearanceCharacteristics.reset(new PdfAppearanceCharacteristics(*mkObj));
        }

    public:
        PdfAppearanceCharacteristics& GetOrCreateAppearanceCharacteristics()
        {
            if (m_AppearanceCharacteristics == nullptr)
            {
                auto& ref = static_cast<T&>(*this);
                m_AppearanceCharacteristics.reset(new PdfAppearanceCharacteristics(ref.GetDocument()));
                ref.GetDictionary().AddKeyIndirect("MK"_n, m_AppearanceCharacteristics->GetObject());
            }

            return *m_AppearanceCharacteristics;
        }

        PdfAppearanceCharacteristics* GetAppearanceCharacteristics()
        {
            return m_AppearanceCharacteristics.get();
        }

        const PdfAppearanceCharacteristics* GetAppearanceCharacteristics() const
        {
            return m_AppearanceCharacteristics.get();
        }

    private:
        std::unique_ptr<PdfAppearanceCharacteristics> m_AppearanceCharacteristics;
    };

    class PODOFO_API PdfAnnotationActionBase : public PdfAnnotation
    {
        friend class PdfAnnotationWidget;
        friend class PdfAnnotationLink;
        friend class PdfAnnotationScreen;

    private:
        PdfAnnotationActionBase(PdfPage& page, PdfAnnotationType annotType, const Rect& rect);
        PdfAnnotationActionBase(PdfObject& obj, PdfAnnotationType annotType);

    public:
        /** Set the action that is executed for this annotation
         *  \param action an action object
         *
         *  \see GetAction
         */
        void SetAction(nullable<const PdfAction&> action);

        /** Get the action that is executed for this annotation
         *  \returns an action object. The action object is owned
         *           by the PdfAnnotation.
         *
         *  \see SetAction
         */
        nullable<PdfAction&> GetAction();
        nullable<const PdfAction&> GetAction() const;

    protected:
        virtual void onActionSet();
        void ResetAction();

    private:
        nullable<PdfAction&> getAction();

    private:
        nullable<std::unique_ptr<PdfAction>> m_Action;
    };
}

#endif // PDF_ANNOTATION_ACTION_BASE_H

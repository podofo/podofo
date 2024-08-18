/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_ANNOTATION_TYPES_H
#define PDF_ANNOTATION_TYPES_H

#include "PdfAnnotationActionBase.h"
#include "PdfFileSpec.h"
#include "PdfDestination.h"
#include "PdfDictionary.h"

namespace PoDoFo {

    template <typename T>
    class PdfQuadPointsProvider
    {
        friend class PdfAnnotationLink;
        friend class PdfAnnotationTextMarkupBase;
        friend class PdfAnnotationRedact;

    public:
        /** Get the quad points associated with the annotation (if appropriate).
         *  This array is used in text markup annotations to describe the
         *  regions affected by the markup (i.e. the highlighted words, one
         *  quadrilateral per word)
         *
         *  \returns a PdfArray of 8xn numbers describing the
         *           x,y coordinates of BL BR TR TL corners of the
         *           quadrilaterals. If inappropriate, returns
         *           an empty array.
         */
        nullable<const PdfArray&> GetQuadPoints() const
        {
            auto& dict = static_cast<const T&>(*this).GetDictionary();
            const PdfArray* arr;
            auto obj = dict.FindKey("QuadPoints");
            if (obj == nullptr || !obj->TryGetArray(arr))
                return { };

            return *arr;
        }

        /** Set the quad points associated with the annotation (if appropriate).
         *  This array is used in text markup annotations to describe the
         *  regions affected by the markup (i.e. the highlighted words, one
         *  quadrilateral per word)
         *
         *  \param quadPoints a PdfArray of 8xn numbers describing the
         *           x,y coordinates of BL BR TR TL corners of the
         *           quadrilaterals.
         */
        void SetQuadPoints(nullable<const PdfArray&> quadPoints)
        {
            auto& dict = static_cast<T&>(*this).GetDictionary();
            if (quadPoints == nullptr)
                dict.RemoveKey("QuadPoints");
            else
                dict.AddKey("QuadPoints"_n, *quadPoints);
        }
    };

    class PODOFO_API PdfAnnotationTextMarkupBase : public PdfAnnotation, public PdfQuadPointsProvider<PdfAnnotationTextMarkupBase>
    {
        friend class PdfAnnotationSquiggly;
        friend class PdfAnnotationHighlight;
        friend class PdfAnnotationStrikeOut;
        friend class PdfAnnotationUnderline;
    private:
        PdfAnnotationTextMarkupBase(PdfPage& page, PdfAnnotationType annotType, const Rect& rect);
        PdfAnnotationTextMarkupBase(PdfObject& obj, PdfAnnotationType annotType);

    };

    class PODOFO_API PdfAnnotationCaret final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationCaret(PdfPage& page, const Rect& rect);
        PdfAnnotationCaret(PdfObject& obj);
    };


    class PODOFO_API PdfAnnotationFileAttachment final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationFileAttachment(PdfPage& page, const Rect& rect);
        PdfAnnotationFileAttachment(PdfObject& obj);

    public:
        /** Set a file attachment for this annotation.
         *  The type of this annotation has to be
         *  PdfAnnotationType::FileAttachement for file
         *  attachements to work.
         *
         *  \param fileSpec a file specification
         */
        void SetFileAttachment(const nullable<PdfFileSpec&>& fileSpec);

        /** Get a file attachement of this annotation.
         *  \returns a file specification object. The file specification object is owned
         *           by the PdfAnnotation.
         *
         *  \see SetFileAttachement
         */
        nullable<PdfFileSpec&> GetFileAttachment();
        nullable<const PdfFileSpec&> GetFileAttachment() const;

    private:
        nullable<PdfFileSpec&> getFileAttachment();

    private:
        nullable<std::unique_ptr<PdfFileSpec>> m_FileSpec;
    };

    class PODOFO_API PdfAnnotationFreeText final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationFreeText(PdfPage& page, const Rect& rect);
        PdfAnnotationFreeText(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationHighlight final : public PdfAnnotationTextMarkupBase
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationHighlight(PdfPage& page, const Rect& rect);
        PdfAnnotationHighlight(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationInk final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationInk(PdfPage& page, const Rect& rect);
        PdfAnnotationInk(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationLine final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationLine(PdfPage& page, const Rect& rect);
        PdfAnnotationLine(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationLink final : public PdfAnnotationActionBase, public PdfQuadPointsProvider<PdfAnnotationLink>
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationLink(PdfPage& page, const Rect& rect);
        PdfAnnotationLink(PdfObject& obj);
    public:
        /** Set the destination for link annotations
         *  \param destination target of the link
         *
         *  \see GetDestination
         */
        void SetDestination(nullable<const PdfDestination&> destination);

        /** Get the destination of a link annotations
         *
         *  \returns a destination object
         *  \see SetDestination
         */
        nullable<PdfDestination&> GetDestination();
        nullable<const PdfDestination&> GetDestination() const;

    private:
        nullable<PdfDestination&> getDestination();
        void onActionSet() override;

    private:
        nullable<std::unique_ptr<PdfDestination>> m_Destination;
    };

    class PODOFO_API PdfAnnotationModel3D final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationModel3D(PdfPage& page, const Rect& rect);
        PdfAnnotationModel3D(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationMovie final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationMovie(PdfPage& page, const Rect& rect);
        PdfAnnotationMovie(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationPolygon final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationPolygon(PdfPage& page, const Rect& rect);
        PdfAnnotationPolygon(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationPolyLine final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationPolyLine(PdfPage& page, const Rect& rect);
        PdfAnnotationPolyLine(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationPopup final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationPopup(PdfPage& page, const Rect& rect);
        PdfAnnotationPopup(PdfObject& obj);
    public:
        /** Sets whether this annotation is initially open.
         *  You should always set this true for popup annotations.
         *  \param b if true open it
         */
        void SetOpen(const nullable<bool>& value);

        /**
         * \returns true if this annotation should be opened immediately
         *          by the viewer
         */
        bool GetOpen() const;
    };

    class PODOFO_API PdfAnnotationPrinterMark final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationPrinterMark(PdfPage& page, const Rect& rect);
        PdfAnnotationPrinterMark(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationScreen final :
        public PdfAnnotationActionBase,
        public PdfAppearanceCharacteristicsProvider<PdfAnnotationScreen>
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationScreen(PdfPage& page, const Rect& rect);
        PdfAnnotationScreen(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationSquiggly final : public PdfAnnotationTextMarkupBase
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationSquiggly(PdfPage& page, const Rect& rect);
        PdfAnnotationSquiggly(PdfObject& obj);

    };

    class PODOFO_API PdfAnnotationStrikeOut final : public PdfAnnotationTextMarkupBase
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationStrikeOut(PdfPage& page, const Rect& rect);
        PdfAnnotationStrikeOut(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationSound final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationSound(PdfPage& page, const Rect& rect);
        PdfAnnotationSound(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationSquare final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationSquare(PdfPage& page, const Rect& rect);
        PdfAnnotationSquare(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationCircle final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationCircle(PdfPage& page, const Rect& rect);
        PdfAnnotationCircle(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationStamp final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationStamp(PdfPage& page, const Rect& rect);
        PdfAnnotationStamp(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationText final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationText(PdfPage& page, const Rect& rect);
        PdfAnnotationText(PdfObject& obj);
    public:
        /** Sets whether this annotation is initially open.
         *  You should always set this true for popup annotations.
         *  \param b if true open it
         */
        void SetOpen(const nullable<bool>& value);

        /**
         * \returns true if this annotation should be opened immediately
         *          by the viewer
         */
        bool GetOpen() const;
    };

    class PODOFO_API PdfAnnotationTrapNet final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationTrapNet(PdfPage& page, const Rect& rect);
        PdfAnnotationTrapNet(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationUnderline final : public PdfAnnotationTextMarkupBase
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationUnderline(PdfPage& page, const Rect& rect);
        PdfAnnotationUnderline(PdfObject& obj);

    };

    class PODOFO_API PdfAnnotationWatermark final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationWatermark(PdfPage& page, const Rect& rect);
        PdfAnnotationWatermark(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationWebMedia final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationWebMedia(PdfPage& page, const Rect& rect);
        PdfAnnotationWebMedia(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationRedact final : public PdfAnnotation, public PdfQuadPointsProvider<PdfAnnotationRedact>
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationRedact(PdfPage& page, const Rect& rect);
        PdfAnnotationRedact(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationProjection final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationProjection(PdfPage& page, const Rect& rect);
        PdfAnnotationProjection(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationRichMedia final : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationRichMedia(PdfPage& page, const Rect& rect);
        PdfAnnotationRichMedia(PdfObject& obj);
    };
}

#endif // PDF_ANNOTATION_TYPES_H

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
         *  regions affected by the markup (i.e. the hilighted words, one
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
         *  regions affected by the markup (i.e. the hilighted words, one
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
                dict.AddKey("QuadPoints", *quadPoints);
        }
    };

    class PODOFO_API PdfAnnotationTextMarkupBase : public PdfAnnotation, public PdfQuadPointsProvider<PdfAnnotationTextMarkupBase>
    {
        friend class PdfAnnotationSquiggly;
        friend class PdfAnnotationHighlight;
        friend class PdfAnnotationStrikeOut;
        friend class PdfAnnotationUnderline;
    private:
        PdfAnnotationTextMarkupBase(PdfPage& page, PdfAnnotationType annotType, const PdfRect& rect);
        PdfAnnotationTextMarkupBase(PdfObject& obj, PdfAnnotationType annotType);

    };

    class PODOFO_API PdfAnnotationCaret : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationCaret(PdfPage& page, const PdfRect& rect);
        PdfAnnotationCaret(PdfObject& obj);
    };


    class PODOFO_API PdfAnnotationFileAttachement : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationFileAttachement(PdfPage& page, const PdfRect& rect);
        PdfAnnotationFileAttachement(PdfObject& obj);

    public:
        /** Set a file attachment for this annotation.
         *  The type of this annotation has to be
         *  PdfAnnotationType::FileAttachement for file
         *  attachements to work.
         *
         *  \param rFileSpec a file specification
         */
        void SetFileAttachement(const std::shared_ptr<PdfFileSpec>& fileSpec);

        /** Get a file attachement of this annotation.
         *  \returns a file specification object. The file specification object is owned
         *           by the PdfAnnotation.
         *
         *  \see SetFileAttachement
         */
        std::shared_ptr<PdfFileSpec> GetFileAttachement() const;

    private:
        std::shared_ptr<PdfFileSpec> getFileAttachment();
    private:
        std::shared_ptr<PdfFileSpec> m_FileSpec;
    };

    class PODOFO_API PdfAnnotationFreeText : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationFreeText(PdfPage& page, const PdfRect& rect);
        PdfAnnotationFreeText(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationHighlight : public PdfAnnotationTextMarkupBase
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationHighlight(PdfPage& page, const PdfRect& rect);
        PdfAnnotationHighlight(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationInk : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationInk(PdfPage& page, const PdfRect& rect);
        PdfAnnotationInk(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationLine : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationLine(PdfPage& page, const PdfRect& rect);
        PdfAnnotationLine(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationLink : public PdfAnnotationActionBase, public PdfQuadPointsProvider<PdfAnnotationLink>
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationLink(PdfPage& page, const PdfRect& rect);
        PdfAnnotationLink(PdfObject& obj);
    public:
        /** Set the destination for link annotations
         *  \param destination target of the link
         *
         *  \see GetDestination
         */
        void SetDestination(const std::shared_ptr<PdfDestination>& destination);

        /** Get the destination of a link annotations
         *
         *  \returns a destination object
         *  \see SetDestination
         */
        std::shared_ptr<PdfDestination> GetDestination() const;

    private:
        std::shared_ptr<PdfDestination> getDestination();

    private:
        std::shared_ptr<PdfDestination> m_Destination;
    };

    class PODOFO_API PdfAnnotationModel3D : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationModel3D(PdfPage& page, const PdfRect& rect);
        PdfAnnotationModel3D(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationMovie : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationMovie(PdfPage& page, const PdfRect& rect);
        PdfAnnotationMovie(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationPolygon : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationPolygon(PdfPage& page, const PdfRect& rect);
        PdfAnnotationPolygon(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationPolyLine : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationPolyLine(PdfPage& page, const PdfRect& rect);
        PdfAnnotationPolyLine(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationPopup : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationPopup(PdfPage& page, const PdfRect& rect);
        PdfAnnotationPopup(PdfObject& obj);
    public:
        /** Sets whether this annotation is initialy open.
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

    class PODOFO_API PdfAnnotationPrinterMark : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationPrinterMark(PdfPage& page, const PdfRect& rect);
        PdfAnnotationPrinterMark(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationScreen :
        public PdfAnnotationActionBase,
        public PdfAppearanceCharacteristicsProvider<PdfAnnotationScreen>
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationScreen(PdfPage& page, const PdfRect& rect);
        PdfAnnotationScreen(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationSquiggly : public PdfAnnotationTextMarkupBase
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationSquiggly(PdfPage& page, const PdfRect& rect);
        PdfAnnotationSquiggly(PdfObject& obj);

    };

    class PODOFO_API PdfAnnotationStrikeOut : public PdfAnnotationTextMarkupBase
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationStrikeOut(PdfPage& page, const PdfRect& rect);
        PdfAnnotationStrikeOut(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationSound : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationSound(PdfPage& page, const PdfRect& rect);
        PdfAnnotationSound(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationSquare : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationSquare(PdfPage& page, const PdfRect& rect);
        PdfAnnotationSquare(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationCircle : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationCircle(PdfPage& page, const PdfRect& rect);
        PdfAnnotationCircle(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationStamp : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationStamp(PdfPage& page, const PdfRect& rect);
        PdfAnnotationStamp(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationText : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationText(PdfPage& page, const PdfRect& rect);
        PdfAnnotationText(PdfObject& obj);
    public:
        /** Sets whether this annotation is initialy open.
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

    class PODOFO_API PdfAnnotationTrapNet : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationTrapNet(PdfPage& page, const PdfRect& rect);
        PdfAnnotationTrapNet(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationUnderline : public PdfAnnotationTextMarkupBase
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationUnderline(PdfPage& page, const PdfRect& rect);
        PdfAnnotationUnderline(PdfObject& obj);

    };

    class PODOFO_API PdfAnnotationWatermark : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationWatermark(PdfPage& page, const PdfRect& rect);
        PdfAnnotationWatermark(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationWebMedia : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationWebMedia(PdfPage& page, const PdfRect& rect);
        PdfAnnotationWebMedia(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationRedact : public PdfAnnotation, public PdfQuadPointsProvider<PdfAnnotationRedact>
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationRedact(PdfPage& page, const PdfRect& rect);
        PdfAnnotationRedact(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationProjection : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationProjection(PdfPage& page, const PdfRect& rect);
        PdfAnnotationProjection(PdfObject& obj);
    };

    class PODOFO_API PdfAnnotationRichMedia : public PdfAnnotation
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationRichMedia(PdfPage& page, const PdfRect& rect);
        PdfAnnotationRichMedia(PdfObject& obj);
    };
}

#endif // PDF_ANNOTATION_TYPES_H

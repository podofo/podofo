/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_ANNOTATION_H
#define PDF_ANNOTATION_H

#include "PdfElement.h"
#include <podofo/auxiliary/Rect.h>
#include "PdfColor.h"

namespace PoDoFo {

class PdfPage;
class PdfXObject;
class PdfAnnotationText;
class PdfAnnotationLink;
class PdfAnnotationFreeText;
class PdfAnnotationLine;
class PdfAnnotationSquare;
class PdfAnnotationCircle;
class PdfAnnotationPolygon;
class PdfAnnotationPolyLine;
class PdfAnnotationHighlight;
class PdfAnnotationUnderline;
class PdfAnnotationSquiggly;
class PdfAnnotationStrikeOut;
class PdfAnnotationStamp;
class PdfAnnotationCaret;
class PdfAnnotationInk;
class PdfAnnotationPopup;
class PdfAnnotationFileAttachment;
class PdfAnnotationSound;
class PdfAnnotationMovie;
class PdfAnnotationWidget;
class PdfAnnotationScreen;
class PdfAnnotationPrinterMark;
class PdfAnnotationTrapNet;
class PdfAnnotationWatermark;
class PdfAnnotationModel3D;
class PdfAnnotationRichMedia;
class PdfAnnotationWebMedia;
class PdfAnnotationRedact;
class PdfAnnotationProjection;

/** A qualified appearance stream, with type and state name
 */
struct PODOFO_API PdfAppearanceStream final
{
    const PdfObject* Object = nullptr;
    PdfAppearanceType Type = PdfAppearanceType::Normal;
    PdfName State;
};

/** An annotation to a PdfPage
 *  To create an annotation use PdfPage::CreateAnnotation
 *
 *  \see PdfPage::CreateAnnotation
 */
class PODOFO_API PdfAnnotation : public PdfDictionaryElement
{
    friend class PdfAnnotationCollection;
    friend class PdfAnnotationTextMarkupBase;
    friend class PdfAnnotationPopup;
    friend class PdfAnnotationText;
    friend class PdfAnnotationCaret;
    friend class PdfAnnotationFileAttachment;
    friend class PdfAnnotationFreeText;
    friend class PdfAnnotationHighlight;
    friend class PdfAnnotationInk;
    friend class PdfAnnotationLine;
    friend class PdfAnnotationModel3D;
    friend class PdfAnnotationMovie;
    friend class PdfAnnotationPolygon;
    friend class PdfAnnotationPolyLine;
    friend class PdfAnnotationPrinterMark;
    friend class PdfAnnotationRichMedia;
    friend class PdfAnnotationScreen;
    friend class PdfAnnotationSquiggly;
    friend class PdfAnnotationStrikeOut;
    friend class PdfAnnotationSound;
    friend class PdfAnnotationSquare;
    friend class PdfAnnotationCircle;
    friend class PdfAnnotationStamp;
    friend class PdfAnnotationTrapNet;
    friend class PdfAnnotationUnderline;
    friend class PdfAnnotationWatermark;
    friend class PdfAnnotationWebMedia;
    friend class PdfAnnotationRedact;
    friend class PdfAnnotationProjection;
    friend class PdfAnnotationActionBase;
    friend class PdfToggleButton;

private:
    PdfAnnotation(PdfPage& page, PdfAnnotationType annotType, const Rect& rect);
    PdfAnnotation(PdfObject& obj, PdfAnnotationType annotType);
    PdfAnnotation(const PdfAnnotation&) = delete;

public:
    static bool TryCreateFromObject(PdfObject& obj, std::unique_ptr<PdfAnnotation>& xobj);

    static bool TryCreateFromObject(const PdfObject& obj, std::unique_ptr<const PdfAnnotation>& xobj);

    template <typename TAnnotation>
    static bool TryCreateFromObject(PdfObject& obj, std::unique_ptr<TAnnotation>& xobj);

    template <typename TAnnotation>
    static bool TryCreateFromObject(const PdfObject& obj, std::unique_ptr<const TAnnotation>& xobj);

public:
    /** Set an appearance stream for this object
     *  to specify its visual appearance
     *  \param xobj an XObject form
     *  \param appearance an appearance type to set
     *  \param state the state for which set it the obj; states depend on the annotation type
     *  \param skipSelectedState skip setting the selected state, if non null
     */
    void SetAppearanceStream(const PdfXObject& xobj, PdfAppearanceType appearance = PdfAppearanceType::Normal,
        const PdfName& state = { }, bool skipSelectedState = false);

    /** Set an appearance stream for this object
     *  to specify its visual appearance without handling page rotations
     *  \param xobj an XObject form
     *  \param appearance an appearance type to set
     *  \param state the state for which set it the obj; states depend on the annotation type
     *  \param skipSelectedState skip setting the selected state, if non null
     */
    void SetAppearanceStreamRaw(const PdfXObject& xobj, PdfAppearanceType appearance = PdfAppearanceType::Normal,
        const PdfName& state = { }, bool skipSelectedState = false);

    /** Get a list of qualified appearance streams
     */
    void GetAppearanceStreams(std::vector<PdfAppearanceStream>& states) const;

    void ClearAppearances();

    /**
    * \returns the appearance /AP object for this annotation
    */
    PdfObject* GetAppearanceDictionaryObject();
    const PdfObject* GetAppearanceDictionaryObject() const;

    /**
    * \returns the appearance stream for this object
     *  \param appearance an appearance type to get
     *  \param state a child state. Meaning depends on the annotation type
    */
    PdfObject* GetAppearanceStream(PdfAppearanceType appearance = PdfAppearanceType::Normal, const std::string_view& state = { });
    const PdfObject* GetAppearanceStream(PdfAppearanceType appearance = PdfAppearanceType::Normal, const std::string_view& state = { }) const;

    /** Get the rectangle of this annotation.
     *  \returns a rectangle. It's oriented according to the canonical PDF coordinate system
     */
    Rect GetRect() const;

    /** Set the rectangle of this annotation.
     * \param rect rectangle to set. It's oriented according to the canonical PDF coordinate system
     */
    void SetRect(const Rect& rect);

    Corners GetRectRaw() const;

    void SetRectRaw(const Corners& rect);

    /** Set the flags of this annotation.
     *  \see GetFlags
     */
    void SetFlags(PdfAnnotationFlags flags);

    /** Get the flags of this annotation.
     *  \returns the flags which is an unsigned 32bit integer with different
     *           PdfAnnotationFlags OR'ed together.
     *
     *  \see SetFlags
     */
    PdfAnnotationFlags GetFlags() const;

    /** Set the annotations border style.
     *  \param hCorner horitzontal corner radius
     *  \param vCorner vertical corner radius
     *  \param width width of border
     */
    void SetBorderStyle(double hCorner, double vCorner, double width);

    /** Set the annotations border style.
     *  \param hCorner horitzontal corner radius
     *  \param dVCorner vertical corner radius
     *  \param width width of border
     *  \param strokeStyle a custom stroke style pattern
     */
    void SetBorderStyle(double hCorner, double vCorner, double width, const PdfArray& strokeStyle);

    /** Set the title of this annotation.
     *  \param title title of the annotation as string in PDF format
     *
     *  \see GetTitle
     */
    void SetTitle(nullable<const PdfString&> title);

    /** Get the title of this annotation
     *
     *  \returns the title of this annotation
     *
     *  \see SetTitle
     */
    nullable<const PdfString&> GetTitle() const;

    /** Set the text of this annotation.
     *
     *  \param contents text of the annotation as string in PDF format
     *
     *  \see GetContents
     */
    void SetContents(nullable<const PdfString&> contents);

    /** Get the text of this annotation
     *
     *  \returns the contents of this annotation
     *
     *  \see SetContents
     */
    nullable<const PdfString&> GetContents() const;

    /** Get the color key of the Annotation dictionary
     *  which defines the color of the annotation,
     *  as per 8.4 of the pdf spec.
     */

    PdfColor GetColor() const;

    /** Set the C key of the Annotation dictionary, which defines the
     *  color of the annotation, as per 8.4 of the pdf spec.
     */
    void SetColor(nullable<const PdfColor&> color);

public:
    /** Get the type of this annotation
     *  \returns the annotation type
     */
    inline PdfAnnotationType GetType() const { return m_AnnotationType; }

    /** Get the page of this PdfField
     *
     *  \returns the page of this PdfField
     */
    inline PdfPage* GetPage() { return m_Page; }
    inline const PdfPage* GetPage() const { return m_Page; }
    PdfPage& MustGetPage();
    const PdfPage& MustGetPage() const;

private:
    template <typename TAnnot>
    static constexpr PdfAnnotationType GetAnnotationType();

    static std::unique_ptr<PdfAnnotation> Create(PdfPage& page, PdfAnnotationType annotType, const Rect& rect);

    void SetPage(PdfPage& page) { m_Page = &page; }

    void PushAppearanceStream(const PdfXObject& xobj, PdfAppearanceType appearance, const PdfName& state, bool raw);

private:
    static bool tryCreateFromObject(const PdfObject& obj, PdfAnnotationType targetType, PdfAnnotation*& xobj);
    static PdfAnnotationType getAnnotationType(const PdfObject& obj);
    PdfObject* getAppearanceStream(PdfAppearanceType appearance, const std::string_view& state) const;
    PdfDictionary* getAppearanceDictionary() const;

private:
    PdfAnnotationType m_AnnotationType;
    PdfPage* m_Page;
};

template<typename TAnnotation>
bool PdfAnnotation::TryCreateFromObject(PdfObject& obj, std::unique_ptr<TAnnotation>& xobj)
{
    PdfAnnotation* xobj_;
    if (!tryCreateFromObject(obj, GetAnnotationType<TAnnotation>(), xobj_))
        return false;

    xobj.reset((TAnnotation*)xobj_);
    return true;
}

template<typename TAnnotation>
bool PdfAnnotation::TryCreateFromObject(const PdfObject& obj, std::unique_ptr<const TAnnotation>& xobj)
{
    PdfAnnotation* xobj_;
    if (!tryCreateFromObject(obj, GetAnnotationType<TAnnotation>(), xobj_))
        return false;

    xobj.reset((const TAnnotation*)xobj_);
    return true;
}

template<typename TAnnot>
constexpr PdfAnnotationType PdfAnnotation::GetAnnotationType()
{
    if (std::is_same_v<TAnnot, PdfAnnotationText>)
        return PdfAnnotationType::Text;
    else if (std::is_same_v<TAnnot, PdfAnnotationLink>)
        return PdfAnnotationType::Link;
    else if (std::is_same_v<TAnnot, PdfAnnotationFreeText>)
        return PdfAnnotationType::FreeText;
    else if (std::is_same_v<TAnnot, PdfAnnotationLine>)
        return PdfAnnotationType::Line;
    else if (std::is_same_v<TAnnot, PdfAnnotationSquare>)
        return PdfAnnotationType::Square;
    else if (std::is_same_v<TAnnot, PdfAnnotationCircle>)
        return PdfAnnotationType::Circle;
    else if (std::is_same_v<TAnnot, PdfAnnotationPolygon>)
        return PdfAnnotationType::Polygon;
    else if (std::is_same_v<TAnnot, PdfAnnotationPolyLine>)
        return PdfAnnotationType::PolyLine;
    else if (std::is_same_v<TAnnot, PdfAnnotationHighlight>)
        return PdfAnnotationType::Highlight;
    else if (std::is_same_v<TAnnot, PdfAnnotationUnderline>)
        return PdfAnnotationType::Underline;
    else if (std::is_same_v<TAnnot, PdfAnnotationSquiggly>)
        return PdfAnnotationType::Squiggly;
    else if (std::is_same_v<TAnnot, PdfAnnotationStrikeOut>)
        return PdfAnnotationType::StrikeOut;
    else if (std::is_same_v<TAnnot, PdfAnnotationStamp>)
        return PdfAnnotationType::Stamp;
    else if (std::is_same_v<TAnnot, PdfAnnotationCaret>)
        return PdfAnnotationType::Caret;
    else if (std::is_same_v<TAnnot, PdfAnnotationInk>)
        return PdfAnnotationType::Ink;
    else if (std::is_same_v<TAnnot, PdfAnnotationPopup>)
        return PdfAnnotationType::Popup;
    else if (std::is_same_v<TAnnot, PdfAnnotationFileAttachment>)
        return PdfAnnotationType::FileAttachement;
    else if (std::is_same_v<TAnnot, PdfAnnotationSound>)
        return PdfAnnotationType::Sound;
    else if (std::is_same_v<TAnnot, PdfAnnotationMovie>)
        return PdfAnnotationType::Movie;
    else if (std::is_same_v<TAnnot, PdfAnnotationWidget>)
        return PdfAnnotationType::Widget;
    else if (std::is_same_v<TAnnot, PdfAnnotationScreen>)
        return PdfAnnotationType::Screen;
    else if (std::is_same_v<TAnnot, PdfAnnotationPrinterMark>)
        return PdfAnnotationType::PrinterMark;
    else if (std::is_same_v<TAnnot, PdfAnnotationTrapNet>)
        return PdfAnnotationType::TrapNet;
    else if (std::is_same_v<TAnnot, PdfAnnotationWatermark>)
        return PdfAnnotationType::Watermark;
    else if (std::is_same_v<TAnnot, PdfAnnotationModel3D>)
        return PdfAnnotationType::Model3D;
    else if (std::is_same_v<TAnnot, PdfAnnotationRichMedia>)
        return PdfAnnotationType::RichMedia;
    else if (std::is_same_v<TAnnot, PdfAnnotationWebMedia>)
        return PdfAnnotationType::WebMedia;
    else if (std::is_same_v<TAnnot, PdfAnnotationRedact>)
        return PdfAnnotationType::Redact;
    else if (std::is_same_v<TAnnot, PdfAnnotationProjection>)
        return PdfAnnotationType::Projection;
    else
        return PdfAnnotationType::Unknown;
}

};

#endif // PDF_ANNOTATION_H

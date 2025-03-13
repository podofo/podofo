/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_PAGE_H
#define PDF_PAGE_H

#include "PdfDeclarations.h"

#include <podofo/auxiliary/Rect.h>

#include "PdfAnnotationCollection.h"
#include "PdfCanvas.h"
#include "PdfContents.h"
#include "PdfField.h"
#include "PdfResources.h"

namespace PoDoFo {

class PdfDocument;
class InputStream;
class PdfPage;

struct PODOFO_API PdfTextEntry final
{
    std::string Text;
    int Page = -1;
    double X = -1;
    double Y = -1;
    double Length = -1;
    nullable<Rect> BoundingBox;
};

struct PODOFO_API PdfTextExtractParams final
{
    nullable<Rect> ClipRect;
    PdfTextExtractFlags Flags = PdfTextExtractFlags::None;

    std::function<bool(int read_cnt)> AbortCheck = nullptr;
};

template <typename TField>
class PdfPageFieldIterableBase final
{
    friend class PdfPage;

public:
    PdfPageFieldIterableBase()
        : m_page(nullptr) { }

private:
    PdfPageFieldIterableBase(PdfPage& page)
        : m_page(&page) { }

public:
    class Iterator final
    {
        friend class PdfPageFieldIterableBase;
    public:
        using difference_type = void;
        using value_type = TField*;
        using pointer = void;
        using reference = void;
        using iterator_category = std::forward_iterator_tag;
    public:
        Iterator()
            : m_Field(nullptr) { }
    private:
        void stepIntoPageAnnot();

        Iterator(PdfAnnotationCollection::iterator begin,
            PdfAnnotationCollection::iterator end)
            : m_annotsIterator(std::move(begin)), m_annotsEnd(std::move(end)), m_Field(nullptr)
        {
            stepIntoPageAnnot();
        }

    public:
        Iterator(const Iterator&) = default;
        Iterator& operator=(const Iterator&) = default;
        bool operator==(const Iterator& rhs) const
        {
            return m_annotsIterator == rhs.m_annotsIterator;
        }
        bool operator!=(const Iterator& rhs) const
        {
            return m_annotsIterator != rhs.m_annotsIterator;
        }
        Iterator& operator++()
        {
            m_annotsIterator++;
            stepIntoPageAnnot();
            return *this;
        }
        Iterator operator++(int)
        {
            auto copy = *this;
            m_annotsIterator++;
            stepIntoPageAnnot();
            return copy;
        }
        value_type operator*() { return m_Field; }
        value_type operator->() { return m_Field; }
    private:
        PdfAnnotationCollection::iterator m_annotsIterator;
        PdfAnnotationCollection::iterator m_annotsEnd;
        value_type m_Field;
        std::unordered_set<PdfReference> m_visitedObjs;
    };

public:
    Iterator begin() const;
    Iterator end() const;

private:
    PdfPage* m_page;
};

using PdfPageFieldIterable = PdfPageFieldIterableBase<PdfField>;
using PdfPageConstFieldIterable = PdfPageFieldIterableBase<const PdfField>;

/** PdfPage is one page in the pdf document.
 *  It is possible to draw on a page using a PdfPainter object.
 *  Every document needs at least one page.
 */
class PODOFO_API PdfPage final : public PdfDictionaryElement, public PdfCanvas
{
    PODOFO_PRIVATE_FRIEND(class PdfPageTest);
    friend class PdfPageCollection;
    friend class PdfDocument;

private:
    /** Create a new PdfPage object.
     *  \param size a Rect specifying the size of the page (i.e the /MediaBox key) in PDF units
     *  \param parent add the page to this parent
     */
    PdfPage(PdfDocument& parent, const Rect& size);

    /** Create a PdfPage based on an existing PdfObject
     *  \param obj an existing PdfObject
     *  \param listOfParents a list of PdfObjects that are
     *                       parents of this page and can be
     *                       queried for inherited attributes.
     *                       The last object in the list is the
     *                       most direct parent of this page.
     */
    PdfPage(PdfObject& obj);
    PdfPage(PdfObject& obj, std::vector<PdfObject*>&& parents);

public:
    void ExtractTextTo(std::vector<PdfTextEntry>& entries,
        const PdfTextExtractParams& params) const;

    void ExtractTextTo(std::vector<PdfTextEntry>& entries,
        const std::string_view& pattern = { },
        const PdfTextExtractParams& params = { }) const;

    /** Get the rectangle of this page.
     *  \returns a rectangle. It's oriented according to the canonical PDF coordinate system
     */
    Rect GetRect() const { return m_Rect; }

    /** Set the rectangle of this annotation.
     * \param rect rectangle to set. It's oriented according to the canonical PDF coordinate system
     */
    void SetRect(const Rect& rect);

    Corners GetRectRaw() const override;

    void SetRectRaw(const Corners& rect);

    bool TryGetRotationRadians(double& teta) const override;

    /** Get the current page rotation in radians
     * \returns a counterclockwise rotation in radians
     */
    double GetRotationRadians() const;

    /** Set the /MediaBox in PDF Units
     * \param rect a Rect in PDF units
     */
    void SetMediaBox(const Rect& rect);

    /** Set the /CropBox in PDF Units
     * \param rect a Rect in PDF units
     */
    void SetCropBox(const Rect& rect);

    /** Set the /TrimBox in PDF Units
     * \param rect a Rect in PDF units
     */
    void SetTrimBox(const Rect& rect);

    /** Set the /BleedBox in PDF Units
     * \param rect a Rect in PDF units
     */
    void SetBleedBox(const Rect& rect);

    /** Set the /ArtBox in PDF Units
     * \param rect a Rect in PDF units
     */
    void SetArtBox(const Rect& rect);

    /** Page number inside of the document. The  first page
     *  has the number 1
     *
     *  \returns the number of the page inside of the document
     */
    unsigned GetPageNumber() const;

    /** Creates a Rect with the page size as values which is needed to create a PdfPage object
     *  from an enum which are defined for a few standard page sizes.
     *
     *  \param pageSize the page size you want
     *  \param landscape create a landscape pagesize instead of portrait (by exchanging width and height)
     *  \returns a Rect object which can be passed to the PdfPage constructor
     */
    static Rect CreateStandardPageSize(const PdfPageSize pageSize, bool landscape = false);

    /** Get the current MediaBox (physical page size) in PDF units.
     *  \returns Rect the page box
     */
    Rect GetMediaBox() const;
    Corners GetMediaBoxRaw() const;

    /** Get the current CropBox (visible page size) in PDF units.
     *  \returns Rect the page box
     */
    Rect GetCropBox() const;
    Corners GetCropBoxRaw() const;

    /** Get the current TrimBox (cut area) in PDF units.
     *  \returns Rect the page box
     */
    Rect GetTrimBox() const;
    Corners GetTrimBoxRaw() const;

    /** Get the current BleedBox (extra area for printing purposes) in PDF units.
     *  \returns Rect the page box
     */
    Rect GetBleedBox() const;
    Corners GetBleedBoxRaw() const;

    /** Get the current ArtBox in PDF units.
     *  \returns Rect the page box
     */
    Rect GetArtBox() const;
    Corners GetArtBoxRaw() const;

    /** Get the normalized page rotation (0, 90, 180 or 270)
     * \returns a clockwise rotation in degrees
     */
    unsigned GetRotation() const { return m_Rotation; }

    /** Get the raw page rotation (if any)
     * \param rotation a clockwise rotation in degrees
     * \remarks it may return an invalid page rotation
     */
    bool TryGetRotationRaw(double& rotation) const;

    /** Set the current page rotation.
     * \param rotation The rotation to set to the page. Must be a multiple of 90
     * \remarks The actual stored rotation will be normalzed to 0, 90, 180 or 270
     */
    void SetRotation(int rotation);

    /** Move the page to the given index
     */
    bool MoveTo(unsigned index);

    template <typename TField>
    TField& CreateField(const std::string_view& name, const Rect& rect);

    PdfField& CreateField(const std::string_view& name, PdfFieldType fieldType, const Rect& rect);

    /**
     * Get an iterator for all fields in the page. All widget annotation fields
     * in the pages will be returned
     */
    PdfPageFieldIterable GetFieldsIterator();
    PdfPageConstFieldIterable GetFieldsIterator() const;

public:
    unsigned GetIndex() const { return m_Index; }
    PdfContents& GetOrCreateContents();
    inline const PdfContents* GetContents() const { return m_Contents.get(); }
    inline PdfContents* GetContents() { return m_Contents.get(); }
    const PdfContents& MustGetContents() const;
    PdfContents& MustGetContents();
    const PdfResources& GetResources() const;
    PdfResources& GetResources();
    inline PdfAnnotationCollection& GetAnnotations() { return m_Annotations; }
    inline const PdfAnnotationCollection& GetAnnotations() const { return m_Annotations; }

private:
    // To be called by PdfPageCollection
    void FlattenStructure();
    void SetIndex(unsigned index) { m_Index = index; }

    void CopyContentsTo(OutputStream& stream) const override;

    PdfObjectStream& GetOrCreateContentsStream(PdfStreamAppendFlags flags) override;

    PdfObjectStream& ResetContentsStream() override;

    PdfResources& GetOrCreateResources() override;

    PdfResources* getResources() override;

    PdfObject* getContentsObject() override;

    PdfDictionaryElement& getElement() override;

    PdfObject* findInheritableAttribute(const std::string_view& name) const;

    PdfObject* findInheritableAttribute(const std::string_view& name, bool& isShallow) const;

    void ensureContentsCreated();

    /** Get the bounds of a specified page box in PDF units.
     * This function is internal, since there are wrappers for all standard boxes
     *  \returns Rect the page box
     */
    Rect getPageBox(const std::string_view& inBox, bool isInheritable) const;

    Corners getPageBoxRaw(const std::string_view& inBox, bool isInheritable) const;

    void setPageBox(const PdfName& inBox, const Rect& rect);

    void adjustRectToCurrentRotation(Rect& rect) const;

private:
    // Remove some PdfCanvas methods to maintain the class API surface clean
    PdfElement& GetElement() = delete;
    const PdfElement& GetElement() const = delete;
    PdfObject* GetContentsObject() = delete;
    const PdfObject* GetContentsObject() const = delete;

private:
    unsigned m_Index;
    unsigned m_Rotation;
    Rect m_Rect;
    std::vector<PdfObject*> m_parents;
    std::unique_ptr<PdfContents> m_Contents;
    std::unique_ptr<PdfResources> m_Resources;
    PdfAnnotationCollection m_Annotations;
};

template<typename TField>
TField& PdfPage::CreateField(const std::string_view& name, const Rect & rect)
{
    return static_cast<TField&>(CreateField(name, PdfField::GetFieldType<TField>(), rect));
}

template<typename TField>
typename PdfPageFieldIterableBase<TField>::Iterator PdfPageFieldIterableBase<TField>::begin() const
{
    if (m_page == nullptr)
        return Iterator();
    else
        return Iterator(m_page->GetAnnotations().begin(), m_page->GetAnnotations().end());
}

template<typename TField>
typename PdfPageFieldIterableBase<TField>::Iterator PdfPageFieldIterableBase<TField>::end() const
{
    if (m_page == nullptr)
        return Iterator();
    else
        return Iterator(m_page->GetAnnotations().end(), m_page->GetAnnotations().end());
}

template<typename TField>
void PdfPageFieldIterableBase<TField>::Iterator::stepIntoPageAnnot()
{
    while (true)
    {
        if (m_annotsIterator == m_annotsEnd)
            break;

        auto& annot = **m_annotsIterator;
        PdfField* field = nullptr;
        if (annot.GetType() == PdfAnnotationType::Widget &&
            (field = &static_cast<PdfAnnotationWidget&>(annot).GetField(),
                m_visitedObjs.find(field->GetObject().GetIndirectReference()) == m_visitedObjs.end()))
        {
            m_Field = field;
            m_visitedObjs.insert(field->GetObject().GetIndirectReference());
            return;
        }

        m_annotsIterator++;
    }

    m_Field = nullptr;
    m_visitedObjs.clear();
}

};

#endif // PDF_PAGE_H

/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_PAGE_H
#define PDF_PAGE_H

#include "PdfDeclarations.h"

#include "PdfAnnotationCollection.h"
#include "PdfCanvas.h"
#include "PdfRect.h"
#include "PdfContents.h"
#include "PdfField.h"
#include "PdfResources.h"

namespace PoDoFo {

class PdfDocument;
class PdfDictionary;
class PdfIndirectObjectList;
class InputStream;

struct PdfTextEntry final
{
    std::string Text;
    int Page;
    double X;
    double Y;
    double Length;
    nullable<PdfRect> BoundingBox;
};

struct PdfTextExtractParams
{
    nullable<PdfRect> ClipRect;
    PdfTextExtractFlags Flags;
};

/** PdfPage is one page in the pdf document.
 *  It is possible to draw on a page using a PdfPainter object.
 *  Every document needs at least one page.
 */
class PODOFO_API PdfPage final : public PdfDictionaryElement, public PdfCanvas
{
    PODOFO_UNIT_TEST(PdfPageTest);
    friend class PdfPageCollection;

private:
    /** Create a new PdfPage object.
     *  \param size a PdfRect specifying the size of the page (i.e the /MediaBox key) in PDF units
     *  \param parent add the page to this parent
     */
    PdfPage(PdfDocument& parent, unsigned index, const PdfRect& size);

    /** Create a PdfPage based on an existing PdfObject
     *  \param obj an existing PdfObject
     *  \param listOfParents a list of PdfObjects that are
     *                       parents of this page and can be
     *                       queried for inherited attributes.
     *                       The last object in the list is the
     *                       most direct parent of this page.
     */
    PdfPage(PdfObject& obj, unsigned index, const std::deque<PdfObject*>& listOfParents);

public:
    void ExtractTextTo(std::vector<PdfTextEntry>& entries,
        const PdfTextExtractParams& params) const;

    void ExtractTextTo(std::vector<PdfTextEntry>& entries,
        const std::string_view& pattern = { },
        const PdfTextExtractParams& params = { }) const;

    PdfRect GetRect() const override;

    bool HasRotation(double& teta) const override;

    // added by Petr P. Petrov 21 Febrary 2010
    /** Set the current page width in PDF Units
     *
     * \returns true if successful, false otherwise
     *
     */
    bool SetPageWidth(int newWidth);

    // added by Petr P. Petrov 21 Febrary 2010
    /** Set the current page height in PDF Units
     *
     * \returns true if successful, false otherwise
     *
     */
    bool SetPageHeight(int newHeight);

    /** Set the mediabox in PDF Units
    *  \param size a PdfRect specifying the mediabox of the page (i.e the /TrimBox key) in PDF units
    */
    void SetMediaBox(const PdfRect& size);

    /** Set the trimbox in PDF Units
     *  \param size a PdfRect specifying the trimbox of the page (i.e the /TrimBox key) in PDF units
     */
    void SetTrimBox(const PdfRect& size);

    /** Page number inside of the document. The  first page
     *  has the number 1, the last page has the number
     *  PdfPageTree:GetTotalNumberOfPages()
     *
     *  \returns the number of the page inside of the document
     *
     *  \see PdfPageTree:GetTotalNumberOfPages()
     */
    unsigned GetPageNumber() const;

    /** Creates a PdfRect with the page size as values which is needed to create a PdfPage object
     *  from an enum which are defined for a few standard page sizes.
     *
     *  \param pageSize the page size you want
     *  \param landscape create a landscape pagesize instead of portrait (by exchanging width and height)
     *  \returns a PdfRect object which can be passed to the PdfPage constructor
     */
    static PdfRect CreateStandardPageSize(const PdfPageSize pageSize, bool landscape = false);

    /** Get the current MediaBox (physical page size) in PDF units.
     *  \returns PdfRect the page box
     */
    PdfRect GetMediaBox() const;

    /** Get the current CropBox (visible page size) in PDF units.
     *  \returns PdfRect the page box
     */
    PdfRect GetCropBox() const;

    /** Get the current TrimBox (cut area) in PDF units.
     *  \returns PdfRect the page box
     */
    PdfRect GetTrimBox() const;

    /** Get the current BleedBox (extra area for printing purposes) in PDF units.
     *  \returns PdfRect the page box
     */
    PdfRect GetBleedBox() const;

    /** Get the current ArtBox in PDF units.
     *  \returns PdfRect the page box
     */
    PdfRect GetArtBox() const;

    /** Get the current page rotation (if any), it's a clockwise rotation
     *  \returns int 0, 90, 180 or 270
     */
    int GetRotationRaw() const;

    /** Set the current page rotation.
     *  \param iRotation Rotation to set to the page. Valid value are 0, 90, 180, 270.
     */
    void SetRotationRaw(int rotation);

    /** Move the page at the given index
     */
    void MoveAt(unsigned index);

    template <typename TField>
    TField& CreateField(const std::string_view& name, const PdfRect& rect);

    PdfField& CreateField(const std::string_view& name, PdfFieldType fieldType, const PdfRect& rect);

    /** Set an ICC profile for this page
     *
     *  \param csTag a ColorSpace tag
     *  \param stream an input stream from which the ICC profiles data can be read
     *  \param colorComponents the number of colorcomponents of the ICC profile (expected is 1, 3 or 4 components)
     *  \param alternateColorSpace an alternate colorspace to use if the ICC profile cannot be used
     *
     *  \see PdfPainter::SetDependICCProfileColor()
     */
    void SetICCProfile(const std::string_view& csTag, InputStream& stream, int64_t colorComponents,
        PdfColorSpace alternateColorSpace = PdfColorSpace::DeviceRGB);

public:
    unsigned GetIndex() const { return m_Index; }
    PdfContents& GetOrCreateContents();
    PdfResources& GetOrCreateResources() override;
    inline const PdfContents* GetContents() const { return m_Contents.get(); }
    inline PdfContents* GetContents() { return m_Contents.get(); }
    const PdfContents& MustGetContents() const;
    PdfContents& MustGetContents();
    inline const PdfResources* GetResources() const { return m_Resources.get(); }
    inline PdfResources* GetResources() { return m_Resources.get(); }
    const PdfResources& MustGetResources() const;
    PdfResources& MustGetResources();
    inline PdfAnnotationCollection& GetAnnotations() { return m_Annotations; }
    inline const PdfAnnotationCollection& GetAnnotations() const { return m_Annotations; }

private:
    PdfField& createField(const std::string_view& name, const std::type_info& typeInfo, const PdfRect& rect);

    PdfResources* getResources() const override;

    PdfObject* getContentsObject() const override;

    PdfElement& getElement() const override;

    PdfObjectStream& GetStreamForAppending(PdfStreamAppendFlags flags) override;

    /**
     * Initialize a new page object.
     * m_Contents must be initialized before calling this!
     *
     * \param size page size
     */
    void initNewPage(const PdfRect& size);

    void ensureContentsCreated();
    void ensureResourcesCreated();

    /** Get the bounds of a specified page box in PDF units.
     * This function is internal, since there are wrappers for all standard boxes
     *  \returns PdfRect the page box
     */
    PdfRect getPageBox(const std::string_view& inBox) const;

private:
    PdfElement& GetElement() = delete;
    const PdfElement& GetElement() const = delete;
    PdfObject* GetContentsObject() = delete;
    const PdfObject* GetContentsObject() const = delete;

private:
    unsigned m_Index;
    std::unique_ptr<PdfContents> m_Contents;
    std::unique_ptr<PdfResources> m_Resources;
    PdfAnnotationCollection m_Annotations;
};

template<typename TField>
TField& PdfPage::CreateField(const std::string_view& name, const PdfRect & rect)
{
    return static_cast<TField&>(createField(name, typeid(TField), rect));
}

};

#endif // PDF_PAGE_H

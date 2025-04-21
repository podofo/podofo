/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_DOCUMENT_H
#define PDF_DOCUMENT_H

#include "PdfTrailer.h"
#include "PdfCatalog.h"
#include "PdfIndirectObjectList.h"
#include "PdfAcroForm.h"
#include "PdfFontManager.h"
#include "PdfMetadata.h"
#include "PdfPageCollection.h"
#include "PdfNameTrees.h"
#include "PdfXObjectForm.h"
#include "PdfImage.h"
#include "PdfColorSpace.h"
#include "PdfPattern.h"
#include "PdfFunction.h"
#include "PdfInfo.h"
#include "PdfOutlines.h"
#include "PdfExtension.h"

namespace PoDoFo {

class PdfAction;
class PdfExtGState;
class PdfEncrypt;
class PdfDocument;

template <typename TField>
class PdfDocumentFieldIterableBase final
{
    friend class PdfDocument;

public:
    PdfDocumentFieldIterableBase()
        : m_doc(nullptr) { }

private:
    PdfDocumentFieldIterableBase(PdfDocument& doc)
        : m_doc(&doc) { }

public:
    class Iterator final
    {
        friend class PdfDocumentFieldIterableBase;
    public:
        using difference_type = void;
        using value_type = TField*;
        using pointer = void;
        using reference = void;
        using iterator_category = std::forward_iterator_tag;
    public:
        Iterator();
    private:
        Iterator(PdfDocument& doc);
    public:
        Iterator(const Iterator&) = default;
        Iterator& operator=(const Iterator&) = default;
        bool operator==(const Iterator& rhs) const;
        bool operator!=(const Iterator& rhs) const;
        Iterator& operator++();
        Iterator operator++(int);
        value_type operator*() { return m_Field; }
        value_type operator->() { return m_Field; }
    private:
        void increment();
        void stepIntoPageOrForm(PdfPageCollection& pages);
        bool stepIntoPageAnnot(PdfAnnotationCollection& annots);
        void stepIntoFormField(PdfAcroForm& form);
    private:
        PdfDocument* m_doc;
        unsigned m_pageIndex;
        PdfAnnotationCollection::iterator m_pageAnnotIterator;
        PdfAcroForm::iterator m_acroFormIterator;
        value_type m_Field;
        std::unordered_set<PdfReference> m_visitedObjs;
    };

public:
    Iterator begin() const;
    Iterator end() const;

private:
    PdfDocument* m_doc;
};

using PdfDocumentFieldIterable = PdfDocumentFieldIterableBase<PdfField>;
using PdfDocumentConstFieldIterable = PdfDocumentFieldIterableBase<const PdfField>;

/** PdfDocument is the core interface for working with PDF documents.
 *
 *  PdfDocument provides easy access to the individual pages
 *  in the PDF file and to certain special dictionaries.
 *
 *  PdfDocument cannot be used directly.
 *  Use PdfMemDocument whenever you want to change the object structure
 *  of a PDF file. 
 *
 *  When you are only creating PDF files, please use PdfStreamedDocument
 *  which is usually faster for creating PDFs.
 *
 *  \see PdfStreamedDocument
 *  \see PdfMemDocument
 */
class PODOFO_API PdfDocument
{
    friend class PdfMetadata;
    friend class PdfXObjectForm;
    friend class PdfPageCollection;
    friend class PdfMemDocument;
    friend class PdfStreamedDocument;

public:
    /** Close down/destruct the PdfDocument
     */
    virtual ~PdfDocument();

    /** Get access to the Outlines (Bookmarks) dictionary
     *  The returned outlines object is owned by the PdfDocument.
     *
     *  \param create create the object if it does not exist (ePdfCreateObject)
     *                 or return nullptr if it does not exist
     *  \returns the Outlines/Bookmarks dictionary
     */
    PdfOutlines& GetOrCreateOutlines();

    /** Get access to the Names dictionary (where all the named objects are stored)
     *  The returned PdfNameTrees object is owned by the PdfDocument.
     *
     *  \param create create the object if it does not exist (ePdfCreateObject)
     *                 or return nullptr if it does not exist
     *  \returns the Names dictionary
     */
    PdfNameTrees& GetOrCreateNames();

    /** Get access to the AcroForm dictionary
     *
     *  \param create create the object if it does not exist (ePdfCreateObject)
     *                 or return nullptr if it does not exist
     *  \param eDefaultAppearance specifies if a default appearance shall be created
     *
     *  \returns PdfObject the AcroForm dictionary
     */
    PdfAcroForm& GetOrCreateAcroForm(PdfAcroFormDefaulAppearance eDefaultAppearance = PdfAcroFormDefaulAppearance::ArialBlack);

    void CollectGarbage();

    /** Construct a new PdfImage object
     */
    std::unique_ptr<PdfImage> CreateImage();

    std::unique_ptr<PdfXObjectForm> CreateXObjectForm(const Rect& rect);

    std::unique_ptr<PdfDestination> CreateDestination();

    std::unique_ptr<PdfColorSpace> CreateColorSpace(PdfColorSpaceFilterPtr filter);

    std::unique_ptr<PdfFunction> CreateFunction(PdfFunctionDefinitionPtr definition);

    std::unique_ptr<PdfUncolouredTilingPattern> CreateTilingPattern(std::shared_ptr<PdfUncolouredTilingPatternDefinition> definition);

    std::unique_ptr<PdfColouredTilingPattern> CreateTilingPattern(std::shared_ptr<PdfColouredTilingPatternDefinition> definition);

    std::unique_ptr<PdfShadingPattern> CreateShadingPattern(PdfShadingPatternDefinitionPtr definition);

    std::unique_ptr<PdfShadingDictionary> CreateShadingDictionary(PdfShadingDefinitionPtr definition);

    std::unique_ptr<PdfExtGState> CreateExtGState(PdfExtGStateDefinitionPtr definition);

    template <typename Taction>
    std::unique_ptr<Taction> CreateAction();

    std::unique_ptr<PdfAction> CreateAction(PdfActionType type);

    std::unique_ptr<PdfFileSpec> CreateFileSpec();

    /** Checks if printing this document is allowed.
     *  Every PDF-consuming application has to adhere to this value!
     *
     *  \returns true if you are allowed to print this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsPrintAllowed() const;

    /** Checks if modifying this document (besides annotations, form fields or substituting pages) is allowed.
     *  Every PDF-consuming application has to adhere to this value!
     *
     *  \returns true if you are allowed to modify this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsEditAllowed() const;

    /** Checks if text and graphics extraction is allowed.
     *  Every PDF-consuming application has to adhere to this value!
     *
     *  \returns true if you are allowed to extract text and graphics from this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsCopyAllowed() const;

    /** Checks if it is allowed to add or modify annotations or form fields.
     *  Every PDF-consuming application has to adhere to this value!
     *
     *  \returns true if you are allowed to add or modify annotations or form fields
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsEditNotesAllowed() const;

    /** Checks if it is allowed to fill in existing form or signature fields.
     *  Every PDF-consuming application has to adhere to this value!
     *
     *  \returns true if you are allowed to fill in existing form or signature fields
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsFillAndSignAllowed() const;

    /** Checks if it is allowed to extract text and graphics to support users with disabilities.
     *  Every PDF-consuming application has to adhere to this value!
     *
     *  \returns true if you are allowed to extract text and graphics to support users with disabilities
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsAccessibilityAllowed() const;

    /** Checks if it is allowed to insert, create, rotate, or delete pages or add bookmarks.
     *  Every PDF-consuming application has to adhere to this value!
     *
     *  \returns true if you are allowed  to insert, create, rotate, or delete pages or add bookmarks
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsDocAssemblyAllowed() const;

    /** Checks if it is allowed to print a high quality version of this document
     *  Every PDF-consuming application has to adhere to this value!
     *
     *  \returns true if you are allowed to print a high quality version of this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    bool IsHighPrintAllowed() const;

    /** Add a vendor-specific extension to the current PDF version.
     *  \param extension extension to add
     */
    void PushPdfExtension(const PdfExtension& extension);

    /** Checks whether the documents is tagged to imlpement a vendor-specific
     *  extension to the current PDF version.
     *  \param ns  namespace of the extension
     *  \param level  level of the extension
     */
    bool HasPdfExtension(const std::string_view& ns, int64_t level) const;

    /** Remove a vendor-specific extension to the current PDF version.
     *  \param ns  namespace of the extension
     *  \param level  level of the extension
     */
    void RemovePdfExtension(const std::string_view& ns, int64_t level);

    /** Return the list of all vendor-specific extensions to the current PDF version.
     *  \param ns  namespace of the extension
     *  \param level  level of the extension
     */
    std::vector<PdfExtension> GetPdfExtensions() const;

    PdfAcroForm& MustGetAcroForm();

    const PdfAcroForm& MustGetAcroForm() const;

    PdfNameTrees& MustGetNames();

    const PdfNameTrees& MustGetNames() const;

    PdfOutlines& MustGetOutlines();

    const PdfOutlines& MustGetOutlines() const;

    /**
     * Get an iterator for all fields in the document. All widget annotation fields
     * in the pages will be returned, plus non annotation fields in the /AcroForm
     * (eg. invisibile signatures)
     */
    PdfDocumentFieldIterable GetFieldsIterator();
    PdfDocumentConstFieldIterable GetFieldsIterator() const;

    /** Clear all internal structures and reset PdfDocument to an empty state.
      */
    void Reset();

public:
    virtual const PdfEncrypt* GetEncrypt() const = 0;

    /**
     * \returns true if this PdfMemDocument creates an encrypted PDF file
     */
    bool IsEncrypted() const;

public:
    /** Get access to the internal Catalog dictionary
     *  or root object.
     *
     *  \returns PdfObject the documents catalog
     */
    PdfCatalog& GetCatalog() { return *m_Catalog; }

    /** Get access to the internal Catalog dictionary
     *  or root object.
     *
     *  \returns PdfObject the documents catalog
     */
    const PdfCatalog& GetCatalog() const { return *m_Catalog; }

    /** Get access to the page tree.
     *  \returns the PdfPageTree of this document.
     */
    PdfPageCollection& GetPages() { return *m_Pages; }

    /** Get access to the page tree.
     *  \returns the PdfPageTree of this document.
     */
    const PdfPageCollection& GetPages() const { return *m_Pages; }

    /** Get access to the internal trailer dictionary
     *  or root object.
     *
     *  \returns PdfObject the documents catalog
     */
    PdfTrailer &GetTrailer() { return *m_Trailer; }

    /** Get access to the internal trailer dictionary
     *  or root object.
     *
     *  \returns PdfObject the documents catalog
     */
    const PdfTrailer& GetTrailer() const { return *m_Trailer; }

    /** Get access to the internal Info dictionary
     *  You can set the author, title etc. of the
     *  document using the info dictionary.
     *
     *  \returns the info dictionary
     */
    const PdfInfo* GetInfo() const { return m_Info.get(); }

    PdfMetadata& GetMetadata() { return m_Metadata; }

    const PdfMetadata& GetMetadata() const { return m_Metadata; }

    /** Get access to the internal vector of objects
     *  or root object.
     *
     *  \returns the vector of objects
     */
    PdfIndirectObjectList& GetObjects() { return m_Objects; }

    /** Get access to the internal vector of objects
     *  or root object.
     *
     *  \returns the vector of objects
     */
    const PdfIndirectObjectList& GetObjects() const { return m_Objects; }

    PdfAcroForm* GetAcroForm() { return m_AcroForm.get(); }

    const PdfAcroForm* GetAcroForm() const { return m_AcroForm.get(); }

    PdfNameTrees* GetNames() { return m_NameTrees.get(); }

    const PdfNameTrees* GetNames() const { return m_NameTrees.get(); }

    PdfOutlines* GetOutlines();

    const PdfOutlines* GetOutlines() const;

    PdfFontManager& GetFonts() { return m_FontManager; }

protected:
    /** Set the trailer of this PdfDocument
     *  deleting the old one.
     *
     *  \param obj the new trailer object
     *         It will be owned by PdfDocument.
     */
    void SetTrailer(std::unique_ptr<PdfObject> obj);

    /** Internal method for initializing the pages tree for this document
     */
    void Init();

    virtual void reset();

    /** Clear all variables that have internal memory usage
      */
    void Clear();

    virtual void clear();

    /** Get the PDF version of the document
     *  \returns PdfVersion version of the pdf document
     */
    virtual PdfVersion GetPdfVersion() const = 0;

    /** Get the PDF version of the document
     *  \returns PdfVersion version of the pdf document
     */
    virtual void SetPdfVersion(PdfVersion version) = 0;

private:
    /** Construct a new (empty) PdfDocument
     *  \param empty if true NO default objects (such as catalog) are created.
     */
    PdfDocument(bool empty = false);

    PdfDocument(const PdfDocument& doc);

    // Called by PdfPageCollection
    void AppendDocumentPages(const PdfDocument& doc);
    void InsertDocumentPageAt(unsigned atIndex, const PdfDocument& doc, unsigned pageIndex);
    void AppendDocumentPages(const PdfDocument& doc, unsigned pageIndex, unsigned pageCount);

    // Called by PdfXObjectForm
    Rect FillXObjectFromPage(PdfXObjectForm& xobj, const PdfPage& page, bool useTrimBox);

    PdfInfo& GetOrCreateInfo();

    void createAction(PdfActionType type, std::unique_ptr<PdfAction>& action);

private:
    void append(const PdfDocument& doc, bool appendAll);
    /** Recursively changes every PdfReference in the PdfObject and in any child
     *  that is either an PdfArray or a direct object.
     *  The reference is changed so that difference is added to the object number
     *  of the reference.
     *  \param obj object to change
     *  \param difference add this value to every reference that is encountered
     */
    void fixObjectReferences(PdfObject& obj, int difference);

    void deletePages(unsigned atIndex, unsigned pageCount);

    void resetPrivate();

    void initOutlines();

private:
    PdfDocument& operator=(const PdfDocument&) = delete;

private:
    PdfIndirectObjectList m_Objects;
    PdfMetadata m_Metadata;
    PdfFontManager m_FontManager;
    std::unique_ptr<PdfObject> m_TrailerObj;
    std::unique_ptr<PdfTrailer> m_Trailer;
    std::unique_ptr<PdfCatalog> m_Catalog;
    std::unique_ptr<PdfInfo> m_Info;
    std::unique_ptr<PdfPageCollection> m_Pages;
    std::unique_ptr<PdfAcroForm> m_AcroForm;
    nullable<std::unique_ptr<PdfOutlines>> m_Outlines;
    std::unique_ptr<PdfNameTrees> m_NameTrees;
};

template<typename TAction>
std::unique_ptr<TAction> PdfDocument::CreateAction()
{
    std::unique_ptr<TAction> ret;
    createAction(PdfAction::GetActionType<TAction>(), reinterpret_cast<std::unique_ptr<PdfAction>&>(ret));
    return ret;
}

template<typename TField>
typename PdfDocumentFieldIterableBase<TField>::Iterator PdfDocumentFieldIterableBase<TField>::begin() const
{
    if (m_doc == nullptr)
        return Iterator();
    else
        return Iterator(*m_doc);
}

template<typename TField>
typename PdfDocumentFieldIterableBase<TField>::Iterator PdfDocumentFieldIterableBase<TField>::end() const
{
    return Iterator();
}

template<typename TField>
PdfDocumentFieldIterableBase<TField>::Iterator::Iterator()
    : m_doc(nullptr), m_pageIndex(0), m_Field(nullptr)
{
}

template<typename TField>
PdfDocumentFieldIterableBase<TField>::Iterator::Iterator(PdfDocument& doc)
    : m_doc(&doc), m_pageIndex(0), m_Field(nullptr)
{
    stepIntoPageOrForm(doc.GetPages());
}

template<typename TField>
bool PdfDocumentFieldIterableBase<TField>::Iterator::operator==(const Iterator& rhs) const
{
    if (m_doc == nullptr && rhs.m_doc == nullptr)
        return true;

    return m_doc == rhs.m_doc && m_pageIndex == rhs.m_pageIndex && m_pageAnnotIterator == rhs.m_pageAnnotIterator && m_acroFormIterator == rhs.m_acroFormIterator;
}

template<typename TField>
bool PdfDocumentFieldIterableBase<TField>::Iterator::operator!=(const Iterator& rhs) const
{
    if (m_doc == nullptr && rhs.m_doc == nullptr)
        return false;

    return m_doc != rhs.m_doc || m_pageIndex != rhs.m_pageIndex || m_pageAnnotIterator != rhs.m_pageAnnotIterator || m_acroFormIterator != rhs.m_acroFormIterator;
}

template<typename TField>
typename PdfDocumentFieldIterableBase<TField>::Iterator& PdfDocumentFieldIterableBase<TField>::Iterator::operator++()
{
    increment();
    return *this;
}

template<typename TField>
typename PdfDocumentFieldIterableBase<TField>::Iterator PdfDocumentFieldIterableBase<TField>::Iterator::operator++(int)
{
    auto copy = *this;
    increment();
    return copy;
}

template<typename TField>
void PdfDocumentFieldIterableBase<TField>::Iterator::increment()
{
    if (m_doc == nullptr)
        return;

    auto& pages = m_doc->GetPages();
    if (m_pageIndex < pages.GetCount())
    {
        m_pageAnnotIterator++;
        if (stepIntoPageAnnot(pages.GetPageAt(m_pageIndex).GetAnnotations()))
            return;

        m_pageIndex++;
        stepIntoPageOrForm(pages);
    }
    else
    {
        m_acroFormIterator++;
        stepIntoFormField(m_doc->MustGetAcroForm());
    }
}

// Update the iterator for the current page index, or swith to form iteration
template<typename TField>
void PdfDocumentFieldIterableBase<TField>::Iterator::stepIntoPageOrForm(PdfPageCollection& pages)
{
    while (true)
    {
        if (m_pageIndex >= pages.GetCount())
            break;

        auto& annots = pages.GetPageAt(m_pageIndex).GetAnnotations();
        m_pageAnnotIterator = annots.begin();
        if (stepIntoPageAnnot(annots))
            return;

        m_pageIndex++;
    }

    auto form = m_doc->GetAcroForm();
    if (form != nullptr)
    {
        m_acroFormIterator = form->begin();
        stepIntoFormField(*form);
        return;
    }

    // End of iteration
    m_doc = nullptr;
    m_Field = nullptr;
    m_visitedObjs.clear();
}

// Verify the current page annotation iterator. It updates the current field
// and returns true if a valid unvisited field is found, false otherwise
template<typename TField>
bool PdfDocumentFieldIterableBase<TField>::Iterator::stepIntoPageAnnot(PdfAnnotationCollection& annots)
{
    while (true)
    {
        if (m_pageAnnotIterator == annots.end())
            break;

        auto& annot = **m_pageAnnotIterator;
        PdfField* field = nullptr;
        if (annot.GetType() == PdfAnnotationType::Widget &&
            (field = &static_cast<PdfAnnotationWidget&>(annot).GetField(),
                m_visitedObjs.find(field->GetObject().GetIndirectReference()) == m_visitedObjs.end()))
        {
            m_Field = field;
            m_visitedObjs.insert(field->GetObject().GetIndirectReference());
            return true;
        }

        m_pageAnnotIterator++;
    }

    return false;
}

// Verify the current AcroForm field iterator. It updates the current field
// if a valid unvisited leaf field is found, or it ends the iteration otherwise
template<typename TField>
void PdfDocumentFieldIterableBase<TField>::Iterator::stepIntoFormField(PdfAcroForm& form)
{
    while (true)
    {
        if (m_acroFormIterator == form.end())
            break;

        auto& field = **m_acroFormIterator;
        if (field.GetChildren().GetCount() == 0
            && m_visitedObjs.find(field.GetObject().GetIndirectReference()) == m_visitedObjs.end())
        {
            m_Field = &field;
            m_visitedObjs.insert(field.GetObject().GetIndirectReference());
            return;
        }

        m_acroFormIterator++;
    }

    // End of iteration
    m_doc = nullptr;
    m_Field = nullptr;
    m_visitedObjs.clear();
}

};


#endif	// PDF_DOCUMENT_H

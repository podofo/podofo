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
#include "PdfNameTree.h"
#include "PdfXObjectForm.h"
#include "PdfImage.h"

namespace PoDoFo {

class PdfDestination;
class PdfFileSpec;
class PdfInfo;
class PdfOutlines;
class PdfEncrypt;

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
     *  The returned PdfNameTree object is owned by the PdfDocument.
     *
     *  \param create create the object if it does not exist (ePdfCreateObject)
     *                 or return nullptr if it does not exist
     *  \returns the Names dictionary
     */
    PdfNameTree& GetOrCreateNameTree();

    /** Get access to the AcroForm dictionary
     *
     *  \param create create the object if it does not exist (ePdfCreateObject)
     *                 or return nullptr if it does not exist
     *  \param eDefaultAppearance specifies if a default appearence shall be created
     *
     *  \returns PdfObject the AcroForm dictionary
     */
    PdfAcroForm& GetOrCreateAcroForm(PdfAcroFormDefaulAppearance eDefaultAppearance = PdfAcroFormDefaulAppearance::BlackText12pt);

    /** Attach a file to the document.
     *  \param rFileSpec a file specification
     */
    void AttachFile(const PdfFileSpec& fileSpec);

    /** Get an attached file's filespec.
     *  \param name the name of the attachment
     *  \return the file specification object if the file exists, nullptr otherwise
     *          The file specification object is not owned by the document and must be deleted by the caller
     */
    PdfFileSpec* GetAttachment(const PdfString& name);

    /** Adds a PdfDestination into the global Names tree
     *  with the specified name, optionally replacing one of the same name.
     *  \param dest the destination to be assigned
     *  \param name the name for the destination
     */
    void AddNamedDestination(const PdfDestination& dest, const PdfString& name);

    void CollectGarbage();

    /** Constuct a new PdfImage object
     *  \param prefix optional prefix for XObject-name
     */
    std::unique_ptr<PdfImage> CreateImage(const std::string_view& prefix = { });

    std::unique_ptr<PdfXObjectForm> CreateXObjectForm(const PdfRect& rect, const std::string_view& prefix = { });

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

    PdfNameTree* GetNames() { return m_NameTree.get(); }

    PdfOutlines* GetOutlines() { return m_Outlines.get(); }

    PdfFontManager& GetFonts() { return m_FontManager; }

protected:
    /** Construct a new (empty) PdfDocument
     *  \param empty if true NO default objects (such as catalog) are created.
     */
    PdfDocument(bool empty = false);

    PdfDocument(const PdfDocument& doc);

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

    /** Clear all internal variables
     *  and reset PdfDocument to an intial state.
     */
    void Clear();

    /** Get the PDF version of the document
     *  \returns PdfVersion version of the pdf document
     */
    virtual PdfVersion GetPdfVersion() const = 0;

    /** Get the PDF version of the document
     *  \returns PdfVersion version of the pdf document
     */
    virtual void SetPdfVersion(PdfVersion version) = 0;

private:
    // Called by PdfPageCollection
    void AppendDocumentPages(const PdfDocument& doc);
    void InsertDocumentPageAt(unsigned atIndex, const PdfDocument& doc, unsigned pageIndex);
    void AppendDocumentPages(const PdfDocument& doc, unsigned pageIndex, unsigned pageCount);

    // Called by PdfXObjectForm
    PdfRect FillXObjectFromPage(PdfXObjectForm& xobj, const PdfPage& page, bool useTrimBox);

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

private:
    PdfDocument& operator=(const PdfDocument&) = delete;

    PdfInfo& GetOrCreateInfo();

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
    std::unique_ptr<PdfOutlines> m_Outlines;
    std::unique_ptr<PdfNameTree> m_NameTree;
};

};


#endif	// PDF_DOCUMENT_H

/**
 * SPDX-FileCopyrightText: (C) 2025 David Lilly <david.lilly@ticketmaster.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_COLLECTION_H
#define PDF_COLLECTION_H

#include <podofo/main/PdfDeclarations.h>
#include <podofo/main/PdfElement.h>
#include <podofo/main/PdfString.h>

namespace PoDoFo {

class PdfDocument;
class PdfCollectionSchema;

/**
 * A PDF Collection (Portfolio) allows multiple files to be embedded with a visual presentation.
 * This class is part of the experimental/staging API and may change in future releases.
 *
 * Per ISO 32000-1 Section 12.3.5, a collection is a dictionary in the PDF catalog that
 * defines the portfolio properties including schema, view mode, sorting, and initial document.
 *
 * ⚠️ EXPERIMENTAL API: This class is in the staging directory and its API may change
 * in future PoDoFo releases. Use with caution in production code.
 */
class PODOFO_API PdfCollection final : public PdfDictionaryElement
{
    friend class PdfDocument;

private:
    /** Create a new collection
     * \param doc the parent document
     */
    PdfCollection(PdfDocument& doc);

    /** Create a collection from an existing object
     * \param obj the existing PDF object
     */
    PdfCollection(PdfObject& obj);

    PdfCollection(const PdfCollection& collection) = delete;

public:
    virtual ~PdfCollection();

    /** Get or create the collection schema
     * \returns reference to the schema
     */
    PdfCollectionSchema& GetOrCreateSchema();

    /** Get the collection schema
     * \returns pointer to the schema or nullptr if not created
     */
    nullable<PdfCollectionSchema&> GetSchema();

    /** Get the collection schema (const version)
     * \returns pointer to the schema or nullptr if not created
     */
    nullable<const PdfCollectionSchema&> GetSchema() const;

    /** Set the initial document to display when opening the portfolio
     * \param filename the filename of the embedded file to open initially
     */
    void SetInitialDocument(nullable<const PdfString&> filename);

    /** Get the initial document filename
     * \returns the initial document filename or nullptr if not set
     */
    nullable<const PdfString&> GetInitialDocument() const;

    /** Set the view mode for the portfolio
     * \param mode the view mode (Details, Tile, or Hidden)
     */
    void SetViewMode(PdfCollectionViewMode mode);

    /** Get the current view mode
     * \returns the view mode (defaults to Details if not set)
     */
    PdfCollectionViewMode GetViewMode() const;

    /** Set the sort configuration for the portfolio
     * \param fieldName the field name to sort by
     * \param ascending true for ascending order, false for descending
     */
    void SetSort(const std::string_view& fieldName, bool ascending = true);

    /** Clear the sort configuration
     */
    void ClearSort();

    /** Check if sorting is configured
     * \returns true if sort configuration exists
     */
    bool HasSort() const;

private:
    void initFromObject();
    static PdfName getViewModeName(PdfCollectionViewMode mode);
    static PdfCollectionViewMode getViewModeFromName(const PdfName& name);

private:
    std::unique_ptr<PdfCollectionSchema> m_Schema;
};

}

#endif // PDF_COLLECTION_H

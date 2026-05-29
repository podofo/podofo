/**
 * SPDX-FileCopyrightText: (C) 2025 David Lilly <david.lilly@ticketmaster.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_COLLECTION_ITEM_H
#define PDF_COLLECTION_ITEM_H

#include <podofo/main/PdfDeclarations.h>
#include <podofo/main/PdfElement.h>
#include <podofo/main/PdfString.h>

namespace PoDoFo {

class PdfDocument;
class PdfDate;

/**
 * A PDF Collection Item stores per-file metadata values for a file in a PDF Portfolio.
 * This class is part of the experimental/staging API and may change in future releases.
 *
 * Per ISO 32000-1 Section 12.3.5, a collection item is a dictionary with keys
 * corresponding to schema field names and values containing the data for that field.
 *
 * ⚠️ EXPERIMENTAL API: This class is in the staging directory and its API may change
 * in future PoDoFo releases. Use with caution in production code.
 */
class PODOFO_API PdfCollectionItem final : public PdfDictionaryElement
{
    friend class PdfFileSpec;
    friend class PdfCollection;

private:
    /** Create a new collection item
     * \param doc the parent document
     */
    PdfCollectionItem(PdfDocument& doc);

    /** Create a collection item from an existing object
     * \param obj the existing PDF object
     */
    PdfCollectionItem(PdfObject& obj);

    PdfCollectionItem(const PdfCollectionItem& item) = default;

public:
    /** Set a string field value
     * \param fieldName the name of the field
     * \param value the string value
     */
    void SetFieldValue(const std::string_view& fieldName, const PdfString& value);

    /** Set a numeric field value
     * \param fieldName the name of the field
     * \param value the numeric value
     */
    void SetFieldValue(const std::string_view& fieldName, double value);

    /** Set a date field value
     * \param fieldName the name of the field
     * \param value the date value
     */
    void SetFieldValue(const std::string_view& fieldName, const PdfDate& value);

    /** Get a field value
     * \param fieldName the name of the field
     * \returns the field value object or nullptr if not found
     */
    nullable<const PdfObject&> GetFieldValue(const std::string_view& fieldName) const;

    /** Get a field value
     * \param fieldName the name of the field
     * \returns the field value object or nullptr if not found
     */
    nullable<PdfObject&> GetFieldValue(const std::string_view& fieldName);

    /** Remove a field from the collection item
     * \param fieldName the name of the field to remove
     */
    void RemoveField(const std::string_view& fieldName);

    /** Get all field names that have values
     * \returns a vector of field names
     */
    std::vector<std::string> GetFieldNames() const;
};

}

#endif // PDF_COLLECTION_ITEM_H

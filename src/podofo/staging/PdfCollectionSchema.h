/**
 * SPDX-FileCopyrightText: (C) 2025 David Lilly <david.lilly@ticketmaster.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_COLLECTION_SCHEMA_H
#define PDF_COLLECTION_SCHEMA_H

#include <podofo/main/PdfDeclarations.h>
#include <podofo/main/PdfElement.h>
#include <podofo/main/PdfString.h>

namespace PoDoFo {

class PdfDocument;

/**
 * A PDF Collection Schema defines the structure of metadata fields for files in a PDF Portfolio.
 * This class is part of the experimental/staging API and may change in future releases.
 *
 * Per ISO 32000-1 Section 12.3.5, a collection schema is a dictionary where each key
 * is a field name and each value is a field definition dictionary with /Type=CollectionField.
 *
 * ⚠️ EXPERIMENTAL API: This class is in the staging directory and its API may change
 * in future PoDoFo releases. Use with caution in production code.
 */
class PODOFO_API PdfCollectionSchema final : public PdfDictionaryElement
{
    friend class PdfCollection;

private:
    /** Create a new collection schema
     * \param doc the parent document
     */
    PdfCollectionSchema(PdfDocument& doc);

    /** Create a collection schema from an existing object
     * \param obj the existing PDF object
     */
    PdfCollectionSchema(PdfObject& obj);

    PdfCollectionSchema(const PdfCollectionSchema& schema) = default;

public:
    /** Add a field definition to the schema
     * \param fieldName the name of the field (used as dictionary key)
     * \param type the data type of the field
     * \param displayName optional display name shown to users
     * \param order optional display order (0-based)
     */
    void AddField(const std::string_view& fieldName,
        PdfCollectionFieldType type,
        nullable<const PdfString&> displayName = nullptr,
        nullable<int64_t> order = nullptr);

    /** Remove a field definition from the schema
     * \param fieldName the name of the field to remove
     */
    void RemoveField(const std::string_view& fieldName);

    /** Check if a field exists in the schema
     * \param fieldName the name of the field
     * \returns true if the field exists
     */
    bool HasField(const std::string_view& fieldName) const;

    /** Get all field names defined in the schema
     * \returns a vector of field names
     */
    std::vector<std::string> GetFieldNames() const;

    /** Set whether a field is editable
     * \param fieldName the name of the field
     * \param editable true if the field can be edited
     */
    void SetFieldEditable(const std::string_view& fieldName, bool editable);

    /** Set whether a field is visible
     * \param fieldName the name of the field
     * \param visible true if the field should be displayed
     */
    void SetFieldVisible(const std::string_view& fieldName, bool visible);

    /** Get the field type for a field
     * \param fieldName the name of the field
     * \returns the field type or nullptr if field not found
     */
    nullable<PdfCollectionFieldType> GetFieldType(const std::string_view& fieldName) const;

private:
    PdfObject* getFieldDict(const std::string_view& fieldName);
    const PdfObject* getFieldDict(const std::string_view& fieldName) const;
    static PdfName getSubtypeForFieldType(PdfCollectionFieldType type);
};

}

#endif // PDF_COLLECTION_SCHEMA_H

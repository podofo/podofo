/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_RESOURCE_OPERATIONS_H
#define PDF_RESOURCE_OPERATIONS_H

#include "PdfDictionary.h"

namespace PoDoFo {

/**
 * Low level interface for resource handling operations
 * Inherited by PdfResources
 */
class PODOFO_API PdfResourceOperations
{
protected:
    PdfResourceOperations();

public:
    /** Add resource by type generating a new unique identifier
     */
    virtual PdfName AddResource(PdfResourceType type, const PdfObject& obj) = 0;

    /** Add resource by type and key
     */
    virtual void AddResource(PdfResourceType type, const PdfName& key, const PdfObject& obj) = 0;

    /** Get resource by type and key
     */
    virtual PdfObject* GetResource(PdfResourceType type, const std::string_view& key) = 0;

    /** Get resource by type and key
     */
    virtual const PdfObject* GetResource(PdfResourceType type, const std::string_view& key) const = 0;

    /** Get resource iterator by type
     */
    virtual PdfDictionaryIndirectIterable GetResourceIterator(PdfResourceType type) = 0;

    /** Get resource iterator by type
     */
    virtual PdfDictionaryConstIndirectIterable GetResourceIterator(PdfResourceType type) const = 0;

    /** Remove resource by type and key
     * \remarks It remove the resource only by reference
     */
    virtual void RemoveResource(PdfResourceType type, const std::string_view& key) = 0;

    /** Remove resources by type
     * \remarks It remove the resources only by reference
     */
    virtual void RemoveResources(PdfResourceType type) = 0;

    // Generic functions to access resources by arbitrary type
    // name. They shouldn't be generally needed, they are provided
    // for custom use

    /** Add resource by type name string generating a new unique identifier
     */
    virtual PdfName AddResource(const PdfName& type, const PdfObject& obj) = 0;

    /** Add resource by type name string and key
     */
    virtual void AddResource(const PdfName& type, const PdfName& key, const PdfObject& obj) = 0;

    /** Get resource by type name string and key
     */
    virtual PdfObject* GetResource(const std::string_view& type, const std::string_view& key) = 0;

    /** Get resource by type name string and key
     */
    virtual const PdfObject* GetResource(const std::string_view& type, const std::string_view& key) const = 0;

    /** Get resource iterator by type name string
     */
    virtual PdfDictionaryIndirectIterable GetResourceIterator(const std::string_view& type) = 0;

    /** Get resource iterator by type name string
     */
    virtual PdfDictionaryConstIndirectIterable GetResourceIterator(const std::string_view& type) const = 0;

    /** Remove resource by type name string and key
     * \remarks It remove the resource only by reference
     */
    virtual void RemoveResource(const std::string_view& type, const std::string_view& key) = 0;

    /** Remove resources by type name string
     * \remarks It remove the resources only by reference
     */
    virtual void RemoveResources(const std::string_view& type) = 0;

protected:
    PdfResourceOperations(const PdfResourceOperations&) = default;
    PdfResourceOperations& operator=(const PdfResourceOperations&) = default;
};

};

#endif // PDF_RESOURCE_OPERATIONS_H

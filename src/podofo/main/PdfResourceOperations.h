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
    // Generic functions to access resources by arbitrary type
    // name. They shouldn't be generally needed, they are provided
    // for custom use
    virtual void AddResource(const PdfName& type, const PdfName& key, const PdfObject& obj) = 0;
    virtual PdfDictionaryIndirectIterable GetResourceIterator(const std::string_view& type) = 0;
    virtual PdfDictionaryConstIndirectIterable GetResourceIterator(const std::string_view& type) const = 0;
    virtual void RemoveResource(const std::string_view& type, const std::string_view& key) = 0;
    virtual void RemoveResources(const std::string_view& type) = 0;
    virtual PdfObject* GetResource(const std::string_view& type, const std::string_view& key) = 0;
    virtual const PdfObject* GetResource(const std::string_view& type, const std::string_view& key) const = 0;

protected:
    PdfResourceOperations(const PdfResourceOperations&) = default;
    PdfResourceOperations& operator=(const PdfResourceOperations&) = default;
};

};

#endif // PDF_RESOURCE_OPERATIONS_H

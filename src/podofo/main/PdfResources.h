/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_RESOURCES_H
#define PDF_RESOURCES_H

#include "PdfElement.h"
#include "PdfDictionary.h"
#include "PdfColor.h"

namespace PoDoFo {

enum class PdfResourceType
{
    Unknown = 0,
    ExtGState,
    ColorSpace,
    Pattern,
    Shading,
    XObject,
    Font,
    Properties
};

class PdfFont;
class PdfCanvas;

/** A interface that provides a wrapper around /Resources
 */
class PODOFO_API PdfResources final : public PdfDictionaryElement
{
    friend class PdfPage;
    friend class PdfXObjectForm;

private:
    PdfResources(PdfObject& obj);
    PdfResources(PdfCanvas& canvas);

public:
    /** Add resource by type generating a new unique identifier
     */
    PdfName AddResource(PdfResourceType type, const PdfObject& obj);
    void AddResource(PdfResourceType type, const PdfName& key, const PdfObject& obj);
    PdfDictionaryIndirectIterable GetResourceIterator(PdfResourceType type);
    PdfDictionaryConstIndirectIterable GetResourceIterator(PdfResourceType type) const;
    void RemoveResource(PdfResourceType type, const std::string_view& key);
    void RemoveResources(PdfResourceType type);
    PdfObject* GetResource(PdfResourceType type, const std::string_view& key);
    const PdfObject* GetResource(PdfResourceType type, const std::string_view& key) const;

    const PdfFont* GetFont(const std::string_view& name) const;

    // Generic functions to access resources by arbitrary type
    // name. They shouldn't be generally needed, they are left
    // for compatibility or custom use
    void AddResource(const PdfName& type, const PdfName& key, const PdfObject& obj);
    PdfDictionaryIndirectIterable GetResourceIterator(const std::string_view& type);
    PdfDictionaryConstIndirectIterable GetResourceIterator(const std::string_view& type) const;
    void RemoveResource(const std::string_view& type, const std::string_view& key);
    void RemoveResources(const std::string_view& type);
    PdfObject* GetResource(const std::string_view& type, const std::string_view& key);
    const PdfObject* GetResource(const std::string_view& type, const std::string_view& key) const;

private:
    PdfObject* getResource(const std::string_view& type, const std::string_view& key) const;
    bool tryGetDictionary(const std::string_view& type, PdfDictionary*& dict) const;
    PdfDictionary& getOrCreateDictionary(const std::string_view& type);

private:
    std::array<unsigned, (unsigned)PdfResourceType::Properties> m_currResourceIds;
};

};

#endif // PDF_RESOURCES_H

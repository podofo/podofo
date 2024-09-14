/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_RESOURCES_H
#define PDF_RESOURCES_H

#include "PdfElement.h"
#include "PdfResourceOperations.h"

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
class PODOFO_API PdfResources final : public PdfDictionaryElement, public PdfResourceOperations
{
    friend class PdfPage;
    friend class PdfXObjectForm;
    friend class PdfPainter;
    friend class PdfAcroForm;
    friend class PdfAnnotation;

private:
    PdfResources(PdfDocument& doc);
    PdfResources(PdfObject& obj);
    PdfResources(PdfCanvas& canvas);

public:
    static bool TryCreateFromObject(PdfObject& obj, std::unique_ptr<PdfResources>& resources);

public:
    /** Add resource by type generating a new unique identifier
     */
    PdfName AddResource(PdfResourceType type, const PdfObject& obj);

    void AddResource(PdfResourceType type, const PdfName& key, const PdfObject& obj);
    PdfObject* GetResource(PdfResourceType type, const std::string_view& key);
    const PdfObject* GetResource(PdfResourceType type, const std::string_view& key) const;
    PdfDictionaryIndirectIterable GetResourceIterator(PdfResourceType type);
    PdfDictionaryConstIndirectIterable GetResourceIterator(PdfResourceType type) const;
    void RemoveResource(PdfResourceType type, const std::string_view& key);
    void RemoveResources(PdfResourceType type);

    const PdfFont* GetFont(const std::string_view& name) const;

private:
    void AddResource(const PdfName& type, const PdfName& key, const PdfObject& obj) override;
    PdfDictionaryIndirectIterable GetResourceIterator(const std::string_view& type) override;
    PdfDictionaryConstIndirectIterable GetResourceIterator(const std::string_view& type) const override;
    void RemoveResource(const std::string_view& type, const std::string_view& key) override;
    void RemoveResources(const std::string_view& type) override;
    PdfObject* GetResource(const std::string_view& type, const std::string_view& key) override;
    const PdfObject* GetResource(const std::string_view& type, const std::string_view& key) const override;

private:
    PdfObject* getResource(const std::string_view& type, const std::string_view& key) const;
    bool tryGetDictionary(const std::string_view& type, PdfDictionary*& dict) const;
    PdfDictionary& getOrCreateDictionary(const PdfName& type);

private:
    std::array<unsigned, (unsigned)PdfResourceType::Properties> m_currResourceIds;
};

};

#endif // PDF_RESOURCES_H

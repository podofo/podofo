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

// TODO: Use it in PdfResources
enum class PdfResourceType
{
    ExtGState,
    ColorSpace,
    Pattern,
    Shading,
    XObject,
    Font,
    Properties
};

/** A interface that provides a wrapper around /Resources
 */
class PODOFO_API PdfResources : public PdfDictionaryElement
{
public:
    PdfResources(PdfObject& obj);
    PdfResources(PdfDictionary& dict);

public:
    void AddResource(const PdfName& type, const PdfName& key, const PdfObject& obj);
    void AddResource(const PdfName& type, const PdfName& key, const PdfObject* obj);
    PdfDictionaryIndirectIterable GetResourceIterator(const std::string_view& type);
    PdfDictionaryConstIndirectIterable GetResourceIterator(const std::string_view& type) const;
    void RemoveResource(const std::string_view& type, const std::string_view& key);
    void RemoveResources(const std::string_view& type);
    PdfObject* GetResource(const std::string_view& type, const std::string_view& key);
    const PdfObject* GetResource(const std::string_view& type, const std::string_view& key) const;
    /** Register a colourspace for a (separation) colour in the resource dictionary
     *  of this page or XObject so that it can be used for any following drawing
     *  operations.
     *
     *  \param color reference to the PdfColor
     */
    void AddColorResource(const PdfColor& color);
private:
    PdfObject* getResource(const std::string_view& type, const std::string_view& key) const;
    bool tryGetDictionary(const std::string_view& type, PdfDictionary*& dict) const;
    PdfDictionary& getOrCreateDictionary(const std::string_view& type);
};

};

#endif // PDF_RESOURCES_H

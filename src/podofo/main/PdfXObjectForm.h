/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_XOBJECT_FORM_H
#define PDF_XOBJECT_FORM_H

#include "PdfXObject.h"
#include "PdfCanvas.h"
#include "PdfResources.h"

namespace PoDoFo {

class PdfPage;

class PODOFO_API PdfXObjectForm final : public PdfXObject, public PdfCanvas
{
    friend class PdfDocument;
    friend class PdfXObject;

private:
    /** Create a new XObject with a specified dimension
     *  in a given document
     *
     *  \param doc the parent document of the XObject
     *  \param rect the size of the XObject
     *  \param prefix optional prefix for XObject-name
     */
    PdfXObjectForm(PdfDocument& doc, const Rect& rect,
        const std::string_view& prefix);

public:
    /** Create a new XObject from a page of another document
     *  in a given document
     *
     *  \param page the document to create the XObject from
     *	\param useTrimBox if true try to use trimbox for size of xobject
     */
    void FillFromPage(const PdfPage& page, bool useTrimBox = false);

    /** Ensure resources initialized on this XObject
     */
    void EnsureResourcesCreated() override;

public:
    PdfResources& GetOrCreateResources() override;

    bool HasRotation(double& teta) const override;

    Rect GetRect() const override;

    /** Set the rectangle of this xobject
     *  \param rect a rectangle
     */
    void SetRect(const Rect& rect);

public:
    inline PdfResources* GetResources() { return m_Resources.get(); }
    inline const PdfResources* GetResources() const { return m_Resources.get(); }

private:
    PdfXObjectForm(PdfObject& obj);

private:
    Rect GetRectRaw() const override;
    PdfObject* getContentsObject() override;
    PdfResources* getResources() override;
    PdfElement& getElement() override;
    PdfObjectStream& GetStreamForAppending(PdfStreamAppendFlags flags) override;
    void initXObject(const Rect& rect);
    void initAfterPageInsertion(const PdfPage& page);

private:
    PdfElement& GetElement() = delete;
    const PdfElement& GetElement() const = delete;
    PdfObject* GetContentsObject() = delete;
    const PdfObject* GetContentsObject() const = delete;

private:
    Rect m_Rect;
    PdfArray m_Matrix;
    std::unique_ptr<PdfResources> m_Resources;
};

}

#endif // PDF_XOBJECT_FORM_H

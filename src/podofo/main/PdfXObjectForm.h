// SPDX-FileCopyrightText: 2007 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2021 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#ifndef PDF_XOBJECT_FORM_H
#define PDF_XOBJECT_FORM_H

#include "PdfXObject.h"
#include "PdfCanvas.h"
#include "PdfResources.h"

namespace PoDoFo {

class PdfPage;
class PdfObjectRelocationMap;

enum class PdfFillFormFlags
{
    None = 0,           ///< Default, intersects with /CropBox
    _Reserved1 = 1,
    _Reserved2 = 2,
    UseTrimBox = 4,     ///< Default, intersects with /TrimBox
    _Reserved8 = 8,
};

class PODOFO_API PdfXObjectForm final : public PdfXObject, public PdfCanvas
{
    friend class PdfDocument;
    friend class PdfXObject;
    friend class PdfAnnotation;

private:
    /// Create a new XObject with a specified dimension
    /// in a given document
    ///
    /// @param doc the parent document of the XObject
    /// @param rect the size of the XObject
    PdfXObjectForm(PdfDocument& doc, const Rect& rect);

public:
    [[deprecated("Use the FillFromPage methods with flags instead")]]
    void FillFromPage(const PdfPage& page, bool useTrimBox);

    /// Create a new XObject from a page of another document
    /// in a given document. /BBox is set in form space (the content-stream
    /// coordinate system); /Matrix applies the source page's /Rotate if present.
    ///
    /// @param page the page to create the XObject from
    /// @param flags flags to control the form filling
    /// @param map a map accumulating relocation entries, which prevents spurious copies on separate fillings
    void FillFromPage(const PdfPage& page, PdfFillFormFlags flags = PdfFillFormFlags::None, PdfObjectRelocationMap* map = nullptr);
    void FillFromPage(const PdfPage& page, PdfObjectRelocationMap* map);

public:
    PdfResources& GetOrCreateResources() override;

    Rect GetRect() const override;

    /// Set the rectangle of this xobject
    /// @param rect a rectangle
    void SetRect(const Rect& rect);

    void SetMatrix(const Matrix& m);

    const Matrix& GetMatrix() const override;

public:
    inline PdfResources* GetResources() { return m_Resources.get(); }
    inline const PdfResources* GetResources() const { return m_Resources.get(); }

protected:
    PdfXObjectForm* getForm() const override;

private:
    PdfXObjectForm(PdfObject& obj);

private:
    bool TryGetRotationRadians(double& teta) const override;
    Corners GetRectRaw() const override;
    PdfObject* getContentsObject() override;
    PdfResources* getResources() override;
    PdfDictionaryElement& getElement() override;
    PdfObjectStream& GetOrCreateContentsStream(PdfStreamAppendFlags flags) override;
    PdfObjectStream& ResetContentsStream() override;
    void CopyContentsTo(OutputStream& stream) const override;
    void initXObject(const Rect& rect);

private:
    // Remove some PdfCanvas methods to maintain the class API surface clean
    PdfElement& GetElement() = delete;
    const PdfElement& GetElement() const = delete;
    PdfObject* GetContentsObject() = delete;
    const PdfObject* GetContentsObject() const = delete;

private:
    Rect m_Rect;
    Matrix m_Matrix;
    std::unique_ptr<PdfResources> m_Resources;
};

}

ENABLE_BITMASK_OPERATORS(PoDoFo::PdfFillFormFlags);

#endif // PDF_XOBJECT_FORM_H

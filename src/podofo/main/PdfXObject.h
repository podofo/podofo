/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_XOBJECT_H
#define PDF_XOBJECT_H

#include "PdfElement.h"
#include "PdfArray.h"
#include <podofo/auxiliary/Matrix.h>
#include <podofo/auxiliary/Rect.h>

namespace PoDoFo {

class PdfImage;
class PdfXObjectForm;
class PdfXObjectPostScript;
class PdfAnnotation;

/** A XObject is a content stream with several drawing commands and data
 *  which can be used throughout a PDF document.
 *
 *  You can draw on a XObject like you would draw onto a page and can draw
 *  this XObject later again using a PdfPainter.
 *
 *  \see PdfPainter
 */
class PODOFO_API PdfXObject : public PdfDictionaryElement
{
    friend class PdfXObjectForm;
    friend class PdfImage;
    friend class PdfXObjectPostScript;
    friend class PdfContentStreamReader;
    friend class PdfAnnotation;

private:
    PdfXObject(PdfDocument& doc, PdfXObjectType subType);
    PdfXObject(PdfObject& obj, PdfXObjectType subType);

public:
    static bool TryCreateFromObject(PdfObject& obj, std::unique_ptr<PdfXObject>& xobj);

    static bool TryCreateFromObject(const PdfObject& obj, std::unique_ptr<const PdfXObject>& xobj);

    template <typename XObjectT>
    static bool TryCreateFromObject(PdfObject& obj, std::unique_ptr<XObjectT>& xobj);

    template <typename XObjectT>
    static bool TryCreateFromObject(const PdfObject& obj, std::unique_ptr<const XObjectT>& xobj);

    virtual Rect GetRect() const = 0;

    virtual const Matrix& GetMatrix() const;

    inline PdfXObjectType GetType() const { return m_Type; }

protected:
    virtual const PdfXObjectForm* GetForm() const;

private:
    // To be called from PdfContentStreamReader
    static std::unique_ptr<PdfXObject> CreateFromObject(const PdfObject& obj, PdfXObjectType reqType, PdfXObjectType& detectedType);

    static PdfXObject* createFromObject(const PdfObject& obj, PdfXObjectType reqType, PdfXObjectType& detectedType);
    static PdfXObjectType getPdfXObjectType(const PdfObject& obj);
    template <typename TXObject>
    static constexpr PdfXObjectType GetXObjectType();

private:
    PdfXObjectType m_Type;
};

template<typename XObjectT>
inline bool PdfXObject::TryCreateFromObject(PdfObject& obj, std::unique_ptr<XObjectT>& xobj)
{
    PdfXObjectType detectedType;
    xobj.reset((XObjectT*)createFromObject(obj, GetXObjectType<XObjectT>(), detectedType));
    return xobj != nullptr;
}

template<typename XObjectT>
inline bool PdfXObject::TryCreateFromObject(const PdfObject& obj, std::unique_ptr<const XObjectT>& xobj)
{
    PdfXObjectType detectedType;
    xobj.reset((const XObjectT*)createFromObject(obj, GetXObjectType<XObjectT>(), detectedType));
    return xobj != nullptr;
}

template<typename TXObject>
constexpr PdfXObjectType PdfXObject::GetXObjectType()
{
    if (std::is_same_v<TXObject, PdfXObjectForm>)
        return PdfXObjectType::Form;
    else if (std::is_same_v<TXObject, PdfImage>)
        return PdfXObjectType::Image;
    else if (std::is_same_v<TXObject, PdfXObjectPostScript>)
        return PdfXObjectType::PostScript;
    else
        return PdfXObjectType::Unknown;
}

};

#endif // PDF_XOBJECT_H



/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_XOBJECT_H
#define PDF_XOBJECT_H

#include "PdfElement.h"
#include "PdfArray.h"
#include "PdfRect.h"
#include "PdfMath.h"

namespace PoDoFo {

class PdfObject;
class PdfImage;
class PdfXObjectForm;
class PdfXObjectPostScript;
class Matrix;

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

private:
    PdfXObject(PdfDocument& doc, PdfXObjectType subType, const std::string_view& prefix);
    PdfXObject(PdfObject& obj, PdfXObjectType subType);

public:
    static bool TryCreateFromObject(PdfObject& obj, std::unique_ptr<PdfXObject>& xobj);

    static bool TryCreateFromObject(const PdfObject& obj, std::unique_ptr<const PdfXObject>& xobj);

    template <typename XObjectT>
    static bool TryCreateFromObject(PdfObject& obj, std::unique_ptr<XObjectT>& xobj);

    template <typename XObjectT>
    static bool TryCreateFromObject(const PdfObject& obj, std::unique_ptr<const XObjectT>& xobj);

    virtual PdfRect GetRect() const = 0;

    Matrix GetMatrix() const;

    void SetMatrix(const Matrix& m);

    /** Get the identifier used for drawig this object
     *  \returns identifier
     */
    inline const PdfName& GetIdentifier() const { return m_Identifier; }

    inline PdfXObjectType GetType() const { return m_Type; }

private:
    static bool tryCreateFromObject(const PdfObject& obj, PdfXObjectType xobjType, PdfXObject*& xobj);
    static bool tryCreateFromObject(const PdfObject& obj, const std::type_info& typeInfo, PdfXObject*& xobj);
    void initIdentifiers(const std::string_view& prefix);
    static PdfXObjectType getPdfXObjectType(const PdfObject& obj);

private:
    PdfXObjectType m_Type;
    PdfName m_Identifier;
};

template<typename XObjectT>
inline bool PdfXObject::TryCreateFromObject(PdfObject& obj, std::unique_ptr<XObjectT>& xobj)
{
    PdfXObject* xobj_;
    if (!tryCreateFromObject(obj, typeid(XObjectT), xobj_))
        return false;

    xobj.reset((XObjectT*)xobj_);
    return true;
}

template<typename XObjectT>
inline bool PdfXObject::TryCreateFromObject(const PdfObject& obj, std::unique_ptr<const XObjectT>& xobj)
{
    PdfXObject* xobj_;
    if (!tryCreateFromObject(obj, typeid(XObjectT), xobj_))
        return false;

    xobj.reset((const XObjectT*)xobj_);
    return true;
}

};

#endif // PDF_XOBJECT_H



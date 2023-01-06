/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfXObject.h"

#include "PdfDictionary.h"
#include "PdfRect.h"
#include "PdfVariant.h"
#include "PdfImage.h"
#include "PdfPage.h"
#include "PdfDocument.h"
#include "PdfXObjectForm.h"
#include "PdfXObjectPostScript.h"
#include "PdfStringStream.h"

using namespace std;
using namespace PoDoFo;

static string_view toString(PdfXObjectType type);
static PdfXObjectType fromString(const string_view& str);

PdfXObject::PdfXObject(PdfDocument& doc, PdfXObjectType subType, const string_view& prefix)
    : PdfDictionaryElement(doc, "XObject"), m_Type(subType)
{
    initIdentifiers(prefix);
    this->GetObject().GetDictionary().AddKey(PdfName::KeySubtype, PdfName(toString(subType)));
}

PdfXObject::PdfXObject(PdfObject& obj, PdfXObjectType subType)
    : PdfDictionaryElement(obj), m_Type(subType)
{
    initIdentifiers({ });
}

bool PdfXObject::TryCreateFromObject(PdfObject& obj, unique_ptr<PdfXObject>& xobj)
{
    PdfXObject* xobj_;
    if (!tryCreateFromObject(obj, PdfXObjectType::Unknown, xobj_))
    {
        xobj.reset();
        return false;
    }

    xobj.reset(xobj_);
    return true;
}

bool PdfXObject::TryCreateFromObject(const PdfObject& obj, unique_ptr<const PdfXObject>& xobj)
{
    PdfXObject* xobj_;
    if (!tryCreateFromObject(obj, PdfXObjectType::Unknown, xobj_))
    {
        xobj.reset();
        return false;
    }

    xobj.reset(xobj_);
    return true;
}

bool PdfXObject::tryCreateFromObject(const PdfObject& obj, PdfXObjectType xobjType, PdfXObject*& xobj)
{
    const PdfDictionary* dict;
    const PdfObject* typeObj;
    const PdfName* name;
    if (!obj.TryGetDictionary(dict)
        || (typeObj = dict->GetKey(PdfName::KeyType)) == nullptr
        || !typeObj->TryGetName(name)
        || name->GetString() != "XObject")
    {
        xobj = nullptr;
        return false;
    }

    auto type = getPdfXObjectType(obj);
    if (xobjType != PdfXObjectType::Unknown && type != xobjType)
    {
        xobj = nullptr;
        return false;
    }

    switch (type)
    {
        case PdfXObjectType::Form:
        {
            xobj = new PdfXObjectForm(const_cast<PdfObject&>(obj));
            return true;
        }
        case PdfXObjectType::PostScript:
        {
            xobj = new PdfXObjectPostScript(const_cast<PdfObject&>(obj));
            return true;
        }
        case PdfXObjectType::Image:
        {
            xobj = new PdfImage(const_cast<PdfObject&>(obj));
            return true;
        }
        default:
        {
            xobj = nullptr;
            return false;
        }
    }
}

PdfXObjectType PdfXObject::getPdfXObjectType(const PdfObject& obj)
{
    const PdfName* name;
    auto subTypeObj = obj.GetDictionary().FindKey(PdfName::KeySubtype);
    if (subTypeObj == nullptr || !subTypeObj->TryGetName(name))
        return PdfXObjectType::Unknown;

    auto subtype = name->GetString();
    return fromString(subtype);
}

Matrix PdfXObject::GetMatrix() const
{
    auto matrixObj = GetObject().GetDictionary().GetKey("Matrix");
    if (matrixObj == nullptr)
        return Matrix();

    auto& arr = matrixObj->GetArray();
    return Matrix::FromArray(arr);
}

void PdfXObject::SetMatrix(const Matrix& m)
{
    PdfArray arr;
    arr.Add(m[0]);
    arr.Add(m[1]);
    arr.Add(m[2]);
    arr.Add(m[3]);
    arr.Add(m[4]);
    arr.Add(m[5]);

    GetObject().GetDictionary().AddKey("Matrix", std::move(arr));
}

bool PdfXObject::tryCreateFromObject(const PdfObject& obj, const type_info& typeInfo, PdfXObject*& xobj)
{
    PdfXObjectType xobjType;
    if (typeInfo == typeid(PdfXObjectForm))
        xobjType = PdfXObjectType::Form;
    else if (typeInfo == typeid(PdfImage))
        xobjType = PdfXObjectType::Image;
    else if (typeInfo == typeid(PdfXObjectPostScript))
        xobjType = PdfXObjectType::PostScript;
    else
        PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);

    return tryCreateFromObject(obj, xobjType, xobj);
}

void PdfXObject::initIdentifiers(const string_view& prefix)
{
    PdfStringStream out;

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /XOb for XObject.
    if (prefix.length() == 0)
        out << "XOb" << this->GetObject().GetIndirectReference().ObjectNumber();
    else
        out << prefix << this->GetObject().GetIndirectReference().ObjectNumber();

    m_Identifier = PdfName(out.GetString());
}

string_view toString(PdfXObjectType type)
{
    switch (type)
    {
        case PdfXObjectType::Form:
            return "Form"sv;
        case PdfXObjectType::Image:
            return "Image"sv;
        case PdfXObjectType::PostScript:
            return "PS"sv;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);
    }
}

PdfXObjectType fromString(const string_view& str)
{
    if (str == "Form")
        return PdfXObjectType::Form;
    else if (str == "Image")
        return PdfXObjectType::Image;
    else if (str == "PS")
        return PdfXObjectType::PostScript;
    else
        return PdfXObjectType::Unknown;
}

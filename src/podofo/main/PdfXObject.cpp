/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfXObject.h"

#include "PdfDictionary.h"
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

PdfXObject::PdfXObject(PdfDocument& doc, PdfXObjectType subType)
    : PdfDictionaryElement(doc, "XObject"_n), m_Type(subType)
{
    this->GetDictionary().AddKey("Subtype"_n, PdfName(toString(subType)));
}

PdfXObject::PdfXObject(PdfObject& obj, PdfXObjectType subType)
    : PdfDictionaryElement(obj), m_Type(subType)
{
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
    // Table 93 of ISO 32000-2:2020(E), the /Type key is optional,
    // so we don't check for it. If present it should be "XObject"
    auto& dict = obj.GetDictionary();
    const PdfName* name;
    auto subTypeObj = dict.FindKey("Subtype");
    if (subTypeObj == nullptr || !subTypeObj->TryGetName(name))
    {
        // NOTE: There are some forms missing both /Type and /Subtype
        // We are a bit lenient here and consider it to be form if
        // it has a "/BBox" and it's not a tiling pattern stream
        if (obj.HasStream() && dict.HasKey("BBox") && !dict.HasKey("PatternType"))
            return PdfXObjectType::Form;

        return PdfXObjectType::Unknown;
    }

    return fromString(name->GetString());
}

Matrix PdfXObject::GetMatrix() const
{
    auto matrixObj = GetDictionary().GetKey("Matrix");
    if (matrixObj == nullptr)
        return Matrix();

    auto& arr = matrixObj->GetArray();
    return Matrix::FromArray(arr);
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

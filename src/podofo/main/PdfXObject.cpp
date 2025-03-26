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
    PdfXObjectType detectedType;
    xobj.reset(createFromObject(obj, PdfXObjectType::Unknown, detectedType));
    return xobj != nullptr;
}

bool PdfXObject::TryCreateFromObject(const PdfObject& obj, unique_ptr<const PdfXObject>& xobj)
{
    PdfXObjectType detectedType;
    xobj.reset(createFromObject(obj, PdfXObjectType::Unknown, detectedType));
    return xobj != nullptr;
}

const PdfXObjectForm* PdfXObject::GetForm() const
{
    return nullptr;
}

unique_ptr<PdfXObject> PdfXObject::CreateFromObject(const PdfObject& obj, PdfXObjectType reqType, PdfXObjectType& detectedType)
{
    return unique_ptr<PdfXObject>(createFromObject(obj, reqType, detectedType));
}

PdfXObject* PdfXObject::createFromObject(const PdfObject& obj, PdfXObjectType reqType, PdfXObjectType& detectedType)
{
    detectedType = getPdfXObjectType(obj);
    if (detectedType == PdfXObjectType::Unknown || (reqType != PdfXObjectType::Unknown && detectedType != reqType))
        return nullptr;

    switch (detectedType)
    {
        case PdfXObjectType::Form:
            return new PdfXObjectForm(const_cast<PdfObject&>(obj));
        case PdfXObjectType::PostScript:
            return new PdfXObjectPostScript(const_cast<PdfObject&>(obj));
        case PdfXObjectType::Image:
            return new PdfImage(const_cast<PdfObject&>(obj));
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
    }
}

PdfXObjectType PdfXObject::getPdfXObjectType(const PdfObject& obj)
{
    // Table 93 of ISO 32000-2:2020(E), the /Type key is optional,
    // so we don't check for it. If present it should be "XObject"
    const PdfDictionary* dict;
    const PdfName* name;
    if (!obj.TryGetDictionary(dict) || !dict->TryFindKeyAs("Subtype", name))
    {
        // NOTE: There are some forms missing both /Type and /Subtype
        // We are a bit lenient here and consider it to be form if
        // it has a "/BBox" and it's not a tiling pattern stream
        if (obj.HasStream() && dict->HasKey("BBox") && !dict->HasKey("PatternType"))
            return PdfXObjectType::Form;

        return PdfXObjectType::Unknown;
    }

    return fromString(name->GetString());
}

const Matrix& PdfXObject::GetMatrix() const
{
    return Matrix::Identity;
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

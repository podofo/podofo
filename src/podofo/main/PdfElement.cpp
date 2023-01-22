/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
 
#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfElement.h"

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfObject.h"

#include "PdfStreamedDocument.h"

using namespace std;
using namespace PoDoFo;

PdfElement::PdfElement(PdfObject& obj)
    : m_Object(&obj)
{
    if (obj.GetDocument() == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);
}

PdfElement::PdfElement(PdfObject& obj, PdfDataType expectedDataType)
    : m_Object(&obj)
{
    if (obj.GetDocument() == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    if (obj.GetDataType() != expectedDataType)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);
}

PdfElement::~PdfElement() { }

PdfDocument& PdfElement::GetDocument() const
{
    return *m_Object->GetDocument();
}

PdfDictionaryElement::PdfDictionaryElement(PdfDocument& parent, const string_view& type,
    const string_view& subtype)
    : PdfElement(parent.GetObjects().CreateDictionaryObject(type, subtype),
        PdfDataType::Dictionary)
{
}

PdfDictionaryElement::PdfDictionaryElement(PdfObject& obj)
    : PdfElement(obj, PdfDataType::Dictionary)
{
}

PdfDictionary& PdfDictionaryElement::GetDictionary()
{
    return GetObject().GetDictionary();
}

const PdfDictionary& PdfDictionaryElement::GetDictionary() const
{
    return GetObject().GetDictionary();
}

PdfArrayElement::PdfArrayElement(PdfDocument& parent)
    : PdfElement(parent.GetObjects().CreateArrayObject(),
        PdfDataType::Array)
{
}

PdfArrayElement::PdfArrayElement(PdfObject& obj)
    : PdfElement(obj, PdfDataType::Array)
{
}

PdfArray& PdfArrayElement::GetArray()
{
    return GetObject().GetArray();
}

const PdfArray& PdfArrayElement::GetArray() const
{
    return GetObject().GetArray();
}

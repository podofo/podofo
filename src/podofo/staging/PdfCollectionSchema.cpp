/**
 * SPDX-FileCopyrightText: (C) 2025 David Lilly <david.lilly@ticketmaster.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfCollectionSchema.h"

#include <podofo/main/PdfDictionary.h>
#include <podofo/main/PdfDocument.h>
#include <podofo/main/PdfObject.h>

using namespace std;
using namespace PoDoFo;

PdfCollectionSchema::PdfCollectionSchema(PdfDocument& doc)
    : PdfDictionaryElement(doc)
{
}

PdfCollectionSchema::PdfCollectionSchema(PdfObject& obj)
    : PdfDictionaryElement(obj)
{
}

void PdfCollectionSchema::AddField(const string_view& fieldName,
    PdfCollectionFieldType type,
    nullable<const PdfString&> displayName,
    nullable<int64_t> order)
{
    // Create the field definition dictionary
    auto& fieldObj = GetDocument().GetObjects().CreateDictionaryObject();
    auto& fieldDict = fieldObj.GetDictionary();

    // Set required /Type key
    fieldDict.AddKey("Type"_n, PdfName("CollectionField"));

    // Set /Subtype based on field type
    fieldDict.AddKey("Subtype"_n, getSubtypeForFieldType(type));

    // Set optional /N (display name)
    if (displayName != nullptr)
        fieldDict.AddKey("N"_n, *displayName);

    // Set optional /O (order)
    if (order != nullptr)
        fieldDict.AddKey("O"_n, static_cast<int64_t>(*order));

    // Add the field to the schema dictionary
    GetDictionary().AddKey(PdfName(fieldName), fieldObj.GetIndirectReference());
}

void PdfCollectionSchema::RemoveField(const string_view& fieldName)
{
    GetDictionary().RemoveKey(PdfName(fieldName));
}

bool PdfCollectionSchema::HasField(const string_view& fieldName) const
{
    return GetDictionary().FindKey(fieldName) != nullptr;
}

vector<string> PdfCollectionSchema::GetFieldNames() const
{
    vector<string> names;
    auto& dict = GetDictionary();

    for (auto& pair : dict)
        names.push_back(string(pair.first.GetString()));

    return names;
}

void PdfCollectionSchema::SetFieldEditable(const string_view& fieldName, bool editable)
{
    auto fieldObj = getFieldDict(fieldName);
    if (fieldObj == nullptr)
        return;

    auto& fieldDict = fieldObj->GetDictionary();
    fieldDict.AddKey("E"_n, editable);
}

void PdfCollectionSchema::SetFieldVisible(const string_view& fieldName, bool visible)
{
    auto fieldObj = getFieldDict(fieldName);
    if (fieldObj == nullptr)
        return;

    auto& fieldDict = fieldObj->GetDictionary();
    fieldDict.AddKey("V"_n, visible);
}

nullable<PdfCollectionFieldType> PdfCollectionSchema::GetFieldType(const string_view& fieldName) const
{
    auto fieldObj = getFieldDict(fieldName);
    if (fieldObj == nullptr)
        return nullptr;

    auto subtypeObj = fieldObj->GetDictionary().FindKey("Subtype");
    if (subtypeObj == nullptr)
        return nullptr;

    auto subtypeName = subtypeObj->GetName();
    auto subtypeStr = subtypeName.GetString();

    // Map PDF name to field type
    if (subtypeStr == "S")
        return PdfCollectionFieldType::String;
    else if (subtypeStr == "D")
        return PdfCollectionFieldType::Date;
    else if (subtypeStr == "N")
        return PdfCollectionFieldType::Number;
    else if (subtypeStr == "F")
        return PdfCollectionFieldType::Filename;
    else if (subtypeStr == "Desc")
        return PdfCollectionFieldType::Description;
    else if (subtypeStr == "ModDate")
        return PdfCollectionFieldType::ModDate;
    else if (subtypeStr == "CreationDate")
        return PdfCollectionFieldType::CreationDate;
    else if (subtypeStr == "Size")
        return PdfCollectionFieldType::Size;

    return nullptr;
}

PdfObject* PdfCollectionSchema::getFieldDict(const string_view& fieldName)
{
    return GetDictionary().FindKey(fieldName);
}

const PdfObject* PdfCollectionSchema::getFieldDict(const string_view& fieldName) const
{
    return GetDictionary().FindKey(fieldName);
}

PdfName PdfCollectionSchema::getSubtypeForFieldType(PdfCollectionFieldType type)
{
    switch (type)
    {
        case PdfCollectionFieldType::String:
            return PdfName("S");
        case PdfCollectionFieldType::Date:
            return PdfName("D");
        case PdfCollectionFieldType::Number:
            return PdfName("N");
        case PdfCollectionFieldType::Filename:
            return PdfName("F");
        case PdfCollectionFieldType::Description:
            return PdfName("Desc");
        case PdfCollectionFieldType::ModDate:
            return PdfName("ModDate");
        case PdfCollectionFieldType::CreationDate:
            return PdfName("CreationDate");
        case PdfCollectionFieldType::Size:
            return PdfName("Size");
        default:
            return PdfName("S"); // Default to string type
    }
}

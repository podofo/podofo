/**
 * SPDX-FileCopyrightText: (C) 2025 David Lilly <david.lilly@ticketmaster.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfCollectionItem.h"

#include <podofo/main/PdfDictionary.h>
#include <podofo/main/PdfDocument.h>
#include <podofo/main/PdfObject.h>
#include <podofo/main/PdfDate.h>

using namespace std;
using namespace PoDoFo;

PdfCollectionItem::PdfCollectionItem(PdfDocument& doc)
    : PdfDictionaryElement(doc, "CollectionItem"_n)
{
}

PdfCollectionItem::PdfCollectionItem(PdfObject& obj)
    : PdfDictionaryElement(obj)
{
}

void PdfCollectionItem::SetFieldValue(const string_view& fieldName, const PdfString& value)
{
    GetDictionary().AddKey(PdfName(fieldName), value);
}

void PdfCollectionItem::SetFieldValue(const string_view& fieldName, double value)
{
    GetDictionary().AddKey(PdfName(fieldName), value);
}

void PdfCollectionItem::SetFieldValue(const string_view& fieldName, const PdfDate& value)
{
    GetDictionary().AddKey(PdfName(fieldName), value.ToString());
}

nullable<const PdfObject&> PdfCollectionItem::GetFieldValue(const string_view& fieldName) const
{
    auto fieldObj = GetDictionary().FindKey(PdfName(fieldName));
    if (fieldObj == nullptr)
        return nullptr;

    return *fieldObj;
}

nullable<PdfObject&> PdfCollectionItem::GetFieldValue(const string_view& fieldName)
{
    auto fieldObj = GetDictionary().FindKey(PdfName(fieldName));
    if (fieldObj == nullptr)
        return nullptr;

    return *fieldObj;
}

void PdfCollectionItem::RemoveField(const string_view& fieldName)
{
    GetDictionary().RemoveKey(PdfName(fieldName));
}

vector<string> PdfCollectionItem::GetFieldNames() const
{
    vector<string> names;
    auto& dict = GetDictionary();

    for (auto& pair : dict)
    {
        // Skip the /Type key
        if (pair.first.GetString() != "Type")
            names.push_back(string(pair.first.GetString()));
    }

    return names;
}

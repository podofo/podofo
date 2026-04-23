/**
 * SPDX-FileCopyrightText: (C) 2025 David Lilly <david.lilly@ticketmaster.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfCollection.h"
#include "PdfCollectionSchema.h"

#include <podofo/main/PdfDictionary.h>
#include <podofo/main/PdfDocument.h>
#include <podofo/main/PdfObject.h>

using namespace std;
using namespace PoDoFo;

PdfCollection::PdfCollection(PdfDocument& doc)
    : PdfDictionaryElement(doc, "Collection"_n)
{
}

PdfCollection::PdfCollection(PdfObject& obj)
    : PdfDictionaryElement(obj)
{
    initFromObject();
}

PdfCollection::~PdfCollection()
{
}

void PdfCollection::initFromObject()
{
    // Load existing schema if present
    auto schemaObj = GetDictionary().FindKey("Schema");
    if (schemaObj != nullptr)
        m_Schema.reset(new PdfCollectionSchema(*schemaObj));
}

PdfCollectionSchema& PdfCollection::GetOrCreateSchema()
{
    if (m_Schema != nullptr)
        return *m_Schema.get();

    // Create new schema
    m_Schema.reset(new PdfCollectionSchema(GetDocument()));
    GetDictionary().AddKey("Schema"_n, m_Schema->GetObject().GetIndirectReference());
    return *m_Schema.get();
}

nullable<PdfCollectionSchema&> PdfCollection::GetSchema()
{
    if (m_Schema == nullptr)
        return nullptr;

    return *m_Schema.get();
}

nullable<const PdfCollectionSchema&> PdfCollection::GetSchema() const
{
    if (m_Schema == nullptr)
        return nullptr;

    return *m_Schema.get();
}

void PdfCollection::SetInitialDocument(nullable<const PdfString&> filename)
{
    auto& dict = GetDictionary();
    if (filename == nullptr)
    {
        dict.RemoveKey("D");
    }
    else
    {
        dict.AddKey("D"_n, *filename);
    }
}

nullable<const PdfString&> PdfCollection::GetInitialDocument() const
{
    auto docObj = GetDictionary().FindKey("D");
    if (docObj == nullptr)
        return nullptr;

    return docObj->GetString();
}

void PdfCollection::SetViewMode(PdfCollectionViewMode mode)
{
    GetDictionary().AddKey("View"_n, getViewModeName(mode));
}

PdfCollectionViewMode PdfCollection::GetViewMode() const
{
    auto viewObj = GetDictionary().FindKey("View");
    if (viewObj == nullptr)
        return PdfCollectionViewMode::Details; // Default

    return getViewModeFromName(viewObj->GetName());
}

void PdfCollection::SetSort(const string_view& fieldName, bool ascending)
{
    // Create the Sort dictionary
    auto& sortObj = GetDocument().GetObjects().CreateDictionaryObject();
    auto& sortDict = sortObj.GetDictionary();

    // Set /S (field name to sort by)
    sortDict.AddKey("S"_n, PdfName(fieldName));

    // Set /A (ascending flag)
    sortDict.AddKey("A"_n, ascending);

    // Add Sort dictionary to collection
    GetDictionary().AddKey("Sort"_n, sortObj.GetIndirectReference());
}

void PdfCollection::ClearSort()
{
    GetDictionary().RemoveKey("Sort");
}

bool PdfCollection::HasSort() const
{
    return GetDictionary().FindKey("Sort") != nullptr;
}

PdfName PdfCollection::getViewModeName(PdfCollectionViewMode mode)
{
    switch (mode)
    {
        case PdfCollectionViewMode::Details:
            return PdfName("D");
        case PdfCollectionViewMode::Tile:
            return PdfName("T");
        case PdfCollectionViewMode::Hidden:
            return PdfName("H");
        default:
            return PdfName("D");
    }
}

PdfCollectionViewMode PdfCollection::getViewModeFromName(const PdfName& name)
{
    auto nameStr = name.GetString();
    if (nameStr == "T")
        return PdfCollectionViewMode::Tile;
    else if (nameStr == "H")
        return PdfCollectionViewMode::Hidden;
    else
        return PdfCollectionViewMode::Details; // Default
}

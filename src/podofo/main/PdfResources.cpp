/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfResources.h"
#include "PdfDictionary.h"
#include "PdfCanvas.h"
#include "PdfColor.h"
#include "PdfDocument.h"

using namespace std;
using namespace PoDoFo;

static PdfArray getProcSet();
static PdfName getResourceTypeName(PdfResourceType type);
PdfResourceType getResourceType(const string_view name);
static string_view getResourceTypePrefix(PdfResourceType type);

PdfResources::PdfResources(PdfDocument& doc) :
    PdfDictionaryElement(doc, "Resources"_n),
    m_currResourceIds{ } { }

PdfResources::PdfResources(PdfObject& obj) :
    PdfDictionaryElement(obj),
    m_currResourceIds{ } { }

PdfResources::PdfResources(PdfCanvas& canvas) :
    PdfDictionaryElement(canvas.GetElement().GetDictionary().AddKey("Resources"_n, PdfDictionary())),
    m_currResourceIds{ }
{
    GetDictionary().AddKey("ProcSet"_n, getProcSet());
}

bool PdfResources::TryCreateFromObject(PdfObject& obj, unique_ptr<PdfResources>& resources)
{
    if (!obj.IsDictionary())
        return false;

    resources.reset(new PdfResources(obj));
    return true;
}

PdfName PdfResources::AddResource(PdfResourceType type, const PdfObject& obj)
{
    return addResource(type, getResourceTypeName(type), obj);
}

void PdfResources::AddResource(PdfResourceType type, const PdfName& key, const PdfObject& obj)
{
    AddResource(getResourceTypeName(type), key, obj);
}

PdfObject* PdfResources::GetResource(PdfResourceType type, const std::string_view& key)
{
    return GetResource(getResourceTypeName(type), key);
}

const PdfObject* PdfResources::GetResource(PdfResourceType type, const std::string_view& key) const
{
    return GetResource(getResourceTypeName(type), key);
}

PdfDictionaryIndirectIterable PdfResources::GetResourceIterator(PdfResourceType type)
{
    return GetResourceIterator(getResourceTypeName(type));
}

PdfDictionaryConstIndirectIterable PdfResources::GetResourceIterator(PdfResourceType type) const
{
    return GetResourceIterator(getResourceTypeName(type));
}

void PdfResources::RemoveResource(PdfResourceType type, const std::string_view& key)
{
    RemoveResource(getResourceTypeName(type), key);
}

void PdfResources::RemoveResources(PdfResourceType type)
{
    RemoveResources(getResourceTypeName(type));
}

PdfName PdfResources::AddResource(const PdfName& typeName, const PdfObject& obj)
{
    return addResource(getResourceType(typeName), typeName, obj);
}

void PdfResources::AddResource(const PdfName& type, const PdfName& key, const PdfObject& obj)
{
    auto& dict = getOrCreateDictionary(type);
    dict.AddKeyIndirectSafe(key, obj);
}

PdfDictionaryIndirectIterable PdfResources::GetResourceIterator(const string_view& type)
{
    PdfDictionary* dict;
    if (!tryGetDictionary(type, dict))
        return PdfDictionaryIndirectIterable();

    return dict->GetIndirectIterator();
}

PdfDictionaryConstIndirectIterable PdfResources::GetResourceIterator(const string_view& type) const
{
    PdfDictionary* dict;
    if (!tryGetDictionary(type, dict))
        return PdfDictionaryConstIndirectIterable();

    return((const PdfDictionary&)*dict).GetIndirectIterator();
}

void PdfResources::RemoveResource(const string_view& type, const string_view& key)
{
    PdfDictionary* dict;
    if (!tryGetDictionary(type, dict))
        return;

    dict->RemoveKey(key);
}

void PdfResources::RemoveResources(const string_view& type)
{
    GetDictionary().RemoveKey(type);
}

PdfObject* PdfResources::GetResource(const string_view& type, const string_view& key)
{
    return getResource(type, key);
}

const PdfObject* PdfResources::GetResource(const string_view& type, const string_view& key) const
{
    return getResource(type, key);
}

const PdfFont* PdfResources::GetFont(const string_view& name) const
{
    return GetDocument().GetFonts().GetLoadedFont(*this, name);
}

PdfName PdfResources::addResource(PdfResourceType type, const PdfName& typeName, const PdfObject& obj)
{
    auto& dict = getOrCreateDictionary(typeName);
    auto prefix = getResourceTypePrefix(type);
    unsigned currId = m_currResourceIds[(unsigned)type];
    string currName;
    while (true)
    {
        currName.clear();
        currName.append(prefix);
        currName.append(std::to_string(currId));
        if (!dict.HasKey(currName))
            break;

        currId = ++m_currResourceIds[(unsigned)type];
    }

    PdfName ret(currName);
    dict.AddKeyIndirectSafe(ret, obj);
    return ret;
}

PdfObject* PdfResources::getResource(const string_view& type, const string_view& key) const
{
    PdfDictionary* dict;
    auto typeObj = const_cast<PdfResources&>(*this).GetDictionary().FindKey(type);
    if (typeObj == nullptr || !typeObj->TryGetDictionary(dict))
        return nullptr;

    return dict->FindKey(key);
}

bool PdfResources::tryGetDictionary(const string_view& type, PdfDictionary*& dict) const
{
    auto typeObj = const_cast<PdfResources&>(*this).GetDictionary().FindKey(type);
    if (typeObj == nullptr)
    {
        dict = nullptr;
        return false;
    }

    return typeObj->TryGetDictionary(dict);
}

PdfDictionary& PdfResources::getOrCreateDictionary(const PdfName& type)
{
    PdfDictionary* dict;
    if (!tryGetDictionary(type, dict))
        dict = &GetDictionary().AddKey(type, PdfDictionary()).GetDictionary();

    return *dict;
}

PdfResourceOperations::PdfResourceOperations() { }

PdfArray getProcSet()
{
    PdfArray procset;
    procset.Add("PDF"_n);
    procset.Add("Text"_n);
    procset.Add("ImageB"_n);
    procset.Add("ImageC"_n);
    procset.Add("ImageI"_n);
    return procset;
}

PdfName getResourceTypeName(PdfResourceType type)
{
    switch (type)
    {
        case PdfResourceType::ExtGState:
            return "ExtGState"_n;
        case PdfResourceType::ColorSpace:
            return "ColorSpace"_n;
        case PdfResourceType::Pattern:
            return "Pattern"_n;
        case PdfResourceType::Shading:
            return "Shading"_n;
        case PdfResourceType::XObject:
            return "XObject"_n;
        case PdfResourceType::Font:
            return "Font"_n;
        case PdfResourceType::Properties:
            return "Properties"_n;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

PdfResourceType getResourceType(const string_view name)
{
    if (name == "ExtGState")
        return PdfResourceType::ExtGState;
    else if (name == "ColorSpace")
        return PdfResourceType::ColorSpace;
    else if (name == "Pattern")
        return PdfResourceType::Pattern;
    else if (name == "Shading")
        return PdfResourceType::Shading;
    else if (name == "XObject")
        return PdfResourceType::XObject;
    else if (name == "Font")
        return PdfResourceType::Font;
    else if (name == "Properties")
        return PdfResourceType::Properties;
    else
        return PdfResourceType::Unknown;
}

string_view getResourceTypePrefix(PdfResourceType type)
{
    switch (type)
    {
        case PdfResourceType::ExtGState:
            return "ExtG"sv;
        case PdfResourceType::ColorSpace:
            return "CS"sv;
        case PdfResourceType::Pattern:
            return "Ptrn"sv;
        case PdfResourceType::Shading:
            return "Shd"sv;
        case PdfResourceType::XObject:
            return "XOb"sv;
        case PdfResourceType::Font:
            return "Ft"sv;
        case PdfResourceType::Properties:
            return "Prop"sv;
        default:
            return "Res"sv;
    }
}


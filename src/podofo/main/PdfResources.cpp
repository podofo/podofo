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

using namespace std;
using namespace PoDoFo;

PdfResources::PdfResources(PdfObject& obj)
    : PdfDictionaryElement(obj) { }

PdfResources::PdfResources(PdfDictionary& dict)
    : PdfDictionaryElement(dict.AddKey("Resources", PdfDictionary()))
{
    GetDictionary().AddKey("ProcSet", PdfCanvas::GetProcSet());
}

void PdfResources::AddResource(const PdfName& type, const PdfName& key, const PdfObject& obj)
{
    auto& dict = getOrCreateDictionary(type);
    dict.AddKeyIndirectSafe(key, obj);
}

void PdfResources::AddResource(const PdfName& type, const PdfName& key, const PdfObject* obj)
{
    auto& dict = getOrCreateDictionary(type);
    if (obj == nullptr)
        dict.RemoveKey(key);
    else
        dict.AddKeyIndirect(key, *obj);
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

void PdfResources::AddColorResource(const PdfColor& color)
{
    switch (color.GetColorSpace())
    {
        case PdfColorSpace::Separation:
        {
            string csPrefix("ColorSpace");
            string csName = color.GetName();
            string temp(csPrefix + csName);

            if (!GetDictionary().HasKey("ColorSpace")
                || !GetDictionary().MustFindKey("ColorSpace").GetDictionary().HasKey(csPrefix + csName))
            {
                // Build color-spaces for separation
                auto csp = color.BuildColorSpace(GetDocument());
                AddResource("ColorSpace", csPrefix + csName, csp);
            }
        }
        break;

        case PdfColorSpace::Lab:
        {
            if (!GetDictionary().HasKey("ColorSpace")
                || !GetDictionary().MustFindKey("ColorSpace").GetDictionary().HasKey("ColorSpaceLab"))
            {
                // Build color-spaces for CIE-lab
                auto csp = color.BuildColorSpace(GetDocument());
                AddResource("ColorSpace", "ColorSpaceCieLab", csp);
            }
        }
        break;

        case PdfColorSpace::DeviceGray:
        case PdfColorSpace::DeviceRGB:
        case PdfColorSpace::DeviceCMYK:
        case PdfColorSpace::Indexed:
            // No colorspace needed
        case PdfColorSpace::Unknown:
        default:
            break;
    }
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

PdfDictionary& PdfResources::getOrCreateDictionary(const string_view& type)
{
    PdfDictionary* dict;
    if (!tryGetDictionary(type, dict))
        dict = &GetDictionary().AddKey(type, PdfDictionary()).GetDictionary();

    return *dict;
}

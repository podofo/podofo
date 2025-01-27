/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfCatalog.h"
#include "PdfDictionary.h"
#include "PdfDocument.h"
#include <podofo/auxiliary/StreamDevice.h>

using namespace std;
using namespace PoDoFo;

PdfCatalog::PdfCatalog(PdfObject& obj)
    : PdfDictionaryElement(obj) { }

PdfObject* PdfCatalog::GetMetadataObject()
{
    return GetDictionary().FindKey("Metadata");
}

const PdfObject* PdfCatalog::GetMetadataObject() const
{
    return GetDictionary().FindKey("Metadata");
}

PdfObject& PdfCatalog::GetOrCreateMetadataObject()
{
    auto& dict = GetDictionary();
    auto metadata = dict.FindKey("Metadata");
    if (metadata != nullptr)
        return *metadata;

    metadata = &GetDocument().GetObjects().CreateDictionaryObject("Metadata"_n, "XML"_n);
    dict.AddKeyIndirect("Metadata"_n, *metadata);
    return *metadata;
}

string PdfCatalog::GetMetadataStreamValue() const
{
    string ret;
    auto obj = GetDictionary().FindKey("Metadata");
    if (obj == nullptr)
        return ret;

    auto stream = obj->GetStream();
    if (stream == nullptr)
        return ret;

    StringStreamDevice ouput(ret);
    stream->CopyTo(ouput);
    return ret;
}

void PdfCatalog::SetMetadataStreamValue(const string_view& value)
{
    auto& obj = GetOrCreateMetadataObject();
    auto& stream = obj.GetOrCreateStream();
    stream.SetData(value, true);

    // Invalidate current metadata
    GetDocument().GetMetadata().Invalidate();
}

PdfObject* PdfCatalog::GetStructTreeRootObject()
{
    return GetDictionary().FindKey("StructTreeRoot");
}

const PdfObject* PdfCatalog::GetStructTreeRootObject() const
{
    return GetDictionary().FindKey("StructTreeRoot");
}

const PdfObject* PdfCatalog::GetMarkInfoObject() const
{
    return GetDictionary().FindKey("MarkInfo");
}

PdfObject* PdfCatalog::GetMarkInfoObject()
{
    return GetDictionary().FindKey("MarkInfo");
}

PdfObject* PdfCatalog::GetLangObject()
{
    return GetDictionary().FindKey("Lang");
}

const PdfObject* PdfCatalog::GetLangObject() const
{
    return GetDictionary().FindKey("Lang");
}

PdfPageMode PdfCatalog::GetPageMode() const
{
    // PageMode is optional; the default value is UseNone
    PdfPageMode thePageMode = PdfPageMode::UseNone;

    auto pageModeObj = GetDictionary().FindKey("PageMode");
    if (pageModeObj != nullptr)
    {
        PdfName pmName = pageModeObj->GetName();
        if (pmName == "UseNone")
            thePageMode = PdfPageMode::UseNone;
        else if (pmName == "UseThumbs")
            thePageMode = PdfPageMode::UseThumbs;
        else if (pmName == "UseOutlines")
            thePageMode = PdfPageMode::UseOutlines;
        else if (pmName == "FullScreen")
            thePageMode = PdfPageMode::FullScreen;
        else if (pmName == "UseOC")
            thePageMode = PdfPageMode::UseOC;
        else if (pmName == "UseAttachments")
            thePageMode = PdfPageMode::UseAttachments;
        else
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidName);
    }

    return thePageMode;
}

void PdfCatalog::SetPageMode(nullable<PdfPageMode> mode)
{
    if (mode == nullptr)
    {
        GetDictionary().RemoveKey("PageMode");
        return;
    }

    switch (*mode)
    {
        case PdfPageMode::UseNone:
            GetDictionary().AddKey("PageMode"_n, "UseNone"_n);
            break;

        case PdfPageMode::UseThumbs:
            GetDictionary().AddKey("PageMode"_n, "UseThumbs"_n);
            break;

        case PdfPageMode::UseOutlines:
            GetDictionary().AddKey("PageMode"_n, "UseOutlines"_n);
            break;

        case PdfPageMode::FullScreen:
            GetDictionary().AddKey("PageMode"_n, "FullScreen"_n);
            break;

        case PdfPageMode::UseOC:
            GetDictionary().AddKey("PageMode"_n, "UseOC"_n);
            break;

        case PdfPageMode::UseAttachments:
            GetDictionary().AddKey("PageMode"_n, "UseAttachments"_n);
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

void PdfCatalog::SetUseFullScreen()
{
    // if current mode is anything but "don't care", we need to move that to non-full-screen
    setViewerPreference("NonFullScreenPageMode"_n, PdfObject(GetDictionary().MustFindKey("PageMode")));

    SetPageMode(PdfPageMode::FullScreen);
}

void PdfCatalog::setViewerPreference(const PdfName& whichPref, const PdfObject& valueObj)
{
    PdfObject* prefsObj = GetDictionary().FindKey("ViewerPreferences");
    if (prefsObj == nullptr)
    {
        // make me a new one and add it
        PdfDictionary vpDict;
        vpDict.AddKey(whichPref, valueObj);
        GetDictionary().AddKey("ViewerPreferences"_n, std::move(vpDict));
    }
    else
    {
        // modify the existing one
        prefsObj->GetDictionary().AddKey(whichPref, valueObj);
    }
}

void PdfCatalog::setViewerPreference(const PdfName& whichPref, bool inValue)
{
    setViewerPreference(whichPref, PdfObject(inValue));
}

void PdfCatalog::SetHideToolbar()
{
    setViewerPreference("HideToolbar"_n, true);
}

void PdfCatalog::SetHideMenubar()
{
    setViewerPreference("HideMenubar"_n, true);
}

void PdfCatalog::SetHideWindowUI()
{
    setViewerPreference("HideWindowUI"_n, true);
}

void PdfCatalog::SetFitWindow()
{
    setViewerPreference("FitWindow"_n, true);
}

void PdfCatalog::SetCenterWindow()
{
    setViewerPreference("CenterWindow"_n, true);
}

void PdfCatalog::SetDisplayDocTitle()
{
    setViewerPreference("DisplayDocTitle"_n, true);
}

void PdfCatalog::SetPrintScaling(const PdfName& scalingType)
{
    setViewerPreference("PrintScaling"_n, scalingType);
}

void PdfCatalog::SetBaseURI(const string_view& inBaseURI)
{
    PdfDictionary uriDict;
    uriDict.AddKey("Base"_n, PdfString(inBaseURI));
    GetDictionary().AddKey("URI"_n, PdfDictionary(uriDict));
}

void PdfCatalog::SetLanguage(const string_view& language)
{
    GetDictionary().AddKey("Lang"_n, PdfString(language));
}

void PdfCatalog::SetBindingDirection(const PdfName& direction)
{
    setViewerPreference("Direction"_n, direction);
}

void PdfCatalog::SetPageLayout(nullable<PdfPageLayout> layout)
{
    if (layout == nullptr)
    {
        GetDictionary().RemoveKey("PageLayout");
        return;
    }

    switch (*layout)
    {
        case PdfPageLayout::SinglePage:
            GetDictionary().AddKey("PageLayout"_n, "SinglePage"_n);
            break;
        case PdfPageLayout::OneColumn:
            GetDictionary().AddKey("PageLayout"_n, "OneColumn"_n);
            break;
        case PdfPageLayout::TwoColumnLeft:
            GetDictionary().AddKey("PageLayout"_n, "TwoColumnLeft"_n);
            break;
        case PdfPageLayout::TwoColumnRight:
            GetDictionary().AddKey("PageLayout"_n, "TwoColumnRight"_n);
            break;
        case PdfPageLayout::TwoPageLeft:
            GetDictionary().AddKey("PageLayout"_n, "TwoPageLeft"_n);
            break;
        case PdfPageLayout::TwoPageRight:
            GetDictionary().AddKey("PageLayout"_n, "TwoPageRight"_n);
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

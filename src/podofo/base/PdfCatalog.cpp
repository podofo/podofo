/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfCatalog.h"
#include "PdfDictionary.h"
#include "PdfDocument.h"
#include "PdfStreamDevice.h"

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

    metadata = &GetDocument().GetObjects().CreateDictionaryObject("Metadata", "XML");
    dict.AddKeyIndirect("Metadata", *metadata);
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

    // We are writing raw clear text, which is required in most
    // relevant scenarions (eg. PDF/A). Remove any possibly
    // existing filter
    obj.GetDictionary().RemoveKey(PdfName::KeyFilter);

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
            thePageMode = PdfPageMode::UseBookmarks;
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

void PdfCatalog::SetPageMode(PdfPageMode inMode)
{
    switch (inMode) {
        default:
        case PdfPageMode::DontCare:
            // this value means leave it alone!
            break;

        case PdfPageMode::UseNone:
            GetDictionary().AddKey("PageMode", PdfName("UseNone"));
            break;

        case PdfPageMode::UseThumbs:
            GetDictionary().AddKey("PageMode", PdfName("UseThumbs"));
            break;

        case PdfPageMode::UseBookmarks:
            GetDictionary().AddKey("PageMode", PdfName("UseOutlines"));
            break;

        case PdfPageMode::FullScreen:
            GetDictionary().AddKey("PageMode", PdfName("FullScreen"));
            break;

        case PdfPageMode::UseOC:
            GetDictionary().AddKey("PageMode", PdfName("UseOC"));
            break;

        case PdfPageMode::UseAttachments:
            GetDictionary().AddKey("PageMode", PdfName("UseAttachments"));
            break;
    }
}

void PdfCatalog::SetUseFullScreen()
{
    // first, we get the current mode
    PdfPageMode	curMode = GetPageMode();

    // if current mode is anything but "don't care", we need to move that to non-full-screen
    if (curMode != PdfPageMode::DontCare)
        setViewerPreference("NonFullScreenPageMode", PdfObject(GetDictionary().MustFindKey("PageMode")));

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
        GetDictionary().AddKey("ViewerPreferences", std::move(vpDict));
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
    setViewerPreference("HideToolbar", true);
}

void PdfCatalog::SetHideMenubar()
{
    setViewerPreference("HideMenubar", true);
}

void PdfCatalog::SetHideWindowUI()
{
    setViewerPreference("HideWindowUI", true);
}

void PdfCatalog::SetFitWindow()
{
    setViewerPreference("FitWindow", true);
}

void PdfCatalog::SetCenterWindow()
{
    setViewerPreference("CenterWindow", true);
}

void PdfCatalog::SetDisplayDocTitle()
{
    setViewerPreference("DisplayDocTitle", true);
}

void PdfCatalog::SetPrintScaling(const PdfName& scalingType)
{
    setViewerPreference("PrintScaling", scalingType);
}

void PdfCatalog::SetBaseURI(const string_view& inBaseURI)
{
    PdfDictionary uriDict;
    uriDict.AddKey("Base", PdfString(inBaseURI));
    GetDictionary().AddKey("URI", PdfDictionary(uriDict));
}

void PdfCatalog::SetLanguage(const string_view& language)
{
    GetDictionary().AddKey("Lang", PdfString(language));
}

void PdfCatalog::SetBindingDirection(const PdfName& direction)
{
    setViewerPreference("Direction", direction);
}

void PdfCatalog::SetPageLayout(PdfPageLayout layout)
{
    switch (layout)
    {
        default:
        case PdfPageLayout::Ignore:
            break;	// means do nothing
        case PdfPageLayout::Default:
            GetDictionary().RemoveKey("PageLayout");
            break;
        case PdfPageLayout::SinglePage:
            GetDictionary().AddKey("PageLayout", PdfName("SinglePage"));
            break;
        case PdfPageLayout::OneColumn:
            GetDictionary().AddKey("PageLayout", PdfName("OneColumn"));
            break;
        case PdfPageLayout::TwoColumnLeft:
            GetDictionary().AddKey("PageLayout", PdfName("TwoColumnLeft"));
            break;
        case PdfPageLayout::TwoColumnRight:
            GetDictionary().AddKey("PageLayout", PdfName("TwoColumnRight"));
            break;
        case PdfPageLayout::TwoPageLeft:
            GetDictionary().AddKey("PageLayout", PdfName("TwoPageLeft"));
            break;
        case PdfPageLayout::TwoPageRight:
            GetDictionary().AddKey("PageLayout", PdfName("TwoPageRight"));
            break;
    }
}

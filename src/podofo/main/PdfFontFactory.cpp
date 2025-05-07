/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfEncodingFactory.h"
#include "PdfFont.h"
#include "PdfFontObject.h"
#include "PdfFontCIDTrueType.h"
#include "PdfFontCIDCFF.h"
#include "PdfFontMetrics.h"
#include "PdfFontMetricsStandard14.h"
#include "PdfFontMetricsObject.h"
#include "PdfFontType1.h"
#include "PdfFontType3.h"
#include "PdfFontTrueType.h"

using namespace std;
using namespace PoDoFo;

unique_ptr<PdfFont> PdfFont::Create(PdfDocument& doc, PdfFontMetricsConstPtr&& metrics,
    const PdfFontCreateParams& createParams, bool isProxy)
{
    bool embeddingEnabled = (createParams.Flags & PdfFontCreateFlags::DontEmbed) == PdfFontCreateFlags::None;
    bool subsettingEnabled = (createParams.Flags & PdfFontCreateFlags::DontSubset) == PdfFontCreateFlags::None;
    bool preferNonCid = (createParams.Flags & PdfFontCreateFlags::PreferNonCID) != PdfFontCreateFlags::None;

    auto font = createFontForType(doc, std::move(metrics), createParams.Encoding, preferNonCid);
    if (font != nullptr)
        font->InitImported(embeddingEnabled, subsettingEnabled, isProxy);

    return font;
}

unique_ptr<PdfFont> PdfFont::createFontForType(PdfDocument& doc, PdfFontMetricsConstPtr&& metrics,
    const PdfEncoding& encoding, bool preferNonCID)
{
    PdfFont* font = nullptr;
    switch (metrics->GetFontFileType())
    {
        case PdfFontFileType::TrueType:
            if (preferNonCID && !encoding.HasCIDMapping())
                font = new PdfFontTrueType(doc, std::move(metrics), encoding);
            else
                font = new PdfFontCIDTrueType(doc, std::move(metrics), encoding);
            break;
        case PdfFontFileType::Type1:
            font = new PdfFontType1(doc, std::move(metrics), encoding);
            break;
        case PdfFontFileType::Type1CFF:
        case PdfFontFileType::CIDKeyedCFF:
        case PdfFontFileType::OpenTypeCFF:
            font = new PdfFontCIDCFF(doc, std::move(metrics), encoding);
            break;
        case PdfFontFileType::Type3:
            font = new PdfFontType3(doc, std::move(metrics), encoding);
            break;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFontFormat, "Unsupported font at this context");
    }

    return unique_ptr<PdfFont>(font);
}

bool PdfFont::TryCreateFromObject(const PdfObject& obj, unique_ptr<const PdfFont>& font)
{
    return TryCreateFromObject(const_cast<PdfObject&>(obj), reinterpret_cast<unique_ptr<PdfFont>&>(font));
}

bool PdfFont::TryCreateFromObject(PdfObject& obj, unique_ptr<PdfFont>& font)
{
    const PdfName* name;
    PdfDictionary* dict;
    if (!obj.TryGetDictionary(dict)
        || !dict->TryFindKeyAs("Type", name)
        || *name != "Font")
    {
    Fail:
        font.reset();
        return false;
    }

    if (!dict->TryFindKeyAs("Subtype", name))
    {
        PoDoFo::LogMessage(PdfLogSeverity::Warning, "Font: No SubType");
        goto Fail;
    }

    PdfFontMetricsConstPtr metrics;
    PdfObject* descendantObj = nullptr;
    if (*name == "Type0")
    {
        // TABLE 5.18 Entries in a Type 0 font dictionary

        // The PDF reference states that DescendantFonts must be an array,
        // some applications (e.g. MS Word) put the array into an indirect object though.
        PdfArray* arr;
        if (!dict->TryFindKeyAs("DescendantFonts", arr))
        {
            PoDoFo::LogMessage(PdfLogSeverity::Warning, "Type0 Font : No DescendantFonts");
            goto Fail;
        }

        const PdfDictionary* descriptorObj = nullptr;
        if (arr->size() != 0)
        {
            descendantObj = &arr->MustFindAt(0);
            descriptorObj = descendantObj->GetDictionary().FindKeyAsSafe<const PdfDictionary*>("FontDescriptor");
        }

        if (descendantObj != nullptr)
            metrics = PdfFontMetricsObject::Create(*descendantObj, descriptorObj);
    }
    else if (*name == "Type1")
    {
        auto descriptorObj = dict->FindKeyAsSafe<const PdfDictionary*>("FontDescriptor");

        // Handle missing FontDescriptor for the 14 standard fonts
        if (descriptorObj == nullptr)
        {
            // Check if it's a PdfFontStandard14
            auto baseFont = dict->FindKey("BaseFont");
            PdfStandard14FontType stdFontType;
            if (baseFont == nullptr
                || !PdfFont::IsStandard14Font(baseFont->GetName().GetString(), stdFontType))
            {
                PoDoFo::LogMessage(PdfLogSeverity::Warning, "No known /BaseFont found");
                goto Fail;
            }

            metrics = PdfFontMetricsStandard14::Create(stdFontType, obj);
        }
        else
        {
            metrics = PdfFontMetricsObject::Create(obj, descriptorObj);
        }
    }
    else if (*name == "Type3")
    {
        metrics = PdfFontMetricsObject::Create(obj, dict->FindKeyAsSafe<const PdfDictionary*>("FontDescriptor"));
    }
    else if (*name == "TrueType")
    {
        metrics = PdfFontMetricsObject::Create(obj, dict->FindKeyAsSafe<const PdfDictionary*>("FontDescriptor"));
    }

    if (metrics == nullptr)
    {
        PoDoFo::LogMessage(PdfLogSeverity::Warning, "Missing font metrics");
        goto Fail;
    }

    auto encoding = PdfEncodingFactory::CreateEncoding(*dict, *metrics, descendantObj);
    if (encoding.IsNull())
    {
        PoDoFo::LogMessage(PdfLogSeverity::Warning, "Missing font encoding");
        goto Fail;
    }

    font = PdfFontObject::Create(obj, std::move(metrics), encoding);
    return true;
}

unique_ptr<PdfFont> PdfFont::CreateStandard14(PdfDocument& doc, PdfStandard14FontType std14Font,
    const PdfFontCreateParams& createParams)
{
    bool embeddingEnabled = (createParams.Flags & PdfFontCreateFlags::DontEmbed) == PdfFontCreateFlags::None;
    bool subsettingEnabled = (createParams.Flags & PdfFontCreateFlags::DontSubset) == PdfFontCreateFlags::None;
    bool preferNonCid;
    if (embeddingEnabled)
    {
        preferNonCid = (createParams.Flags & PdfFontCreateFlags::PreferNonCID) != PdfFontCreateFlags::None;
    }
    else
    {
        // Standard14 fonts when not embedding should be non CID
        preferNonCid = true;
    }

    PdfFontMetricsConstPtr metrics = PdfFontMetricsStandard14::Create(std14Font);
    unique_ptr<PdfFont> font;
    if (preferNonCid && !createParams.Encoding.HasCIDMapping())
        font.reset(new PdfFontType1(doc, std::move(metrics), createParams.Encoding));
    else
        font.reset(new PdfFontCIDCFF(doc, std::move(metrics), createParams.Encoding));

    if (font != nullptr)
        font->InitImported(embeddingEnabled, subsettingEnabled, false);

    return font;
}

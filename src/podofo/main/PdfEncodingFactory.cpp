/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfEncodingFactory.h"

#include <podofo/private/PdfEncodingPrivate.h>

#include "PdfObject.h"
#include "PdfDictionary.h"
#include "PdfEncodingMapFactory.h"
#include "PdfIdentityEncoding.h"
#include "PdfDifferenceEncoding.h"
#include "PdfCMapEncoding.h"
#include "PdfFontMetrics.h"
#include "PdfEncodingMapFactory.h"
#include "PdfPredefinedToUnicodeCMap.h"

using namespace std;
using namespace PoDoFo;

PdfEncoding PdfEncodingFactory::CreateEncoding(const PdfObject& fontObj, const PdfFontMetrics& metrics)
{
    // The /Encoding entry can be a predefined encoding, a CMap
    PdfEncodingMapConstPtr encoding;

    auto encodingObj = fontObj.GetDictionary().FindKey("Encoding");
    if (encodingObj != nullptr)
        encoding = createEncodingMap(*encodingObj, metrics);

    PdfEncodingMapConstPtr implicitEncoding;
    if (encoding == nullptr && metrics.TryGetImplicitEncoding(implicitEncoding))
        encoding = implicitEncoding;

    // TODO: Implement full text extraction, including search in predefined
    // CMap(s) as described in Pdf Reference and here https://stackoverflow.com/a/26910569/213871

    // The /ToUnicode CMap is the main entry to search
    // for text extraction
    PdfEncodingMapConstPtr toUnicode;
    auto toUnicodeObj = fontObj.GetDictionary().FindKey("ToUnicode");
    if (toUnicodeObj != nullptr)
        toUnicode = createEncodingMap(*toUnicodeObj, metrics);

    if (encoding == nullptr)
    {
        if (toUnicode == nullptr)
        {
            // We don't have enough info to create an encoding and
            // we don't know how to read an built-in font encoding
            return PdfEncoding();
        }
        else
        {
            // As a fallback, create an identity encoding of the size size of the /ToUnicode mapping
            encoding = std::make_shared<PdfIdentityEncoding>(toUnicode->GetLimits().MaxCodeSize);
        }
    }
    else
    {
        if (toUnicode == nullptr && encoding->GetPredefinedEncodingType() == PdfPredefinedEncodingType::PredefinedCMap)
        {
            auto predefinedCIDMap = std::dynamic_pointer_cast<const PdfCMapEncoding>(encoding);
            // ISO 32000-2:2020 "9.10.2 Mapping character codes to Unicode values"
            // "c. Construct a second CMap name by concatenating the registry and ordering obtained in step (b)
            // in the format registry–ordering–UCS2(for example, Adobe–Japan1–UCS2)"
            string toUnicodeMapName = (string)predefinedCIDMap->GetCIDSystemInfo().Registry.GetString();
            toUnicodeMapName.push_back('-');
            toUnicodeMapName.append(predefinedCIDMap->GetCIDSystemInfo().Ordering.GetString());
            toUnicodeMapName.append("-UCS2");
            auto toUnicodeMap = PdfEncodingMapFactory::GetPredefinedCMap(toUnicodeMapName);
            if (toUnicodeMap == nullptr)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "A ToUnicode map with name {} was not found", toUnicodeMapName);

            toUnicode = shared_ptr<PdfPredefinedToUnicodeCMap>(new PdfPredefinedToUnicodeCMap(
                std::move(toUnicodeMap), std::move(predefinedCIDMap)));
        }
    }

    PdfEncodingLimits parsedLimits;
    auto firstCharObj = fontObj.GetDictionary().FindKey("FirstChar");
    if (firstCharObj != nullptr)
        parsedLimits.FirstChar = PdfCharCode(static_cast<unsigned>(firstCharObj->GetNumber()));

    auto lastCharObj = fontObj.GetDictionary().FindKey("LastChar");
    if (lastCharObj != nullptr)
        parsedLimits.LastChar = PdfCharCode(static_cast<unsigned>(lastCharObj->GetNumber()));

    if (parsedLimits.LastChar.Code > parsedLimits.FirstChar.Code)
    {
        // If found valid /FirstChar and /LastChar, valorize
        //  also the code size limits
        parsedLimits.MinCodeSize = utls::GetCharCodeSize(parsedLimits.FirstChar.Code);
        parsedLimits.MaxCodeSize = utls::GetCharCodeSize(parsedLimits.LastChar.Code);
    }

    return PdfEncoding::Create(parsedLimits, encoding, toUnicode);
}

PdfEncodingMapConstPtr PdfEncodingFactory::createEncodingMap(const PdfObject& obj,
    const PdfFontMetrics& metrics)
{
    const PdfName* name;
    const PdfDictionary* dict;
    if (obj.TryGetName(name))
    {
        if (*name == "WinAnsiEncoding")
            return PdfEncodingMapFactory::WinAnsiEncodingInstance();
        else if (*name == "MacRomanEncoding")
            return PdfEncodingMapFactory::MacRomanEncodingInstance();
        else if (*name == "MacExpertEncoding")
            return PdfEncodingMapFactory::MacExpertEncodingInstance();

        // TABLE 5.15 Predefined CJK CMap names: the generip H-V identifies
        // are mappings for 2-byte CID. "It maps 2-byte character codes ranging
        // from 0 to 65,535 to the same 2 - byte CID value, interpreted high
        // order byte first"
        else if (*name == "Identity-H")
            return PdfEncodingMapFactory::TwoBytesHorizontalIdentityEncodingInstance();
        else if (*name == "Identity-V")
            return PdfEncodingMapFactory::TwoBytesVerticalIdentityEncodingInstance();
        else
            return PdfEncodingMapFactory::GetPredefinedCMap(*name);
    }
    else if (obj.TryGetDictionary(dict))
    {
        if (dict->TryFindKeyAs("CMapName", name))
        {
            if (*name == "Identity-H")
                return PdfEncodingMapFactory::TwoBytesHorizontalIdentityEncodingInstance();

            if (*name == "Identity-V")
                return PdfEncodingMapFactory::TwoBytesVerticalIdentityEncodingInstance();
        }

        unique_ptr<PdfEncodingMap> cmapEnc;
        if (PdfEncodingMapFactory::TryParseCMapEncoding(obj, cmapEnc))
            return cmapEnc;

        unique_ptr<PdfDifferenceEncoding> diffEnc;
        if (PdfDifferenceEncoding::TryCreateFromObject(obj, metrics, diffEnc))
            return diffEnc;
    }

    return nullptr;
}

PdfEncoding PdfEncodingFactory::CreateWinAnsiEncoding()
{
    return PdfEncoding(WinAnsiEncodingId, PdfEncodingMapFactory::WinAnsiEncodingInstance());
}

PdfEncoding PdfEncodingFactory::CreateMacRomanEncoding()
{
    return PdfEncoding(MacRomanEncodingId, PdfEncodingMapFactory::MacRomanEncodingInstance());
}

PdfEncoding PdfEncodingFactory::CreateMacExpertEncoding()
{
    return PdfEncoding(MacExpertEncodingId, PdfEncodingMapFactory::MacExpertEncodingInstance());
}

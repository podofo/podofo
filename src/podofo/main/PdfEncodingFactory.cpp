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
#include "PdfEncodingShim.h"
#include "PdfFontMetrics.h"
#include "PdfEncodingMapFactory.h"

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

    return PdfEncoding(fontObj, encoding, toUnicode);
}

PdfEncodingMapConstPtr PdfEncodingFactory::createEncodingMap(
    const PdfObject& obj, const PdfFontMetrics& metrics)
{
    if (obj.IsName())
    {
        auto& name = obj.GetName();
        if (name == "WinAnsiEncoding")
            return PdfEncodingMapFactory::WinAnsiEncodingInstance();
        else if (name == "MacRomanEncoding")
            return PdfEncodingMapFactory::MacRomanEncodingInstance();
        else if (name == "MacExpertEncoding")
            return PdfEncodingMapFactory::MacExpertEncodingInstance();

        // TABLE 5.15 Predefined CJK CMap names: the generip H-V identifies
        // are mappings for 2-byte CID. "It maps 2-byte character codes ranging
        // from 0 to 65,535 to the same 2 - byte CID value, interpreted high
        // order byte first"
        else if (name == "Identity-H")
            return PdfEncodingMapFactory::TwoBytesHorizontalIdentityEncodingInstance();
        else if (name == "Identity-V")
            return PdfEncodingMapFactory::TwoBytesVerticalIdentityEncodingInstance();
    }
    else if (obj.IsDictionary())
    {
        auto& dict = obj.GetDictionary();
        auto* cmapName = dict.FindKey("CMapName");
        if (cmapName != nullptr)
        {
            if (cmapName->GetName() == "Identity-H")
                return PdfEncodingMapFactory::TwoBytesHorizontalIdentityEncodingInstance();

            if (cmapName->GetName() == "Identity-V")
                return PdfEncodingMapFactory::TwoBytesVerticalIdentityEncodingInstance();
        }

        if (obj.HasStream())
            return PdfCMapEncoding::CreateFromObject(obj);

        // CHECK-ME: should we verify if it's a reference by searching /Differences?
        return PdfDifferenceEncoding::Create(obj, metrics);
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

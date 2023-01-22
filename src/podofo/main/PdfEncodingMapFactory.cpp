/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfEncodingMapFactory.h"
#include "PdfPredefinedEncoding.h"
#include "PdfIdentityEncoding.h"
#include "PdfDifferenceEncoding.h"

using namespace std;
using namespace PoDoFo;

PdfEncodingMapConstPtr PdfEncodingMapFactory::WinAnsiEncodingInstance()
{
    static shared_ptr<PdfWinAnsiEncoding> s_istance(new PdfWinAnsiEncoding());
    return s_istance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::MacRomanEncodingInstance()
{
    static shared_ptr<PdfMacRomanEncoding> s_istance(new PdfMacRomanEncoding());
    return s_istance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::MacExpertEncodingInstance()
{
    static shared_ptr<PdfMacExpertEncoding> s_istance(new PdfMacExpertEncoding());
    return s_istance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::TwoBytesHorizontalIdentityEncodingInstance()
{
    static shared_ptr<PdfIdentityEncoding> s_istance(new PdfIdentityEncoding(PdfIdentityOrientation::Horizontal));
    return s_istance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::TwoBytesVerticalIdentityEncodingInstance()
{
    static shared_ptr<PdfIdentityEncoding> s_istance(new PdfIdentityEncoding(PdfIdentityOrientation::Vertical));
    return s_istance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::GetNullEncodingMap()
{
    static PdfEncodingMapConstPtr s_instance(new PdfNullEncodingMap());
    return s_instance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::StandardEncodingInstance()
{
    static shared_ptr<PdfStandardEncoding> s_istance(new PdfStandardEncoding());
    return s_istance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::SymbolEncodingInstance()
{
    static shared_ptr<PdfSymbolEncoding> s_istance(new PdfSymbolEncoding());
    return s_istance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::ZapfDingbatsEncodingInstance()
{
    static shared_ptr<PdfZapfDingbatsEncoding> s_istance(new PdfZapfDingbatsEncoding());
    return s_istance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::GetStandard14FontEncodingMap(PdfStandard14FontType stdFont)
{
    switch (stdFont)
    {
        case PdfStandard14FontType::TimesRoman:
        case PdfStandard14FontType::TimesItalic:
        case PdfStandard14FontType::TimesBold:
        case PdfStandard14FontType::TimesBoldItalic:
        case PdfStandard14FontType::Helvetica:
        case PdfStandard14FontType::HelveticaOblique:
        case PdfStandard14FontType::HelveticaBold:
        case PdfStandard14FontType::HelveticaBoldOblique:
        case PdfStandard14FontType::Courier:
        case PdfStandard14FontType::CourierOblique:
        case PdfStandard14FontType::CourierBold:
        case PdfStandard14FontType::CourierBoldOblique:
            return StandardEncodingInstance();
        case PdfStandard14FontType::Symbol:
            return SymbolEncodingInstance();
        case PdfStandard14FontType::ZapfDingbats:
            return ZapfDingbatsEncodingInstance();
        case PdfStandard14FontType::Unknown:
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "Invalid Standard14 font type");
    }
}

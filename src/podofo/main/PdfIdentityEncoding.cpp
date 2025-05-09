/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfIdentityEncoding.h"

#include <utf8cpp/utf8.h>

#include <podofo/private/PdfEncodingPrivate.h>

#include "PdfDictionary.h"
#include "PdfFont.h"

using namespace std;
using namespace PoDoFo;

static PdfEncodingLimits getLimits(unsigned char codeSpaceSize);

PdfIdentityEncoding::PdfIdentityEncoding(PdfEncodingMapType type, unsigned char codeSpaceSize)
    : PdfIdentityEncoding(type, getLimits(codeSpaceSize), PdfIdentityOrientation::Unkwnown) { }

PdfIdentityEncoding::PdfIdentityEncoding(PdfIdentityOrientation orientation)
    : PdfIdentityEncoding(PdfEncodingMapType::CMap, getLimits(2), orientation)
{
    if (orientation == PdfIdentityOrientation::Unkwnown)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Unsupported orientation");
}

// PdfIdentityEncoding represents either Identity-H/Identity-V
// predefined CMap names
PdfIdentityEncoding::PdfIdentityEncoding(PdfEncodingMapType type,
    const PdfEncodingLimits& limits, PdfIdentityOrientation orientation) :
    PdfEncodingMap(type),
    m_Limits(limits),
    m_orientation(orientation)
{
}

bool PdfIdentityEncoding::tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const
{
    PODOFO_INVARIANT(m_Limits.MinCodeSize == m_Limits.MaxCodeSize);
    if (utls::GetCharCodeSize(codePoint) > m_Limits.MaxCodeSize)
    {
        codeUnit = { };
        return false;
    }

    codeUnit = { (unsigned)codePoint, m_Limits.MaxCodeSize };
    return true;
}

bool PdfIdentityEncoding::tryGetCodePoints(const PdfCharCode& codeUnit, const unsigned* cidId, CodePointSpan& codePoints) const
{
    (void)cidId;
    codePoints = CodePointSpan((codepoint)codeUnit.Code);
    return true;
}

void PdfIdentityEncoding::getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const
{
    (void)objects;
    (void)obj;

    switch (m_orientation)
    {
        case PdfIdentityOrientation::Horizontal:
            name = "Identity-H"_n;
            break;
        case PdfIdentityOrientation::Vertical:
            name = "Identity-V"_n;
            break;
        default:
            // NOTE: Return no export object, assume exporting
            // will be done by writing CMaps externally
            break;
    }
}

void PdfIdentityEncoding::AppendCIDMappingEntries(OutputStream& stream, const PdfFont& font, charbuff& temp) const
{
    (void)font;

    // Just do a single cidrange
    // CHECK-ME: This has not been verified for correctness

    stream.Write("1 begincidrange\n");
    m_Limits.FirstChar.WriteHexTo(temp);
    stream.Write(temp);
    stream.Write(" ");
    m_Limits.LastChar.WriteHexTo(temp);
    stream.Write(temp);
    stream.Write(" ");
    utls::FormatTo(temp, m_Limits.FirstChar.Code);
    stream.Write(temp);
    stream.Write("\nendcidrange\n");
}

void PdfIdentityEncoding::AppendToUnicodeEntries(OutputStream& stream, charbuff& temp) const
{
    // Just do a single bfrange
    // Use PdfEncodingMap::AppendUTF16CodeTo

    u16string u16temp;
    stream.Write("1 beginbfrange\n");
    m_Limits.FirstChar.WriteHexTo(temp);
    stream.Write(temp);
    stream.Write(" ");
    m_Limits.LastChar.WriteHexTo(temp);
    stream.Write(temp);
    stream.Write(" ");
    PoDoFo::AppendUTF16CodeTo(stream, m_Limits.FirstChar.Code, u16temp);
    stream.Write("\nendbfrange\n");
}

const PdfEncodingLimits& PdfIdentityEncoding::GetLimits() const
{
    return m_Limits;
}

PdfPredefinedEncodingType PdfIdentityEncoding::GetPredefinedEncodingType() const
{
    switch (m_orientation)
    {
        case PdfIdentityOrientation::Horizontal:
        case PdfIdentityOrientation::Vertical:
            return PdfPredefinedEncodingType::IdentityCMap;
        default:
            return PdfPredefinedEncodingType::Indeterminate;
    }
}

PdfEncodingLimits getLimits(unsigned char codeSpaceSize)
{
    if (codeSpaceSize == 0 || codeSpaceSize > 4)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Code space size can't be zero or bigger than 4");

    return { codeSpaceSize, codeSpaceSize, PdfCharCode(0, codeSpaceSize),
        PdfCharCode((unsigned)std::pow(2, codeSpaceSize * CHAR_BIT) - 1, codeSpaceSize) };
}

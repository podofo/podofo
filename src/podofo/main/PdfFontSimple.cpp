/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontSimple.h"

#include "PdfDocument.h"
#include "PdfDifferenceEncoding.h"

using namespace std;
using namespace PoDoFo;

PdfFontSimple::PdfFontSimple(PdfDocument& doc, PdfFontType type,
        PdfFontMetricsConstPtr&& metrics, const PdfEncoding& encoding)
    : PdfFont(doc, type, std::move(metrics), encoding), m_Descriptor(nullptr)
{
}

void PdfFontSimple::getWidthsArray(PdfArray& arr) const
{
    unsigned gid;
    auto& matrix = m_Metrics->GetMatrix();
    unsigned code = GetEncoding().GetFirstChar().Code;
    unsigned last = GetEncoding().GetLastChar().Code;
    arr.Clear();
    arr.reserve(last - code + 1);
    for (; code <= last; code++)
    {
        (void)GetEncoding().TryGetCIDId(PdfCharCode(code), gid);
        // NOTE: In non CID-keyed fonts char codes are equivalent to CID
        arr.Add(PdfObject(GetCIDWidth(code) / matrix[0]));
    }
}

void PdfFontSimple::initImported()
{
    PdfName subType;
    switch (GetType())
    {
        case PdfFontType::Type1:
            subType = "Type1"_n;
            break;
        case PdfFontType::TrueType:
            subType = "TrueType"_n;
            break;
        case PdfFontType::Type3:
            subType = "Type3"_n;
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    auto& dict = GetDictionary();
    dict.AddKey("Subtype"_n, subType);
    dict.AddKey("BaseFont"_n, PdfName(GetName()));

    m_Encoding->ExportToFont(*this);

    if (!GetMetrics().IsStandard14FontMetrics() || IsEmbeddingEnabled())
    {
        // NOTE: Non Standard14 fonts need at least the metrics
        // descriptor. Instead Standard14 fonts don't need any
        // metrics descriptor if the font is not embedded
        auto& descriptorObj = GetDocument().GetObjects().CreateDictionaryObject("FontDescriptor"_n);
        dict.AddKeyIndirect("FontDescriptor"_n, descriptorObj);
        WriteDescriptors(dict, descriptorObj.GetDictionary());
        m_Descriptor = &descriptorObj;
    }
}

void PdfFontSimple::embedFont()
{
    PODOFO_ASSERT(m_Descriptor != nullptr);
    PdfArray arr;
    this->getWidthsArray(arr);

    auto& widthsObj = GetDocument().GetObjects().CreateObject(std::move(arr));
    GetDictionary().AddKeyIndirect("Widths"_n, widthsObj);

    if (GetType() == PdfFontType::Type3)
        m_Metrics->ExportType3GlyphData(GetDictionary(), { });
    else
        EmbedFontFile(m_Descriptor->GetDictionary());
}

void PdfFontSimple::embedFontSubset()
{
    // NOTE: For now it's supported only for Type 3 fonts
    PODOFO_ASSERT(GetType() == PdfFontType::Type3);

    auto& matrix = m_Metrics->GetMatrix();
    auto diffEncoding = dynamic_cast<const PdfDifferenceEncoding*>(&m_Encoding->GetEncodingMap());
    if (diffEncoding == nullptr)
    {
        m_Metrics->ExportType3GlyphData(GetDictionary(), { });
    }
    else
    {
        auto cidInfos = GetCharGIDInfos();
        unsigned first = GetEncoding().GetFirstChar().Code;
        unsigned last = GetEncoding().GetLastChar().Code;

        vector<string_view> glyphs;
        vector<double> widths(last - first + 1);
        const PdfName* name;
        for (unsigned i = 0; i < cidInfos.size(); i++)
        {
            auto& cidInfo = cidInfos[i];
            if (!diffEncoding->GetDifferences().TryGetMappedName((unsigned char)cidInfo.OrigCid, name))
                continue;

            // Check for overflows before insertion. Missing widths are already handled
            // by default value (/MissingWidth in the descriptor)
            if (cidInfo.OrigCid - first < widths.size())
                widths[cidInfo.OrigCid - first] = m_Metrics->GetGlyphWidth(cidInfo.Gid.MetricsId) / matrix[0];

            glyphs.push_back(name->GetString());
        }

        m_Metrics->ExportType3GlyphData(GetDictionary(), glyphs);

        PdfArray arr;
        arr.reserve(widths.size());
        for (unsigned i = 0; i < widths.size(); i++)
            arr.Add(PdfObject(widths[i]));

        auto& widthsObj = GetDocument().GetObjects().CreateObject(std::move(arr));
        GetDictionary().AddKeyIndirect("Widths"_n, widthsObj);
    }
}

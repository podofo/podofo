/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontSimple.h"

#include "PdfDocument.h"

using namespace std;
using namespace PoDoFo;

PdfFontSimple::PdfFontSimple(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
    const PdfEncoding& encoding)
    : PdfFont(doc, metrics, encoding), m_Descriptor(nullptr)
{
}

void PdfFontSimple::getWidthsArray(PdfArray& arr) const
{
    vector<double> widths;
    unsigned gid;
    for (unsigned code = GetEncoding().GetFirstChar().Code, last = GetEncoding().GetLastChar().Code;
        code <= last; code++)
    {
        (void)GetEncoding().TryGetCIDId(PdfCharCode(code), gid);
        // NOTE: In non CID-keyed fonts char codes are equivalent to CID
        widths.push_back(GetCIDLengthRaw(code));
    }

    arr.Clear();
    arr.reserve(widths.size());

    auto matrix = m_Metrics->GetMatrix();
    for (unsigned i = 0; i < widths.size(); i++)
        arr.Add(PdfObject(static_cast<int64_t>(std::round(widths[i] / matrix[0]))));
}

void PdfFontSimple::getFontMatrixArray(PdfArray& fontMatrix) const
{
    fontMatrix.Clear();
    fontMatrix.Reserve(6);

    auto matrix = m_Metrics->GetMatrix();
    for (unsigned i = 0; i < 6; i++)
        fontMatrix.Add(PdfObject(matrix[i]));
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

    this->GetDictionary().AddKey("Subtype"_n, subType);
    this->GetDictionary().AddKey("BaseFont"_n, PdfName(GetName()));
    m_Encoding->ExportToFont(*this);

    if (!GetMetrics().IsStandard14FontMetrics() || IsEmbeddingEnabled())
    {
        // NOTE: Non Standard14 fonts need at least the metrics
        // descriptor. Instead Standard14 fonts don't need any
        // metrics descriptor if the font is not embedded
        auto& descriptorObj = GetDocument().GetObjects().CreateDictionaryObject("FontDescriptor"_n);
        this->GetDictionary().AddKeyIndirect("FontDescriptor"_n, descriptorObj);
        FillDescriptor(descriptorObj.GetDictionary());
        m_Descriptor = &descriptorObj;
    }
}

void PdfFontSimple::embedFont()
{
    PODOFO_ASSERT(m_Descriptor != nullptr);
    this->GetDictionary().AddKey("FirstChar"_n, PdfVariant(static_cast<int64_t>(m_Encoding->GetFirstChar().Code)));
    this->GetDictionary().AddKey("LastChar"_n, PdfVariant(static_cast<int64_t>(m_Encoding->GetLastChar().Code)));

    PdfArray arr;
    this->getWidthsArray(arr);

    auto& widthsObj = GetDocument().GetObjects().CreateObject(std::move(arr));
    this->GetDictionary().AddKeyIndirect("Widths"_n, widthsObj);

    if (GetType() == PdfFontType::Type3)
    {
        getFontMatrixArray(arr);
        GetDictionary().AddKey("FontMatrix"_n, std::move(arr));

        GetBoundingBox(arr);
        GetDictionary().AddKey("FontBBox"_n, std::move(arr));
    }

    EmbedFontFile(*m_Descriptor);
}

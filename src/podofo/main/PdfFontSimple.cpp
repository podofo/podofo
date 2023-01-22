/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontSimple.h"

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfFilter.h"
#include "PdfName.h"
#include "PdfObjectStream.h"

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

void PdfFontSimple::Init()
{
    PdfName subType;
    switch (GetType())
    {
        case PdfFontType::Type1:
            subType = PdfName("Type1");
            break;
        case PdfFontType::TrueType:
            subType = PdfName("TrueType");
            break;
        case PdfFontType::Type3:
            subType = PdfName("Type3");
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    this->GetObject().GetDictionary().AddKey(PdfName::KeySubtype, PdfName(subType));
    this->GetObject().GetDictionary().AddKey("BaseFont", PdfName(GetName()));
    m_Encoding->ExportToFont(*this);

    if (!GetMetrics().IsStandard14FontMetrics() || IsEmbeddingEnabled())
    {
        // NOTE: Non Standard14 fonts need at least the metrics
        // descriptor. Instead Standard14 fonts don't need any
        // metrics descriptor if the font is not embedded
        auto& descriptorObj = GetDocument().GetObjects().CreateDictionaryObject("FontDescriptor");
        this->GetObject().GetDictionary().AddKeyIndirect("FontDescriptor", descriptorObj);
        FillDescriptor(descriptorObj.GetDictionary());
        m_Descriptor = &descriptorObj;
    }
}

void PdfFontSimple::embedFont()
{
    PODOFO_ASSERT(m_Descriptor != nullptr);
    this->GetObject().GetDictionary().AddKey("FirstChar", PdfVariant(static_cast<int64_t>(m_Encoding->GetFirstChar().Code)));
    this->GetObject().GetDictionary().AddKey("LastChar", PdfVariant(static_cast<int64_t>(m_Encoding->GetLastChar().Code)));

    PdfArray arr;
    this->getWidthsArray(arr);

    auto& widthsObj = GetDocument().GetObjects().CreateObject(std::move(arr));
    this->GetObject().GetDictionary().AddKeyIndirect("Widths", widthsObj);

    if (GetType() == PdfFontType::Type3)
    {
        getFontMatrixArray(arr);
        GetObject().GetDictionary().AddKey("FontMatrix", std::move(arr));

        GetBoundingBox(arr);
        GetObject().GetDictionary().AddKey("FontBBox", std::move(arr));
    }

    EmbedFontFile(*m_Descriptor);
}

void PdfFontSimple::initImported()
{
    this->Init();
}

/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontCID.h"

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfObjectStream.h"
#include "PdfFontMetricsFreetype.h"
#include <podofo/auxiliary/InputDevice.h>
#include <podofo/auxiliary/OutputDevice.h>

using namespace std;
using namespace PoDoFo;

class WidthExporter
{
private:
    WidthExporter(unsigned cid, double width);
public:
    static PdfArray GetPdfWidths(const cspan<PdfCharGIDInfo>& infos, const PdfFontMetrics& metrics);
private:
    void update(unsigned cid, double width);
    void finish();
    void reset(unsigned cid, double width);
    void emitSameWidth();
    void emitArrayWidths();
    static double getPdfWidth(unsigned gid, const PdfFontMetrics& metrics, const Matrix& matrix);

private:
    PdfArray m_output;
    PdfArray m_widths;         // array of consecutive different widths
    unsigned m_start;          // glyphIndex of start range
    double m_width;
    unsigned m_rangeCount;     // number of processed glyphIndex'es since start of range
};

PdfFontCID::PdfFontCID(PdfDocument& doc, PdfFontType type,
        PdfFontMetricsConstPtr&& metrics, const PdfEncoding& encoding) :
    PdfFont(doc, type, std::move(metrics), encoding),
    m_DescendantFont(nullptr),
    m_Descriptor(nullptr)
{
}

bool PdfFontCID::SupportsSubsetting() const
{
    return true;
}

void PdfFontCID::initImported()
{
    PdfArray arr;

    // Now setting each of the entries of the font
    this->GetDictionary().AddKey("Subtype"_n, "Type0"_n);
    this->GetDictionary().AddKey("BaseFont"_n, PdfName(this->GetName()));

    // The descendant font is a CIDFont:
    m_DescendantFont = &this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject("Font"_n);

    // The DecendantFonts, should be an indirect object:
    arr.Add(m_DescendantFont->GetIndirectReference());
    this->GetDictionary().AddKey("DescendantFonts"_n, std::move(arr));

    // Setting the /DescendantFonts
    PdfFontType fontType = GetType();
    PdfName subtype;
    switch (fontType)
    {
        case PdfFontType::CIDCFF:
            subtype = "CIDFontType0"_n;
            break;
        case PdfFontType::CIDTrueType:
            subtype = "CIDFontType2"_n;
            // CIDToGIDMap is required for CIDFontType2 with embedded font program
            m_DescendantFont->GetDictionary().AddKey("CIDToGIDMap"_n, "Identity"_n);
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
    }
    m_DescendantFont->GetDictionary().AddKey("Subtype"_n, subtype);

    // Same base font as the owner font:
    m_DescendantFont->GetDictionary().AddKey("BaseFont"_n, PdfName(this->GetName()));

    // The FontDescriptor, should be an indirect object:
    auto& descriptorObj = this->GetDocument().GetObjects().CreateDictionaryObject("FontDescriptor"_n);
    m_DescendantFont->GetDictionary().AddKeyIndirect("FontDescriptor"_n, descriptorObj);
    WriteDescriptors(GetDictionary(), descriptorObj.GetDictionary());
    m_Descriptor = &descriptorObj;
}

void PdfFontCID::embedFont()
{
    PODOFO_ASSERT(m_Descriptor != nullptr);
    auto infos = GetCharGIDInfos();
    createWidths(m_DescendantFont->GetDictionary(), infos);
    m_Encoding->ExportToFont(*this, GetCIDSystemInfo());
    EmbedFontFile(m_Descriptor->GetDictionary());
}

void PdfFontCID::embedFontSubset()
{
    auto subsetInfos = GetCharGIDInfos();
    createWidths(GetDescendantFont().GetDictionary(), subsetInfos);

    auto cidInfo = GetCIDSystemInfo();
    m_Encoding->ExportToFont(*this, cidInfo);

    embedFontFileSubset(subsetInfos, cidInfo);

    auto pdfaLevel = GetDocument().GetMetadata().GetPdfALevel();
    if (pdfaLevel == PdfALevel::L1A || pdfaLevel == PdfALevel::L1B)
    {
        // We prepare the /CIDSet content now. NOTE: The CIDSet
        // entry is optional and it's actually deprecated in PDF 2.0
        // but it's required for PDF/A-1 compliance in TrueType CID fonts.
        // Newer compliances remove this requirement, but if present
        // it has even sillier requirements
        string cidSetData;
        for (unsigned i = 0; i < subsetInfos.size(); i++)
        {
            // ISO 32000-1:2008: Table 124 – Additional font descriptor entries for CIDFonts
            // CIDSet "The stream’s data shall be organized as a table of bits
            // indexed by CID. The bits shall be stored in bytes with the
            // high - order bit first.Each bit shall correspond to a CID.
            // The most significant bit of the first byte shall correspond
            // to CID 0, the next bit to CID 1, and so on"

            constexpr char bits[] = { '\x80', '\x40', '\x20', '\x10', '\x08', '\x04', '\x02', '\x01' };
            auto& info = subsetInfos[i];
            unsigned cid = info.Cid;
            unsigned dataIndex = cid >> 3;
            if (cidSetData.size() < dataIndex + 1)
                cidSetData.resize(dataIndex + 1);

            cidSetData[dataIndex] |= bits[cid & 7];
        }

        auto& cidSetObj = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject();
        cidSetObj.GetOrCreateStream().SetData(cidSetData);
        GetDescriptor().GetDictionary().AddKeyIndirect("CIDSet"_n, cidSetObj);
    }
}

PdfObject* PdfFontCID::getDescendantFontObject()
{
    return m_DescendantFont;
}

void PdfFontCID::createWidths(PdfDictionary& fontDict, const cspan<PdfCharGIDInfo>& infos)
{
    auto& metrics = GetMetrics();
    PdfArray arr = WidthExporter::GetPdfWidths(infos, metrics);
    if (arr.size() == 0)
        return;

    fontDict.AddKey("W"_n, std::move(arr));
    double defaultWidth;
    if ((defaultWidth = metrics.GetDefaultWidthRaw()) >= 0)
    {
        // Default of /DW is 1000
        fontDict.AddKey("DW"_n, PdfObject(defaultWidth / metrics.GetMatrix()[0]));
    }
}

WidthExporter::WidthExporter(unsigned cid, double width)
{
    reset(cid, width);
}

void WidthExporter::update(unsigned cid, double width)
{
    if (cid == (m_start + m_rangeCount))
    {
        // continuous gid
        if (width - m_width != 0)
        {
            // different width, so emit if previous range was with same width
            if ((m_rangeCount != 1) && m_widths.IsEmpty())
            {
                emitSameWidth();
                reset(cid, width);
                return;
            }
            m_widths.Add(PdfObject(m_width));
            m_width = width;
            m_rangeCount++;
            return;
        }
        // two or more gids with same width
        if (!m_widths.IsEmpty())
        {
            emitArrayWidths();
            /* setup previous width as start position */
            m_start += m_rangeCount - 1;
            m_rangeCount = 2;
            return;
        }
        // consecutive range of same widths
        m_rangeCount++;
        return;
    }
    // gid gap (font subset)
    finish();
    reset(cid, width);
}

void WidthExporter::finish()
{
    // if there is a single glyph remaining, emit it as array
    if (!m_widths.IsEmpty() || m_rangeCount == 1)
    {
        m_widths.Add(PdfObject(m_width));
        emitArrayWidths();
        return;
    }

    emitSameWidth();
}

PdfArray WidthExporter::GetPdfWidths(const cspan<PdfCharGIDInfo>& infos, const PdfFontMetrics& metrics)
{
    if (infos.size() == 0)
        return PdfArray();

    auto& matrix = metrics.GetMatrix();
    // Always initialize the exporter with CID 0
    WidthExporter exporter(0, getPdfWidth(0, metrics, matrix));
    for (unsigned i = 0; i < infos.size(); i++)
    {
        auto& info = infos[i];
        // If the CID 0 is present in the map, just skip it
        if (info.Cid == 0)
            continue;

        exporter.update(info.Cid, getPdfWidth(info.Gid.MetricsId, metrics, matrix));
    }

    exporter.finish();
    return std::move(exporter.m_output);
}

void WidthExporter::reset(unsigned cid, double width)
{
    m_start = cid;
    m_width = width;
    m_rangeCount = 1;
}

void WidthExporter::emitSameWidth()
{
    m_output.Add(static_cast<int64_t>(m_start));
    m_output.Add(static_cast<int64_t>(m_start + m_rangeCount - 1));
    m_output.Add(m_width);
}

void WidthExporter::emitArrayWidths()
{
    m_output.Add(static_cast<int64_t>(m_start));
    m_output.Add(std::move(m_widths));
}

// Return thousands of PDF units
double WidthExporter::getPdfWidth(unsigned gid, const PdfFontMetrics& metrics, const Matrix& matrix)
{
    return metrics.GetGlyphWidth(gid) / matrix[0];
}

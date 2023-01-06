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
#include "PdfInputDevice.h"
#include "PdfOutputDevice.h"

using namespace std;
using namespace PoDoFo;

class WidthExporter
{
private:
    WidthExporter(unsigned cid, unsigned width);
public:
    static PdfArray GetPdfWidths(const CIDToGIDMap& glyphWidths,
        const PdfFontMetrics& metrics);
private:
    void update(unsigned cid, unsigned width);
    PdfArray finish();
    void reset(unsigned cid, unsigned width);
    void emitSameWidth();
    void emitArrayWidths();
    static unsigned getPdfWidth(unsigned gid, const PdfFontMetrics& metrics,
        const Matrix2D& matrix);

private:
    PdfArray m_output;
    PdfArray m_widths;         // array of consecutive different widths
    unsigned m_start;          // glyphIndex of start range
    unsigned m_width;
    unsigned m_rangeCount;     // number of processed glyphIndex'es since start of range
};

PdfFontCID::PdfFontCID(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding) :
    PdfFont(doc, metrics, encoding),
    m_descendantFont(nullptr),
    m_descriptor(nullptr)
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
    this->GetObject().GetDictionary().AddKey(PdfName::KeySubtype, PdfName("Type0"));
    this->GetObject().GetDictionary().AddKey("BaseFont", PdfName(this->GetName()));

    // The descendant font is a CIDFont:
    m_descendantFont = &this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject("Font");

    // The DecendantFonts, should be an indirect object:
    arr.Add(m_descendantFont->GetIndirectReference());
    this->GetObject().GetDictionary().AddKey("DescendantFonts", arr);

    // Setting the /DescendantFonts
    PdfFontType fontType = GetType();
    PdfName subtype;
    switch (fontType)
    {
        case PdfFontType::CIDType1:
            subtype = "CIDFontType0";
            break;
        case PdfFontType::CIDTrueType:
            subtype = "CIDFontType2";
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
    }
    m_descendantFont->GetDictionary().AddKey(PdfName::KeySubtype, subtype);

    // Same base font as the owner font:
    m_descendantFont->GetDictionary().AddKey("BaseFont", PdfName(this->GetName()));
    m_descendantFont->GetDictionary().AddKey("CIDToGIDMap", PdfName("Identity"));

    // The FontDescriptor, should be an indirect object:
    auto& descriptorObj = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject("FontDescriptor");
    m_descendantFont->GetDictionary().AddKeyIndirect("FontDescriptor", descriptorObj);
    FillDescriptor(descriptorObj.GetDictionary());
    m_descriptor = &descriptorObj;
}

void PdfFontCID::embedFont()
{
    PODOFO_ASSERT(m_descriptor != nullptr);
    createWidths(m_descendantFont->GetDictionary(), getIdentityCIDToGIDMap());
    m_Encoding->ExportToFont(*this);
    EmbedFontFile(*m_descriptor);
}

PdfObject* PdfFontCID::getDescendantFontObject()
{
    return m_descendantFont;
}

void PdfFontCID::createWidths(PdfDictionary& fontDict, const CIDToGIDMap& cidToGidMap)
{
    auto& metrics = GetMetrics();
    PdfArray arr = WidthExporter::GetPdfWidths(cidToGidMap, metrics);
    if (arr.size() == 0)
        return;

    fontDict.AddKey("W", arr);
    double defaultWidth;
    if ((defaultWidth = GetMetrics().GetDefaultWidthRaw()) >= 0)
    {
        // Default of /DW is 1000
        fontDict.AddKey("DW", static_cast<int64_t>(
            std::round(defaultWidth / metrics.GetMatrix()[0])));
    }
}

CIDToGIDMap PdfFontCID::getIdentityCIDToGIDMap()
{
    PODOFO_ASSERT(!IsSubsettingEnabled());
    CIDToGIDMap ret;
    unsigned gidCount = GetMetrics().GetGlyphCount();
    for (unsigned gid = 0; gid < gidCount; gid++)
        ret.insert(std::make_pair(gid, gid));

    return ret;
}

CIDToGIDMap PdfFontCID::getCIDToGIDMapSubset(const UsedGIDsMap& usedGIDs)
{
    CIDToGIDMap ret;
    for (auto& pair : usedGIDs)
    {
        unsigned gid = pair.first;
        unsigned cid = pair.second.Id;
        ret.insert(std::make_pair(cid, gid));
    }

    return ret;
}

WidthExporter::WidthExporter(unsigned cid, unsigned width)
{
    reset(cid, width);
}

void WidthExporter::update(unsigned cid, unsigned width)
{
    if (cid == (m_start + m_rangeCount))
    {
        // continous gid
        if (width - m_width != 0)
        {
            // different width, so emit if previous range was with same width
            if ((m_rangeCount != 1) && m_widths.IsEmpty())
            {
                emitSameWidth();
                reset(cid, width);
                return;
            }
            m_widths.Add(PdfObject(static_cast<int64_t>(m_width)));
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

PdfArray WidthExporter::finish()
{
    // if there is a single glyph remaining, emit it as array
    if (!m_widths.IsEmpty() || m_rangeCount == 1)
    {
        m_widths.Add(PdfObject(static_cast<int64_t>(m_width)));
        emitArrayWidths();
        return m_output;
    }
    emitSameWidth();

    return m_output;
}

PdfArray WidthExporter::GetPdfWidths(const CIDToGIDMap& cidToGidMap,
    const PdfFontMetrics& metrics)
{
    if (cidToGidMap.size() == 0)
        return PdfArray();

    auto& matrix = metrics.GetMatrix();
    WidthExporter exporter(0, getPdfWidth(0, metrics, matrix));
    for (auto& pair : cidToGidMap)
        exporter.update(pair.first, getPdfWidth(pair.second, metrics, matrix));

    return exporter.finish();
}

void WidthExporter::reset(unsigned cid, unsigned width)
{
    m_start = cid;
    m_width = width;
    m_rangeCount = 1;
}

void WidthExporter::emitSameWidth()
{
    m_output.Add(static_cast<int64_t>(m_start));
    m_output.Add(static_cast<int64_t>(m_start + m_rangeCount - 1));
    m_output.Add(static_cast<int64_t>(std::round(m_width)));
}

void WidthExporter::emitArrayWidths()
{
    m_output.Add(static_cast<int64_t>(m_start));
    m_output.Add(m_widths);
    m_widths.Clear();
}

// Return thousands of PDF units
unsigned WidthExporter::getPdfWidth(unsigned gid, const PdfFontMetrics& metrics,
    const Matrix2D& matrix)
{
    return (unsigned)std::round(metrics.GetGlyphWidth(gid) / matrix[0]);
}

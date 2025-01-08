/**
 * SPDX-FileCopyrightText: (C) 2008 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// The code was initially based on work by ZhangYang
// (张杨.国际) <zhang_yang@founder.com>

#include "PdfDeclarationsPrivate.h"
#include "FontTrueTypeSubset.h"

#include <algorithm>

#include <podofo/private/FreetypePrivate.h>
#include FT_TRUETYPE_TABLES_H
#include FT_TRUETYPE_TAGS_H

#include <podofo/auxiliary/StreamDevice.h>

using namespace std;
using namespace PoDoFo;

// Required TrueType tables
enum class ReqTable
{
    none = 0,
    head = 1,
    hhea = 2,
    loca = 4,
    maxp = 8,
    glyf = 16,
    hmtx = 32,
    all = head | hhea | loca | maxp | glyf | hmtx,
};

ENABLE_BITMASK_OPERATORS(ReqTable);

static constexpr unsigned LENGTH_HEADER12 = 12;
static constexpr unsigned LENGTH_OFFSETTABLE16 = 16;

//Get the number of bytes to pad the ul, because of 4-byte-alignment.
static uint32_t GetTableCheksum(const char* buf, uint32_t size);

static bool TryAdvanceCompoundOffset(unsigned& offset, unsigned flags);

FontTrueTypeSubset::FontTrueTypeSubset(InputStreamDevice& device, const PdfFontMetrics& metrics) :
    m_device(&device),
    m_metrics(&metrics),
    m_isLongLoca(false),
    m_glyphCount(0),
    m_HMetricsCount(0),
    m_unitsPerEM(0),
    m_hmtxTableOffset(0),
    m_leftSideBearingsOffset(0)
{
}

void FontTrueTypeSubset::BuildFont(const PdfFontMetrics& metrics, const cspan<PdfCharGIDInfo>& infos, charbuff& output)
{
    if (infos.empty())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "The cid/gid map must not be empty");

    if (metrics.GetFontFileType() != PdfFontFileType::TrueType)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontData, "The font to be subsetted is not a TrueType font");

    SpanStreamDevice input(metrics.GetOrLoadFontFileData());
    FontTrueTypeSubset subset(input, metrics);
    subset.BuildFont(infos, output);
}

void FontTrueTypeSubset::BuildFont(const cspan<PdfCharGIDInfo>& infos, charbuff& output)
{
    Init();

    GlyphContext context;
    context.GlyfTableOffset = GetTableOffset(TTAG_glyf);
    context.LocaTableOffset = GetTableOffset(TTAG_loca);

    // For any fonts, assume that glyph 0 is needed.
    LoadGlyphData(context, 0);
    for (unsigned i = 0; i < infos.size(); i++)
        LoadGlyphData(context, infos[i].Gid.Id);

    LoadGlyphMetrics(infos);
    WriteTables(output);
}

void FontTrueTypeSubset::Init()
{
    InitTables();
    GetNumberOfGlyphs();
    SeeIfLongLocaOrNot();
}

unsigned FontTrueTypeSubset::GetTableOffset(unsigned tag)
{
    for (auto& table : m_tables)
    {
        if (table.Tag == tag)
            return table.Offset;
    }
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "table missing");
}

void FontTrueTypeSubset::GetNumberOfGlyphs()
{
    unsigned offset = GetTableOffset(TTAG_maxp);

    m_device->Seek(offset + sizeof(uint32_t) * 1);
    utls::ReadUInt16BE(*m_device, m_glyphCount);

    offset = GetTableOffset(TTAG_hhea);

    m_device->Seek(offset + sizeof(uint16_t) * 17);
    utls::ReadUInt16BE(*m_device, m_HMetricsCount);

    m_hmtxTableOffset = GetTableOffset(TTAG_hmtx);
    m_leftSideBearingsOffset = m_hmtxTableOffset + m_HMetricsCount * sizeof(LongHorMetrics);

    m_unitsPerEM = m_metrics->GetFaceHandle()->units_per_EM;
}

void FontTrueTypeSubset::InitTables()
{
    uint16_t tableCount;
    m_device->Seek(sizeof(uint32_t) * 1);
    utls::ReadUInt16BE(*m_device, tableCount);

    ReqTable tableMask = ReqTable::none;
    TrueTypeTable tbl;

    for (unsigned short i = 0; i < tableCount; i++)
    {
        // Name of each table:
        m_device->Seek(LENGTH_HEADER12 + LENGTH_OFFSETTABLE16 * i);
        utls::ReadUInt32BE(*m_device, tbl.Tag);

        // Checksum of each table:
        m_device->Seek(LENGTH_HEADER12 + LENGTH_OFFSETTABLE16 * i + sizeof(uint32_t) * 1);
        utls::ReadUInt32BE(*m_device, tbl.Checksum);

        // Offset of each table:
        m_device->Seek(LENGTH_HEADER12 + LENGTH_OFFSETTABLE16 * i + sizeof(uint32_t) * 2);
        utls::ReadUInt32BE(*m_device, tbl.Offset);

        // Length of each table:
        m_device->Seek(LENGTH_HEADER12 + LENGTH_OFFSETTABLE16 * i + sizeof(uint32_t) * 3);
        utls::ReadUInt32BE(*m_device, tbl.Length);

        // PDF 32000-1:2008 9.9 Embedded Font Programs
        // "These TrueType tables shall always be present if present in the original TrueType font program:
        // 'head', 'hhea', 'loca', 'maxp', 'cvt','prep', 'glyf', 'hmtx' and 'fpgm'. [..]  If used with a
        // CIDFont dictionary, the 'cmap' table is not needed and shall not be present

        bool skipTable = false;
        switch (tbl.Tag)
        {
            case TTAG_head:
                tableMask |= ReqTable::head;
                break;
            case TTAG_hhea:
                // required to get numHMetrics
                tableMask |= ReqTable::hhea;
                break;
            case TTAG_loca:
                tableMask |= ReqTable::loca;
                break;
            case TTAG_maxp:
                tableMask |= ReqTable::maxp;
                break;
            case TTAG_glyf:
                tableMask |= ReqTable::glyf;
                break;
            case TTAG_hmtx:
                // advance width
                tableMask |= ReqTable::hmtx;
                break;
            case TTAG_cvt:
            case TTAG_fpgm:
            case TTAG_prep:
                // Just include these tables unconditionally if present
                // in the original font
                break;
            case TTAG_post:
                if (tbl.Length < 32)
                    skipTable = true;

                // Reduce table size, leter we will change format to 'post' Format 3
                tbl.Length = 32;
                break;
            // Exclude all other tables, including cmap which
            // is not required
            case TTAG_cmap:
            default:
                skipTable = true;
                break;
        }
        if (!skipTable)
            m_tables.push_back(tbl);
    }

    if ((tableMask & ReqTable::all) == ReqTable::none)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFontFormat, "Required TrueType table missing");
}

void FontTrueTypeSubset::SeeIfLongLocaOrNot()
{
    unsigned headOffset = GetTableOffset(TTAG_head);
    uint16_t isLong;
    m_device->Seek(headOffset + 50);
    utls::ReadUInt16BE(*m_device, isLong);
    m_isLongLoca = (isLong == 0 ? false : true);  // 1 for long
}

void FontTrueTypeSubset::LoadGlyphMetrics(const cspan<PdfCharGIDInfo>& infos)
{
    // Map original GIDs to a new index as they will appear in the subset
    map<unsigned, unsigned> glyphIndexMap;
    glyphIndexMap.insert({ 0, 0 });

    // Ensure the first glyph is always the first one
    m_subsetGIDs.push_back({ 0, GetGlyphMetricsPdfAdvance(0, 0) });
    for (unsigned i = 0; i < infos.size(); i++)
    {
        auto& info = infos[i];
        glyphIndexMap.insert({ info.Gid.Id, (unsigned)m_subsetGIDs.size() });
        m_subsetGIDs.push_back({ info.Gid.Id, GetGlyphMetricsPdfAdvance(info.Gid.Id, info.Gid.MetricsId) });
    }

    for (auto& pair : m_glyphDatas)
    {
        // Iterate all loaded for coumpond glyphs to subset
        auto& glyphData = pair.second;
        if (!glyphData.IsCompound)
            continue;

        GlyphCompoundData cmpData;
        unsigned offset = 0;
        while (true)
        {
            unsigned componentGlyphIdOffset = glyphData.GlyphAdvOffset + offset;
            ReadGlyphCompoundData(cmpData, componentGlyphIdOffset);
            // Try remap the GID
            auto inserted = glyphIndexMap.insert({ cmpData.GlyphIndex, (unsigned)m_subsetGIDs.size() });
            if (inserted.second)
            {
                // If insertion occurred, insert it to the GIDs ordered list
                m_subsetGIDs.push_back({ cmpData.GlyphIndex, GetGlyphMetrics(cmpData.GlyphIndex) });
            }

            // Insert the compound component using the actual assigned GID
            glyphData.CompoundComponents.push_back(
                { (unsigned)(componentGlyphIdOffset + sizeof(uint16_t)) - glyphData.GlyphOffset, inserted.first->second });
            if (!TryAdvanceCompoundOffset(offset, cmpData.Flags))
                break;
        }
    }
}

FontTrueTypeSubset::LongHorMetrics FontTrueTypeSubset::GetGlyphMetrics(unsigned gid)
{
    LongHorMetrics ret;
    if (gid < m_HMetricsCount)
    {
        // The full horizontal metrics exists
        m_device->Seek(m_hmtxTableOffset + gid * sizeof(LongHorMetrics));
        utls::ReadUInt16BE(*m_device, ret.AdvanceWidth);
        utls::ReadInt16BE(*m_device, ret.LeftSideBearing);
    }
    else
    {
        // The full horizontal metrics doesn't exists, just copy the left side bearings
        // at the end of the metrics. From the specification:
        // "As an optimization, the number of records can be less than the number
        // of glyphs, in which case the advance width value of the last record applies
        // to all remaining glyph IDs"

        m_device->Seek(m_hmtxTableOffset + (m_HMetricsCount - 1) * sizeof(LongHorMetrics));
        utls::ReadUInt16BE(*m_device, ret.AdvanceWidth);

        m_device->Seek(m_leftSideBearingsOffset + sizeof(int16_t) * (gid - m_HMetricsCount));
        utls::ReadInt16BE(*m_device, ret.LeftSideBearing);
    }

    return ret;
}

// Read the metrics with PDF read advance
FontTrueTypeSubset::LongHorMetrics FontTrueTypeSubset::GetGlyphMetricsPdfAdvance(unsigned gid, unsigned metricsId)
{
    auto ret = GetGlyphMetrics(gid);
    // NOTE: Retrieve the actual CID width and write it in the measure unit as found in the font
    ret.AdvanceWidth = (uint16_t)std::round(m_metrics->GetGlyphWidth(metricsId) * m_unitsPerEM);
    return ret;
}

void FontTrueTypeSubset::LoadGlyphData(GlyphContext& ctx, unsigned gid)
{
    if (gid >= m_glyphCount)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "GID out of range");

    if (m_glyphDatas.find(gid) != m_glyphDatas.end())
        return;

    // https://docs.microsoft.com/en-us/typography/opentype/spec/loca
    auto& glyphData = m_glyphDatas[gid] = { };
    if (m_isLongLoca)
    {
        uint32_t offset1;
        uint32_t offset2;
        m_device->Seek(ctx.LocaTableOffset + sizeof(uint32_t) * gid);
        utls::ReadUInt32BE(*m_device, offset1);

        m_device->Seek(ctx.LocaTableOffset + sizeof(uint32_t) * (gid + 1));
        utls::ReadUInt32BE(*m_device, offset2);

        glyphData.GlyphLength = offset2 - offset1;
        glyphData.GlyphOffset = ctx.GlyfTableOffset + offset1;
    }
    else
    {
        uint16_t offset1;
        uint16_t offset2;

        m_device->Seek(ctx.LocaTableOffset + sizeof(uint16_t) * gid);
        utls::ReadUInt16BE(*m_device, offset1);
        unsigned uoffset1 = (unsigned)offset1 << 1u; // Handle the possible overflow

        m_device->Seek(ctx.LocaTableOffset + sizeof(uint16_t) * (gid + 1));
        utls::ReadUInt16BE(*m_device, offset2);
        unsigned uoffset2 = (unsigned)offset2 << 1u; // Handle the possible overflow

        glyphData.GlyphLength = uoffset2 - uoffset1;
        glyphData.GlyphOffset = ctx.GlyfTableOffset + uoffset1;
    }

    glyphData.GlyphAdvOffset = glyphData.GlyphOffset + 5 * sizeof(uint16_t);

    // NOTE: Some fonts may truncate countour section,
    // skip reading on EOF in that case
    m_device->Seek(glyphData.GlyphOffset);
    if (!m_device->Eof())
    {
        utls::ReadInt16BE(*m_device, ctx.ContourCount);
        if (ctx.ContourCount < 0)
        {
            glyphData.IsCompound = true;
            LoadCompound(ctx, glyphData);
        }
    }
}

void FontTrueTypeSubset::LoadCompound(GlyphContext& ctx, const GlyphData& data)
{
    GlyphCompoundData cmpData;
    unsigned offset = 0;
    while (true)
    {
        ReadGlyphCompoundData(cmpData, data.GlyphAdvOffset + offset);
        LoadGlyphData(ctx, cmpData.GlyphIndex);
        if (!TryAdvanceCompoundOffset(offset, cmpData.Flags))
            break;
    }
}

// Ref: https://docs.microsoft.com/en-us/typography/opentype/spec/glyf
void FontTrueTypeSubset::WriteGlyphTable(OutputStream& output)
{
    for (unsigned i = 0; i < m_subsetGIDs.size(); i++)
    {
        auto& gid = m_subsetGIDs[i];
        auto& glyphData = m_glyphDatas[gid.Id];
        if (glyphData.IsCompound)
        {
            // Fix the compound glyph data to remap original GIDs indices
            // as they will appear in the subset

            m_tmpBuffer.resize(glyphData.GlyphLength);
            m_device->Seek(glyphData.GlyphOffset);
            m_device->Read(m_tmpBuffer.data(), glyphData.GlyphLength);
            for (auto& component : glyphData.CompoundComponents)
                utls::WriteUInt16BE(m_tmpBuffer.data() + component.Offset, (uint16_t)component.GlyphIndex);
            output.Write(m_tmpBuffer);
        }
        else
        {
            // The simple glyph data doesn't need to be fixed
            CopyData(output, glyphData.GlyphOffset, glyphData.GlyphLength);
        }
    }
}

// The 'hmtx' table contains the horizontal metrics for each glyph in the font
// https://docs.microsoft.com/en-us/typography/opentype/spec/hmtx
void FontTrueTypeSubset::WriteHmtxTable(OutputStream& output)
{
    for (unsigned i = 0; i < m_subsetGIDs.size(); i++)
    {
        auto& metrics = m_subsetGIDs[i].Metrics;
        utls::WriteUInt16BE(output, metrics.AdvanceWidth);
        utls::WriteInt16BE(output, metrics.LeftSideBearing);
    }
}

// "The 'loca' table stores the offsets to the locations
// of the glyphs in the font relative to the beginning of
// the 'glyf' table. [..] To make it possible to compute
// the length of the last glyph element, there is an extra
// entry after the offset that points to the last valid
// index. This index points to the end of the glyph data"
// Ref: https://docs.microsoft.com/en-us/typography/opentype/spec/loca
void FontTrueTypeSubset::WriteLocaTable(OutputStream& output)
{
    uint32_t glyphAddress = 0;
    if (m_isLongLoca)
    {
        for (unsigned i = 0; i < m_subsetGIDs.size(); i++)
        {
            unsigned gid = m_subsetGIDs[i].Id;
            auto& glyphData = m_glyphDatas[gid];
            utls::WriteUInt32BE(output, glyphAddress);
            glyphAddress += glyphData.GlyphLength;
        }

        // Last "extra" entry
        utls::WriteUInt32BE(output, glyphAddress);
    }
    else
    {
        for (unsigned i = 0; i < m_subsetGIDs.size(); i++)
        {
            unsigned gid = m_subsetGIDs[i].Id;
            auto& glyphData = m_glyphDatas[gid];
            utls::WriteUInt16BE(output, static_cast<uint16_t>(glyphAddress >> 1));
            glyphAddress += glyphData.GlyphLength;
        }

        // Last "extra" entry
        utls::WriteUInt16BE(output, static_cast<uint16_t>(glyphAddress >> 1));
    }
}

void FontTrueTypeSubset::WriteTables(charbuff& buffer)
{
    StringStreamDevice output(buffer);

    uint16_t entrySelector = (uint16_t)std::ceil(std::log2(m_tables.size()));
    uint16_t searchRange = (uint16_t)std::pow(2, entrySelector);
    uint16_t rangeShift = (16 * (uint16_t)m_tables.size()) - searchRange;

    // Write the font directory table
    // https://docs.microsoft.com/en-us/typography/opentype/spec/otff#tabledirectory
    utls::WriteUInt32BE(output, 0x00010000);     // Scaler type, 0x00010000 is True type font
    utls::WriteUInt16BE(output, (uint16_t)m_tables.size());
    utls::WriteUInt16BE(output, searchRange);
    utls::WriteUInt16BE(output, entrySelector);
    utls::WriteUInt16BE(output, rangeShift);

    size_t directoryTableOffset = output.GetPosition();

    // Prepare table offsets
    for (unsigned i = 0; i < m_tables.size(); i++)
    {
        auto& table = m_tables[i];
        utls::WriteUInt32BE(output, table.Tag);
        // Write empty placeholders
        utls::WriteUInt32BE(output, 0); // Table checksum
        utls::WriteUInt32BE(output, 0); // Table offset
        utls::WriteUInt32BE(output, 0); // Table length (actual length not padded length)
    }

    nullable<size_t> headOffset;
    size_t tableOffset;
    for (unsigned i1 = 0; i1 < m_tables.size(); i1++)
    {
        auto& table = m_tables[i1];
        tableOffset = output.GetPosition();
        switch (table.Tag)
        {
            case TTAG_head:
                headOffset = tableOffset;
                CopyData(output, table.Offset, table.Length);
                // Set the checkSumAdjustment to 0
                utls::WriteUInt32BE(buffer.data() + tableOffset + 4, 0);
                break;
            case TTAG_maxp:
                // https://docs.microsoft.com/en-us/typography/opentype/spec/maxp
                CopyData(output, table.Offset, table.Length);
                // Write the number of glyphs in the font
                utls::WriteUInt16BE(buffer.data() + tableOffset + 4, (uint16_t)m_subsetGIDs.size());
                break;
            case TTAG_hhea:
                // https://docs.microsoft.com/en-us/typography/opentype/spec/hhea
                CopyData(output, table.Offset, table.Length);
                // Write numOfLongHorMetrics, see also 'hmtx' table
                utls::WriteUInt16BE(buffer.data() + tableOffset + 34, (uint16_t)m_subsetGIDs.size());
                break;
            case TTAG_post:
                // https://docs.microsoft.com/en-us/typography/opentype/spec/post
                CopyData(output, table.Offset, table.Length);
                // Enforce 'post' Format 3, written as a Fixed 16.16 number
                utls::WriteUInt32BE(buffer.data() + tableOffset, 0x00030000);
                // Clear Type42/Type1 font information
                memset(buffer.data() + tableOffset + 16, 0, 16);
                break;
            case TTAG_glyf:
                WriteGlyphTable(output);
                break;
            case TTAG_loca:
                WriteLocaTable(output);
                break;
            case TTAG_hmtx:
                WriteHmtxTable(output);
                break;
            case TTAG_cvt:
            case TTAG_fpgm:
            case TTAG_prep:
                CopyData(output, table.Offset, table.Length);
                break;
            default:
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Unsupported table at this context");
        }

        // Align the table length to 4 bytes and pad remaining space with zeroes
        size_t tableLength = output.GetPosition() - tableOffset;
        size_t tableLengthPadded = (tableLength + 3) & ~3;
        for (size_t i2 = tableLength; i2 < tableLengthPadded; i2++)
            output.Write('\0');

        // Write dynamic font directory table entries
        size_t currDirTableOffset = directoryTableOffset + i1 * LENGTH_OFFSETTABLE16;
        utls::WriteUInt32BE(buffer.data() + currDirTableOffset + 4, GetTableCheksum(buffer.data() + tableOffset, (uint32_t)tableLength));
        utls::WriteUInt32BE(buffer.data() + currDirTableOffset + 8, (uint32_t)tableOffset);
        utls::WriteUInt32BE(buffer.data() + currDirTableOffset + 12, (uint32_t)tableLength);
    }

    // Check for head table
    if (!headOffset.has_value())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "'head' table missing");

    // As explained in the "Table Directory"
    // https://docs.microsoft.com/en-us/typography/opentype/spec/otff#tabledirectory
    uint32_t fontChecksum = 0xB1B0AFBA - GetTableCheksum(buffer.data(), (uint32_t)output.GetPosition());
    utls::WriteUInt32BE(buffer.data() + *headOffset + 4, fontChecksum);
}

void FontTrueTypeSubset::ReadGlyphCompoundData(GlyphCompoundData& data, unsigned offset)
{
    uint16_t temp;
    m_device->Seek(offset);
    utls::ReadUInt16BE(*m_device, temp);
    data.Flags = temp;

    m_device->Seek(offset + sizeof(uint16_t));
    utls::ReadUInt16BE(*m_device, temp);
    data.GlyphIndex = temp;
}

bool TryAdvanceCompoundOffset(unsigned& offset, unsigned flags)
{
    constexpr unsigned ARG_1_AND_2_ARE_WORDS = 0x01;
    constexpr unsigned WE_HAVE_A_SCALE = 0x08;
    constexpr unsigned MORE_COMPONENTS = 0x20;
    constexpr unsigned WE_HAVE_AN_X_AND_Y_SCALE = 0x40;
    constexpr unsigned WE_HAVE_TWO_BY_TWO = 0x80;

    if ((flags & MORE_COMPONENTS) == 0)
        return false;

    offset += (flags & ARG_1_AND_2_ARE_WORDS) ? 4 * sizeof(uint16_t) : 3 * sizeof(uint16_t);
    if ((flags & WE_HAVE_A_SCALE) != 0)
        offset += sizeof(uint16_t);
    else if ((flags & WE_HAVE_AN_X_AND_Y_SCALE) != 0)
        offset += 2 * sizeof(uint16_t);
    else if ((flags & WE_HAVE_TWO_BY_TWO) != 0)
        offset += 4 * sizeof(uint16_t);

    return true;
}

void FontTrueTypeSubset::CopyData(OutputStream& output, unsigned offset, unsigned size)
{
    m_device->Seek(offset);
    m_tmpBuffer.resize(size);
    m_device->Read(m_tmpBuffer.data(), size);
    output.Write(m_tmpBuffer);
}

uint32_t GetTableCheksum(const char* buf, uint32_t size)
{
    // As explained in the "Table Directory"
    // https://docs.microsoft.com/en-us/typography/opentype/spec/otff#tabledirectory
    uint32_t sum = 0;
    uint32_t nLongs = (size + 3) / 4;
    while (nLongs-- > 0)
        sum += *buf++;

    return sum;
}

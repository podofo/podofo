/**
 * SPDX-FileCopyrightText: (C) 2008 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FONT_TTF_SUBSET_H
#define PDF_FONT_TTF_SUBSET_H

#include <podofo/main/PdfFontMetrics.h>

namespace PoDoFo {

class InputStreamDevice;
class OutputStream;

/**
 * This class is able to build a new TTF font with only
 * certain glyphs from an existing font.
 *
 */
class FontTrueTypeSubset final
{
private:
    FontTrueTypeSubset(InputStreamDevice& device, const PdfFontMetrics& metrics);

public:
    /**
     * Actually generate the subsetted font
     * Create a new PdfFontTrueTypeSubset from an existing
     * TTF font file retrieved from a font metrics
     *
     * \param output write the font to this buffer
     * \param metrics the metrics of the font to subset
     * \param gids a list of glyphs to subset
     */
    static void BuildFont(const PdfFontMetrics& metrics, const cspan<PdfCharGIDInfo>& infos, charbuff& output);

private:
    FontTrueTypeSubset(const FontTrueTypeSubset& rhs) = delete;
    FontTrueTypeSubset& operator=(const FontTrueTypeSubset& rhs) = delete;

    void buildFont(const cspan<PdfCharGIDInfo>& infos, charbuff& output);

    void init();
    unsigned getTableOffset(unsigned tag);
    void getNumberOfGlyphs();
    void determineLongLocaTable();
    void initTables();

    void copyData(OutputStream& output, unsigned offset, unsigned size);

    struct GlyphCompoundComponentData
    {
        unsigned Offset = 0;
        unsigned GlyphIndex = 0;
    };

    /** GlyphData contains the glyph address relative
     *  to the beginning of the "glyf" table.
     */
    struct GlyphData
    {
        bool IsCompound = false;
        unsigned GlyphOffset = 0;       // Offset of common "glyph" data
        unsigned GlyphLength = 0;
        unsigned GlyphAdvOffset = 0;    // Offset of uncommon simple/compound "glyph" data
        std::vector<GlyphCompoundComponentData> CompoundComponents;
    };

    // A CID indexed glyph map
    using GlyphDatas = std::map<unsigned, GlyphData>;

    struct GlyphContext
    {
        unsigned GlyfTableOffset = 0;
        unsigned LocaTableOffset = 0;
        // Used internally during recursive load
        int16_t ContourCount = 0;
    };

    struct GlyphCompoundData
    {
        unsigned Flags = 0;
        unsigned GlyphIndex = 0;
    };

    struct LongHorMetrics
    {
        uint16_t AdvanceWidth = 0;
        int16_t LeftSideBearing = 0;
    };

    void loadGlyphData(GlyphContext& ctx, unsigned gid);
    void loadCompound(GlyphContext& ctx, const GlyphData& data);
    void loadGlyphMetrics(const cspan<PdfCharGIDInfo>& infos);
    LongHorMetrics getGlyphMetrics(unsigned gid);
    LongHorMetrics getGlyphMetricsPdfAdvance(unsigned gid, unsigned metricsId);
    void writeGlyphTable(OutputStream& output);
    void writeHmtxTable(OutputStream& output);
    void writeLocaTable(OutputStream& output);
    void writeTables(charbuff& output);
    void readGlyphCompoundData(GlyphCompoundData& data, unsigned offset);

    /** Information of TrueType tables.
     */
    struct TrueTypeTable
    {
        uint32_t Tag = 0;
        uint32_t Checksum = 0;
        uint32_t Length = 0;
        uint32_t Offset = 0;
    };

    struct GIDInfo
    {
        unsigned Id = 0;
        LongHorMetrics Metrics;
    };

private:
    InputStreamDevice* m_device;
    const PdfFontMetrics* m_metrics;

    bool m_isLongLoca; ///< Determines if the glyph locations are 16 or 32 bits
    uint16_t m_glyphCount;
    uint16_t m_HMetricsCount;
    uint16_t m_unitsPerEM;
    unsigned m_hmtxTableOffset;
    unsigned m_leftSideBearingsOffset;

    std::vector<TrueTypeTable> m_tables;
    GlyphDatas m_glyphDatas;
    // Ordered list GIDs as they will appear in the subset with their metrics
    std::vector<GIDInfo> m_subsetGIDs;
    charbuff m_tmpBuffer;
};

}

#endif // PDF_FONT_TRUE_TYPE_H

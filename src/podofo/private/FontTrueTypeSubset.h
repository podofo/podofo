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
 * Internal enum specifying the type of a fontfile.
 */
enum class TrueTypeFontFileType
{
    Unknown, ///< Unknown
    TTF,    ///< TrueType Font
    TTC,    ///< TrueType Collection
    OTF,    ///< OpenType Font
};

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
    static void BuildFont(std::string& output, const PdfFontMetrics& metrics, const cspan<PdfCharGIDInfo>& infos);

private:
    FontTrueTypeSubset(const FontTrueTypeSubset& rhs) = delete;
    FontTrueTypeSubset& operator=(const FontTrueTypeSubset& rhs) = delete;

    void BuildFont(std::string& buffer, const cspan<PdfCharGIDInfo>& infos);

    void Init();
    unsigned GetTableOffset(unsigned tag);
    void GetNumberOfGlyphs();
    void SeeIfLongLocaOrNot();
    void InitTables();

    void CopyData(OutputStream& output, unsigned offset, unsigned size);

private:
    /** Information of TrueType tables.
     */
    struct TrueTypeTable
    {
        uint32_t Tag = 0;
        uint32_t Checksum = 0;
        uint32_t Length = 0;
        uint32_t Offset = 0;
    };

    struct GlyphCompoundComponentData
    {
        unsigned Offset;
        unsigned GlyphIndex;
    };

    /** GlyphData contains the glyph address relative
     *  to the beginning of the "glyf" table.
     */
    struct GlyphData
    {
        bool IsCompound;
        unsigned GlyphOffset;       // Offset of common "glyph" data
        unsigned GlyphLength;
        unsigned GlyphAdvOffset;    // Offset of uncommon simple/compound "glyph" data
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
        unsigned Flags;
        unsigned GlyphIndex;
    };

    struct LongHorMetrics
    {
        uint16_t AdvanceWidth;
        int16_t LeftSideBearing;
    };

    void LoadGlyphData(GlyphContext& ctx, unsigned gid);
    void LoadCompound(GlyphContext& ctx, const GlyphData& data);
    void LoadGlyphMetrics(const cspan<PdfCharGIDInfo>& infos);
    LongHorMetrics GetGlyphMetrics(unsigned gid);
    LongHorMetrics GetGlyphMetricsPdfAdvance(unsigned gid, unsigned metricsId);
    void WriteGlyphTable(OutputStream& output);
    void WriteHmtxTable(OutputStream& output);
    void WriteLocaTable(OutputStream& output);
    void WriteTables(std::string& buffer);
    void ReadGlyphCompoundData(GlyphCompoundData& data, unsigned offset);

    struct GIDInfo
    {
        unsigned Id;
        LongHorMetrics Metrics;
    };

private:
    InputStreamDevice* m_device;
    const PdfFontMetrics* m_metrics;

    bool m_isLongLoca;
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

};

#endif // PDF_FONT_TRUE_TYPE_H

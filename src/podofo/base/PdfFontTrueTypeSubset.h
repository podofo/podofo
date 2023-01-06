/**
 * SPDX-FileCopyrightText: (C) 2008 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FONT_TTF_SUBSET_H
#define PDF_FONT_TTF_SUBSET_H

#include "PdfDeclarations.h"
#include "PdfFontMetrics.h"

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

using GIDList = cspan<unsigned>;

/**
 * This class is able to build a new TTF font with only
 * certain glyphs from an existing font.
 *
 */
class PODOFO_API PdfFontTrueTypeSubset final
{
private:
    PdfFontTrueTypeSubset(InputStreamDevice& device);

public:
    /**
     * Actually generate the subsetted font
     * Create a new PdfFontTrueTypeSubset from an existing
     * TTF font file retrived from a font metrics
     *
     * \param output write the font to this buffer
     * \param metrics font metrics object for this font
     * \param gidList a list of gids to load
     */
    static void BuildFont(std::string& output, const PdfFontMetrics& metrics,
        const GIDList& gidList);

private:
    PdfFontTrueTypeSubset(const PdfFontTrueTypeSubset& rhs) = delete;
    PdfFontTrueTypeSubset& operator=(const PdfFontTrueTypeSubset& rhs) = delete;

    void BuildFont(std::string& buffer, const GIDList& gidList);

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
        // Used internaly during recursive load
        int16_t ContourCount = 0;
    };

    struct GlyphCompoundData
    {
        unsigned Flags;
        unsigned GlyphIndex;
    };

    void LoadGlyphs(GlyphContext& ctx, const GIDList& gidList);
    void LoadGID(GlyphContext& ctx, unsigned gid);
    void LoadCompound(GlyphContext& ctx, const GlyphData& data);
    void WriteGlyphTable(OutputStream& output);
    void WriteHmtxTable(OutputStream& output);
    void WriteLocaTable(OutputStream& output);
    void WriteTables(std::string& buffer);
    void ReadGlyphCompoundData(GlyphCompoundData& data, unsigned offset);

private:
    InputStreamDevice* m_device;

    bool m_isLongLoca;
    uint16_t m_glyphCount;
    uint16_t m_HMetricsCount;
    uint16_t m_HMetricsCountNew;

    std::vector<TrueTypeTable> m_tables;
    GlyphDatas m_glyphDatas;
    std::vector<unsigned> m_orderedGIDs; // Ordered list of original GIDs as they will appear in the subset
    charbuff m_tmpBuffer;
};

};

#endif // PDF_FONT_TRUE_TYPE_H

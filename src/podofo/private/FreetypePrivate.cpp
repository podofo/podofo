/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include "PdfDeclarationsPrivate.h"
#include "FreetypePrivate.h"
#include FT_TRUETYPE_TAGS_H
#include FT_TRUETYPE_TABLES_H
#include FT_FONT_FORMATS_H

using namespace std;
using namespace PoDoFo;

constexpr unsigned TableDirectoryFixedSize = 12;

namespace
{
    struct TTCF_Header
    {
        uint32_t ttcTag;
        uint16_t majorVersion;
        uint16_t minorVersion;
        uint32_t numFonts;
    };

    struct TT_TableHeader
    {
        uint32_t tableTag;
        uint32_t checksum;
        uint32_t offset;
        uint32_t length;
    };

    struct TableInfo
    {
        FT_ULong Tag;
        FT_ULong Size;
    };
}

static PdfFontFileType determineTrueTypeFormat(FT_Face face);
static unsigned determineFaceSize(FT_Face face, vector<TableInfo>& tables, unsigned& tableDirSize);
static FT_Face createFaceFromBuffer(const bufferview& view, unsigned faceIndex);
static bool tryExtractDataFromTTC(FT_Face face, charbuff& buffer);
static void getDataFromFace(FT_Face face, charbuff& buffer);

FT_Library FT::GetLibrary()
{
    struct Init
    {
        Init() : Library(nullptr)
        {
            // Initialize all the fonts stuff
            if (FT_Init_FreeType(&Library))
                PODOFO_RAISE_ERROR(PdfErrorCode::FreeType);
        }

        ~Init()
        {

            FT_Done_FreeType(Library);
        }

        FT_Library Library;     // Handle to the freetype library
    };

    static Init init;
    return init.Library;
}

FT_Face FT::CreateFaceFromBuffer(const bufferview& view, unsigned faceIndex,
    charbuff& buffer)
{
    // Extract data and re-create the face
    unique_ptr<struct FT_FaceRec_, decltype(&FT_Done_Face)> face(createFaceFromBuffer(view, faceIndex), FT_Done_Face);
    if (tryExtractDataFromTTC(face.get(), buffer))
    {
        return createFaceFromBuffer(buffer, 0);
    }
    else
    {
        buffer = view;
        return face.release();
    }
}

// No check for TTC fonts
FT_Face FT::CreateFaceFromBuffer(const bufferview& view)
{
    return createFaceFromBuffer(view, 0);
}

FT_Face FT::CreateFaceFromFile(const string_view& filepath, unsigned faceIndex,
    charbuff& buffer)
{
    FT_Error rc;
    FT_Face face_;
    rc = FT_New_Face(FT::GetLibrary(), filepath.data(), faceIndex, &face_);
    if (rc != 0)
        return nullptr;

    // Extract data and re-create the face
    unique_ptr<struct FT_FaceRec_, decltype(&FT_Done_Face)> face(face_, FT_Done_Face);
    if (tryExtractDataFromTTC(face.get(), buffer))
    {
        return createFaceFromBuffer(buffer, 0);
    }
    else
    {
        utls::ReadTo(buffer, filepath);
        return face.release();
    }
}

charbuff FT::GetDataFromFace(FT_Face face)
{
    charbuff buffer;
    if (!tryExtractDataFromTTC(face, buffer))
        getDataFromFace(face, buffer);

    return buffer;
}

bool FT::TryGetFontFileFormat(FT_Face face, PdfFontFileType& format)
{
    string_view formatstr = FT_Get_Font_Format(face);
    if (formatstr == "TrueType")
        format = determineTrueTypeFormat(face);
    else if (formatstr == "Type 1")
        format = PdfFontFileType::Type1;
    else if (formatstr == "CID Type 1")
        format = PdfFontFileType::CIDType1;
    else if (formatstr == "CFF")
        format = PdfFontFileType::Type1CCF;
    else
    {
        format = PdfFontFileType::Unknown;
        return false;
    }

    return true;
}

bool FT::IsPdfSupported(FT_Face face)
{
    PdfFontFileType format;
    if (!FT::TryGetFontFileFormat(face, format) ||
        !(format == PdfFontFileType::TrueType || format == PdfFontFileType::OpenType))
    {
        return false;
    }

    return true;
}

FT_Face createFaceFromBuffer(const bufferview& view, unsigned faceIndex)
{
    FT_Error rc;
    FT_Open_Args openArgs{ };
    // NOTE: Data is not copied
    // https://freetype.org/freetype2/docs/reference/ft2-base_interface.html#ft_open_args
    openArgs.flags = FT_OPEN_MEMORY;
    openArgs.memory_base = (const FT_Byte*)view.data();
    openArgs.memory_size = (FT_Long)view.size();

    FT_Face face;
    rc = FT_Open_Face(FT::GetLibrary(), &openArgs, faceIndex, &face);
    if (rc != 0)
        return nullptr;

    return face;
}

// Try to handle TTC font collections
bool tryExtractDataFromTTC(FT_Face face, charbuff& buffer)
{
    FT_Error rc;
    FT_ULong size;

    uint32_t tag;
    size = sizeof(FT_ULong);
    rc = FT_Load_Sfnt_Table(face, 0, 0, (FT_Byte*)&tag, &size);
    if (rc == 0 || FROM_BIG_ENDIAN(tag) != TTAG_ttcf)
        return false;

    // First read the TTC font header and then determine the face offset
    TTCF_Header header;
    size = sizeof(TTCF_Header);
    rc = FT_Load_Sfnt_Table(face, 0, 0, (FT_Byte*)&header, &size);
    CHECK_FT_RC(rc, FT_Load_Sfnt_Table);
    unsigned numFonts = FROM_BIG_ENDIAN(header.numFonts);
    vector<uint32_t> offsets(numFonts);
    size = numFonts * sizeof(uint32_t);
    rc = FT_Load_Sfnt_Table(face, 0, sizeof(TTCF_Header), (FT_Byte*)offsets.data(), &size);
    CHECK_FT_RC(rc, FT_Load_Sfnt_Table);
    uint32_t faceOffset = FROM_BIG_ENDIAN(offsets[face->face_index]);

    // Prepare the final buffer
    vector<TableInfo> tables;
    unsigned tableDirSize;
    buffer.resize(determineFaceSize(face, tables, tableDirSize));

    // Read the Table Directory with an absolute offset
    size = tableDirSize;
    rc = FT_Load_Sfnt_Table(face, 0, faceOffset, (FT_Byte*)buffer.data(), &size);
    CHECK_FT_RC(rc, FT_Load_Sfnt_Table);

    auto tableRecords = (TT_TableHeader*)(buffer.data() + TableDirectoryFixedSize);
    unsigned offset = tableDirSize;
    for (FT_ULong i = 0; i < tables.size(); i++)
    {
        // Read all the tables
        auto& table = tables[i];
        size = table.Size;
        rc = FT_Load_Sfnt_Table(face, table.Tag, 0, (FT_Byte*)buffer.data() + offset, &size);
        CHECK_FT_RC(rc, FT_Load_Sfnt_Table);

        // Fix the table offset in the table directory
        tableRecords[i].offset = AS_BIG_ENDIAN(offset);
        offset += table.Size;
    }

    return true;
}

// Get font data accessing whole file
// TODO: Make it working for all font types, not only TTF
void getDataFromFace(FT_Face face, charbuff& buffer)
{
    FT_Error rc;
    FT_ULong size = 0;

    // https://freetype.org/freetype2/docs/reference/ft2-truetype_tables.html#ft_load_sfnt_table
    // Use value 0 if you want to access the whole font file

    // Just read the whole font
    rc = FT_Load_Sfnt_Table(face, 0, 0, nullptr, &size);
    CHECK_FT_RC(rc, FT_Load_Sfnt_Table);

    buffer.resize(size);
    rc = FT_Load_Sfnt_Table(face, 0, 0, (FT_Byte*)buffer.data(), &size);
    CHECK_FT_RC(rc, FT_Load_Sfnt_Table);
}

// Determines if the font is legacy TrueType or OpenType
PdfFontFileType determineTrueTypeFormat(FT_Face face)
{
    FT_Error rc;
    FT_ULong size;
    FT_ULong tag;
    rc = FT_Sfnt_Table_Info(face, 0, nullptr, &size);
    CHECK_FT_RC(rc, FT_Sfnt_Table_Info);
    for (FT_ULong i = 0, count = size; i < count; i++)
    {
        rc = FT_Sfnt_Table_Info(face, i, &tag, &size);
        CHECK_FT_RC(rc, FT_Sfnt_Table_Info);
        switch (tag)
        {
            // Legacy TrueType tables
            // https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6.html
            case TTAG_acnt:
            case TTAG_ankr:
            case TTAG_avar:
            case TTAG_bdat:
            case TTAG_bhed:
            case TTAG_bloc:
            case TTAG_bsln:
            case TTAG_cmap:
            case TTAG_cvar:
            case TTAG_cvt:
            case TTAG_EBSC:
            case TTAG_fdsc:
            case TTAG_feat:
            case TTAG_fmtx:
            case TTAG_fond:
            case TTAG_fpgm:
            case TTAG_fvar:
            case TTAG_gasp:
            case TTAG_gcid:
            case TTAG_glyf:
            case TTAG_gvar:
            case TTAG_hdmx:
            case TTAG_head:
            case TTAG_hhea:
            case TTAG_hmtx:
            case TTAG_just:
            case TTAG_kern:
            case TTAG_kerx:
            case TTAG_lcar:
            case TTAG_loca:
            case TTAG_ltag:
            case TTAG_maxp:
            case TTAG_meta:
            case TTAG_mort:
            case TTAG_morx:
            case TTAG_name:
            case TTAG_opbd:
            case TTAG_OS2:
            case TTAG_post:
            case TTAG_prep:
            case TTAG_prop:
            case TTAG_sbix:
            case TTAG_trak:
            case TTAG_vhea:
            case TTAG_vmtx:
            case TTAG_xref:
            case TTAG_Zapf:
                // Continue on legacy tables
                break;
            default:
                // Return OpenType on all other tables
                return PdfFontFileType::OpenType;
        }
    }

    // Default legay TrueType
    return PdfFontFileType::TrueType;
}

unsigned determineFaceSize(FT_Face face, vector<TableInfo>& tables, unsigned& tableDirSize)
{
    FT_Error rc;
    FT_ULong size;

    rc = FT_Sfnt_Table_Info(face, 0, nullptr, &size);
    CHECK_FT_RC(rc, FT_Sfnt_Table_Info);

    unsigned faceSize = TableDirectoryFixedSize + (sizeof(TT_TableHeader) * size);
    tableDirSize = faceSize;
    tables.resize(size);
    for (FT_ULong i = 0; i < size; i++)
    {
        auto& table = tables[i];
        rc = FT_Sfnt_Table_Info(face, i, &table.Tag, &table.Size);
        CHECK_FT_RC(rc, FT_Sfnt_Table_Info);
        faceSize += table.Size;
    }

    return faceSize;
}

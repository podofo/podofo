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

static PdfFontFileType determineTrueTypeFormat(FT_Face face);

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

bool FT::TryCreateFaceFromBuffer(const bufferview& view, FT_Face& face)
{
    return TryCreateFaceFromBuffer(view, 0, face);
}

bool FT::TryCreateFaceFromBuffer(const bufferview& view, unsigned faceIndex, FT_Face& face)
{
    FT_Error rc;
    FT_Open_Args openArgs{ };
    // NOTE: Data is not copied
    // https://freetype.org/freetype2/docs/reference/ft2-base_interface.html#ft_open_args
    openArgs.flags = FT_OPEN_MEMORY;
    openArgs.memory_base = (const FT_Byte*)view.data();
    openArgs.memory_size = (FT_Long)view.size();

    rc = FT_Open_Face(FT::GetLibrary(), &openArgs, faceIndex, &face);
    if (rc != 0)
    {
        face = nullptr;
        return false;
    }

    return true;
}

FT_Face FT::CreateFaceFromBuffer(const bufferview& view, unsigned faceIndex)
{
    FT_Face face;
    if (!TryCreateFaceFromBuffer(view, faceIndex, face))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::FreeType, "Error loading FreeType face");

    return face;
}

bool FT::TryCreateFaceFromFile(const string_view& filepath, FT_Face& face)
{
    return TryCreateFaceFromFile(filepath, 0, face);
}

bool FT::TryCreateFaceFromFile(const string_view& filepath, unsigned faceIndex, FT_Face& face)
{
    FT_Error rc;
    unique_ptr<charbuff> buffer;
    rc = FT_New_Face(FT::GetLibrary(), filepath.data(), faceIndex, &face);
    if (rc != 0)
    {
        face = nullptr;
        return false;
    }

    return true;
}

FT_Face FT::CreateFaceFromFile(const string_view& filepath, unsigned faceIndex)
{
    FT_Face face;
    if (!TryCreateFaceFromFile(filepath, faceIndex, face))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::FreeType, "Error loading FreeType face");

    return face;
}

charbuff FT::GetDataFromFace(FT_Face face)
{
    FT_Error rc;

    // https://freetype.org/freetype2/docs/reference/ft2-truetype_tables.html#ft_load_sfnt_table
    // Use value 0 if you want to access the whole font file
    FT_ULong size = 0;
    rc = FT_Load_Sfnt_Table(face, 0, 0, nullptr, &size);
    CHECK_FT_RC(rc, FT_Load_Sfnt_Table);

    charbuff buffer(size);
    rc = FT_Load_Sfnt_Table(face, 0, 0, (FT_Byte*)buffer.data(), &size);
    CHECK_FT_RC(rc, FT_Load_Sfnt_Table);
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

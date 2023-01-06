/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include "PdfDeclarationsPrivate.h"
#include "FreetypePrivate.h"
#include FT_TRUETYPE_TABLES_H

using namespace PoDoFo;

FT_Library PoDoFo::GetFreeTypeLibrary()
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

bool PoDoFo::TryCreateFreeTypeFace(const bufferview& view, FT_Face& face)
{
    FT_Error rc;
    FT_Open_Args openArgs{ };
    // NOTE: Data is not copied
    // https://freetype.org/freetype2/docs/reference/ft2-base_interface.html#ft_open_args
    openArgs.flags = FT_OPEN_MEMORY;
    openArgs.memory_base = (const FT_Byte*)view.data();
    openArgs.memory_size = (FT_Long)view.size();

    rc = FT_Open_Face(PoDoFo::GetFreeTypeLibrary(), &openArgs, 0, &face);
    if (rc != 0)
    {
        face = nullptr;
        return false;
    }

    return true;
}

FT_Face PoDoFo::CreateFreeTypeFace(const bufferview& view)
{
    FT_Face face;
    if (!TryCreateFreeTypeFace(view, face))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::FreeType, "Error loading FreeType face");

    return face;
}

charbuff PoDoFo::GetDataFromFace(FT_Face face)
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

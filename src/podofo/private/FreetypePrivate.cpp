/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include "PdfDeclarationsPrivate.h"
#include "FreetypePrivate.h"
#include FT_TRUETYPE_TABLES_H

using namespace std;
using namespace PoDoFo;

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

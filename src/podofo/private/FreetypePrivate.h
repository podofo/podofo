/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PODOFO_FREETYPE_PRIVATE_H
#define PODOFO_FREETYPE_PRIVATE_H

// Old freetype versions requires <ft2build.h> to be included first
#include <ft2build.h>
#include FT_FREETYPE_H

#include <podofo/main/PdfDeclarations.h>

#define CHECK_FT_RC(rc, func) if (rc != 0)\
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::FreeTypeError, "Function " #func " failed")

namespace FT
{
    FT_Library GetLibrary();
    /**
     * \param buffer a copy of the buffer from which the face will be loaded.
     * It must be retained
     */
    FT_Face CreateFaceFromFile(const std::string_view& filepath, unsigned faceIndex,
        PoDoFo::charbuff& buffer);
    /**
     * \param buffer a copy of the buffer from which the face will be loaded.
     * It must be retained
     */
    FT_Face CreateFaceFromBuffer(const PoDoFo::bufferview& view, unsigned faceIndex,
        PoDoFo::charbuff& buffer);
    // Extract a CFF table from a OpenType CFF font
    FT_Face ExtractCFFFont(FT_Face face, PoDoFo::charbuff& buffer);
    // No check for TTC fonts
    FT_Face CreateFaceFromBuffer(const PoDoFo::bufferview& view);
    PoDoFo::charbuff GetDataFromFace(FT_Face face);
    bool TryGetFontFileFormat(FT_Face face, PoDoFo::PdfFontFileType& format);
    bool IsPdfSupported(FT_Face face);
    std::unordered_map<std::string_view, unsigned> GetPostMap(FT_Face face);
}

// Other legacy TrueType tables defined in Apple documentation
// https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6.html
#define TTAG_acnt FT_MAKE_TAG('a', 'c', 'n', 't')
#define TTAG_ankr FT_MAKE_TAG('a', 'n', 'k', 'r')
#define TTAG_kerx FT_MAKE_TAG('k', 'e', 'r', 'x')
#define TTAG_fdsc FT_MAKE_TAG('f', 'd', 's', 'c')
#define TTAG_fmtx FT_MAKE_TAG('f', 'm', 't', 'x')
#define TTAG_fond FT_MAKE_TAG('f', 'o', 'n', 'd')
#define TTAG_gcid FT_MAKE_TAG('g', 'c', 'i', 'd')
#define TTAG_ltag FT_MAKE_TAG('l', 't', 'a', 'g')
#define TTAG_meta FT_MAKE_TAG('m', 'e', 't', 'a')
#define TTAG_xref FT_MAKE_TAG('x', 'r', 'e', 'f')
#define TTAG_Zapf FT_MAKE_TAG('Z', 'a', 'p', 'f')

#endif // PODOFO_FREETYPE_PRIVATE_H

/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include "PdfDeclarationsPrivate.h"
#include "FreetypePrivate.h"

#include <atomic>

#include FT_TRUETYPE_TAGS_H
#include FT_TRUETYPE_TABLES_H
#include FT_FONT_FORMATS_H
#include FT_CID_H

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

    // A manually handled FreeType smart pointer with
    // automatically accessed reference counter. It's
    // initialized with count 1
    struct FT_LibraryPtr
    {
        FT_Library Ptr = nullptr;

        void Ref();
        void Unref();

    private:
        std::atomic<size_t> m_count = 1;
    };
}

static PdfFontFileType determineFormatCFF(FT_Face face);
static unsigned determineFaceSize(FT_Face face, vector<TableInfo>& tables, unsigned& tableDirSize);
static FT::FT_FacePtr createFaceFromBuffer(const bufferview& view, unsigned faceIndex, bool bare = false);
static bool isTTCFont(FT_Face face);
static bool isTTCFont(const bufferview& face);
static bool tryExtractDataFromTTC(FT_Face face, charbuff& buffer);
static void getDataFromFace(FT_Face face, charbuff& buffer);
static FT_LibraryPtr& getLibrary();
static void freeFaceBare(FT_Face face);
static void noOpFace(FT_Face face);

FT::FT_FacePtr FT::CreateFaceFromBuffer(const bufferview& view, unsigned faceIndex,
    charbuff& buffer)
{
    if (isTTCFont(view))
    {
        auto face = createFaceFromBuffer(view, faceIndex, true);
        // Try to extract data from the TTC font, or just copy
        // existing view to buffer if it fails
        if (!tryExtractDataFromTTC(face.get(), buffer))
            buffer = view;

        // Unconditionally re-create the face from the copied buffer
        return createFaceFromBuffer(buffer, 0);
    }
    else
    {
        buffer = view;
        return createFaceFromBuffer(buffer, 0);
    }
}

FT::FT_FacePtr FT::ExtractCFFFont(FT_Face face, charbuff& buffer)
{
    FT_ULong size = 0;
    FT_Error rc = FT_Load_Sfnt_Table(face, TTAG_CFF, 0, nullptr, &size);
    CHECK_FT_RC(rc, FT_Load_Sfnt_Table);
    buffer.resize(size);
    rc = FT_Load_Sfnt_Table(face, TTAG_CFF, 0, (FT_Byte*)buffer.data(), &size);
    return createFaceFromBuffer(buffer, 0);
}

// No check for TTC fonts
FT::FT_FacePtr FT::CreateFaceFromBuffer(const bufferview& view)
{
    return createFaceFromBuffer(view, 0);
}

void FT::FreeFace(FT_Face face)
{
    if (face == nullptr)
        return;

    // NOTE: Store the hidden library pointer before freeing the face
    auto lib = static_cast<FT_LibraryPtr*>(face->extensions);

    FT_Done_Face(face);

    // Dereference the hidden library pointer
    lib->Unref();
}

void FT::ReferenceFace(FT_Face face)
{
    // Reference the hidden library pointer
    static_cast<FT_LibraryPtr*>(face->extensions)->Ref();
    FT_Reference_Face(face);
}

FT::FT_FacePtr FT::CreateFaceFromFile(const string_view& filepath, unsigned faceIndex,
    charbuff& buffer)
{
    utls::ReadTo(buffer, filepath, sizeof(TTAG_ttcf));
    if (isTTCFont(buffer))
    {
        auto& lib = getLibrary();
        FT_Error rc;
        FT_Face face;
        rc = FT_New_Face(lib.Ptr, filepath.data(), faceIndex, &face);
        if (rc != 0)
            return FT_FacePtr(nullptr, FT::FreeFace);

        unique_ptr<struct FT_FaceRec_, decltype(&FT_Done_Face)> face_(face, FT_Done_Face);

        // Try to extract data from the TTC font and re-create the face
        if (tryExtractDataFromTTC(face, buffer))
            return createFaceFromBuffer(buffer, 0);
    }

    // Unconditionally copy the font file and create
    // the face from the copied buffer
    utls::ReadTo(buffer, filepath);
    return createFaceFromBuffer(buffer, 0);
}

charbuff FT::GetDataFromFace(FT_Face face)
{
    charbuff buffer;
    if (!isTTCFont(face) || !tryExtractDataFromTTC(face, buffer))
        getDataFromFace(face, buffer);

    return buffer;
}

bool FT::TryGetFontFileFormat(FT_Face face, PdfFontFileType& format)
{
    string_view formatstr = FT_Get_Font_Format(face);
    if (formatstr == "TrueType")
    {
        format = PdfFontFileType::TrueType;
    }
    else if (formatstr == "Type 1")
    {
        format = PdfFontFileType::Type1;
    }
    else if (formatstr == "CID Type 1")
    {
        // CID Type 1 fonts are a special PostScript font that are described
        // in "Adobe Technical Note #5014, Adobe CMap and CIDFont Files
        // Specification". The CIDFont format described there does not
        // seems to be directly supported by PDF, and in ISO 32000-2:2020
        // comments in this way "As mentioned earlier, PDF does not support
        // the entire CID - keyed font architecture, which is independent
        // of PDF; CID - keyed fonts may be used in other environments".
        // See also https://github.com/pdf-association/pdf-issues/issues/497
        format = PdfFontFileType::Unknown;
        return false;
    }
    else if (formatstr == "CFF")
    {
        format = determineFormatCFF(face);
    }
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
    if (!FT::TryGetFontFileFormat(face, format))
        return false;

    return true;
}

unordered_map<string_view, unsigned> FT::GetPostMap(FT_Face face)
{
    unordered_map<string_view, unsigned> ret;
    if (!FT_HAS_GLYPH_NAMES(face))
        return ret;

    FT_Error rc;
    char buffer[64];
    for (FT_Long i = 0; i < face->num_glyphs; i++)
    {
        rc = FT_Get_Glyph_Name(face, (FT_UInt)i, buffer, (FT_UInt)std::size(buffer));
        if (rc != 0)
            continue;

        ret[string_view(buffer)] = (FT_UInt)i;
    }

    return ret;
}

/// <param name="bare">True if the smart pointer </param>
FT::FT_FacePtr createFaceFromBuffer(const bufferview& view, unsigned faceIndex, bool bare)
{
    FT_Error rc;
    FT_Open_Args openArgs{ };
    // NOTE: Data is not copied
    // https://freetype.org/freetype2/docs/reference/ft2-base_interface.html#ft_open_args
    openArgs.flags = FT_OPEN_MEMORY;
    openArgs.memory_base = (const FT_Byte*)view.data();
    openArgs.memory_size = (FT_Long)view.size();

    auto& lib = getLibrary();
    FT_Face face;
    rc = FT_Open_Face(lib.Ptr, &openArgs, faceIndex, &face);
    if (rc != 0)
        return FT::FT_FacePtr(nullptr, noOpFace);

    if (bare)
    {
        // Don't install any extension
        return FT::FT_FacePtr(face, freeFaceBare);
    }
    else
    {
        // Install an extension that binds the library
        // to the lifetime of this face. It will be
        // dereferenced with FT::FreeFace
        lib.Ref();
        face->extensions = &lib;
        return FT::FT_FacePtr(face, FT::FreeFace);
    }
}

bool isTTCFont(FT_Face face)
{
    FT_Error rc;
    FT_ULong size;

    uint32_t tag;
    size = sizeof(uint32_t);
    rc = FT_Load_Sfnt_Table(face, 0, 0, (FT_Byte*)&tag, &size);
    if (rc == 0 && FROM_BIG_ENDIAN(tag) == TTAG_ttcf)
        return true;

    return false;
}

bool isTTCFont(const bufferview& face)
{
    uint32_t tag;
    if (face.size() < sizeof(tag))
        return false;

    std::memcpy(&tag, face.data(), sizeof(tag));
    if (FROM_BIG_ENDIAN(tag) == TTAG_ttcf)
        return true;

    return false;
}

// Try to handle TTC font collections
bool tryExtractDataFromTTC(FT_Face face, charbuff& buffer)
{
    FT_Error rc;
    FT_ULong size;

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
    if (face->face_index < 0 || static_cast<size_t>(face->face_index) >= offsets.size())
        return false;

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

// Determines if the font is a CCF table with an OTF container or not
PdfFontFileType determineFormatCFF(FT_Face face)
{
    FT_Error rc;
    FT_ULong size;
    rc = FT_Sfnt_Table_Info(face, 0, nullptr, &size);
    if (rc == 0)
    {
        return PdfFontFileType::OpenTypeCFF;
    }
    else
    {
        // NOTE: Technical Note #5176 "The Compact Font Format Specification"
        // says "The Top DICT begins with the SyntheticBase and ROS operators
        // for synthetic and CIDFonts, respectively. Regular Type 1 fonts begin
        // with some other operator. (This permits the determination of the
        // kind of font without parsing the entire Top DICT)". We assume FreeType
        // is able to make this distinction using the FT_IS_CID_KEYED macro

        FT_Bool isCid = 0;
        (void)FT_Get_CID_Is_Internally_CID_Keyed(face, &isCid);
        if (isCid == 1)
            return PdfFontFileType::CIDKeyedCFF;
        else
            return PdfFontFileType::Type1CFF;
    }
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

FT_LibraryPtr& getLibrary()
{
    struct Init
    {
        Init()
        {
            // Initialize all the fonts stuff
            if (FT_Init_FreeType(&Library.Ptr))
                PODOFO_RAISE_ERROR(PdfErrorCode::FreeTypeError);
        }

        ~Init()
        {
            // Dereference the library on thread exit
            Library.Unref();
        }

        FT_LibraryPtr Library;     // Handle to the freetype library
    };

    thread_local Init init;
    return init.Library;
}

void freeFaceBare(FT_Face face)
{
    // Just call the bare FT free face function
    (void)FT_Done_Face(face);
}

void noOpFace(FT_Face face)
{
    (void)face;
    // Do nothing
}

void FT_LibraryPtr::Ref()
{
    // It's a normal counter that just get incremented and
    // decremented, so we can relax the memory order
    size_t prev = m_count.fetch_add(1, std::memory_order_relaxed);
    PODOFO_ASSERT(prev != 0);
    (void)prev;
}
void FT_LibraryPtr::Unref()
{
    // It's a normal counter that just get incremented and
    // decremented, so we can relax the memory order
    auto prev = m_count.fetch_sub(1, std::memory_order_relaxed);
    PODOFO_ASSERT(prev != 0);
    if (prev == 1)
        (void)FT_Done_FreeType(Ptr);
}

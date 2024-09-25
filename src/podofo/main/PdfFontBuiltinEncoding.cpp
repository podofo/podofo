/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontMetricsObject.h"
#include <podofo/private/FreetypePrivate.h>

#include "PdfIdentityEncoding.h"

using namespace std;
using namespace PoDoFo;

namespace
{
    /** A built-in encoding for a /Type1 font program
     */
    class PODOFO_API PdfFontBuiltinType1Encoding final : public PdfEncodingMapBase
    {
    public:
        PdfFontBuiltinType1Encoding(PdfCharCodeMap && map)
            : PdfEncodingMapBase(std::move(map), PdfEncodingMapType::Simple) { }

    protected:
        void getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const override
        {
            (void)objects;
            (void)name;
            (void)obj;
            // Do nothing. encoding is implicit in the font program
        }
    };
}

PdfEncodingMapConstPtr PdfFontMetrics::getFontType1ImplicitEncoding(FT_Face face)
{
    PdfCharCodeMap codeMap;
    FT_Error rc;
    FT_ULong code;
    FT_UInt index;

    auto oldCharmap = face->charmap;

    // NOTE: Unicode maps may have multiple mappings to same glyphs,
    // hence we prefer multimap over map here
    multimap<FT_UInt, FT_ULong> unicodeMap;
    rc = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    if (rc == 0)
    {
        // If an Unicode compatible charmap is found,
        // create a an unicode map
        code = FT_Get_First_Char(face, &index);
        while (index != 0)
        {
            unicodeMap.insert({ index, code });
            code = FT_Get_Next_Char(face, code, &index);
        }
    }

    // Search for a custom char map that will defines actual CIDs.
    // NOTE: It may be have different size than the unicode map,
    // but if a Type1 implicit encoding is required we assume the
    // PDF will reference just these CIDs
    map<FT_UInt, FT_ULong> customMap;
    rc = FT_Select_Charmap(face, FT_ENCODING_ADOBE_CUSTOM);
    if (rc == 0)
    {
        code = FT_Get_First_Char(face, &index);
        while (index != 0)
        {
            customMap[index] = code;
            code = FT_Get_Next_Char(face, code, &index);
        }

        // NOTE: Initial charmap may be null
        if (oldCharmap != nullptr)
        {
            rc = FT_Set_Charmap(face, oldCharmap);
            CHECK_FT_RC(rc, FT_Select_Charmap);
        }

        // Map CIDs to Unicode code points
        for (auto& pair : customMap)
        {
            // NOTE: Because the unicode map is a multiple entries associative
            // container, we assume the first mapping found is the correct one
            // and that the PDF will reference only CIDs defined in the custom map
            auto found = unicodeMap.find(pair.first);
            if (found == unicodeMap.end())
            {
                // Some symbol characters may have no unicode representation
                codeMap.PushMapping(PdfCharCode((unsigned)pair.second), U'\0');
                continue;
            }

            codeMap.PushMapping(PdfCharCode((unsigned)pair.second), (char32_t)found->second);
        }
    }
    else
    {
        // NOTE: Some very strange CFF fonts just supply an unicode map
        // For these, we just assume code identity with Unicode codepoint
        for (auto& pair : unicodeMap)
            codeMap.PushMapping(PdfCharCode((unsigned)pair.second), (char32_t)pair.second);
    }

    return PdfEncodingMapConstPtr(new PdfFontBuiltinType1Encoding(std::move(codeMap)));
}

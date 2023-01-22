/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_CID_TO_GID_MAP_H
#define PDF_CID_TO_GID_MAP_H

#include "PdfDeclarations.h"
#include "PdfObject.h"

namespace PoDoFo
{
    /** Helper class to handle the /CIDToGIDMap entry in a Type2 CID font
     * or /TrueType fonts implicit CID to GID mapping
     */
    class PdfCIDToGIDMap final
    {
    public:
        using iterator = CIDToGIDMap::const_iterator;

    public:
        PdfCIDToGIDMap(CIDToGIDMap&& map, PdfGlyphAccess access);
        PdfCIDToGIDMap(const PdfCIDToGIDMap&) = default;
        PdfCIDToGIDMap(PdfCIDToGIDMap&&) noexcept = default;

        static PdfCIDToGIDMap Create(const PdfObject& cidToGidMapObj, PdfGlyphAccess access);

    public:
        bool TryMapCIDToGID(unsigned cid, unsigned& gid) const;
        void ExportTo(PdfObject& descendantFont);

        /** Determines if the current map provides the queried glyph access
         */
        bool HasGlyphAccess(PdfGlyphAccess access) const;

    public:
        unsigned GetSize() const;
        iterator begin() const;
        iterator end() const;

    private:
        CIDToGIDMap m_cidToGidMap;
        PdfGlyphAccess m_access;
    };

    using PdfCIDToGIDMapConstPtr = std::shared_ptr<const PdfCIDToGIDMap>;
}

#endif // PDF_CID_TO_GID_MAP_H

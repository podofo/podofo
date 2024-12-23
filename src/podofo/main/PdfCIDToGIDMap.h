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
    /** A backing storage for a CID to GID map
     * \remarks It must preserve ordering
     */
    using CIDToGIDMap = std::map<unsigned, unsigned>;

    /** Helper class to handle the /CIDToGIDMap entry in a Type2 CID font
     * or /TrueType fonts implicit CID to GID mapping
     */
    class PdfCIDToGIDMap final
    {
    public:
        using iterator = CIDToGIDMap::const_iterator;

    public:
        PdfCIDToGIDMap(CIDToGIDMap&& map);
        PdfCIDToGIDMap(const PdfCIDToGIDMap&) = default;
        PdfCIDToGIDMap(PdfCIDToGIDMap&&) noexcept = default;

        static PdfCIDToGIDMap Create(const PdfObject& cidToGidMapObj);

    public:
        bool TryMapCIDToGID(unsigned cid, unsigned& gid) const;
        void ExportTo(PdfObject& descendantFont);

    public:
        unsigned GetSize() const;
        iterator begin() const;
        iterator end() const;

    private:
        CIDToGIDMap m_cidToGidMap;
    };

    using PdfCIDToGIDMapConstPtr = std::shared_ptr<const PdfCIDToGIDMap>;
}

#endif // PDF_CID_TO_GID_MAP_H

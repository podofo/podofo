/**
 * SPDX-FileCopyrightText: (C) 2009 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once
#ifndef PDF_XREF_ENTRY_H
#define PDF_XREF_ENTRY_H

#include <podofo/main/PdfDeclarations.h>

namespace PoDoFo
{
    // Values cast directly to XRefStm binary representation
    enum class PdfXRefEntryType : int8_t
    {
        Unknown = -1,
        Free = 0,
        InUse = 1,
        Compressed = 2,
    };

    struct PdfXRefEntry final
    {
        PdfXRefEntry();

        static PdfXRefEntry CreateFree(uint32_t object, uint16_t generation);

        static PdfXRefEntry CreateInUse(uint64_t offset, uint16_t generation);

        static PdfXRefEntry CreateCompressed(uint32_t object, unsigned index);

        // The following aliasing should be allowed in C++
        // https://stackoverflow.com/a/15278030/213871
        union
        {
            uint64_t ObjectNumber;  // Object number in Free and Compressed entries
            uint64_t Offset;        // Unused in InUse entries
            uint64_t Unknown1;
        };

        union
        {
            uint32_t Generation;    // The generation of the object in Free and InUse entries
            uint32_t Index;         // Index of the object in the stream for Compressed entries
            uint32_t Unknown2;
        };
        PdfXRefEntryType Type;
        bool Parsed;
    };

    class PdfXRefEntries final
    {
    public:
        unsigned GetSize() const;

        /** Resize the internal entries structure in a safe manner, only if needed
         *  The limit for the maximum number of indirect objects in a PDF file is checked by this method.
         *  The maximum is 2^23-1 (8.388.607).
         *
         *  \param newSize new size of the vector. It's in64_t to detect possible overflows
         */
        void Enlarge(unsigned newSize);
        void Clear();
        PdfXRefEntry& operator[](unsigned index);
        const PdfXRefEntry& operator[](unsigned index) const;
    private:
        std::vector<PdfXRefEntry> m_entries;
    };
};

#endif // PDF_XREF_ENTRY_H

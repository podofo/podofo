/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_XREF_STREAM_H
#define PDF_XREF_STREAM_H

#include <podofo/main/PdfArray.h>
#include "PdfXRef.h"

namespace PoDoFo {

/**
 * Creates an XRef table that is a stream object.
 * Requires at least PDF 1.5. XRef streams are more
 * compact than normal XRef tables.
 *
 * This is an internal class of PoDoFo used by PdfWriter.
 */
class PdfXRefStream final : public PdfXRef
{
    friend class PdfWriter;
    friend class PdfImmediateWriter;

private:
    /** Create a new XRef table
     *
     *  \param writer is needed to fill the trailer directory
     *                 correctly which is included into the XRef
     *  \param parent a vector of PdfObject is required
     *                 to create a PdfObject for the XRef
     */
    PdfXRefStream(PdfWriter& writer);

public:
    uint64_t GetOffset() const override;

    bool ShouldSkipWrite(const PdfReference& ref) override;

protected:
    void BeginWrite(OutputStreamDevice& device, charbuff& buffer) override;
    void WriteSubSection(OutputStreamDevice& device, uint32_t first, uint32_t count,
        charbuff& buffer) override;
    void WriteXRefEntry(OutputStreamDevice& device, const PdfReference& ref,
        const PdfXRefEntry& entry, charbuff& buffer) override;
    void EndWriteImpl(OutputStreamDevice& device, charbuff& buffer) override;

private:
#pragma pack(push, 1)
    // TODO: Handle for different byte size for object number/offset/generation
    struct XRefStreamEntry
    {
        uint8_t Type;
        uint32_t Variant; // Can be an object number or an offset
        uint16_t Generation;
    };
#pragma pack(pop)

private:
    std::vector<XRefStreamEntry> m_rawEntries;
    int m_xrefStreamEntryIndex;
    PdfObject* m_xrefStreamObj;
    PdfArray m_indices;
    int64_t m_offset;
};

};

#endif // PDF_XREF_STREAM_H

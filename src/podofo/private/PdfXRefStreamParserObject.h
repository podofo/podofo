/**
 * SPDX-FileCopyrightText: (C) 2009 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_XREF_STREAM_PARSER_OBJECT_H
#define PDF_XREF_STREAM_PARSER_OBJECT_H

#include "PdfParserObject.h"
#include "PdfXRefEntry.h"

namespace PoDoFo
{

// CHECK-ME: Consider make this class not inherit PdfParserObject and consider mark that final
/**
 * A utility class for PdfParser that can parse
 * an XRef stream object.
 *
 * It is mainly here to make PdfParser more modular.
 * This is only marked PODOFO_API for the benefit of the tests,
 * the class is for internal use only.
 */
class PdfXRefStreamParserObject final : public PdfParserObject
{
    friend class PdfParser;

    static constexpr unsigned W_ARRAY_SIZE = 3;
    static constexpr unsigned W_MAX_BYTES = 8;

private:
    /** Parse the object data from the given file handle starting at
     * the current position.
     * To be called by PdfParser
     *  \param doc document where to resolve object references
     *  \param device an open reference counted input device which is positioned in
     *                 front of the object which is going to be parsed.
     *  \param buffer buffer to use for parsing to avoid reallocations
     */
    PdfXRefStreamParserObject(PdfDocument& doc, InputStreamDevice& device, PdfXRefEntries& entries);

public:
    /**
     *  \warning This constructor is for testing usage only
     */
    PdfXRefStreamParserObject(InputStreamDevice& device, PdfXRefEntries& entries);

public:
    void delayedLoad() override;

    void ReadXRefTable();

    /**
     * \returns the offset of the previous XRef table
     */
    bool TryGetPreviousOffset(size_t& previousOffset) const;

private:
    PdfXRefStreamParserObject(PdfDocument* doc, InputStreamDevice& device,
        PdfXRefEntries& entries);

    /**
     * Read the /Index key from the current dictionary
     * and write it to a vector.
     *
     * \param indices store the indices hare
     * \param size default value from /Size key
     */
    void getIndices(std::vector<int64_t>& indices, int64_t size);

    /**
     * Parse the stream contents
     *
     * \param wArray /W key
     * \param indices indices as filled by GetIndices
     *
     * \see GetIndices
     */
    void parseStream(const int64_t wArray[W_ARRAY_SIZE], const std::vector<int64_t>& indices);

    void readXRefStreamEntry(PdfXRefEntry& entry, char* buffer, const int64_t wArray[W_ARRAY_SIZE]);

private:
    ssize_t m_NextOffset;
    PdfXRefEntries* m_entries;
};

};

#endif // PDF_XREF_STREAM_PARSER_OBJECT_H

// SPDX-FileCopyrightText: 2010 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#ifndef PDF_OBJECT_STREAM_PARSER_OBJECT_H
#define PDF_OBJECT_STREAM_PARSER_OBJECT_H

#include "PdfParserObject.h"

namespace PoDoFo {

class PdfIndirectObjectList;

/// A utility class for PdfParser that reads the objects stored in
/// an object stream object (ISO 32000-2:2020 7.5.7 Object streams)
///
/// A single instance is shared by all the PdfCompressedObject instances
/// created for one object stream: its contents are decoded only when the
/// first of them is accessed and are released as soon as all of them
/// have been loaded
class PdfObjectStreamParser final
{
    friend class PdfCompressedObject;

public:
    /// Identifies a compressed object, as reported by an XRef stream entry
    struct Entry final
    {
        uint32_t ObjectNumber;
        unsigned Index;         ///< Index of the object within the object stream
    };

public:
    /// Create lazily loaded objects for the objects stored in the given
    /// object stream and push them to the object list
    ///
    /// @param streamObj the object stream object
    /// @param objects the list where the created objects are pushed
    /// @param buffer use this allocated buffer for tokenization
    /// @param entries the compressed objects to create. If nullptr, objects are
    ///     created for all the entries found in the stream, which requires
    ///     decoding its contents immediately
    static void Parse(PdfParserObject& streamObj, PdfIndirectObjectList& objects,
        const std::shared_ptr<charbuff>& buffer, const std::vector<Entry>* entries);

private:
    struct ObjectOffset final
    {
        uint32_t ObjectNumber;
        size_t Offset;          ///< Offset of the object within the decoded contents
    };

private:
    PdfObjectStreamParser(const PdfObject& streamObj, PdfIndirectObjectList& objects,
        const std::shared_ptr<charbuff>& buffer);

    PdfObjectStreamParser(const PdfObjectStreamParser&) = delete;
    PdfObjectStreamParser& operator=(const PdfObjectStreamParser&) = delete;

private:
    /// Read the variant of the object with the given number, which is
    /// expected to be stored at the given index in the stream
    /// @returns false if the object is not stored in the stream
    /// @remarks called by PdfCompressedObject
    bool TryReadObject(uint32_t objNum, unsigned index, PdfVariant& variant);

private:
    /// To be called when one of the created objects reclaims its memory
    void notifyObjectUnloaded();

    static void pushObject(PdfIndirectObjectList& objects,
        const std::shared_ptr<PdfObjectStreamParser>& parser, const Entry& entry);

    void ensureDecoded();

    /// Decode the stream contents and read the offsets of the stored objects
    void decode(const PdfObject& streamObj);

    /// Release the decoded contents when there's nothing left to read
    void objectLoaded();

private:
    PdfIndirectObjectList* m_objects;
    PdfReference m_streamRef;
    std::shared_ptr<charbuff> m_buffer;
    PdfTokenizerParams m_params;
    charbuff m_contents;                    ///< The decoded stream contents
    std::vector<ObjectOffset> m_offsets;
    unsigned m_pendingCount;                ///< Objects that were not loaded yet
    bool m_decoding;
    bool m_decoded;
};

};

#endif // PDF_OBJECT_STREAM_PARSER_OBJECT_H

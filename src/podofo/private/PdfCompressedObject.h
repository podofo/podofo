// SPDX-FileCopyrightText: 2026 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#ifndef PDF_COMPRESSED_OBJECT_H
#define PDF_COMPRESSED_OBJECT_H

#include "PdfObjectStreamParser.h"

namespace PoDoFo {

/// An object stored in an object stream (ISO 32000-2:2020 7.5.7 Object streams)
///
/// The object is parsed lazily, decoding the contents of the object stream
/// on the first access. Modifying contained objects doesn't invalidate the
/// object stream itself, so objects won't be unrolled on saves
class PdfCompressedObject final : public PdfObject
{
    friend class PdfObjectStreamParser;

private:
    PdfCompressedObject(const PdfReference& indirectReference,
        const std::shared_ptr<PdfObjectStreamParser>& parser, unsigned index);

public:
    bool TryUnload() override;

protected:
    void delayedLoad() override;

    void SetRevised() override;

private:
    PdfCompressedObject(const PdfCompressedObject&) = delete;
    PdfCompressedObject& operator=(const PdfCompressedObject&) = delete;

private:
    std::shared_ptr<PdfObjectStreamParser> m_parser;
    unsigned m_index;
    bool m_isRevised;         ///< True if the object was irreversibly modified since first read
};

};

#endif // PDF_COMPRESSED_OBJECT_H

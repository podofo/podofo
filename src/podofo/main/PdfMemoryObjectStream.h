/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_MEMORY_OBJECT_STREAM_H
#define PDF_MEMORY_OBJECT_STREAM_H

#include "PdfDeclarations.h"

#include "PdfObjectStreamProvider.h"

namespace PoDoFo {

/** A PDF stream can be appended to any PdfObject
 *  and can contain arbitrary data.
 *
 *  A PDF memory stream is held completely in memory.
 *
 *  Most of the time it will contain either drawing commands
 *  to draw onto a page or binary data like a font or an image.
 *
 *  A PdfMemoryObjectStream is implicitly shared and can therefore be copied very quickly.
 */
class PODOFO_API PdfMemoryObjectStream final : public PdfObjectStreamProvider
{
    friend class PdfObject;
    friend class PdfIndirectObjectList;
    PODOFO_PRIVATE_FRIEND(class PdfImmediateWriter);

private:
    PdfMemoryObjectStream();

public:
    void Init(PdfObject& obj) override;

    void Clear() override;

    bool TryCopyFrom(const PdfObjectStreamProvider& rhs) override;

    bool TryMoveFrom(PdfObjectStreamProvider&& rhs) override;

    std::unique_ptr<InputStream> GetInputStream(PdfObject& obj) override;

    std::unique_ptr<OutputStream> GetOutputStream(PdfObject& obj) override;

    void Write(OutputStream& stream, const PdfStatefulEncrypt* encrypt) override;

    size_t GetLength() const override;

    const charbuff& GetBuffer() const { return m_buffer; }

 private:
    charbuff m_buffer;
};

};

#endif // PDF_MEMORY_OBJECT_STREAM_H

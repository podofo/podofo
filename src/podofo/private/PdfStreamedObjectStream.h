/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_STREAMED_OBJECT_STREAM_H
#define PDF_STREAMED_OBJECT_STREAM_H

#include <podofo/main/PdfObjectStreamProvider.h>
#include <podofo/main/PdfEncrypt.h>

namespace PoDoFo {

class OutputStreamDevice;

/** A PDF stream can be appended to any PdfObject
 *  and can contain arbitrary data.
 *
 *  Most of the time it will contain either drawing commands
 *  to draw onto a page or binary data like a font or an image.
 *
 *  A PdfFileObjectStream writes all data directly to an output device
 *  without keeping it in memory.
 *  PdfFileObjectStream is used automatically when creating PDF files
 *  using PdfImmediateWriter.
 *
 *  \see PdfIndirectObjectList
 *  \see PdfObjectStream
 */
class PdfStreamedObjectStream final : public PdfObjectStreamProvider
{
    class ObjectOutputStream;
    friend class ObjectOutputStream;
    friend class PdfImmediateWriter;

private:
    /** Create a new PdfDeviceObjectStream object which has a parent PdfObject.
     *  The stream will be deleted along with the parent.
     *  This constructor will be called by PdfObject::Stream() for you.
     *
     *  \param device output device
     */
    PdfStreamedObjectStream(OutputStreamDevice& device);

public:
    void Init(PdfObject& obj) override;

    void Clear() override;

    bool TryCopyFrom(const PdfObjectStreamProvider& rhs) override;

    bool TryMoveFrom(PdfObjectStreamProvider&& rhs) override;

    std::unique_ptr<InputStream> GetInputStream(PdfObject& ob) override;

    std::unique_ptr<OutputStream> GetOutputStream(PdfObject& ob) override;

    void Write(OutputStream& stream, const PdfStatefulEncrypt* encrypt) override;

    size_t GetLength() const override;

private:
    /** Set an encryption object which is used to encrypt
     *  all data written to this stream.
     *
     *  \param encrypt an encryption object or nullptr if no encryption should be done
     */
    void SetEncrypt(PdfEncrypt& encrypt, PdfEncryptContext& context);

    void FinishOutput();

private:
    OutputStreamDevice* m_Device;
    PdfEncrypt* m_Encrypt;
    PdfEncryptContext* m_EncryptContext;
    size_t m_Length;
    PdfObject* m_LengthObj;
};

};

#endif // PDF_STREAMED_OBJECT_STREAM_H

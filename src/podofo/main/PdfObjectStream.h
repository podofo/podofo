/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_OBJECT_STREAM_H
#define PDF_OBJECT_STREAM_H

#include "PdfDeclarations.h"

#include "PdfEncrypt.h"
#include <podofo/auxiliary/OutputStream.h>
#include <podofo/auxiliary/InputStream.h>
#include "PdfObjectStreamProvider.h"

namespace PoDoFo {

class PdfObject;
class PdfObjectStream;

class PODOFO_API PdfObjectInputStream final : public InputStream
{
    friend class PdfObjectStream;
public:
    PdfObjectInputStream();
    ~PdfObjectInputStream();
    PdfObjectInputStream(PdfObjectInputStream&& rhs) noexcept;
private:
    PdfObjectInputStream(PdfObjectStream& stream, bool raw);
public:
    const PdfFilterList& GetMediaFilters() const { return m_MediaFilters; }
    const std::vector<const PdfDictionary*>& GetMediaDecodeParms() const { return m_MediaDecodeParms; }
protected:
    size_t readBuffer(char* buffer, size_t size, bool& eof) override;
    bool readChar(char& ch) override;
public:
    PdfObjectInputStream& operator=(PdfObjectInputStream&& rhs) noexcept;
private:
    PdfObjectStream* m_stream;
    std::unique_ptr<InputStream> m_input;
    PdfFilterList m_MediaFilters;
    std::vector<const PdfDictionary*> m_MediaDecodeParms;
};

class PODOFO_API PdfObjectOutputStream final : public OutputStream
{
    friend class PdfObjectStream;
public:
    PdfObjectOutputStream();
    ~PdfObjectOutputStream();
    PdfObjectOutputStream(PdfObjectOutputStream&& rhs) noexcept;
private:
    /**
     * \param raw when true, ignore filters on the creation of the stream,
     *      while still setting them on the object 
     */
    PdfObjectOutputStream(PdfObjectStream& stream, PdfFilterList&& filters,
        bool raw, bool append);
    PdfObjectOutputStream(PdfObjectStream& stream);
private:
    PdfObjectOutputStream(PdfObjectStream& stream, nullable<PdfFilterList> filters,
        bool raw, bool append, bool skipSetDirty);
protected:
    void writeBuffer(const char* buffer, size_t size) override;
    void flush() override;
public:
    PdfObjectOutputStream& operator=(PdfObjectOutputStream&& rhs) noexcept;
private:
    PdfObjectStream* m_stream;
    std::unique_ptr<OutputStream> m_output;
};

/** A PDF stream can be appended to any PdfObject
 *  and can contain arbitrary data.
 *
 *  Most of the time it will contain either drawing commands
 *  to draw onto a page or binary data like a font or an image.
 *
 *  You have to use a concrete implementation of a stream,
 *  which can be retrieved from a StreamFactory.
 *  \see PdfIndirectObjectList
 *  \see PdfMemoryObjectStream
 *  \see PdfFileObjectStream
 */
class PODOFO_API PdfObjectStream final
{
    friend class PdfObject;
    friend class PdfObjectInputStream;
    friend class PdfObjectOutputStream;
    PODOFO_PRIVATE_FRIEND(class PdfParserObject);
    PODOFO_PRIVATE_FRIEND(class PdfImmediateWriter);

private:
    /** Create a new PdfObjectStream object which has a parent PdfObject.
     *  The stream will be deleted along with the parent.
     *  This constructor will be called by PdfObject::Stream() for you.
     *  \param parent parent object
     */
    PdfObjectStream(PdfObject& parent, std::unique_ptr<PdfObjectStreamProvider>&& provider);

public:
    virtual ~PdfObjectStream();

    PdfObjectOutputStream GetOutputStreamRaw(bool append = false);

    PdfObjectOutputStream GetOutputStreamRaw(const PdfFilterList& filters, bool append = false);

    PdfObjectOutputStream GetOutputStream(bool append = false);

    PdfObjectOutputStream GetOutputStream(const PdfFilterList& filters, bool append = false);

    PdfObjectInputStream GetInputStream(bool raw = false) const;

    /** Set the data contents copying from a buffer
     *  All data will be Flate-encoded.
     *
     *  \param buffer buffer containing the stream data
     *  \param raw if true the data will be set without applying any filter,
     *      clearing the filters list unconditionally
     */
    void SetData(const bufferview& buffer, bool raw = false);

    /** Set the data contents copying from a buffer
     *
     * Use PdfFilterFactory::CreateFilterList() if you want to use the contents
     * of the stream dictionary's existing filter key.
     *
     *  \param buffer buffer containing the stream data
     *  \param filters a list of filters to use when appending data
     *  \param raw if true the data will be set without applying any filter, setting the
     *      supplied filters list unconditionally
     */
    void SetData(const bufferview& buffer, const PdfFilterList& filters, bool raw = false);

    /** Set the data contents reading from an InputStream
     *  All data will be Flate-encoded.
     *
     *  \param stream read stream contents from this InputStream
     *  \param raw if true the data will be set without applying any filter,
     *      clearing the filters list unconditionally
     */
    void SetData(InputStream& stream, bool raw = false);

    /** Set the data contents reading from an InputStream
     *
     * Use PdfFilterFactory::CreateFilterList() if you want to use the contents
     * of the stream dictionary's existing filter key.
     *
     *  \param stream read stream contents from this InputStream
     *  \param filters a list of filters to use when appending data
     *  \param raw if true the data will be set without applying any filter, setting the
     *      supplied filters list unconditionally
     */
    void SetData(InputStream& stream, const PdfFilterList& filters, bool raw = false);

    /** Get an unwrapped copy of the stream, unpacking non media filters
     * \remarks throws if the stream contains media filters, like DCTDecode
     */
    charbuff GetCopy(bool raw = false) const;

    /** Get an unwrapped copy of the stream, unpacking non media filters
     */
    charbuff GetCopySafe() const;

    /** Unwrap the stream to the given buffer, unpacking non media filters
     * \remarks throws if the stream contains media filters, like DCTDecode.
     * It clears the buffer before copying
     */
    void CopyTo(charbuff& buffer, bool raw = false) const;

    /** Unwrap the stream to the given buffer, unpacking non media filters
     */
    void CopyToSafe(charbuff& buffer) const;

    /** Unwrap the stream and write it to the given stream, unpacking non media filters
     * \remarks throws if the stream contains media filters, like DCTDecode.
     * It clears the buffer before copying
     */
    void CopyTo(OutputStream& stream, bool raw = false) const;

    /** Unwrap the stream and write it to the given stream, unpacking non media filters
     */
    void CopyToSafe(OutputStream& stream) const;

    /** Unpack non media filters
     */
    void Unwrap();

    /** Clear the stream and reset filters
     */
    void Clear();

    /** Get the stream's length with all filters applied (e.g. if the stream is
     * Flate-compressed, the length of the compressed data stream).
     *
     *  \returns the length of the internal buffer
     */
    size_t GetLength() const;

    const PdfFilterList& GetFilters() { return m_Filters; }

    /** Create a copy of a PdfObjectStream object
     *  \param rhs the object to clone
     *  \returns a reference to this object
     */
    PdfObjectStream& operator=(const PdfObjectStream& rhs);

    PdfObjectStream& operator=(PdfObjectStream&& rhs) noexcept;

    const PdfObjectStreamProvider& GetProvider() const { return *m_Provider; }

    const PdfObject& GetParent() const { return *m_Parent; }

    PdfObject& GetParent() { return *m_Parent; }

private:
    /** Write the stream to an output device
     *  \param device write to this outputdevice.
     *  \param encrypt encrypt stream data using this object
     */
    void Write(OutputStream& stream, const PdfStatefulEncrypt* encrypt);

    void SetParent(PdfObject& parent) { m_Parent = &parent; }

    void InitData(InputStream& stream, size_t len, PdfFilterList&& filterList);

    /** Copy data and non data fields from rhs
     */
    void CopyFrom(const PdfObjectStream& rhs);

    /** Copy data and non data fields from rhs
     */
    void MoveFrom(PdfObjectStream& rhs);

    PdfObjectStreamProvider& GetProvider() { return *m_Provider; }

private:
    void ensureClosed() const;

    std::unique_ptr<InputStream> getInputStream(bool raw, PdfFilterList& mediaFilters,
        std::vector<const PdfDictionary*>& decodeParms);

    void setData(InputStream& stream, PdfFilterList filters, bool raw,
        ssize_t size, bool markObjectDirty);

private:
    PdfObjectStream(const PdfObjectStream& rhs) = delete;

private:
    PdfObject* m_Parent;
    std::unique_ptr<PdfObjectStreamProvider> m_Provider;
    PdfFilterList m_Filters;
    bool m_locked;
};

};

#endif // PDF_OBJECT_STREAM_H

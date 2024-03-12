/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FILTER_H
#define PDF_FILTER_H

#include <podofo/main/PdfDeclarations.h>

#include <podofo/auxiliary/OutputStream.h>

namespace PoDoFo {

class PdfDictionary;

/** Every filter in PoDoFo has to implement this interface.
 *
 *  The two methods Encode() and Decode() have to be implemented
 *  for every filter.
 */
class PdfFilter
{
protected:
    /** Construct and initialize a new filter
     */
    PdfFilter();

public:
    /** All classes with virtual functions need a virtual destructor
     */
    virtual ~PdfFilter();

    /** Check whether encoding is implemented for this filter.
     *
     *  \returns true if the filter is able to encode data
     */
    virtual bool CanEncode() const = 0;

    /** Encodes a buffer using a filter
     *
     *  This function uses BeginEncode()/EncodeBlock()/EndEncode()
     *  internally, so it's not safe to use when progressive encoding
     *  is in progress.
     *
     *  \param outBuffer receives pointer to the buffer of the encoded data
     *  \param inBuffer input buffer
     */
    void EncodeTo(charbuff& outBuffer, const bufferview& inBuffer) const;
    void EncodeTo(OutputStream& stream, const bufferview& inBuffer) const;

    /** Begin progressively encoding data using this filter.
     *
     *  This method sets the filter's output stream and may
     *  perform other operations defined by particular filter
     *  implementations. It calls BeginEncodeImpl().
     *
     *  \param output Encoded data will be written to this stream.
     *
     *  Call EncodeBlock() to encode blocks of data and use EndEncode()
     *  to finish the encoding process.
     *
     *  \see EncodeBlock
     *  \see EndEncode
     */
    void BeginEncode(OutputStream& output);

    /** Encode a block of data and write it to the OutputStream
     *  specified by BeginEncode(). Ownership of the block is not taken
     *  and remains with the caller.
     *
     *  The filter implementation need not immediately process the buffer,
     *  and might internally buffer some or all of it. However, if it does
     *  this the buffer's contents will be copied, so it is guaranteed to be
     *  safe to free the passed buffer after this call returns.
     *
     *  This method is a wrapper around EncodeBlockImpl().
     *
     *  BeginEncode() must be called before this function.
     *
     *  \param view a buffer with data to encode
     *
     *  Call EndEncode() after all data has been encoded.
     *
     *  \see BeginEncode
     *  \see EndEncode
     */
    void EncodeBlock(const bufferview& view);

    /**
     *  Finish encoding of data and reset the stream's state.
     *
     *  \see BeginEncode
     *  \see EncodeBlock
     */
    void EndEncode();

    /** Check whether the decoding is implemented for this filter.
     *
     *  \returns true if the filter is able to decode data
     */
    virtual bool CanDecode() const = 0;

    /** Decodes a buffer using a filter. The buffer has
     *  to be free'd by the caller.
     *
     *  \param outBuffer receives pointer to the buffer of the encoded data
     *  \param inBuffer input buffer
     *  \param decodeParms optional pointer to a decode-parameters dictionary
     *                      containing additional information to decode
     *                      the data. This pointer must be nullptr if no
     *                      decode-parameters dictionary is available.
     */
    void DecodeTo(charbuff& outBuffer, const bufferview& inBuffer, const PdfDictionary* decodeParms = nullptr) const;
    void DecodeTo(OutputStream& stream, const bufferview& inBuffer, const PdfDictionary* decodeParms = nullptr) const;

    /** Begin progressively decoding data using this filter.
     *
     *  This method sets the filter's output stream and may
     *  perform other operations defined by particular filter
     *  implementations. It calls BeginDecodeImpl().
     *
     *  \param output decoded data will be written to this stream
     *  \param decodeParms a dictionary containing additional information
     *                      for decoding
     *
     *  Call DecodeBlock() to decode blocks of data and use EndDecode()
     *  to finish the decoding process.
     *
     *  \see DecodeBlock
     *  \see EndDecode
     */
    void BeginDecode(OutputStream& output, const PdfDictionary* decodeParms = nullptr);

    /** Decode a block of data and write it to the OutputStream
     *  specified by BeginDecode(). Ownership of the block is not taken
     *  and remains with the caller.
     *
     *  The filter implementation need not immediately process the buffer,
     *  and might internally buffer some or all of it. However, if it does
     *  this the buffer's contents will be copied, so it is guaranteed to be
     *  safe to free the passed buffer after this call returns.
     *
     *  This method is a wrapper around DecodeBlockImpl().
     *
     *  BeginDecode() must be called before this function.
     *
     *  \param view a buffer with data to encode
     *
     *  Call EndDecode() after all data has been decoded.
     *
     *  \see BeginDecode
     *  \see EndDecode
     */
    void DecodeBlock(const bufferview& view);

    /**
     *  Finish decoding of data and reset the stream's state.
     *
     *  \see BeginDecode
     *  \see DecodeBlock
     */
    void EndDecode();

    /** Type of this filter.
     *  \returns the type of this filter
     */
    virtual PdfFilterType GetType() const = 0;

protected:
    /**
     * Indicate that the filter has failed, and will be non-functional
     * until BeginEncode() or BeginDecode() is next called. Call this
     * instead of EndEncode() or EndDecode if something went wrong.
     * It clears the stream output but otherwise does nothing.
     *
     * After this method is called further calls to EncodeBlock(),
     * DecodeBlock(), EndDecode() and EndEncode() before the next
     * BeginEncode() or BeginDecode() are guaranteed to throw
     * without calling their virtual implementations.
     */
    void FailEncodeDecode();

    /** Real implementation of BeginEncode(). NEVER call this method directly.
     *
     *  By default this function does nothing. If your filter needs to do setup
     *  for encoding, you should override this method.
     *
     *  PdfFilter ensures that a valid stream is available when this method is
     *  called, and that EndEncode() was called since the last BeginEncode()/
     *  EncodeBlock().
     * \see BeginEncode
     */
    virtual void BeginEncodeImpl();

    /** Real implementation of EncodeBlock(). NEVER call this method directly.
     *
     *  You must method-override it to encode the buffer passed by the caller.
     *
     *  You are not obliged to immediately process any or all of the data in
     *  the passed buffer, but you must ensure that you have processed it and
     *  written it out by the end of EndEncodeImpl(). You must copy the buffer
     *  if you're going to store it, as ownership is not transferred to the
     *  filter and the caller may free the buffer at any time.
     *
     *  PdfFilter ensures that a valid stream is available when this method is
     *  called, ensures that BeginEncode() has been called, and ensures that
     *  EndEncode() has not been called since the last BeginEncode().
     *
     * \see EncodeBlock
     */
    virtual void EncodeBlockImpl(const char* buffer, size_t len) = 0;

    /** Real implementation of EndEncode(). NEVER call this method directly.
     *
     * By the time this method returns, all filtered data must be written to
     * the stream and the filter must be in a state where BeginEncode() can
     * be safely called.
     * PdfFilter ensures that a valid stream is available when this method is
     * called, and ensures that BeginEncodeImpl() has been called.
     *
     * \see EndEncode
     */
    virtual void EndEncodeImpl();

    /** Real implementation of BeginDecode(). NEVER call this method directly.
     *
     *  By default this function does nothing. If your filter needs to do setup
     *  for decoding, you should override this method.
     *
     *  PdfFilter ensures that a valid stream is available when this method is
     *  called, and that EndDecode() was called since the last BeginDecode()/
     *  DecodeBlock().
     * \see BeginDecode
     */
    virtual void BeginDecodeImpl(const PdfDictionary*);

    /** Real implementation of DecodeBlock(). NEVER call this method directly.
     *
     *  You must method-override it to decode the buffer passed by the caller.
     *
     *  You are not obliged to immediately process any or all of the data in
     *  the passed buffer, but you must ensure that you have processed it and
     *  written it out by the end of EndDecodeImpl(). You must copy the buffer
     *  if you're going to store it, as ownership is not transferred to the
     *  filter and the caller may free the buffer at any time.
     *
     *  PdfFilter ensures that a valid stream is available when this method is
     *  called, ensures that BeginDecode() has been called, and ensures that
     *  EndDecode() has not been called since the last BeginDecode().
     *
     * \see DecodeBlock
     */
    virtual void DecodeBlockImpl(const char* buffer, size_t len) = 0;

    /** Real implementation of EndDecode(). NEVER call this method directly.
     *
     *  By the time this method returns, all filtered data must be written to
     *  the stream and the filter must be in a state where BeginDecode() can be
     *  safely called.
     *  PdfFilter ensures that a valid stream is available when this method is
     *  called, and ensures that BeginDecodeImpl() has been called.
     *
     * \see EndDecode
     */
    virtual void EndDecodeImpl();

protected:
    inline OutputStream& GetStream() const { return *m_OutputStream; }
private:
    void encodeTo(OutputStream& stream, const bufferview& inBuffer);
    void decodeTo(OutputStream& stream, const bufferview& inBuffer, const PdfDictionary* decodeParms);

private:
    OutputStream* m_OutputStream;
};

}

#endif // PDF_FILTER_H

// SPDX-FileCopyrightText: 2006 Dominik Seichter <domseichter@web.de>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFilter.h"

#include <podofo/auxiliary/StreamDevice.h>

using namespace std;
using namespace PoDoFo;

PdfFilter::PdfFilter()
    : m_OutputStream(nullptr)
{
}

PdfFilter::~PdfFilter()
{
    // Whoops! Didn't call EndEncode() before destroying the filter!
    // Note that we can't do this for the user, since EndEncode() might
    // throw and we can't safely have that in a dtor. That also means
    // we can't throw here, but must abort.
    PODOFO_ASSERT(m_OutputStream == nullptr);
}

void PdfFilter::EncodeTo(charbuff& outBuffer, const bufferview& inBuffer) const
{
    if (!this->CanEncode())
        PODOFO_RAISE_ERROR(PdfErrorCode::UnsupportedFilter);

    BufferStreamDevice stream(outBuffer);
    const_cast<PdfFilter&>(*this).encodeTo(stream, inBuffer);
}

void PdfFilter::EncodeTo(OutputStream& stream, const bufferview& inBuffer) const
{
    if (!this->CanEncode())
        PODOFO_RAISE_ERROR(PdfErrorCode::UnsupportedFilter);

    const_cast<PdfFilter&>(*this).encodeTo(stream, inBuffer);
}

void PdfFilter::encodeTo(OutputStream& stream, const bufferview& inBuffer)
{
    BeginEncode(stream);
    EncodeBlock(inBuffer);
    EndEncode();
}

void PdfFilter::DecodeTo(charbuff& outBuffer, const bufferview& inBuffer,
    const PdfDictionary* decodeParms) const
{
    if (!this->CanDecode())
        PODOFO_RAISE_ERROR(PdfErrorCode::UnsupportedFilter);

    BufferStreamDevice stream(outBuffer);
    const_cast<PdfFilter&>(*this).decodeTo(stream, inBuffer, decodeParms);
}

void PdfFilter::DecodeTo(OutputStream& stream, const bufferview& inBuffer, const PdfDictionary* decodeParms) const
{
    if (!this->CanDecode())
        PODOFO_RAISE_ERROR(PdfErrorCode::UnsupportedFilter);

    const_cast<PdfFilter&>(*this).decodeTo(stream, inBuffer, decodeParms);
}

void PdfFilter::decodeTo(OutputStream& stream, const bufferview& inBuffer, const PdfDictionary* decodeParms)
{
    BeginDecode(stream, decodeParms);
    DecodeBlock(inBuffer);
    EndDecode();
}

void PdfFilter::BeginEncode(OutputStream& output)
{
    PODOFO_ASSERT(m_OutputStream == nullptr && "BeginEncode() on failed filter or without EndEncode()");
    m_OutputStream = &output;

    try
    {
        BeginEncodeImpl();
    }
    catch (...)
    {
        this->failEncodeDecode();
        throw;
    }
}

void PdfFilter::EncodeBlock(const bufferview& view)
{
    PODOFO_ASSERT(m_OutputStream != nullptr && "EncodeBlock() without BeginEncode() or on failed filter");

    try
    {
        EncodeBlockImpl(view.data(), view.size());
    }
    catch (...)
    {
        this->failEncodeDecode();
        throw;
    }
}

void PdfFilter::EndEncode()
{
    PODOFO_ASSERT(m_OutputStream != nullptr && "EndEncode() without BeginEncode() or on failed filter");

    try
    {
        EndEncodeImpl();
    }
    catch (...)
    {
        this->failEncodeDecode();
        throw;
    }

    closeEncodeDecode();
}

void PdfFilter::BeginDecode(OutputStream& output, const PdfDictionary* decodeParms)
{
    PODOFO_ASSERT(m_OutputStream == nullptr && "BeginDecode() on failed filter or without EndDecode()");
    m_OutputStream = &output;

    try
    {
        BeginDecodeImpl(decodeParms);
    }
    catch (...)
    {
        this->failEncodeDecode();
        throw;
    }
}

void PdfFilter::DecodeBlock(const bufferview& view)
{
    PODOFO_ASSERT(m_OutputStream != nullptr && "DecodeBlock() without BeginDecode() or on failed filter")

    try
    {
        DecodeBlockImpl(view.data(), view.size());
    }
    catch (...)
    {
        this->failEncodeDecode();
        throw;
    }
}

void PdfFilter::EndDecode()
{
    PODOFO_ASSERT(m_OutputStream != nullptr && "EndDecode() without BeginDecode() or on failed filter")

    try
    {
        EndDecodeImpl();
    }
    catch (...)
    {
        this->failEncodeDecode();
        throw;
    }

    closeEncodeDecode();
}

void PdfFilter::failEncodeDecode()
{
    try
    {
        m_OutputStream->Flush();
    }
    catch (...)
    {
        // Ignore errors flushing the stream, we're already in an
        // error state and just want to clean up as best we can
    }
    m_OutputStream = nullptr;
}

void PdfFilter::closeEncodeDecode()
{
    try
    {
        m_OutputStream->Flush();
    }
    catch (PdfError& e)
    {
        PODOFO_PUSH_FRAME_INFO(e, "Exception caught flushing filter's output stream");
        m_OutputStream = nullptr;
        throw;
    }
    catch (...)
    {
        m_OutputStream = nullptr;
        throw;
    }

    m_OutputStream = nullptr;
}

void PdfFilter::BeginEncodeImpl()
{
    // Do nothing by default
}

void PdfFilter::EndDecodeImpl()
{
    // Do nothing by default
}

void PdfFilter::BeginDecodeImpl(const PdfDictionary*)
{
    // Do nothing by default
}

void PdfFilter::EndEncodeImpl()
{
    // Do nothing by default
}

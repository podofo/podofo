/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

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
    PODOFO_RAISE_LOGIC_IF(m_OutputStream != nullptr, "BeginEncode() on failed filter or without EndEncode()");
    m_OutputStream = &output;

    try
    {
        BeginEncodeImpl();
    }
    catch (...)
    {
        // Clean up and close stream
        this->FailEncodeDecode();
        throw;
    }
}

void PdfFilter::EncodeBlock(const bufferview& view)
{
    PODOFO_RAISE_LOGIC_IF(m_OutputStream == nullptr, "EncodeBlock() without BeginEncode() or on failed filter");

    try
    {
        EncodeBlockImpl(view.data(), view.size());
    }
    catch (...)
    {
        // Clean up and close stream
        this->FailEncodeDecode();
        throw;
    }
}

void PdfFilter::EndEncode()
{
    PODOFO_RAISE_LOGIC_IF(m_OutputStream == nullptr, "EndEncode() without BeginEncode() or on failed filter");

    try
    {
        EndEncodeImpl();
    }
    catch (...)
    {
        // Clean up and close stream
        this->FailEncodeDecode();
        throw;
    }

    m_OutputStream->Flush();
    m_OutputStream = nullptr;
}

void PdfFilter::BeginDecode(OutputStream& output, const PdfDictionary* decodeParms)
{
    PODOFO_RAISE_LOGIC_IF(m_OutputStream != nullptr, "BeginDecode() on failed filter or without EndDecode()");
    m_OutputStream = &output;

    try
    {
        BeginDecodeImpl(decodeParms);
    }
    catch (...)
    {
        // Clean up and close stream
        this->FailEncodeDecode();
        throw;
    }
}

void PdfFilter::DecodeBlock(const bufferview& view)
{
    PODOFO_RAISE_LOGIC_IF(m_OutputStream == nullptr, "DecodeBlock() without BeginDecode() or on failed filter")

    try
    {
        DecodeBlockImpl(view.data(), view.size());
    }
    catch (...)
    {
        // Clean up and close stream
        this->FailEncodeDecode();
        throw;
    }
}

void PdfFilter::EndDecode()
{
    PODOFO_RAISE_LOGIC_IF(m_OutputStream == nullptr, "EndDecode() without BeginDecode() or on failed filter")

    try
    {
        EndDecodeImpl();
    }
    catch (PdfError& e)
    {
        PODOFO_PUSH_FRAME(e);
        // Clean up and close stream
        this->FailEncodeDecode();
        throw;
    }
    try
    {
        if (m_OutputStream != nullptr)
        {
            m_OutputStream->Flush();
            m_OutputStream = nullptr;
        }
    }
    catch (PdfError& e)
    {
        PODOFO_PUSH_FRAME_INFO(e, "Exception caught closing filter's output stream");
        // Closing stream failed, just get rid of it
        m_OutputStream = nullptr;
        throw;
    }
}

void PdfFilter::FailEncodeDecode()
{
    if (m_OutputStream != nullptr)
        m_OutputStream->Flush();

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

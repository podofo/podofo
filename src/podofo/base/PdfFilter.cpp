/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFilter.h"

#include <podofo/private/PdfFiltersPrivate.h>

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfStreamDevice.h"

using namespace std;
using namespace PoDoFo;

// An OutputStream class that actually perform the encoding
class PdfFilteredEncodeStream : public OutputStream
{
private:
    void init(OutputStream& outputStream, PdfFilterType filterType)
    {
        m_filter = PdfFilterFactory::Create(filterType);
        if (m_filter == nullptr)
            PODOFO_RAISE_ERROR(PdfErrorCode::UnsupportedFilter);

        m_filter->BeginEncode(outputStream);
    }
    ~PdfFilteredEncodeStream()
    {
        m_filter->EndEncode();
    }
public:
    PdfFilteredEncodeStream(const shared_ptr<OutputStream>& outputStream, PdfFilterType filterType)
        : m_OutputStream(outputStream)
    {
        init(*outputStream, filterType);
    }
protected:
    void writeBuffer(const char* buffer, size_t len) override
    {
        m_filter->EncodeBlock({ buffer, len });
    }
private:
    shared_ptr<OutputStream> m_OutputStream;
    unique_ptr<PdfFilter> m_filter;
};

// An OutputStream class that actually perform the deecoding
class PdfFilteredDecodeStream : public OutputStream
{
private:
    void init(OutputStream& outputStream, const PdfFilterType filterType,
        const PdfDictionary* decodeParms)
    {
        m_filter = PdfFilterFactory::Create(filterType);
        if (m_filter == nullptr)
            PODOFO_RAISE_ERROR(PdfErrorCode::UnsupportedFilter);

        m_filter->BeginDecode(outputStream, decodeParms);
    }

public:
    PdfFilteredDecodeStream(OutputStream& outputStream, const PdfFilterType filterType,
        const PdfDictionary* decodeParms)
        : m_FilterFailed(false)
    {
        init(outputStream, filterType, decodeParms);
    }

    PdfFilteredDecodeStream(unique_ptr<OutputStream> outputStream, const PdfFilterType filterType,
        const PdfDictionary* decodeParms)
        : m_OutputStream(std::move(outputStream)), m_FilterFailed(false)
    {
        if (m_OutputStream == nullptr)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Output stream must be not null");

        init(*m_OutputStream, filterType, decodeParms);
    }

protected:
    void writeBuffer(const char* buffer, size_t len) override
    {
        try
        {
            m_filter->DecodeBlock({ buffer, len });
        }
        catch (PdfError& e)
        {
            PODOFO_PUSH_FRAME(e);
            m_FilterFailed = true;
            throw;
        }
    }
    void flush() override
    {
        try
        {
            if (!m_FilterFailed)
                m_filter->EndDecode();
        }
        catch (PdfError& e)
        {
            PODOFO_PUSH_FRAME_INFO(e, "PdfFilter::EndDecode() failed in filter of type {}",
                PoDoFo::FilterToName(m_filter->GetType()));
            m_FilterFailed = true;
            throw;
        }
    }

private:
    shared_ptr<OutputStream> m_OutputStream;
    unique_ptr<PdfFilter> m_filter;
    bool m_FilterFailed;
};

// An InputStream class that will actually perform the decoding
class PdfBufferedDecodeStream : public InputStream, private OutputStream
{
public:
    PdfBufferedDecodeStream(const shared_ptr<InputStream>& inputStream, const PdfFilterList& filters,
            const vector<const PdfDictionary*>& decodeParms)
        : m_inputEof(false), m_inputStream(inputStream), m_offset(0)
    {
        PODOFO_INVARIANT(filters.size() != 0);
        int i = (int)filters.size() - 1;
        m_filterStream.reset(new PdfFilteredDecodeStream(*this, filters[i], decodeParms[i]));
        i--;

        while (i >= 0)
        {
            m_filterStream.reset(new PdfFilteredDecodeStream(std::move(m_filterStream), filters[i], decodeParms[i]));
            i--;
        }
    }
protected:
    size_t readBuffer(char* buffer, size_t size, bool& eof) override
    {
        if (m_offset < m_buffer.size())
        {
            size = std::min(size, m_buffer.size() - m_offset);
            std::memcpy(buffer, m_buffer.data() + m_offset, size);
            m_offset += size;
            eof = false;
            return size;
        }

        if (m_inputEof)
        {
            eof = true;
            return 0;
        }

        auto readSize = ReadBuffer(*m_inputStream, buffer, size, m_inputEof);
        m_buffer.clear();
        m_filterStream->Write(buffer, readSize);
        if (m_inputEof)
            m_filterStream->Flush();

        size = std::min(size, m_buffer.size());
        std::memcpy(buffer, m_buffer.data(), size);
        m_offset = size;
        eof = false;
        return size;
    }

    void writeBuffer(const char* buffer, size_t size) override
    {
        m_buffer.append(buffer, size);
    }
private:
    bool m_inputEof;
    shared_ptr<InputStream> m_inputStream;
    size_t m_offset;
    charbuff m_buffer;
    unique_ptr<OutputStream> m_filterStream;
};

//
// Actual PdfFilter code
//

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

//
// PdfFilterFactory code
//

unique_ptr<PdfFilter> PdfFilterFactory::Create(PdfFilterType filterType)
{
    PdfFilter* filter = nullptr;
    switch (filterType)
    {
        case PdfFilterType::ASCIIHexDecode:
            filter = new PdfHexFilter();
            break;
        case PdfFilterType::ASCII85Decode:
            filter = new PdfAscii85Filter();
            break;
        case PdfFilterType::LZWDecode:
            filter = new PdfLZWFilter();
            break;
        case PdfFilterType::FlateDecode:
            filter = new PdfFlateFilter();
            break;
        case PdfFilterType::RunLengthDecode:
            filter = new PdfRLEFilter();
            break;
        case PdfFilterType::None:
        case PdfFilterType::DCTDecode:
        case PdfFilterType::CCITTFaxDecode:
        case PdfFilterType::JBIG2Decode:
        case PdfFilterType::JPXDecode:
        case PdfFilterType::Crypt:
        default:
            break;
    }

    return unique_ptr<PdfFilter>(filter);
}

unique_ptr<OutputStream> PdfFilterFactory::CreateEncodeStream(const shared_ptr<OutputStream>& stream,
    const PdfFilterList& filters)
{
    PODOFO_RAISE_LOGIC_IF(!filters.size(), "Cannot create an EncodeStream from an empty list of filters");

    PdfFilterList::const_iterator it = filters.begin();
    unique_ptr<OutputStream> filter(new PdfFilteredEncodeStream(stream, *it));
    it++;

    while (it != filters.end())
    {
        filter.reset(new PdfFilteredEncodeStream(std::move(filter), *it));
        it++;
    }

    return filter;
}

unique_ptr<InputStream> PdfFilterFactory::CreateDecodeStream(const shared_ptr<InputStream>& stream,
    const PdfFilterList& filters, const std::vector<const PdfDictionary*>& decodeParms)
{
    PODOFO_RAISE_LOGIC_IF(stream == nullptr, "Cannot create an DecodeStream from an empty stream");
    PODOFO_RAISE_LOGIC_IF(filters.size() == 0, "Cannot create an DecodeStream from an empty list of filters");
    return std::make_unique<PdfBufferedDecodeStream>(stream, filters, decodeParms);
}

PdfFilterList PdfFilterFactory::CreateFilterList(const PdfObject& filtersObj)
{
    PdfFilterList filters;
    const PdfObject* filterKeyObj = nullptr;
    if (filtersObj.IsDictionary()
        && (filterKeyObj = filtersObj.GetDictionary().FindKey(PdfName::KeyFilter)) == nullptr
        && filtersObj.IsArray())
    {
        filterKeyObj = &filtersObj;
    }
    else if (filtersObj.IsName())
    {
        filterKeyObj = &filtersObj;
    }

    if (filterKeyObj == nullptr)
    {
        // Object had no /Filter key . Return a null filter list.
        return filters;
    }

    if (filterKeyObj->IsName())
    {
        addFilterTo(filters, filterKeyObj->GetName().GetString());
    }
    else if (filterKeyObj->IsArray())
    {
        for (auto filter : filterKeyObj->GetArray().GetIndirectIterator())
        {
            if (!filter->IsName())
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Filter array contained unexpected non-name type");

            addFilterTo(filters, filter->GetName().GetString());
        }
    }
    else
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Unexpected filter container type");
    }

    return filters;
}


void PdfFilterFactory::addFilterTo(PdfFilterList& filters, const string_view& filter)
{
    auto type = PoDoFo::NameToFilter(filter);
    filters.push_back(type);
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

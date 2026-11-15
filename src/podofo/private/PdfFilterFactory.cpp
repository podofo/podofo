// SPDX-FileCopyrightText: 2006 Dominik Seichter <domseichter@web.de>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include "PdfDeclarationsPrivate.h"
#include "PdfFilterFactory.h"

#include "PdfFiltersImpl.h"

#include <podofo/main/PdfDictionary.h>
#include <podofo/main/PdfArray.h>
#include <podofo/main/PdfCommon.h>

using namespace std;
using namespace PoDoFo;

// Real files chain a handful of filters at most: the filter chains are
// flushed and destroyed recursively, so their depth is bounded
constexpr unsigned MaxFilterCount = 32;

static void checkFilterCount(const PdfFilterList& filters);

namespace
{
    // Caps the bytes produced by all the filters of a decode chain, including the
    // intermediate ones a last filter may discard, such as whitespace in hex
    class StreamDecodeCap final
    {
    public:
        StreamDecodeCap()
            : m_size(0), m_maxSize(PdfCommon::GetMaxDecodedStreamSize()) { }

        void Consume(size_t size)
        {
            if (size > m_maxSize - m_size)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "The decoded stream exceeds the maximum size of {} bytes", m_maxSize);

            m_size += size;
        }

    private:
        size_t m_size;
        size_t m_maxSize;
    };

    // An OutputStream class that actually perform the encoding
    class PdfFilteredEncodeStream : public OutputStream
    {
        static constexpr size_t BUFFER_SIZE = 65536;

    private:
        void init(OutputStream& outputStream, PdfFilterType filterType)
        {
            m_filter = PdfFilterFactory::Create(filterType);
            if (!m_filter->CanEncode())
            {
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Filter {} doesn't support encoding",
                    PoDoFo::FilterToName(filterType));
            }

            m_filter->BeginEncode(outputStream);
        }
        ~PdfFilteredEncodeStream()
        {
            // The filter can fail here as well, and a destructor can't throw.
            // A dropped filter already reported its failure
            try
            {
                if (m_filter != nullptr)
                {
                    encodePendingBytes();
                    m_filter->EndEncode();
                }
            }
            catch (PdfError& e)
            {
                PoDoFo::LogMessage(PdfLogSeverity::Warning, "Error while ending the encode stream: {}", PdfError::ErrorName(e.GetCode()));
            }
            catch (...)
            {
                PoDoFo::LogMessage(PdfLogSeverity::Warning, "Error while ending the encode stream");
            }

            // NOTE: Disarm the window as required by ~OutputStream()
            disableWriteWindow();
        }
    public:
        PdfFilteredEncodeStream(shared_ptr<OutputStream>&& outputStream, PdfFilterType filterType)
            : m_OutputStream(std::move(outputStream)), m_buffer(BUFFER_SIZE)
        {
            // NOTE: Arm the window only after the filter is created, so a
            // failure doesn't leave it armed for ~OutputStream()
            init(*m_OutputStream, filterType);
            enableWriteWindow(m_buffer.data(), m_buffer.data() + BUFFER_SIZE);
        }
    protected:
        void writeBuffer(const char* buffer, size_t len) override
        {
            if (len >= BUFFER_SIZE)
            {
                // Big enough to encode on its own: don't pay for a copy
                encodePendingBytes();
                encodeBlock({ buffer, len });
                return;
            }

            // NOTE: Append after the bytes already pending in the window,
            // draining only when they wouldn't fit together
            if ((size_t)(m_wend - m_wcur) < len)
                encodePendingBytes();

            std::memcpy(m_wcur, buffer, len);
            m_wcur += len;
        }
        void flush() override
        {
            encodePendingBytes();
        }
    private:
        void encodePendingBytes()
        {
            size_t len = (size_t)(m_wcur - m_buffer.data());
            if (len == 0)
                return;

            // The pending bytes are dropped also on failure, the filter can't take them again
            m_wcur = m_buffer.data();
            encodeBlock({ m_buffer.data(), len });
        }
        void encodeBlock(const bufferview& view)
        {
            if (m_filter == nullptr)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The encode filter has failed");

            try
            {
                m_filter->EncodeBlock(view);
            }
            catch (...)
            {
                // The filter dropped its output stream and can't be used again
                m_filter = nullptr;
                throw;
            }
        }
    private:
        shared_ptr<OutputStream> m_OutputStream;
        unique_ptr<PdfFilter> m_filter;
        charbuff m_buffer;
    };

    // An OutputStream class that actually perform the deecoding
    class PdfFilteredDecodeStream : public OutputStream
    {
    private:
        void init(OutputStream& outputStream, const PdfFilterType filterType,
            const PdfDictionary* decodeParms)
        {
            m_filter = PdfFilterFactory::Create(filterType);
            m_filter->BeginDecode(outputStream, decodeParms);
        }

    public:
        /// @param decodeCap counts the bytes written to this filter, or nullptr
        /// for the first filter of the chain, which decodes the stored data
        PdfFilteredDecodeStream(OutputStream& outputStream, const PdfFilterType filterType,
            const PdfDictionary* decodeParms, StreamDecodeCap* decodeCap)
            : m_decodeCap(decodeCap)
        {
            init(outputStream, filterType, decodeParms);
        }

        PdfFilteredDecodeStream(unique_ptr<OutputStream> outputStream, const PdfFilterType filterType,
            const PdfDictionary* decodeParms, StreamDecodeCap* decodeCap)
            : m_OutputStream(std::move(outputStream)), m_decodeCap(decodeCap)
        {
            if (m_OutputStream == nullptr)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Output stream must be not null");

            init(*m_OutputStream, filterType, decodeParms);
        }

        ~PdfFilteredDecodeStream()
        {
            // Prevent and exception in the descructor: a read abandoned before
            // the end leaves the filter live, and the bytes it flushes may be
            // rejected downstream.
            try
            {
                if (m_filter != nullptr)
                    m_filter->EndDecode();
            }
            catch (PdfError& e)
            {
                PoDoFo::LogMessage(PdfLogSeverity::Warning, "Error while ending the decode stream: {}", PdfError::ErrorName(e.GetCode()));
            }
            catch (...)
            {
                PoDoFo::LogMessage(PdfLogSeverity::Warning, "Error while ending the decode stream");
            }
        }

    protected:
        void writeBuffer(const char* buffer, size_t len) override
        {
            // NOTE: The buffered stream stops writing after a failure
            PODOFO_ASSERT(m_filter != nullptr);
            // NOTE: Checked out of the try block: the filter is still intact, and
            // the previous one ends it by flushing this stream on the failure
            if (m_decodeCap != nullptr)
                m_decodeCap->Consume(len);

            try
            {
                m_filter->DecodeBlock({ buffer, len });
            }
            catch (PdfError& e)
            {
                PODOFO_PUSH_FRAME(e);
                m_filter = nullptr;
                throw;
            }
            catch (...)
            {
                m_filter = nullptr;
                throw;
            }
        }
        void flush() override
        {
            try
            {
                if (m_filter != nullptr)
                    m_filter->EndDecode();

                m_filter = nullptr;
            }
            catch (PdfError& e)
            {
                PODOFO_PUSH_FRAME_INFO(e, "PdfFilter::EndDecode() failed in filter of type {}",
                    PoDoFo::FilterToName(m_filter->GetType()));
                m_filter = nullptr;
                throw;
            }
            catch (...)
            {
                m_filter = nullptr;
                throw;
            }
        }

    private:
        shared_ptr<OutputStream> m_OutputStream;
        StreamDecodeCap* m_decodeCap;
        unique_ptr<PdfFilter> m_filter;
    };

    // An InputStream class that will actually perform the decoding
    class PdfBufferedDecodeStream : public InputStream, private OutputStream
    {
    public:
        PdfBufferedDecodeStream(shared_ptr<InputStream>&& inputStream, const PdfFilterList& filters,
            const vector<const PdfDictionary*>& decodeParms)
            : m_inputEof(false), m_failed(false), m_inputStream(std::move(inputStream)), m_offset(0)
        {
            PODOFO_INVARIANT(filters.size() != 0);
            int i = (int)filters.size() - 1;
            m_filterStream.reset(new PdfFilteredDecodeStream(*this, filters[i], decodeParms[i],
                i == 0 ? nullptr : &m_decodeCap));
            i--;

            while (i >= 0)
            {
                m_filterStream.reset(new PdfFilteredDecodeStream(std::move(m_filterStream), filters[i], decodeParms[i],
                    i == 0 ? nullptr : &m_decodeCap));
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

            // A failed decode dropped the filters: the data ends there
            if (m_inputEof || m_failed)
            {
                eof = true;
                return 0;
            }

            auto readSize = ReadBuffer(*m_inputStream, buffer, size, m_inputEof);
            m_buffer.clear();
            try
            {
                m_filterStream->Write(buffer, readSize);
                if (m_inputEof)
                    m_filterStream->Flush();
            }
            catch (...)
            {
                m_failed = true;
                throw;
            }

            size = std::min(size, m_buffer.size());
            std::memcpy(buffer, m_buffer.data(), size);
            m_offset = size;
            eof = false;
            return size;
        }

        void writeBuffer(const char* buffer, size_t size) override
        {
            m_decodeCap.Consume(size);
            m_buffer.append(buffer, size);
        }
    private:
        bool m_inputEof;
        bool m_failed;
        shared_ptr<InputStream> m_inputStream;
        size_t m_offset;
        charbuff m_buffer;
        // NOTE: Declared before the filters, which may still write while being destroyed
        StreamDecodeCap m_decodeCap;
        unique_ptr<OutputStream> m_filterStream;
    };
}

unique_ptr<PdfFilter> PdfFilterFactory::Create(PdfFilterType filterType)
{
    unique_ptr<PdfFilter> ret;
    if (!TryCreate(filterType, ret))
        PODOFO_RAISE_ERROR(PdfErrorCode::UnsupportedFilter);

    return ret;
}

bool PdfFilterFactory::TryCreate(PdfFilterType filterType, unique_ptr<PdfFilter>& filter)
{
    switch (filterType)
    {
        case PdfFilterType::ASCIIHexDecode:
            filter = unique_ptr<PdfFilter>(new PdfHexFilter());
            return true;
        case PdfFilterType::ASCII85Decode:
            filter = unique_ptr<PdfFilter>(new PdfAscii85Filter());
            return true;
        case PdfFilterType::LZWDecode:
            filter = unique_ptr<PdfFilter>(new PdfLZWFilter());
            return true;
        case PdfFilterType::FlateDecode:
            filter = unique_ptr<PdfFilter>(new PdfFlateFilter());
            return true;
        case PdfFilterType::RunLengthDecode:
            filter = unique_ptr<PdfFilter>(new PdfRLEFilter());
            return true;
        case PdfFilterType::Crypt:
            filter = unique_ptr<PdfFilter>(new PdfCryptFilter());
            return true;
        case PdfFilterType::None:
        case PdfFilterType::DCTDecode:
        case PdfFilterType::CCITTFaxDecode:
        case PdfFilterType::JBIG2Decode:
        case PdfFilterType::JPXDecode:
        default:
            return false;
    }
}

unique_ptr<OutputStream> PdfFilterFactory::CreateEncodeStream(shared_ptr<OutputStream> stream,
    const PdfFilterList& filters)
{
    PODOFO_RAISE_LOGIC_IF(!filters.size(), "Cannot create an EncodeStream from an empty list of filters");
    checkFilterCount(filters);

    PdfFilterList::const_iterator it = filters.begin();
    unique_ptr<OutputStream> filter(new PdfFilteredEncodeStream(std::move(stream), *it));
    it++;

    while (it != filters.end())
    {
        filter.reset(new PdfFilteredEncodeStream(std::move(filter), *it));
        it++;
    }

    return filter;
}

unique_ptr<InputStream> PdfFilterFactory::CreateDecodeStream(shared_ptr<InputStream> stream,
    const PdfFilterList& filters, const std::vector<const PdfDictionary*>& decodeParms)
{
    PODOFO_RAISE_LOGIC_IF(stream == nullptr, "Cannot create an DecodeStream from an empty stream");
    PODOFO_RAISE_LOGIC_IF(filters.size() == 0, "Cannot create an DecodeStream from an empty list of filters");
    checkFilterCount(filters);
    return std::make_unique<PdfBufferedDecodeStream>(std::move(stream), filters, decodeParms);
}

PdfFilterList PdfFilterFactory::CreateFilterList(const PdfObject& filtersObj_)
{
    PdfFilterList filters;
    const PdfDictionary* dict;
    const PdfName* name;
    const PdfArray* arr;
    auto filtersObj = &filtersObj_;
    if (filtersObj->TryGetDictionary(dict))
    {
        filtersObj = dict->FindKey("Filter");
        if (filtersObj == nullptr)
        {
            // Invalid /Filter key/object. Return a null filter list.
            return filters;
        }
    }

    if (filtersObj->TryGetName(name))
    {
        addFilterTo(filters, name->GetString());
    }
    else if (filtersObj->TryGetArray(arr))
    {
        for (auto filter : arr->GetIndirectIterator())
        {
            if (!filter->TryGetName(name))
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Filter array contained unexpected non-name type");

            addFilterTo(filters, name->GetString());
        }
    }
    else
    {
        // Invalid /Filter key/object. Return a null filter list.
        return filters;
    }

    return filters;
}


void PdfFilterFactory::addFilterTo(PdfFilterList& filters, const string_view& filter)
{
    auto type = PoDoFo::NameToFilter(filter, true);
    filters.push_back(type);
}

void checkFilterCount(const PdfFilterList& filters)
{
    if (filters.size() > MaxFilterCount)
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Filter list with {} entries exceeds the maximum of {}",
            filters.size(), MaxFilterCount);
    }
}

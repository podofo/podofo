/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfObjectStream.h"

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfFilter.h"
#include "PdfInputDevice.h"
#include "PdfDictionary.h"
#include "PdfStreamDevice.h"

using namespace std;
using namespace PoDoFo;

constexpr PdfFilterType DefaultFilter = PdfFilterType::FlateDecode;
static const PdfName DecodeParmsKey("DecodeParms");

static bool isMediaFilter(PdfFilterType filterType);
static PdfFilterList stripMediaFilters(const PdfFilterList& filters, PdfFilterList& mediaFilters);

PdfObjectStream::PdfObjectStream(PdfObject& parent, std::unique_ptr<PdfObjectStreamProvider>&& provider)
    : m_Parent(&parent), m_Provider(std::move(provider)), m_locked(false)
{
    m_Provider->Init(parent);
}

PdfObjectStream::~PdfObjectStream() { }

PdfObjectOutputStream PdfObjectStream::GetOutputStreamRaw(bool append)
{
    ensureClosed();
    return PdfObjectOutputStream(*this, PdfFilterList(), true, append);
}

PdfObjectOutputStream PoDoFo::PdfObjectStream::GetOutputStreamRaw(const PdfFilterList& filters, bool append)
{
    ensureClosed();
    return PdfObjectOutputStream(*this, PdfFilterList(filters), true, append);
}

PdfObjectOutputStream PdfObjectStream::GetOutputStream(bool append)
{
    ensureClosed();
    return PdfObjectOutputStream(*this, { DefaultFilter }, false, append);
}

PdfObjectOutputStream PdfObjectStream::GetOutputStream(const PdfFilterList& filters, bool append)
{
    ensureClosed();
    return PdfObjectOutputStream(*this, PdfFilterList(filters), false, append);
}

PdfObjectInputStream PdfObjectStream::GetInputStream(bool raw) const
{
    ensureClosed();
    return PdfObjectInputStream(const_cast<PdfObjectStream&>(*this), raw);
}

void PdfObjectStream::CopyTo(charbuff& buffer, bool raw) const
{
    buffer.clear();
    BufferStreamDevice stream(buffer);
    CopyTo(stream, raw);
}

void PdfObjectStream::CopyToSafe(charbuff& buffer) const
{
    buffer.clear();
    BufferStreamDevice stream(buffer);
    CopyToSafe(stream);
}

void PdfObjectStream::CopyTo(OutputStream& stream, bool raw) const
{
    PdfFilterList mediaFilters;
    vector<const PdfDictionary*> decodeParms;
    auto inputStream = const_cast<PdfObjectStream&>(*this).getInputStream(raw, mediaFilters, decodeParms);
    if (mediaFilters.size() != 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Unsupported expansion with media filters. Use GetInputStream(true) instead");
        
    inputStream->CopyTo(stream);
    stream.Flush();
}

void PdfObjectStream::CopyToSafe(OutputStream& stream) const
{
    PdfFilterList mediaFilters;
    vector<const PdfDictionary*> decodeParms;
    auto inputStream = const_cast<PdfObjectStream&>(*this).getInputStream(false, mediaFilters, decodeParms);
    inputStream->CopyTo(stream);
    stream.Flush();
}

charbuff PdfObjectStream::GetCopy(bool raw) const
{
    charbuff ret;
    StringStreamDevice stream(ret);
    CopyTo(stream, raw);
    return ret;
}

charbuff PdfObjectStream::GetCopySafe() const
{
    charbuff ret;
    StringStreamDevice stream(ret);
    CopyToSafe(stream);
    return ret;
}

void PdfObjectStream::Unwrap()
{
    if (m_Filters.size() == 0)
        return;

    PdfObject obj;
    auto& objectStream = obj.GetOrCreateStream();
    {
        auto inputStream = GetInputStream();
        auto& mediaFilters = inputStream.GetMediaFilters();
        if (m_Filters.size() == 1 && mediaFilters.size() == 1)
        {
            // Ignore unwrapping when only media
            // filters are found
            return;
        }

        auto outputStream = objectStream.GetOutputStreamRaw();
        inputStream.CopyTo(outputStream);

        // Handle left over media filters/decode parameters
        auto& dict = obj.GetDictionary();
        auto& mediaDecodeParms = inputStream.GetMediaDecodeParms();
        if (mediaFilters.size() == 1)
        {
            dict.AddKey(PdfName::KeyFilter, PdfName(PoDoFo::FilterToName(mediaFilters[0])));
            if (mediaDecodeParms[0] != nullptr)
                dict.AddKeyIndirectSafe(DecodeParmsKey, *mediaDecodeParms[0]);
        }
        else if (mediaFilters.size() > 1)
        {
            PdfArray filters;
            for (unsigned i = 0; i < mediaFilters.size(); i++)
                filters.Add(PdfName(PoDoFo::FilterToName(mediaFilters[i])));

            dict.AddKey(PdfName::KeyFilter, filters);

            if (mediaDecodeParms.size() != 0)
            {
                PdfArray decodeParms;
                decodeParms.Reserve(mediaDecodeParms.size());
                for (unsigned i = 0; i < mediaDecodeParms.size(); i++)
                {
                    auto decodeParmsDict = mediaDecodeParms[i];
                    if (decodeParmsDict == nullptr)
                    {
                        decodeParms.Add(PdfObject::Null);
                        continue;
                    }

                    const PdfObject* owner;
                    if ((owner = decodeParmsDict->GetOwner()) != nullptr
                        && owner->IsIndirect())
                    {
                        decodeParms.Add(owner->GetIndirectReference());
                    }
                    else
                    {
                        decodeParms.Add(*decodeParmsDict);
                    }
                }

                dict.AddKey(DecodeParmsKey, decodeParms);
            }
        }
    }

    MoveFrom(objectStream);
}

PdfObjectStream& PdfObjectStream::operator=(const PdfObjectStream& rhs)
{
    CopyFrom(rhs);
    return (*this);
}

PdfObjectStream& PdfObjectStream::operator=(PdfObjectStream&& rhs) noexcept
{
    MoveFrom(rhs);
    return (*this);
}

void PdfObjectStream::Write(OutputStream& stream, const PdfStatefulEncrypt& encrypt)
{
    m_Provider->Write(stream, encrypt);
}

size_t PdfObjectStream::GetLength() const
{
    return m_Provider->GetLength();
}

void PdfObjectStream::MoveFrom(PdfObjectStream& rhs)
{
    ensureClosed();
    if (!m_Provider->TryMoveFrom(std::move(*rhs.m_Provider)))
    {
        auto stream = rhs.GetInputStream(true);
        this->SetData(stream, true);
        m_Provider->Clear();
    }

    // Fix the /Filter and /DecodeParms keys for
    // both objects after the stream has been moved
    auto& lhsDict = m_Parent->GetDictionary();
    auto& rhsDict = rhs.m_Parent->GetDictionary();
    auto filter = rhsDict.FindKey(PdfName::KeyFilter);
    if (filter == nullptr)
    {
        lhsDict.RemoveKey(PdfName::KeyFilter);
    }
    else
    {
        lhsDict.AddKey(PdfName::KeyFilter, *filter);
        rhsDict.RemoveKey(PdfName::KeyFilter);
    }

    auto decodeParms = rhsDict.FindKey(DecodeParmsKey);
    if (decodeParms == nullptr)
    {
        lhsDict.RemoveKey(DecodeParmsKey);
    }
    else
    {
        lhsDict.AddKey(DecodeParmsKey, *decodeParms);
        rhsDict.RemoveKey(DecodeParmsKey);
    }

    m_Filters = std::move(rhs.m_Filters);
}

void PdfObjectStream::CopyFrom(const PdfObjectStream& rhs)
{
    ensureClosed();
    if (!m_Provider->TryCopyFrom(*rhs.m_Provider))
    {
        auto stream = rhs.GetInputStream(true);
        this->SetData(stream, true);
    }

    // Copy the /Filter and /DecodeParms keys
    auto& lhsDict = m_Parent->GetDictionary();
    auto& rhsDict = rhs.m_Parent->GetDictionary();
    auto filter = rhsDict.FindKey(PdfName::KeyFilter);
    if (filter == nullptr)
        lhsDict.RemoveKey(PdfName::KeyFilter);
    else
        lhsDict.AddKey(PdfName::KeyFilter, *filter);

    auto decodeParms = rhsDict.FindKey(DecodeParmsKey);
    if (decodeParms == nullptr)
        lhsDict.RemoveKey(DecodeParmsKey);
    else
        lhsDict.AddKey(DecodeParmsKey, *decodeParms);

    m_Filters = rhs.m_Filters;
}

void PdfObjectStream::SetData(const bufferview& buffer, bool raw)
{
    ensureClosed();
    SpanStreamDevice stream(buffer);
    if (raw)
        setData(stream, { }, raw, -1, true);
    else
        setData(stream, { DefaultFilter }, raw, -1, true);
}

void PdfObjectStream::SetData(const bufferview& buffer, const PdfFilterList& filters, bool raw)
{
    ensureClosed();
    SpanStreamDevice stream(buffer);
    setData(stream, filters, raw, -1, true);
}

void PdfObjectStream::SetData(InputStream& stream, bool raw)
{
    ensureClosed();
    if (raw)
        setData(stream, { }, raw, -1, true);
    else
        setData(stream, { DefaultFilter }, raw, -1, true);
}

void PdfObjectStream::SetData(InputStream& stream, const PdfFilterList& filters, bool raw)
{
    ensureClosed();
    setData(stream, filters, raw, -1, true);
}

unique_ptr<InputStream> PdfObjectStream::getInputStream(bool raw, PdfFilterList& mediaFilters,
    vector<const PdfDictionary*>& mediaDecodeParms)
{
    if (raw || m_Filters.size() == 0)
    {
        return m_Provider->GetInputStream(*m_Parent);
    }
    else
    {
        vector<const PdfDictionary*> decodeParms(m_Filters.size());
        auto decodeParmsObj = m_Parent->GetDictionary().FindKey(DecodeParmsKey);
        if (decodeParmsObj != nullptr)
        {
            const PdfDictionary* decodeParmsDict;
            const PdfArray* decodeParmsArr;
            if (decodeParmsObj->TryGetDictionary(decodeParmsDict))
            {
                std::fill(decodeParms.begin(), decodeParms.end(), decodeParmsDict);
            }
            else if (decodeParmsObj->TryGetArray(decodeParmsArr))
            {
                for (unsigned i = 0; i < decodeParmsArr->GetSize(); i++)
                {
                    auto decodeParmsEntry = decodeParmsArr->FindAt(i);
                    if (decodeParmsEntry == nullptr || !decodeParmsEntry->TryGetDictionary(decodeParmsDict))
                        continue;

                    decodeParms[i] = decodeParmsDict;
                }
            }
            // Else ignore it
            // TODO: Warning
        }

        auto nonMediaFilters = stripMediaFilters(m_Filters, mediaFilters);
        if (mediaFilters.size() != 0)
        {
            // Split media and non media filters
            mediaDecodeParms.assign(decodeParms.begin() + nonMediaFilters.size(), decodeParms.end());
            decodeParms.resize(nonMediaFilters.size());
        }

        if (nonMediaFilters.size() == 0)
        {
            return m_Provider->GetInputStream(*m_Parent);
        }
        else
        {
            return PdfFilterFactory::CreateDecodeStream(
                m_Provider->GetInputStream(*m_Parent), nonMediaFilters, decodeParms);
        }
    }
}

void PdfObjectStream::setData(InputStream& stream, PdfFilterList filters,
    bool raw, ssize_t size, bool markObjectDirty)
{
    if (markObjectDirty)
    {
        // We must make sure the parent will be set dirty. All methods
        // writing to the stream will call this method first
        m_Parent->SetDirty();
    }

    PdfObjectOutputStream output(*this, std::move(filters), raw, false);
    if (size < 0)
        stream.CopyTo(output);
    else
        stream.CopyTo(output, (size_t)size);
}

void PdfObjectStream::InitData(InputStream& stream, size_t size, PdfFilterList&& filterList)
{
    PdfObjectOutputStream output(*this);
    stream.CopyTo(output, size);
    m_Filters = std::move(filterList);
}

void PdfObjectStream::ensureClosed() const
{
    PODOFO_RAISE_LOGIC_IF(m_locked, "The stream should have no read/write operations in progress");
}

PdfObjectInputStream::PdfObjectInputStream()
    : m_stream(nullptr) { }

PdfObjectInputStream::~PdfObjectInputStream()
{
    if (m_stream != nullptr)
        m_stream->m_locked = false;
}

PdfObjectInputStream::PdfObjectInputStream(PdfObjectInputStream&& rhs) noexcept
{
    utls::move(rhs.m_stream, m_stream);
    utls::move(rhs.m_MediaDecodeParms, m_MediaDecodeParms);
}

PdfObjectInputStream::PdfObjectInputStream(PdfObjectStream& stream, bool raw)
    : m_stream(&stream)
{
    m_stream->m_locked = true;
    m_input = stream.getInputStream(raw, m_MediaFilters, m_MediaDecodeParms);
}

size_t PdfObjectInputStream::readBuffer(char* buffer, size_t size, bool& eof)
{
    return ReadBuffer(*m_input, buffer, size, eof);
}

bool PdfObjectInputStream::readChar(char& ch)
{
    return ReadChar(*m_input, ch);
}

PdfObjectInputStream& PdfObjectInputStream::operator=(PdfObjectInputStream&& rhs) noexcept
{
    utls::move(rhs.m_stream, m_stream);
    return *this;
}

PdfObjectOutputStream::PdfObjectOutputStream()
    : m_stream(nullptr), m_raw(false) { }

PdfObjectOutputStream::~PdfObjectOutputStream()
{
    if (m_stream != nullptr)
    {
        // Set filters on the stream and on the parent object
        // NOTE: if filters are not defined assume we will
        // preserve them on the parent
        if (m_filters.has_value())
        {
            auto& filters = *m_filters;
            if (filters.size() == 0)
            {
                m_stream->GetParent().GetDictionary().RemoveKey(PdfName::KeyFilter);
            }
            else if (filters.size() == 1)
            {
                m_stream->GetParent().GetDictionary().AddKey(PdfName::KeyFilter,
                    PdfName(PoDoFo::FilterToName(filters.front())));
            }
            else // filters.size() > 1
            {
                PdfArray arrFilters;
                for (auto filterType : filters)
                    arrFilters.Add(PdfName(PoDoFo::FilterToName(filterType)));

                m_stream->GetParent().GetDictionary().AddKey(PdfName::KeyFilter, arrFilters);
            }

            m_stream->m_Filters = std::move(filters);
        }

        // Unlock the stream
        m_stream->m_locked = false;

        auto document = m_stream->GetParent().GetDocument();
        if (document != nullptr)
            document->GetObjects().BeginAppendStream(*m_stream);
    }
}

PdfObjectOutputStream::PdfObjectOutputStream(PdfObjectOutputStream&& rhs) noexcept
    : m_filters(std::move(rhs.m_filters))
{
    utls::move(rhs.m_stream, m_stream);
    utls::move(rhs.m_raw, m_raw);
}

PdfObjectOutputStream::PdfObjectOutputStream(PdfObjectStream& stream,
        PdfFilterList&& filters, bool raw, bool append)
    : PdfObjectOutputStream(stream, nullable<PdfFilterList>(std::move(filters)),
        raw, append)
{
}

PdfObjectOutputStream::PdfObjectOutputStream(PdfObjectStream& stream)
    : PdfObjectOutputStream(stream, nullptr, false, false)
{
}

PdfObjectOutputStream::PdfObjectOutputStream(PdfObjectStream& stream,
        nullable<PdfFilterList> filters, bool raw, bool append)
    : m_stream(&stream), m_filters(std::move(filters)), m_raw(raw)
{
    auto document = stream.GetParent().GetDocument();
    if (document != nullptr)
        document->GetObjects().BeginAppendStream(stream);

    charbuff buffer;
    if (append)
        stream.CopyTo(buffer);

    if (m_filters.has_value())
    {
        auto& filters = *m_filters;
        if (filters.size() == 0 || raw)
        {
            m_output = stream.m_Provider->GetOutputStream(stream.GetParent());
        }
        else
        {
            m_output = PdfFilterFactory::CreateEncodeStream(
                stream.m_Provider->GetOutputStream(stream.GetParent()), filters);
        }
    }
    else
    {
        m_output = stream.m_Provider->GetOutputStream(stream.GetParent());
    }

    m_stream->m_locked = true;

    if (buffer.size() != 0)
        WriteBuffer(*m_output, buffer.data(), buffer.size());
}

void PdfObjectOutputStream::writeBuffer(const char* buffer, size_t size)
{
    WriteBuffer(*m_output, buffer, size);
}

void PdfObjectOutputStream::flush()
{
    Flush(*m_output);
}

PdfObjectOutputStream& PdfObjectOutputStream::operator=(PdfObjectOutputStream&& rhs) noexcept
{
    utls::move(rhs.m_stream, m_stream);
    utls::move(rhs.m_raw, m_raw);
    m_output = std::move(rhs.m_output);
    m_filters = std::move(rhs.m_filters);
    return *this;
}

PdfObjectStreamProvider::~PdfObjectStreamProvider() { }

bool PdfObjectStreamProvider::IsLengthHandled() const
{
    return false;
}

// Strip media filters from regular ones
PdfFilterList stripMediaFilters(const PdfFilterList& filters, PdfFilterList& mediaFilters)
{
    PdfFilterList ret;
    for (unsigned i = 0; i < filters.size(); i++)
    {
        PdfFilterType type = filters[i];
        if (isMediaFilter(type))
        {
            mediaFilters.push_back(type);
        }
        else
        {
            if (mediaFilters.size() != 0)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Inconsistent filter with regular filters after media ones");

            ret.push_back(type);
        }
    }

    return ret;
}

bool isMediaFilter(PdfFilterType filterType)
{
    switch (filterType)
    {
        case PdfFilterType::ASCIIHexDecode:
        case PdfFilterType::ASCII85Decode:
        case PdfFilterType::LZWDecode:
        case PdfFilterType::FlateDecode:
        case PdfFilterType::RunLengthDecode:
        case PdfFilterType::Crypt:
            return false;
        case PdfFilterType::CCITTFaxDecode:
        case PdfFilterType::JBIG2Decode:
        case PdfFilterType::DCTDecode:
        case PdfFilterType::JPXDecode:
            return true;
        case PdfFilterType::None:
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

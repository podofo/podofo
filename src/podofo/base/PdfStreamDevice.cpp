/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfStreamDevice.h"

#include <fstream>

#include <podofo/private/FileSystem.h>

using namespace std;
using namespace PoDoFo;

template <typename TStream>
size_t getPosition(TStream& stream)
{
    streampos ret;
    if (stream.eof())
    {
        // tellg() will set failbit when called on a stream that
        // is eof. Reset eofbit and restore it after calling tellg()
        // https://stackoverflow.com/q/18490576/213871
        PODOFO_ASSERT(!stream.fail());
        stream.clear();
        ret = utls::stream_helper<TStream>::tell(stream);
        stream.clear(ios_base::eofbit);
    }
    else
    {
        ret = utls::stream_helper<TStream>::tell(stream);
    }

    return (size_t)ret;
}

template <typename TStream>
size_t getLength(TStream& stream)
{
    streampos currpos;
    if (stream.eof())
    {
        // tellg() will set failbit when called on a stream that
        // is eof. Reset eofbit and restore it after calling tellg()
        // https://stackoverflow.com/q/18490576/213871
        PODOFO_ASSERT(!stream.fail());
        stream.clear();
        currpos = utls::stream_helper<TStream>::tell(stream);
        stream.clear(ios_base::eofbit);
    }
    else
    {
        streampos prevpos = utls::stream_helper<TStream>::tell(stream);
        (void)utls::stream_helper<TStream>::seek(stream, 0, ios_base::end);
        currpos = utls::stream_helper<TStream>::tell(stream);
        if (currpos != prevpos)
            (void)utls::stream_helper<TStream>::seek(stream, prevpos);
    }

    return (size_t)currpos;
}

template <typename TStream>
void seek(TStream& stream, ssize_t pos, SeekDirection direction)
{
    ios_base::seekdir seekdir;
    switch (direction)
    {
        case SeekDirection::Begin:
            seekdir = ios_base::beg;
            break;
        case SeekDirection::Current:
            seekdir = ios_base::cur;
            break;
        case SeekDirection::End:
            seekdir = ios_base::end;
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
    (void)utls::stream_helper<TStream>::seek(stream, (std::streampos)pos, seekdir);
}

StreamDevice::StreamDevice(DeviceAccess access)
    : InputStreamDevice(false), OutputStreamDevice(false)
{
    SetAccess(access);
}

size_t StreamDevice::SeekPosition(size_t curpos, size_t devlen, ssize_t offset, SeekDirection direction)
{
    switch (direction)
    {
        case SeekDirection::Begin:
        {
            if (offset < 0)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Invalid negative seek");
            else if ((size_t)offset > devlen)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Invalid seek out of bounds");

            return (size_t)offset;
        }
        case SeekDirection::Current:
        {
            if (offset == 0)
            {
                // No modification
                return curpos;
            }
            else if (offset > 0)
            {
                if ((size_t)offset > devlen - curpos)
                    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Invalid seek out of bounds");
            }
            else
            {
                if ((size_t)-offset > curpos)
                    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Invalid seek out of bounds");
            }

            return curpos + (size_t)offset;
        }
        case SeekDirection::End:
        {
            if (offset > 0)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Invalid positive seek");
            else if ((size_t)-offset > devlen)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Invalid seek out of bounds");

            return (size_t)(devlen + offset);
        }
        default:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
        }
    }
}

StandardStreamDevice::StandardStreamDevice(ostream& stream)
    : StandardStreamDevice(DeviceAccess::Write, &stream, nullptr, &stream, false)
{
    if (stream.fail())
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDeviceOperation);
}

StandardStreamDevice::StandardStreamDevice(istream& stream)
    : StandardStreamDevice(DeviceAccess::Read, &stream, &stream, nullptr, false)
{
    if (stream.fail())
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDeviceOperation);
}

StandardStreamDevice::StandardStreamDevice(iostream& stream)
    : StandardStreamDevice(DeviceAccess::ReadWrite, &stream, &stream, &stream, false)
{
    if (stream.fail())
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDeviceOperation);

    if (stream.tellp() != stream.tellg())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation,
            "Unsupported mistmatch between read and read position in stream");
}

StandardStreamDevice::StandardStreamDevice(DeviceAccess access, ios& stream, bool streamOwned)
    : StandardStreamDevice(access, &stream, dynamic_cast<istream*>(&stream), dynamic_cast<ostream*>(&stream), streamOwned)
{
}

StandardStreamDevice::StandardStreamDevice(DeviceAccess access, ios* stream, istream* istream, ostream* ostream, bool streamOwned) :
    StreamDevice(access),
    m_Stream(stream),
    m_istream(istream),
    m_ostream(ostream),
    m_StreamOwned(streamOwned)
{
    // TODO1: check stream is either istream/ostream/iostream
}

StandardStreamDevice::~StandardStreamDevice()
{
    if (m_StreamOwned)
        delete m_Stream;
}

size_t StandardStreamDevice::GetLength() const
{
    size_t ret;
    switch (GetAccess())
    {
        case DeviceAccess::Read:
        {
            PODOFO_INVARIANT(m_istream != nullptr);
            ret = getLength(*m_istream);
            break;
        }
        case DeviceAccess::Write:
        case DeviceAccess::ReadWrite:
        {
            PODOFO_INVARIANT(m_ostream != nullptr);
            ret = getLength(*m_ostream);
            break;
        }
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    if (m_Stream->fail())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to retrieve length for this stream");

    return ret;
}

size_t StandardStreamDevice::GetPosition() const
{
    size_t ret;
    switch (GetAccess())
    {
        case DeviceAccess::Read:
        {
            PODOFO_INVARIANT(m_istream != nullptr);
            ret = getPosition(*m_istream);
            break;
        }
        case DeviceAccess::Write:
        case DeviceAccess::ReadWrite:
        {
            PODOFO_INVARIANT(m_ostream != nullptr);
            ret = getPosition(*m_ostream);
            break;
        }
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    if (m_Stream->fail())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to get current position in the stream");

    return ret;
}

bool StandardStreamDevice::CanSeek() const
{
    return true;
}

bool StandardStreamDevice::Eof() const
{
    return m_Stream->eof();
}

void StandardStreamDevice::writeBuffer(const char* buffer, size_t size)
{
    PODOFO_INVARIANT(m_ostream != nullptr);
    m_ostream->write(buffer, size);
    if (m_ostream->fail())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to write the given buffer");
}

void StandardStreamDevice::flush()
{
    PODOFO_INVARIANT(m_ostream != nullptr);
    m_ostream->flush();
    if (m_ostream->fail())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to flush the stream");
}

size_t StandardStreamDevice::readBuffer(char* buffer, size_t size, bool& eof)
{
    PODOFO_INVARIANT(m_Stream != nullptr);
    if (m_istream->eof())
    {
        eof = true;
        return 0;
    }

    return utls::ReadBuffer(*m_istream, buffer, size, eof);
}

bool StandardStreamDevice::readChar(char& ch)
{
    PODOFO_INVARIANT(m_Stream != nullptr);
    if (m_istream->eof())
    {
        ch = '\0';
        return false;
    }

    return utls::ReadChar(*m_istream, ch);
}

bool StandardStreamDevice::peek(char& ch) const
{
    PODOFO_INVARIANT(m_istream != nullptr);

    int read;

    // NOTE: We don't want a peek() call to set failbit
    if (m_istream->eof())
        goto Eof;

    read = m_istream->peek();
    if (m_istream->fail())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to peek current character");

    if (read == char_traits<char>::eof())
        goto Eof;

    ch = (char)read;
    return true;

Eof:
    ch = '\0';
    return false;
}

void StandardStreamDevice::seek(ssize_t offset, SeekDirection direction)
{
    // NOTE: Some c++ libraries don't reset eofbit prior seeking
    m_Stream->clear(m_Stream->rdstate() & ~ios_base::eofbit);
    if ((GetAccess() & DeviceAccess::Read) != DeviceAccess{ })
    {
        PODOFO_INVARIANT(m_istream != nullptr);
        ::seek(*m_istream, offset, direction);
    }

    if ((GetAccess() & DeviceAccess::Write) != DeviceAccess{ })
    {
        PODOFO_INVARIANT(m_ostream != nullptr);
        ::seek(*m_ostream, offset, direction);
    }

    if (m_Stream->fail())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to seek to given position in the stream");
}

FileStreamDevice::FileStreamDevice(const string_view& filepath)
    : FileStreamDevice(filepath, FileMode::Open, DeviceAccess::Read)
{
}

FileStreamDevice::FileStreamDevice(const string_view& filepath, FileMode mode)
    : FileStreamDevice(filepath, mode, mode == FileMode::Append ? DeviceAccess::Write : DeviceAccess::ReadWrite)
{
}

FileStreamDevice::FileStreamDevice(const string_view& filepath, FileMode mode, DeviceAccess access)
    : StandardStreamDevice(access, *getFileStream(filepath, mode, access), true), m_Filepath(filepath)
{
}

void FileStreamDevice::close()
{
    dynamic_cast<fstream&>(GetStream()).close();
}

fstream* FileStreamDevice::getFileStream(const string_view& filename, FileMode mode, DeviceAccess access)
{
    switch (mode)
    {
        case FileMode::CreateNew:
            if (access == DeviceAccess::Read)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Invalid combination FileMode::CreateNew and DeviceAccess::Read");
            break;
        case FileMode::Create:
            if (access == DeviceAccess::Read)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Invalid combination FileMode::Create and DeviceAccess::Read");
            break;
        case FileMode::OpenOrCreate:
            if (access == DeviceAccess::Read)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Invalid combination FileMode::OpenOrCreate and DeviceAccess::Read");
            break;
        case FileMode::Truncate:
            if (access == DeviceAccess::Read)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Invalid combination FileMode::Truncate and DeviceAccess::Read");
            break;
        case FileMode::Append:
            if ((access & DeviceAccess::Read) != DeviceAccess{ })
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Invalid combination FileMode::Append and DeviceAccess::Read or DeviceAccess::ReadWrite");
            break;
        default:
            break;
    }

    ios_base::openmode openmode = fstream::binary;
    switch (access)
    {
        case DeviceAccess::Read:
            openmode |= ios_base::in;
            break;
        case DeviceAccess::Write:
            openmode |= ios_base::out;
            break;
        case DeviceAccess::ReadWrite:
            openmode |= ios_base::in | ios_base::out;
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    switch (mode)
    {
        case FileMode::CreateNew:
            if (fs::exists(filename))
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "The file must not exist");
            break;
        case FileMode::Create:
            openmode |= ios_base::trunc;
            break;
        case FileMode::Open:
            if ((access & DeviceAccess::Write) != DeviceAccess{ } && !fs::exists(filename))
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "The file must exist");
            break;
        case FileMode::OpenOrCreate:
            break;
        case FileMode::Truncate:
            if (!fs::exists(filename))
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "The file must exist");
            openmode |= ios_base::trunc;
            break;
        case FileMode::Append:
            // NOTE: ios_base::app and ios_base::ate don't work as intended,
            // as tellp returns 0. Also opening the file with ios_base::out
            // will truncate the file anyway, even without ios_base::trunc.
            // So, we have to create the file read/write and do our seeking
            openmode |= ios_base::in;
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    auto stream = new fstream(utls::open_fstream(filename, openmode));
    if (mode == FileMode::Append)
    {
        // NOTE: Do our manual seeking at the end
        stream->seekg(0, ios_base::end);
        stream->seekp(0, ios_base::end);

        // TODO: We should prevent seeking before the initial
        // position
    }

    if (stream->fail())
    {
        delete stream;
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, filename);
    }

    return stream;
}

NullStreamDevice::NullStreamDevice()
    : StreamDevice(DeviceAccess::ReadWrite), m_Length(0), m_Position(0)
{
}

bool NullStreamDevice::peek(char& ch) const
{
    ch = '\0';
    return m_Length != m_Position;
}

size_t NullStreamDevice::GetLength() const
{
    return m_Length;
}

size_t NullStreamDevice::GetPosition() const
{
    return m_Position;
}

bool NullStreamDevice::Eof() const
{
    return m_Position == m_Length;
}

void NullStreamDevice::writeBuffer(const char* buffer, size_t size)
{
    (void)buffer;
    m_Position = m_Position + size;
    if (m_Position > m_Length)
        m_Length = m_Position;
}

size_t NullStreamDevice::readBuffer(char* buffer, size_t size, bool& eof)
{
    (void)buffer;
    size_t prevpos = m_Position;
    m_Position = std::min(m_Length, m_Position + size);
    eof = m_Position == m_Length;
    return m_Position - prevpos;
}

bool NullStreamDevice::readChar(char& ch)
{
    ch = '\0';
    if (m_Position == m_Length)
        return false;

    m_Position++;
    return false;
}

void NullStreamDevice::seek(ssize_t offset, SeekDirection direction)
{
    m_Position = SeekPosition(m_Position, m_Length, offset, direction);
}

SpanStreamDevice::SpanStreamDevice(const char* buffer, size_t size)
    : StreamDevice(DeviceAccess::Read), m_buffer(const_cast<char*>(buffer)), m_Length(size), m_Position(0)
{
}

SpanStreamDevice::SpanStreamDevice(const bufferview& buffer)
    : SpanStreamDevice(buffer.data(), buffer.size())
{
}

SpanStreamDevice::SpanStreamDevice(const string_view& view)
    : SpanStreamDevice(view.data(), view.size())
{
}

SpanStreamDevice::SpanStreamDevice(const string& str)
    : SpanStreamDevice(str.data(), str.size())
{
}

SpanStreamDevice::SpanStreamDevice(string& str, DeviceAccess access)
    : SpanStreamDevice(str.data(), str.size(), access)
{
}

SpanStreamDevice::SpanStreamDevice(const char* str)
    : SpanStreamDevice(str, char_traits<char>::length(str))
{
}

SpanStreamDevice::SpanStreamDevice(char* buffer, size_t size, DeviceAccess access)
    : StreamDevice(access), m_buffer(buffer), m_Length(size), m_Position(0)
{
}

SpanStreamDevice::SpanStreamDevice(const bufferspan& span, DeviceAccess access)
    : SpanStreamDevice(span.data(), span.size(), access)
{
}

size_t SpanStreamDevice::GetLength() const
{
    return m_Length;
}

size_t SpanStreamDevice::GetPosition() const
{
    return m_Position;
}

bool SpanStreamDevice::Eof() const
{
    return m_Position == m_Length;
}

bool SpanStreamDevice::CanSeek() const
{
    return true;
}

void SpanStreamDevice::writeBuffer(const char* buffer, size_t size)
{
    if (m_Position + size > m_Length)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Attempt to write out of span bounds");

    std::memcpy(m_buffer + m_Position, buffer, size);
    m_Position += size;
}

size_t SpanStreamDevice::readBuffer(char* buffer, size_t size, bool& eof)
{
    size_t readCount = std::min(size, m_Length - m_Position);
    std::memcpy(buffer, m_buffer + m_Position, readCount);
    m_Position += readCount;
    eof = m_Position == m_Length;
    return readCount;
}

bool SpanStreamDevice::readChar(char& ch)
{
    if (m_Position == m_Length)
    {
        ch = '\0';
        return false;
    }

    ch = m_buffer[m_Position];
    m_Position++;
    return true;
}

bool SpanStreamDevice::peek(char& ch) const
{
    if (m_Position == m_Length)
    {
        ch = '\0';
        return false;
    }

    ch = m_buffer[m_Position];
    return true;
}

void SpanStreamDevice::seek(ssize_t offset, SeekDirection direction)
{
    m_Position = SeekPosition(m_Position, m_Length, offset, direction);
}

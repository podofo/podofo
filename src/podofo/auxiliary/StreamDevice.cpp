/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include <podofo/auxiliary/StreamDevice.h>

#include <fstream>

#include <podofo/private/FileSystem.h>

using namespace std;
using namespace PoDoFo;

namespace
{
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
    switch (direction)
    {
        case SeekDirection::Begin:
            (void)utls::stream_helper<TStream>::seek(stream, (std::streampos)pos, ios_base::beg);
            break;
        case SeekDirection::Current:
            (void)utls::stream_helper<TStream>::seek(stream, (std::streampos)pos, ios_base::cur);
            break;
        case SeekDirection::End:
            (void)utls::stream_helper<TStream>::seek(stream, (std::streampos)pos, ios_base::end);
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

}

static FILE* createFile(const string_view& filename, FileMode mode, DeviceAccess access);

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
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Invalid negative seek");
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
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Invalid positive seek");
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
        PODOFO_RAISE_ERROR(PdfErrorCode::IOError);
}

StandardStreamDevice::StandardStreamDevice(istream& stream)
    : StandardStreamDevice(DeviceAccess::Read, &stream, &stream, nullptr, false)
{
    if (stream.fail())
        PODOFO_RAISE_ERROR(PdfErrorCode::IOError);
}

StandardStreamDevice::StandardStreamDevice(iostream& stream)
    : StandardStreamDevice(DeviceAccess::ReadWrite, &stream, &stream, &stream, false)
{
    if (stream.fail())
        PODOFO_RAISE_ERROR(PdfErrorCode::IOError);

    if (stream.tellp() != stream.tellg())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError,
            "Unsupported mismatch between read and read position in stream");
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
        case DeviceAccess::ReadWrite: // We just take the input stream as the reference
        {
            PODOFO_INVARIANT(m_istream != nullptr);
            ret = getLength(*m_istream);
            break;
        }
        case DeviceAccess::Write:
        {
            PODOFO_INVARIANT(m_ostream != nullptr);
            ret = getLength(*m_ostream);
            break;
        }
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    if (m_Stream->fail())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to retrieve length for this stream");

    return ret;
}

size_t StandardStreamDevice::GetPosition() const
{
    size_t ret;
    switch (GetAccess())
    {
        case DeviceAccess::Read:
        case DeviceAccess::ReadWrite: // We just take the input stream as the reference
        {
            PODOFO_INVARIANT(m_istream != nullptr);
            ret = getPosition(*m_istream);
            break;
        }
        case DeviceAccess::Write:
        {
            PODOFO_INVARIANT(m_ostream != nullptr);
            ret = getPosition(*m_ostream);
            break;
        }
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    if (m_Stream->fail())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to get current position in the stream");

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
    switch (GetAccess())
    {
        case DeviceAccess::Write:
        {
            PODOFO_INVARIANT(m_ostream != nullptr);
            m_ostream->write(buffer, size);
            if (m_ostream->fail())
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to write the given buffer");
            break;
        }
        case DeviceAccess::ReadWrite:
        {
            PODOFO_INVARIANT(m_ostream != nullptr && m_istream != nullptr);
            // Since some iostreams such as std::stringstream have different position
            // indicators for input and output sequences. Synchronize the ostream
            // position indicator with the istream (the reference one) before writing
            // NOTE: Some c++ libraries don't reset eofbit prior seeking
            auto pos = getPosition(*m_istream);
            m_Stream->clear(m_Stream->rdstate() & ~ios_base::eofbit);
            ::seek(*m_ostream, (ssize_t)pos, SeekDirection::Begin);
            m_ostream->write(buffer, size);
            if (m_ostream->fail())
                goto Fail;

            // After writing, finally synchronize the istream position indicator
            ::seek(*m_istream, (ssize_t)(pos + size), SeekDirection::Begin);
            if (m_istream->fail())
                goto Fail;

            break;

        Fail:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to write the given buffer");
            break;
        }
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
    }
}

void StandardStreamDevice::flush()
{
    PODOFO_INVARIANT(m_ostream != nullptr);
    m_ostream->flush();
    if (m_ostream->fail())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to flush the stream");
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
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to peek current character");

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
    switch (GetAccess())
    {
        case DeviceAccess::Read:
        case DeviceAccess::ReadWrite:
        {
            PODOFO_INVARIANT(m_istream != nullptr);
            ::seek(*m_istream, offset, direction);
            break;
        }
        case DeviceAccess::Write:
        {
            PODOFO_INVARIANT(m_ostream != nullptr);
            ::seek(*m_ostream, offset, direction);
            break;
        }
    }

    if (m_Stream->fail())
        goto Fail;

    return;

Fail:
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to seek to given position in the stream");
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
    : StreamDevice(access), m_file(createFile(filepath, mode, access))
{
}

FileStreamDevice::~FileStreamDevice()
{
    try
    {
        close();
    }
    catch (...)
    {
        // Do nothing, it should not throw
    }
}

size_t FileStreamDevice::GetLength() const
{
    ssize_t offset;
    ssize_t previousOffset;
    previousOffset = utls::ftell(m_file);
    if (previousOffset == -1)
        goto Fail;

    if (utls::fseek(m_file, 0, SEEK_END) != 0)
        goto Fail;

    offset = utls::ftell(m_file);
    if (offset == -1)
        goto Fail;

    if (utls::fseek(m_file, previousOffset, SEEK_SET) != 0)
        goto Fail;

    return offset;

Fail:
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to determine the current file length");
}

size_t FileStreamDevice::GetPosition() const
{
    ssize_t offset = utls::ftell(m_file);
    if (offset == -1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to read the current file position");

    return offset;
}

bool FileStreamDevice::CanSeek() const
{
    return true;
}

bool FileStreamDevice::Eof() const
{
    return std::feof(m_file) != 0;
}

void FileStreamDevice::writeBuffer(const char* buffer, size_t size)
{
    if (std::fwrite(buffer, sizeof(char), size, m_file) != size)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to write the given buffer");
}

void FileStreamDevice::flush()
{
    int rc = std::fflush(m_file);
    if (rc == EOF)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to flush the stream");
}

size_t FileStreamDevice::readBuffer(char* buffer, size_t size, bool& eof)
{
    size_t ret = std::fread(buffer, 1, (size_t)size, m_file);
    if (std::ferror(m_file) != 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to read the amount of bytes requested");

    if (std::feof(m_file) != 0)
    {
        eof = true;
        return ret;
    }

    eof = false;
    return ret;
}

bool FileStreamDevice::readChar(char& ch)
{
    int rc = std::fgetc(m_file);
    if (std::ferror(m_file) != 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Stream I/O error while reading");

    if (std::feof(m_file) != 0)
    {
        ch = '\0';
        return false;
    }

    ch = (char)(unsigned char)rc;
    return true;
}

bool FileStreamDevice::peek(char& ch) const
{
    int rc = std::fgetc(m_file);
    if (std::ferror(m_file) != 0)
        goto Fail;

    if (std::feof(m_file) != 0)
    {
        ch = '\0';
        return false;
    }

    if (std::ungetc(rc, m_file) == EOF)
        goto Fail;

    ch = (char)(unsigned char)rc;
    return true;

Fail:
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Stream I/O error while reading");
}

void FileStreamDevice::seek(ssize_t offset, SeekDirection direction)
{
    int origin;
    switch (direction)
    {
        case SeekDirection::Begin:
            origin = SEEK_SET;
            break;
        case SeekDirection::Current:
            origin = SEEK_CUR;
            break;
        case SeekDirection::End:
            origin = SEEK_END;
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    if (utls::fseek(m_file, offset, origin) != 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to seek to given position in the stream");
}

void FileStreamDevice::close()
{
    if (m_file == nullptr)
        return;

    if (std::fclose(m_file) == EOF)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to close stream");

    m_file = nullptr;
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

FILE* createFile(const string_view& filepath, FileMode mode, DeviceAccess access)
{
    string cmode;
    switch (mode)
    {
        case FileMode::CreateNew:
        {
            if (access == DeviceAccess::Read)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Invalid combination FileMode::CreateNew and DeviceAccess::Read");

            if (fs::exists(fs::u8path(filepath)))
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "The file {} must not exist", filepath);

            switch (access)
            {
                case DeviceAccess::Write:
                    cmode.append("w");
                    break;
                case DeviceAccess::ReadWrite:
                    cmode.append("w+");
                    break;
                default:
                    PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
            }

            break;
        }
        case FileMode::Create:
        {
            if (access == DeviceAccess::Read)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Invalid combination FileMode::Create and DeviceAccess::Read");

            switch (access)
            {
                case DeviceAccess::Write:
                    cmode.append("w");
                    break;
                case DeviceAccess::ReadWrite:
                    cmode.append("w+");
                    break;
                default:
                    PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
            }

            break;
        }
        case FileMode::Open:
        {
            if ((access & DeviceAccess::Write) != DeviceAccess{ } && !fs::exists(fs::u8path(filepath)))
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "The file {} must exist", filepath);

            switch (access)
            {
                case DeviceAccess::Read:
                    cmode.append("r");
                    break;
                case DeviceAccess::Write:
                    cmode.append("w");
                    break;
                case DeviceAccess::ReadWrite:
                    cmode.append("r+");
                    break;
                default:
                    PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
            }

            break;
        }
        case FileMode::OpenOrCreate:
        {
            if (access == DeviceAccess::Read)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Invalid combination FileMode::OpenOrCreate and DeviceAccess::Read");

            bool exists = fs::exists(fs::u8path(filepath));
            switch (access)
            {
                case DeviceAccess::Write:
                    cmode.append("w");
                    break;
                case DeviceAccess::ReadWrite:
                    cmode.append(exists ? "r+" : "w+");
                    break;
                default:
                    PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
            }

            break;
        }
        case FileMode::Truncate:
        {
            if (access == DeviceAccess::Read)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Invalid combination FileMode::Truncate and DeviceAccess::Read");

            if (!fs::exists(fs::u8path(filepath)))
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "The file {} must exist", filepath);

            switch (access)
            {
                case DeviceAccess::Write:
                    cmode.append("w");
                    break;
                case DeviceAccess::ReadWrite:
                    cmode.append("w+");
                    break;
                default:
                    PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
            }
            break;
        }
        case FileMode::Append:
            if (access == DeviceAccess::Read)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Invalid combination FileMode::Append and DeviceAccess::Read");

            switch (access)
            {
                case DeviceAccess::Write:
                    cmode.append("a");
                    break;
                case DeviceAccess::ReadWrite:
                    cmode.append("a+");
                    break;
                default:
                    PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
            }

            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
#if _WIN32
    cmode.append("b");
#endif // _WIN32

    auto stream = utls::fopen(filepath, cmode.data());
    if (stream == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Error accessing file {}", filepath);

    return stream;
}

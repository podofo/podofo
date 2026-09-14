// SPDX-FileCopyrightText: 2006 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include <podofo/auxiliary/StreamDevice.h>

#include <fstream>

#include <climits>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#else // !_WIN32
#include <cerrno>
#include <unistd.h>
#endif // _WIN32

#include <podofo/private/FileSystem.h>

using namespace std;
using namespace PoDoFo;

// Capacity of the FileStreamDevice buffer, which serves
// both directions, one at a time
constexpr size_t BufferSize = 4096;

static int openFile(const string_view& filename, FileMode mode, DeviceAccess access);
static size_t readFd(int fd, char* buffer, size_t size);
static void writeFd(int fd, const char* buffer, size_t size);
static size_t seekFd(int fd, ssize_t offset, int origin);
static size_t getFdLength(int fd);
static int closeFd(int fd);

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

StreamDevice::StreamDevice(DeviceAccess access)
    : InputStreamDevice(false), OutputStreamDevice(false)
{
    SetAccess(access);
}

void StreamDevice::resetBuffers()
{
    InputStreamDevice::resetBuffers();
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

void StandardStreamDevice::truncate()
{
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
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
    : StreamDevice(access),
    m_Filepath(filepath),
    m_Buffer(new char[BufferSize]),
    m_BufferOffset(0),
    m_Filled(0),
    m_Pending(0),
    m_Position(0),
    m_FdOffset(0),
    m_Direction(BufferDirection::None),
    m_Eof(false),
    m_fd(openFile(filepath, mode, access))
{
    if (mode == FileMode::Append)
    {
        // NOTE: O_APPEND is deliberately not used. It would leave the
        // initial position at 0 and relocate every write to the end of
        // file, making a derived write position disagree with where the
        // bytes land. This seek reproduces fopen("a") initial position
        m_FdOffset = m_Position = seekFd(m_fd, 0, SEEK_END);
    }
}

FileStreamDevice::~FileStreamDevice()
{
    try
    {
        close();
    }
    catch (...)
    {
        // NOTE: With a locally owned write buffer a failed final
        // flush would be silent data loss otherwise
        LogMessage(PdfLogSeverity::Error, "Failed to flush and close the file {}", m_Filepath);
    }
}

size_t FileStreamDevice::GetLength() const
{
    ensureOpen();
    size_t length = getFdLength(m_fd);
    if (m_Direction == BufferDirection::Write)
    {
        // The post flush length, without flushing
        return std::max(length, m_BufferOffset + m_Pending);
    }

    return length;
}

size_t FileStreamDevice::GetPosition() const
{
    switch (m_Direction)
    {
        case BufferDirection::Read:
            return m_BufferOffset + (size_t)(m_head - m_Buffer.get());
        case BufferDirection::Write:
            return m_BufferOffset + m_Pending;
        default:
            return m_Position;
    }
}

bool FileStreamDevice::CanSeek() const
{
    return true;
}

bool FileStreamDevice::Eof() const
{
    return m_head == m_tail && m_Eof;
}

void FileStreamDevice::truncate()
{
    // NOTE: An implementation must keep this invalidation, or the retained
    // buffer content could outlive the bytes it describes
    m_Filled = 0;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

void FileStreamDevice::writeBuffer(const char* buffer, size_t size)
{
    assertInvariants();
    if (m_Direction != BufferDirection::Write)
        beginWrite();

    if (size >= BufferSize)
    {
        // Bypass the buffer for large writes, as fwrite does
        flushWrite();
        writeFd(m_fd, buffer, size);
        m_FdOffset += size;
        m_BufferOffset = m_FdOffset;
        return;
    }

    if (m_Pending + size > BufferSize)
        flushWrite();

    std::memcpy(m_Buffer.get() + m_Pending, buffer, size);
    m_Pending += size;
}

void FileStreamDevice::flush()
{
    flushWrite();
}

size_t FileStreamDevice::readBuffer(char* buffer, size_t size, bool& eof)
{
    assertInvariants();
    if (size == 0)
    {
        // Nothing to serve: refilling here would just discard the retained content
        eof = false;
        return 0;
    }

    // Serve from the read window first: while armed it owns the logical position
    size_t count = 0;
    if (m_head != m_tail)
    {
        count = std::min(size, (size_t)(m_tail - m_head));
        std::memcpy(buffer, m_head, count);
        m_head += count;
        if (count == size)
        {
            eof = false;
            return count;
        }
    }

    size_t remaining = size - count;
    if (remaining < BufferSize)
    {
        refill();
        size_t read = std::min(remaining, (size_t)(m_tail - m_head));
        std::memcpy(buffer + count, m_head, read);
        m_head += read;
        eof = m_head == m_tail && m_Eof;
        return count + read;
    }

    // Read straight into the caller's buffer, as fread does for large requests
    dropBuffers();
    ensureOpen();
    syncFdOffset();
    m_Filled = 0;
    size_t read = readFd(m_fd, buffer + count, remaining);
    m_FdOffset += read;
    m_Position = m_FdOffset;
    m_Eof = read != remaining;
    eof = m_Eof;
    return count + read;
}

bool FileStreamDevice::readChar(char& ch)
{
    if (m_head == m_tail)
    {
        refill();
        if (m_head == m_tail)
        {
            ch = '\0';
            return false;
        }
    }

    ch = *m_head++;
    return true;
}

bool FileStreamDevice::peek(char& ch) const
{
    if (m_head == m_tail)
    {
        // NOTE: peek() is the only method that genuinely has to
        // mutate, since its slow path refills
        const_cast<FileStreamDevice&>(*this).refill();
        if (m_head == m_tail)
        {
            ch = '\0';
            return false;
        }
    }

    ch = *m_head;
    return true;
}

void FileStreamDevice::seek(ssize_t offset, SeekDirection direction)
{
    ensureOpen();
    size_t pos;
    switch (direction)
    {
        case SeekDirection::Begin:
        {
            if (offset < 0)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to seek to given position in the stream");

            pos = (size_t)offset;
            break;
        }
        case SeekDirection::Current:
        {
            // NOTE: resetBuffers() committed the logical position to m_Position,
            // while the OS file offset may still be ahead of it after a read
            if (offset < 0 && (size_t)-offset > m_Position)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to seek to given position in the stream");

            pos = m_Position + (size_t)offset;
            break;
        }
        case SeekDirection::End:
        {
            // The length is needed anyway, so just let the OS compute the position
            m_FdOffset = m_Position = seekFd(m_fd, offset, SEEK_END);
            return;
        }
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    if (tryRetainBuffer(pos))
        return;

    m_FdOffset = m_Position = seekFd(m_fd, (ssize_t)pos, SEEK_SET);
}

void FileStreamDevice::close()
{
    if (m_fd == -1)
        return;

    int fd = m_fd;
    try
    {
        flushWrite();
    }
    catch (...)
    {
        m_fd = -1;
        (void)closeFd(fd);
        throw;
    }

    m_fd = -1;
    if (closeFd(fd) != 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to close stream");
}

void FileStreamDevice::resetBuffers()
{
    dropBuffers();
    // NOTE: A hand rolled EOF flag is sticky unless something clears
    // it, while fseek() cleared stdio's EOF indicator
    m_Eof = false;
    StreamDevice::resetBuffers();
}

void FileStreamDevice::refill()
{
    assertInvariants();
    PODOFO_ASSERT(m_head == m_tail);
    ensureOpen();
    if (m_Direction == BufferDirection::Write)
    {
        // Never fill over unflushed data
        flushWrite();
        m_Position = m_FdOffset;
        m_Direction = BufferDirection::None;
    }

    if (m_Direction == BufferDirection::None)
        syncFdOffset();

    size_t read = readFd(m_fd, m_Buffer.get(), BufferSize);
    m_BufferOffset = m_FdOffset;
    m_Filled = read;
    m_FdOffset += read;
    m_head = m_Buffer.get();
    m_tail = m_Buffer.get() + read;
    m_Direction = BufferDirection::Read;
    // A short read on a regular file means the end was reached
    m_Eof = read != BufferSize;
}

void FileStreamDevice::flushWrite()
{
    if (m_Direction != BufferDirection::Write || m_Pending == 0)
        return;

    assertInvariants();
    ensureOpen();
    writeFd(m_fd, m_Buffer.get(), m_Pending);
    m_FdOffset += m_Pending;
    // NOTE: Re-anchoring here is what keeps GetPosition() flush invariant
    m_BufferOffset = m_FdOffset;
    m_Pending = 0;
}

void FileStreamDevice::beginWrite()
{
    dropBuffers();
    ensureOpen();
    // Read ahead may have left the OS file offset past the logical position
    syncFdOffset();
    m_BufferOffset = m_FdOffset;
    m_Filled = 0;
    m_Pending = 0;
    // NOTE: This is the one path into the Write direction that doesn't go
    // through resetBuffers(), so a read that had reached EOF before the
    // switch would leave Eof() reporting true on a device being written
    m_Eof = false;
    m_Direction = BufferDirection::Write;
}

void FileStreamDevice::dropBuffers()
{
    switch (m_Direction)
    {
        case BufferDirection::Read:
            m_Position = m_BufferOffset + (size_t)(m_head - m_Buffer.get());
            break;
        case BufferDirection::Write:
            flushWrite();
            m_Position = m_BufferOffset + m_Pending;
            break;
        default:
            return;
    }

    m_head = nullptr;
    m_tail = nullptr;
    m_Pending = 0;
    m_Direction = BufferDirection::None;
}

void FileStreamDevice::syncFdOffset()
{
    PODOFO_ASSERT(m_Direction == BufferDirection::None);
    if (m_FdOffset == m_Position)
        return;

    m_FdOffset = seekFd(m_fd, (ssize_t)m_Position, SEEK_SET);
}

bool FileStreamDevice::tryRetainBuffer(size_t pos)
{
    PODOFO_ASSERT(m_Direction == BufferDirection::None);
    if (m_Filled == 0 || pos < m_BufferOffset || pos >= m_BufferOffset + m_Filled)
        return false;

    // The content is valid, but a previous seek left the OS file offset
    // elsewhere: restore it so the next refill() resumes after the buffer
    size_t end = m_BufferOffset + m_Filled;
    if (m_FdOffset != end)
        m_FdOffset = seekFd(m_fd, (ssize_t)end, SEEK_SET);

    m_head = m_Buffer.get() + (pos - m_BufferOffset);
    m_tail = m_Buffer.get() + m_Filled;
    m_Position = pos;
    m_Direction = BufferDirection::Read;
    return true;
}

void FileStreamDevice::ensureOpen() const
{
    if (m_fd == -1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "The file {} is closed", m_Filepath);
}

void FileStreamDevice::assertInvariants() const
{
    PODOFO_ASSERT(m_Direction != BufferDirection::Write || m_BufferOffset == m_FdOffset);
    PODOFO_ASSERT(m_Direction != BufferDirection::Read || (m_head != nullptr && m_head <= m_tail
        && (size_t)(m_tail - m_Buffer.get()) == m_Filled && m_BufferOffset + m_Filled == m_FdOffset));
    PODOFO_ASSERT(m_Direction != BufferDirection::Write || m_head == nullptr);
    PODOFO_ASSERT(m_Direction != BufferDirection::None || (m_head == nullptr && m_Pending == 0));
    PODOFO_ASSERT(m_Filled == 0 || m_Pending == 0);
    PODOFO_ASSERT(m_Filled <= BufferSize && m_Pending <= BufferSize);
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
    return true;
}

void NullStreamDevice::seek(ssize_t offset, SeekDirection direction)
{
    m_Position = SeekPosition(m_Position, m_Length, offset, direction);
}

void NullStreamDevice::truncate()
{
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

SpanStreamDevice::SpanStreamDevice(const char* buffer, size_t size)
    : StreamDevice(DeviceAccess::Read), m_buffer(const_cast<char*>(buffer)), m_Length(size), m_Position(0)
{
    tryEnableReadWindow();
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
    tryEnableReadWindow();
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
    return getPos();
}

bool SpanStreamDevice::Eof() const
{
    return getPos() == m_Length;
}

bool SpanStreamDevice::CanSeek() const
{
    return true;
}

void SpanStreamDevice::writeBuffer(const char* buffer, size_t size)
{
    size_t pos = getPos();
    if (pos + size > m_Length)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Attempt to write out of span bounds");

    std::memcpy(m_buffer + pos, buffer, size);
    setPos(pos + size);
}

size_t SpanStreamDevice::readBuffer(char* buffer, size_t size, bool& eof)
{
    size_t pos = getPos();
    size_t readCount = std::min(size, m_Length - pos);
    std::memcpy(buffer, m_buffer + pos, readCount);
    setPos(pos + readCount);
    eof = pos + readCount == m_Length;
    return readCount;
}

bool SpanStreamDevice::readChar(char& ch)
{
    size_t pos = getPos();
    if (pos == m_Length)
    {
        ch = '\0';
        return false;
    }

    ch = m_buffer[pos];
    setPos(pos + 1);
    return true;
}

bool SpanStreamDevice::peek(char& ch) const
{
    size_t pos = getPos();
    if (pos == m_Length)
    {
        ch = '\0';
        return false;
    }

    ch = m_buffer[pos];
    return true;
}

void SpanStreamDevice::seek(ssize_t offset, SeekDirection direction)
{
    // NOTE: resetBuffers() already committed the position to m_Position
    m_Position = SeekPosition(m_Position, m_Length, offset, direction);
    tryEnableReadWindow();
}

void SpanStreamDevice::truncate()
{
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

void SpanStreamDevice::resetBuffers()
{
    m_Position = getPos();
    StreamDevice::resetBuffers();
}

size_t SpanStreamDevice::getPos() const
{
    // NOTE: Test the arm flag: a fully consumed span is armed and
    // empty and its position still lives in the window
    return m_tail == nullptr ? m_Position : (size_t)(m_head - m_buffer);
}

void SpanStreamDevice::setPos(size_t pos)
{
    m_Position = pos;
    if (m_tail != nullptr)
        m_head = m_buffer + pos;
}

void SpanStreamDevice::tryEnableReadWindow()
{
    if ((GetAccess() & DeviceAccess::Read) != DeviceAccess{ })
        enableReadWindow(m_buffer + m_Position, m_buffer + m_Length);
}

int openFile(const string_view& filepath, FileMode mode, DeviceAccess access)
{
    int flags;
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
                    flags = O_WRONLY | O_CREAT | O_EXCL;
                    break;
                case DeviceAccess::ReadWrite:
                    flags = O_RDWR | O_CREAT | O_EXCL;
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
                    flags = O_WRONLY | O_CREAT | O_TRUNC;
                    break;
                case DeviceAccess::ReadWrite:
                    flags = O_RDWR | O_CREAT | O_TRUNC;
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
                    flags = O_RDONLY;
                    break;
                case DeviceAccess::Write:
                    // NOTE: Pre-existing quirk, this truncates the file despite
                    // being "Open". The must exist check above prevents creation
                    flags = O_WRONLY | O_TRUNC;
                    break;
                case DeviceAccess::ReadWrite:
                    flags = O_RDWR;
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

            switch (access)
            {
                case DeviceAccess::Write:
                    flags = O_WRONLY | O_CREAT | O_TRUNC;
                    break;
                case DeviceAccess::ReadWrite:
                    flags = O_RDWR | O_CREAT;
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
                    flags = O_WRONLY | O_TRUNC;
                    break;
                case DeviceAccess::ReadWrite:
                    flags = O_RDWR | O_TRUNC;
                    break;
                default:
                    PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
            }

            break;
        }
        case FileMode::Append:
        {
            if (access == DeviceAccess::Read)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Invalid combination FileMode::Append and DeviceAccess::Read");

            // NOTE: The device seeks to the end after opening, see the constructor
            switch (access)
            {
                case DeviceAccess::Write:
                    flags = O_WRONLY | O_CREAT;
                    break;
                case DeviceAccess::ReadWrite:
                    flags = O_RDWR | O_CREAT;
                    break;
                default:
                    PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
            }

            break;
        }
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    int fd = utls::openFd(filepath, flags);
    if (fd == -1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Error accessing file {}", filepath);

    return fd;
}

size_t readFd(int fd, char* buffer, size_t size)
{
    size_t read = 0;
    while (read < size)
    {
#ifdef _WIN32
        int rc = _read(fd, buffer + read, (unsigned)std::min(size - read, (size_t)INT_MAX));
#else
        ssize_t rc = ::read(fd, buffer + read, std::min(size - read, (size_t)SSIZE_MAX));
        if (rc < 0 && errno == EINTR)
            continue;
#endif
        if (rc < 0)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to read the amount of bytes requested");

        if (rc == 0)
            break;

        read += (size_t)rc;
    }

    return read;
}

void writeFd(int fd, const char* buffer, size_t size)
{
    size_t written = 0;
    while (written < size)
    {
#ifdef _WIN32
        int rc = _write(fd, buffer + written, (unsigned)std::min(size - written, (size_t)INT_MAX));
#else
        ssize_t rc = ::write(fd, buffer + written, std::min(size - written, (size_t)SSIZE_MAX));
        if (rc < 0 && errno == EINTR)
            continue;
#endif
        if (rc <= 0)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to write the given buffer");

        written += (size_t)rc;
    }
}

size_t seekFd(int fd, ssize_t offset, int origin)
{
#ifdef _WIN32
    int64_t off = _lseeki64(fd, offset, origin);
#else
    off_t off = ::lseek(fd, (off_t)offset, origin);
#endif
    if (off < 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to seek to given position in the stream");

    return (size_t)off;
}

size_t getFdLength(int fd)
{
#ifdef _WIN32
    int64_t length = _filelengthi64(fd);
#else
    struct stat info;
    if (::fstat(fd, &info) != 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to determine the current file length");

    off_t length = info.st_size;
#endif
    if (length < 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Failed to determine the current file length");

    return (size_t)length;
}

int closeFd(int fd)
{
#ifdef _WIN32
    return _close(fd);
#else
    return ::close(fd);
#endif
}

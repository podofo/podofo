/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef AUX_STREAM_DEVICE_H
#define AUX_STREAM_DEVICE_H

#include <cstring>
#include <ostream>
#include <fstream>
#include <cstdio>
#include <vector>

#include "basetypes.h"

#include "InputDevice.h"
#include "OutputDevice.h"

namespace PoDoFo {

/** This class provides an output device which operates
 *  either on a file or on a buffer in memory.
 *  Additionally it can count the bytes written to the device.
 *
 *  This class is suitable for inheritance to provide output
 *  devices of your own. Just override the required virtual methods.
 */
class PODOFO_API StreamDevice : public InputStreamDevice, public OutputStreamDevice
{
protected:
    StreamDevice(DeviceAccess access);

protected:
    static size_t SeekPosition(size_t curpos, size_t devlen, ssize_t offset, SeekDirection direction);
};

class PODOFO_API StandardStreamDevice : public StreamDevice
{
public:
    /** Construct a new StreamDevice that writes all data to a std::ostream.
     *
     *  \param stream write to this std::ostream
     */
    StandardStreamDevice(std::ostream& stream);

    StandardStreamDevice(std::istream& stream);

    /** Construct a new StreamDevice that writes all data to a std::iostream
     *  and reads from it as well.
     *  \param stream read/write from/to this std::iostream
     */
    StandardStreamDevice(std::iostream& stream);

    ~StandardStreamDevice();

public:
    size_t GetLength() const override;

    size_t GetPosition() const override;

    bool CanSeek() const override;

    bool Eof() const override;

protected:
    StandardStreamDevice(DeviceAccess access, std::ios& stream, bool streamOwned);
    void writeBuffer(const char* buffer, size_t size) override;
    void flush() override;
    size_t readBuffer(char* buffer, size_t size, bool& eof) override;
    bool readChar(char& ch) override;
    bool peek(char& ch) const override;
    void seek(ssize_t offset, SeekDirection direction) override;

    inline std::ios& GetStream() { return *m_Stream; }

private:
    StandardStreamDevice(DeviceAccess access, std::ios* stream, std::istream* istream, std::ostream* ostream, bool streamOwned);

private:
    std::ios* m_Stream;
    std::istream* m_istream;
    std::ostream* m_ostream;
    bool m_StreamOwned;
};

// These are the .NET System.IO file opening modes
// https://docs.microsoft.com/en-us/dotnet/api/system.io.filemode?view=net-6.0
enum class FileMode : uint8_t
{
    CreateNew = 1,     ///< Create a new file (throw if existing) for writing/reading
    Create,            ///< Create a new file or truncate existing one for writing/reading
    Open,              ///< Open an existing file for reading and/or writing
    OpenOrCreate,      ///< Open an existing file or create a new one for writing/reading
    Truncate,          ///< Truncate an existing file for writing/reading
    Append,            ///< Open an existing file and seek to the end for writing
};

class PODOFO_API FileStreamDevice : public StreamDevice
{
public:
    /** Open for reading the supplied filepath
     */
    FileStreamDevice(const std::string_view & filepath);

    /** Open for reading/writing the supplied filepath with the given filemode
     */
    FileStreamDevice(const std::string_view & filepath, FileMode mode);

    /** Open for the supplied filepath with the given filemode and access
     */
    FileStreamDevice(const std::string_view& filepath, FileMode mode,
        DeviceAccess access);

    ~FileStreamDevice();

public:
    const std::string& GetFilepath() const { return m_Filepath; }

    size_t GetLength() const override;

    size_t GetPosition() const override;

    bool CanSeek() const override;

    bool Eof() const override;

protected:
    void writeBuffer(const char* buffer, size_t size) override;
    void flush() override;
    size_t readBuffer(char* buffer, size_t size, bool& eof) override;
    bool readChar(char& ch) override;
    bool peek(char& ch) const override;
    void seek(ssize_t offset, SeekDirection direction) override;
    void close() override;

private:
    FILE* m_file;
    std::string m_Filepath;
};

template <typename TContainer>
class ContainerStreamDevice : public StreamDevice
{
public:
    ContainerStreamDevice(TContainer& container,
        DeviceAccess access, bool ate) :
        StreamDevice(access),
        m_container(&container),
        m_Position(ate ? container.size() : 0) { }

    /**
     * \remarks by default it set the current position at the begin of the container
     */
    ContainerStreamDevice(const TContainer& container) :
        ContainerStreamDevice(const_cast<TContainer&>(container), DeviceAccess::Read, false) { }

    /**
     * \remarks by default it set the current position at the end of the container 
     */
    ContainerStreamDevice(TContainer& container) :
        ContainerStreamDevice(container, DeviceAccess::ReadWrite, true) { }

public:
    size_t GetLength() const override { return m_container->size(); }

    size_t GetPosition() const override { return m_Position; }

    bool CanSeek() const override { return true; }

    bool Eof() const override { return m_Position == m_container->size(); }

protected:
    void writeBuffer(const char* buffer, size_t size) override
    {
        if (m_Position + size > m_container->size())
            m_container->resize(m_Position + size);

        std::memcpy(m_container->data() + m_Position, buffer, size);
        m_Position += size;
    }

    size_t readBuffer(char* buffer, size_t size, bool& eof) override
    {
        size_t readCount = std::min(size, m_container->size() - m_Position);
        std::memcpy(buffer, m_container->data() + m_Position, readCount);
        m_Position += readCount;
        eof = m_Position == m_container->size();
        return readCount;
    }

    bool readChar(char& ch) override
    {
        if (m_Position == m_container->size())
        {
            ch = '\0';
            return false;
        }

        ch = m_container->data()[m_Position];
        m_Position++;
        return true;
    }

    bool peek(char& ch) const override
    {
        if (m_Position == m_container->size())
        {
            ch = '\0';
            return false;
        }

        ch = m_container->data()[m_Position];
        return true;
    }

    void seek(ssize_t offset, SeekDirection direction) override
    {
        m_Position = SeekPosition(m_Position, m_container->size(), offset, direction);
    }

private:
    TContainer* m_container;
    size_t m_Position;
};

class PODOFO_API SpanStreamDevice : public StreamDevice
{
public:
    /** Construct a new StreamDevice that reads all data from a memory buffer.
     *  The buffer is temporarily binded
     */
    SpanStreamDevice(const char* buffer, size_t size);
    SpanStreamDevice(const bufferview& buffer);
    SpanStreamDevice(const std::string_view& view);
    SpanStreamDevice(const std::string& str);
    SpanStreamDevice(std::string& str,
        DeviceAccess access = DeviceAccess::ReadWrite);
    SpanStreamDevice(const char* str);
    SpanStreamDevice(char* buffer, size_t size,
        DeviceAccess access = DeviceAccess::ReadWrite);
    SpanStreamDevice(const bufferspan& span,
        DeviceAccess access = DeviceAccess::ReadWrite);

public:
    size_t GetLength() const override;

    size_t GetPosition() const override;

    bool Eof() const override;

    bool CanSeek() const override;

protected:
    void writeBuffer(const char* buffer, size_t size) override;
    size_t readBuffer(char* buffer, size_t size, bool& eof) override;
    bool readChar(char& ch) override;
    bool peek(char& ch) const override;
    void seek(ssize_t offset, SeekDirection direction) override;

private:
    SpanStreamDevice(std::nullptr_t) = delete;

private:
    char* m_buffer;
    size_t m_Length;
    size_t m_Position;
};

/**
 * An StreamDevice device that does nothing
 */
class PODOFO_API NullStreamDevice final : public StreamDevice
{
public:
    NullStreamDevice();

public:
    size_t GetLength() const override;

    size_t GetPosition() const override;

    bool Eof() const override;

protected:
    void writeBuffer(const char* buffer, size_t size) override;
    size_t readBuffer(char* buffer, size_t size, bool& eof) override;
    bool readChar(char& ch) override;
    bool peek(char& ch) const override;
    void seek(ssize_t offset, SeekDirection direction) override;

private:
    size_t m_Length;
    size_t m_Position;
};

using VectorStreamDevice = ContainerStreamDevice<std::vector<char>>;
using StringStreamDevice = ContainerStreamDevice<std::string>;
using BufferStreamDevice = ContainerStreamDevice<charbuff>;

}

#endif // AUX_STREAM_DEVICE_H

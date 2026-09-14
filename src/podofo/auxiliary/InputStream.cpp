// SPDX-FileCopyrightText: 2007 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "InputStream.h"

#include "OutputStream.h"

using namespace std;
using namespace PoDoFo;

constexpr size_t BUFFER_SIZE = 4096;

InputStream::InputStream()
    : m_head(nullptr), m_tail(nullptr) { }

InputStream::~InputStream() { }

PODOFO_INLINE void InputStream::Read(char* buffer, size_t size)
{
    if (size != 0 && size <= (size_t)(m_tail - m_head) && buffer != nullptr)
    {
        // Serve the whole request from the read window
        std::memcpy(buffer, m_head, size);
        m_head += size;
        return;
    }

    readSlowPath(buffer, size);
}

void InputStream::readSlowPath(char* buffer, size_t size)
{
    if (buffer == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Invalid buffer");

    checkRead();
    bool eof;
    size_t read = 0;
    do
    {
        read += readBuffer(buffer + read, size - read, eof);
        if (read == size)
            return;

    } while (!eof);

    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Unexpected EOF when reading from stream");
}

PODOFO_INLINE char InputStream::ReadChar()
{
    if (m_head != m_tail)
        return *m_head++;

    return readCharSlowPath();
}

char InputStream::readCharSlowPath()
{
    checkRead();
    char ch;
    if (!readChar(ch))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::IOError, "Reached EOF while reading from the stream");

    return ch;
}

PODOFO_INLINE bool InputStream::Read(char& ch)
{
    if (m_head != m_tail)
    {
        ch = *m_head++;
        return true;
    }

    return tryReadCharSlowPath(ch);
}

bool InputStream::tryReadCharSlowPath(char& ch)
{
    checkRead();
    return readChar(ch);
}

PODOFO_INLINE size_t InputStream::Read(char* buffer, size_t size, bool& eof)
{
    if (size != 0 && size < (size_t)(m_tail - m_head) && buffer != nullptr)
    {
        // NOTE: Don't satisfy an exact drain (hence < and not <=),
        // so the actual device is consulted for EOF
        std::memcpy(buffer, m_head, size);
        m_head += size;
        eof = false;
        return size;
    }

    return readSlowPath(buffer, size, eof);
}

size_t InputStream::readSlowPath(char* buffer, size_t size, bool& eof)
{
    if (buffer == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Invalid buffer");

    checkRead();
    size_t read = 0;
    do
    {
        read += readBuffer(buffer + read, size - read, eof);
        if (read == size)
            return read;

    } while (!eof);

    return read;
}

void InputStream::CopyTo(OutputStream& stream)
{
    bool eof;
    size_t read = 0;
    char buffer[BUFFER_SIZE];

    try
    {
        do
        {
            read = readBuffer(buffer, BUFFER_SIZE, eof);
            stream.Write(buffer, read);
        } while (!eof);

        stream.Flush();
    }
    catch (PdfError& err)
    {
        if (err.GetCode() == PdfErrorCode::FlateError)
        {
            // We want to ignore compression errors, as most
            // implementations do, but we also try to clear
            // the stream as it may contain truncated/invalid
            // content
            auto deviceStream = dynamic_cast<OutputStreamDevice*>(&stream);
            if (deviceStream != nullptr)
            {
                deviceStream->Seek(0, SeekDirection::Begin);
                deviceStream->Truncate();
            }
        }
        else
        {
            throw;
        }
    }
}

void InputStream::CopyTo(OutputStream& stream, size_t size)
{
    bool eof;
    size_t read = 0;
    char buffer[BUFFER_SIZE];

    try
    {
        do
        {
            read = readBuffer(buffer, std::min(BUFFER_SIZE, size), eof);
            size -= read;
            stream.Write(buffer, read);
        } while (size > 0 && !eof);

        stream.Flush();
    }
    catch (PdfError& err)
    {
        if (err.GetCode() == PdfErrorCode::FlateError)
        {
            // We want to ignore compression errors, as most
            // implementations do, but we also try to clear
            // the stream as it may contain truncated/invalid
            // content
            auto deviceStream = dynamic_cast<OutputStreamDevice*>(&stream);
            if (deviceStream != nullptr)
            {
                deviceStream->Seek(0, SeekDirection::Begin);
                deviceStream->Truncate();
            }
        }
        else
        {
            throw;
        }
    }
}

bool InputStream::readChar(char& ch)
{
    ch = '\0';
    bool eof;
    do
    {
        if (readBuffer(&ch, 1, eof) == 1)
            return true;

    } while (!eof);

    return  false;
}

void InputStream::checkRead() const
{
    // Do nothing
}

size_t InputStream::ReadBuffer(InputStream& stream, char* buffer, size_t size, bool& eof)
{
    return stream.readBuffer(buffer, size, eof);
}

bool InputStream::ReadChar(InputStream& stream, char& ch)
{
    return stream.readChar(ch);
}

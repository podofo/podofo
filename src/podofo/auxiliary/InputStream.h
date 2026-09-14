// SPDX-FileCopyrightText: 2007 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#ifndef AUX_INPUT_STREAM_H
#define AUX_INPUT_STREAM_H

#include "basedefs.h"

namespace PoDoFo {

class OutputStream;

/// An interface for reading blocks of data from a data source.
/// It supports non-blocking read operations
class PODOFO_API InputStream
{
public:
    InputStream();
    virtual ~InputStream();

    /// Read data from the device
    /// @param buffer a pointer to the data buffer
    /// @param size length of the output buffer
    /// @remarks throws if EOF is encountered before
    /// reading the required size
    void Read(char* buffer, size_t size);

    /// Read data from the device
    /// @param buffer a pointer to the data buffer
    /// @param size length of the output buffer
    /// @param eof stream encountered EOF during the read
    /// @returns Number of read bytes
    size_t Read(char* buffer, size_t size, bool& eof);

    /// Get next char from stream.
    /// @returns the next character from the stream
    /// @remarks throws if EOF is encountered before
    /// reading the character
    char ReadChar();

    /// Get next char from stream.
    /// @param ch the read character
    /// @returns true if success, false if EOF is encountered
    /// before reading the character
    bool Read(char& ch);

    /// Copy this stream to another
    /// @remarks The copy begins from the current position and
    ///      it's not reset after the operation is completed
    void CopyTo(OutputStream& stream);

    /// Copy this stream to another
    /// @remarks The copy begins from the current position and
    ///      it's not reset after the operation is completed
    void CopyTo(OutputStream& stream, size_t size);

protected:
    static size_t ReadBuffer(InputStream& stream, char* buffer, size_t size, bool& eof);
    static bool ReadChar(InputStream& stream, char& ch);

protected:
    /// Read a buffer from the stream
    /// @param eof true if the stream reached eof during read
    /// @returns number of read bytes
    virtual size_t readBuffer(char* buffer, size_t size, bool& eof) = 0;

    /// Read the next char in stream.
    /// @returns true if success, false if EOF
    virtual bool readChar(char& ch);

    /// Optional checks before reading
    /// By default does nothing
    virtual void checkRead() const;

protected:
    /// Inline read window over memory that already holds the bytes at the
    /// current logical position. It has three states:
    /// - unarmed: m_tail == nullptr, the device counter owns the position
    /// - armed, empty: m_tail != nullptr && m_head == m_tail, the window is
    ///   authoritative for the position but holds no bytes
    /// - armed, non empty: m_head < m_tail, reads are served from the window
    /// @remarks m_tail is the arm flag, m_head != m_tail the fast path test.
    /// While armed m_head is authoritative for the logical position, so every
    /// readBuffer()/readChar()/peek() override of an arming device must drain
    /// or derive from it. The window is armed only when read access is granted,
    /// since the fast paths skip checkRead()
    const char* m_head;
    const char* m_tail;

private:
    /// Slow paths of the read frontends, taken when the read
    /// window can't satisfy the request on its own
    void readSlowPath(char* buffer, size_t size);
    size_t readSlowPath(char* buffer, size_t size, bool& eof);
    char readCharSlowPath();
    bool tryReadCharSlowPath(char& ch);

private:
    InputStream(const InputStream&) = delete;
    InputStream& operator=(const InputStream&) = delete;
};

};

#endif // AUX_INPUT_STREAM_H

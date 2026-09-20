// SPDX-FileCopyrightText: 2007 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#ifndef AUX_OUTPUT_STREAM_H
#define AUX_OUTPUT_STREAM_H

#include "basedefs.h"

namespace PoDoFo {

/// An interface for writing blocks of data to
/// a data source.
class PODOFO_API OutputStream
{
public:
    OutputStream();
    virtual ~OutputStream();

    /// Write the character in the device
    ///
    /// @param ch the character to write
    void Write(char ch);

    /// Write the view to the OutputStream
    ///
    /// @param view the view to be written
    void Write(const std::string_view& view);

    /// Write data to the output stream
    ///
    /// @param buffer the data is read from this buffer
    /// @param size    the size of the buffer
    void Write(const char* buffer, size_t size);

    void Flush();

protected:
    /// Write to another stream, skipping its checkWrite() but still
    /// serving its inline write window
    static void WriteBuffer(OutputStream& stream, const char* buffer, size_t size);
    static void Flush(OutputStream& stream);

protected:
    virtual void writeBuffer(const char* buffer, size_t size) = 0;
    virtual void flush();

    /// Optional checks before writing
    /// By default does nothing
    virtual void checkWrite() const;

    /// Enable the inline write window over the supplied memory region
    /// @remarks Must be armed only once write access is granted, since the
    /// fast paths skip checkWrite()
    void enableWriteWindow(char* cur, char* end);

    /// Disarm the window, once its pending bytes have been handed to the sink
    /// @remarks An arming implementation must call this from its own destructor,
    /// after the final drain, as ~OutputStream() asserts the window is disarmed
    void disableWriteWindow();

protected:
    /// Inline write window over memory owned by the implementation, mirroring
    /// InputStream's read window. It has three states:
    /// - unarmed: m_wend == nullptr, every write takes the virtual slow path
    /// - armed, full: m_wend != nullptr && m_wcur == m_wend, no room left
    /// - armed, with room: m_wcur < m_wend, writes are served by the window
    /// Unlike the read window, which can be dropped at any time, the bytes
    /// before m_wcur are pending output: an arming implementation must keep
    /// them ahead of what writeBuffer() receives, and hand them to its sink
    /// in flush() and in its own destructor
    /// While armed, writes served by the window never reach writeBuffer(), so
    /// an implementation that counts or inspects bytes there must derive the
    /// count from m_wcur instead, or not arm at all
    char* m_wcur;
    char* m_wend;

private:
    /// Slow path of the write frontends, taken when the write
    /// window can't satisfy the request on its own
    void writeSlowPath(const char* buffer, size_t size);

private:
    OutputStream(const OutputStream&) = delete;
    OutputStream& operator=(const OutputStream&) = delete;
};

};

#endif // AUX_OUTPUT_STREAM_H

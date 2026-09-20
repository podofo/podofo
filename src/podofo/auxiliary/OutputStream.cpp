// SPDX-FileCopyrightText: 2007 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "OutputStream.h"

using namespace std;
using namespace PoDoFo;

OutputStream::OutputStream()
    : m_wcur(nullptr), m_wend(nullptr) { }

OutputStream::~OutputStream()
{
    // An arming implementation must disarm in its own destructor, which runs
    // before this one. A window still armed here means the implementation
    // may have left pending bytes in the window
    PODOFO_ASSERT(m_wcur == nullptr);
}

PODOFO_INLINE void OutputStream::Write(char ch)
{
    if (m_wcur != m_wend)
    {
        *m_wcur++ = ch;
        return;
    }

    writeSlowPath(&ch, 1);
}

PODOFO_INLINE void OutputStream::Write(const string_view& view)
{
    size_t size = view.size();
    if (size != 0 && size <= (size_t)(m_wend - m_wcur))
    {
        // The window has room for the whole request
        std::memcpy(m_wcur, view.data(), size);
        m_wcur += size;
        return;
    }

    writeSlowPath(view.data(), size);
}

PODOFO_INLINE void OutputStream::Write(const char* buffer, size_t size)
{
    if (size != 0 && size <= (size_t)(m_wend - m_wcur))
    {
        // The window has room for the whole request
        std::memcpy(m_wcur, buffer, size);
        m_wcur += size;
        return;
    }

    writeSlowPath(buffer, size);
}

void OutputStream::writeSlowPath(const char* buffer, size_t size)
{
    if (size == 0)
        return;

    checkWrite();
    writeBuffer(buffer, size);
}

void OutputStream::enableWriteWindow(char* cur, char* end)
{
    m_wcur = cur;
    m_wend = end;
}

void OutputStream::disableWriteWindow()
{
    m_wcur = nullptr;
    m_wend = nullptr;
}

void OutputStream::Flush()
{
    flush();
}

PODOFO_INLINE void OutputStream::WriteBuffer(OutputStream& stream, const char* buffer, size_t size)
{
    if (size != 0 && size <= (size_t)(stream.m_wend - stream.m_wcur))
    {
        // The window has room for the whole request
        std::memcpy(stream.m_wcur, buffer, size);
        stream.m_wcur += size;
        return;
    }

    // NOTE: Deliberately skipping checkWrite(), which is what this
    // frontend is for
    stream.writeBuffer(buffer, size);
}

void OutputStream::Flush(OutputStream& stream)
{
    stream.flush();
}

void OutputStream::flush()
{
    // Do nothing
}

void OutputStream::checkWrite() const
{
    // Do nothing
}

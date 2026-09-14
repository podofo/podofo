// SPDX-FileCopyrightText: 2022 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfCanvasInputDevice.h"
#include "PdfCanvas.h"

using namespace std;
using namespace PoDoFo;

PdfCanvasInputDevice::PdfCanvasInputDevice(const PdfCanvas& canvas)
    : m_eof(false), m_deviceSwitchOccurred(false)
{
    auto contents = canvas.GetContentsObject();
    if (contents != nullptr)
    {
        if (contents->IsArray())
        {
            auto& contentsArr = contents->GetArray();
            for (unsigned i = 0; i < contentsArr.GetSize(); i++)
            {
                auto streamObj = contentsArr.FindAt(i);
                if (streamObj == nullptr)
                    continue;

                m_contents.push_back(streamObj);
            }
        }
        else if (contents->IsDictionary())
        {
            // NOTE: Pages are allowed to be empty
            if (contents->HasStream())
                m_contents.push_back(contents);
        }
        else
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Page /Contents not stream or array of streams");
        }
    }

    // NOTE: The initial pop owes no newline separator, so the
    // window is armed straight away
    if (tryPopNextBuffer())
        enableBuffer();
    else
        m_eof = true;
}

bool PdfCanvasInputDevice::peek(char& ch) const
{
    if (m_eof)
    {
        ch = '\0';
        return false;
    }

    auto& mref = const_cast<PdfCanvasInputDevice&>(*this);
    while (true)
    {
        if (m_deviceSwitchOccurred)
        {
            // Handle device switch by returning
            // a newline separator. NOTE: Don't
            // reset the switch flag
            ch = '\n';
            return true;
        }

        size_t pos = getPos();
        if (pos != m_buffer.size())
        {
            ch = m_buffer[pos];
            return true;
        }

        if (!mref.tryGetNextBuffer())
        {
            mref.setEOF();
            ch = '\0';
            return false;
        }
    }
}

size_t PdfCanvasInputDevice::readBuffer(char* buffer, size_t size, bool& eof)
{
    PODOFO_ASSERT(size != 0);
    if (m_eof)
    {
        eof = true;
        return 0;
    }

    size_t count = 0;
    while (true)
    {
        if (size == 0)
        {
            eof = false;
            return count;
        }

        if (m_deviceSwitchOccurred)
        {
            // Handle device switch by inserting
            // a newline separator in the buffer
            // and reset the flag
            *(buffer + count) = '\n';
            size -= 1;
            count += 1;
            m_deviceSwitchOccurred = false;
            enableBuffer();
            continue;
        }

        // Span reads into multiple contents streams
        size_t pos = getPos();
        size_t read = std::min(size, m_buffer.size() - pos);
        if (read != 0)
        {
            std::memcpy(buffer + count, m_buffer.data() + pos, read);
            setPos(pos + read);
            size -= read;
            count += read;
            continue;
        }

        if (!tryGetNextBuffer())
        {
            setEOF();
            eof = true;
            return count;
        }
    }
}

bool PdfCanvasInputDevice::readChar(char& ch)
{
    if (m_eof)
    {
        ch = '\0';
        return false;
    }

    while (true)
    {
        if (m_deviceSwitchOccurred)
        {
            // Handle device switch by returning a
            // newline separator and reset the flag
            ch = '\n';
            m_deviceSwitchOccurred = false;
            enableBuffer();
            return true;
        }

        size_t pos = getPos();
        if (pos != m_buffer.size())
        {
            ch = m_buffer[pos];
            setPos(pos + 1);
            return true;
        }

        if (!tryGetNextBuffer())
        {
            setEOF();
            ch = '\0';
            return false;
        }
    }
}

size_t PdfCanvasInputDevice::GetLength() const
{
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Unsupported");
}

size_t PdfCanvasInputDevice::GetPosition() const
{
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Unsupported");
}

bool PdfCanvasInputDevice::tryGetNextBuffer()
{
    if (!tryPopNextBuffer())
        return false;

    // ISO 32000-1:2008: Table 30 – Entries in a page object,
    // /Contents: "The division between streams may occur
    // only at the boundaries between lexical tokens".
    // We will handle the device switch by adding a
    // newline separator
    m_deviceSwitchOccurred = true;
    return true;
}

// Returns true if one contents stream was successfully
// popped out of the queue and is not empty
bool PdfCanvasInputDevice::tryPopNextBuffer()
{
    while (m_contents.size() != 0)
    {
        auto contents = m_contents.front()->GetStream();
        m_contents.pop_front();
        if (contents == nullptr)
            continue;

        contents->CopyTo(m_buffer);
        if (m_buffer.size() == 0)
            continue;

        // NOTE: CopyTo() reallocates the buffer, so the window must not be
        // left pointing into the previous one. It stays unarmed until the
        // newline separator owed for this switch is actually consumed
        m_head = nullptr;
        m_tail = nullptr;
        return true;
    }

    return false;
}

void PdfCanvasInputDevice::setEOF()
{
    m_deviceSwitchOccurred = false;
    m_eof = true;
}

void PdfCanvasInputDevice::enableBuffer()
{
    PODOFO_INVARIANT(!m_deviceSwitchOccurred);
    // NOTE: A freshly popped buffer is always read from its beginning
    enableReadWindow(m_buffer.data(), m_buffer.data() + m_buffer.size());
}

size_t PdfCanvasInputDevice::getPos() const
{
    PODOFO_ASSERT(m_tail != nullptr);
    return (size_t)(m_head - m_buffer.data());
}

void PdfCanvasInputDevice::setPos(size_t pos)
{
    PODOFO_ASSERT(m_tail != nullptr);
    m_head = m_buffer.data() + pos;
}

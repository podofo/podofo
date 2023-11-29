/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfCanvasInputDevice.h"
#include "PdfCanvas.h"
#include <podofo/auxiliary/StreamDevice.h>

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

    if (!tryPopNextDevice())
        m_eof = true;
}

bool PdfCanvasInputDevice::peek(char& ch) const
{
    if (m_eof)
    {
        ch = '\0';
        return false;
    }

    InputStreamDevice* device = nullptr;
    auto& mref = const_cast<PdfCanvasInputDevice&>(*this);
    while (true)
    {
        if (!mref.tryGetNextDevice(device))
        {
            mref.setEOF();
            ch = '\0';
            return false;
        }

        if (m_deviceSwitchOccurred)
        {
            // Handle device switch by returning
            // a newline separator. NOTE: Don't
            // reset the switch flag
            ch = '\n';
            return true;
        }

        if (!device->Peek(ch))
            continue;

        return true;
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

    size_t read;
    size_t count = 0;
    InputStreamDevice* device = nullptr;
    while (true)
    {
        if (!tryGetNextDevice(device))
        {
            setEOF();
            eof = true;
            return count;
        }

        if (size == 0)
            return count;

        if (m_deviceSwitchOccurred)
        {
            // Handle device switch by inserting
            // a newline separator in the buffer
            // and reset the flag
            *(buffer + count) = '\n';
            size -= 1;
            count += 1;
            m_deviceSwitchOccurred = false;
            if (size == 0)
                return count;
        }

        // Span reads into multiple input devices
        // NOTE: we ignore if the device reached EOF, and
        // we try pop another device at the next iteration
        read = device->Read(buffer + count, size, eof);
        size -= read;
        count += read;
    }
}

bool PdfCanvasInputDevice::readChar(char& ch)
{
    if (m_eof)
    {
        ch = '\0';
        return false;
    }

    InputStreamDevice* device = nullptr;
    while (true)
    {
        if (!tryGetNextDevice(device))
        {
            setEOF();
            return false;
        }

        if (m_deviceSwitchOccurred)
        {
            // Handle device switch by returning a
            // newline separator and reset the flag
            ch = '\n';
            m_deviceSwitchOccurred = false;
            return true;
        }

        if (!device->Read(ch))
            continue;

        return true;
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

bool PdfCanvasInputDevice::tryGetNextDevice(InputStreamDevice*& device)
{
    PODOFO_ASSERT(m_currDevice != nullptr);
    if (device == nullptr)
    {
        // Initial step, just return back current device
        device = m_currDevice.get();
        return true;
    }

    if (!tryPopNextDevice())
    {
        device = nullptr;
        return false;
    }

    // ISO 32000-1:2008: Table 30 – Entries in a page object,
    // /Contents: "The division between streams may occur
    // only at the boundaries between lexical tokens".
    // We will handle the device switch by addind a
    // newline separator
    m_deviceSwitchOccurred = true;
    device = m_currDevice.get();
    return true;
}

// Returns true if one device was successfully
// popped out of the queue and is not EOF
bool PdfCanvasInputDevice::tryPopNextDevice()
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

        m_currDevice = std::make_unique<SpanStreamDevice>(m_buffer);
        return true;
    }

    return false;
}

void PdfCanvasInputDevice::setEOF()
{
    m_deviceSwitchOccurred = false;
    m_eof = true;
}

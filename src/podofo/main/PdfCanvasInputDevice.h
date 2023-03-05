/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_CANVAS_INPUT_DEVICE_H
#define PDF_CANVAS_INPUT_DEVICE_H

#include <podofo/auxiliary/InputDevice.h>

#include <list>

namespace PoDoFo {

class PdfCanvas;
class PdfObject;

/**
 * There are Pdfs spanning delimiters or begin/end tags into
 * contents streams. Let's create a device correctly spanning
 * I/O reads into these
 */
class PODOFO_API PdfCanvasInputDevice final : public InputStreamDevice
{
public:
    PdfCanvasInputDevice(const PdfCanvas& canvas);
public:
    size_t GetLength() const override;
    size_t GetPosition() const override;
    bool Eof() const override { return m_eof; }
private:
    bool tryGetNextDevice(InputStreamDevice*& device);
    bool tryPopNextDevice();
    void setEOF();
protected:
    size_t readBuffer(char* buffer, size_t size, bool& eof) override;
    bool readChar(char& ch) override;
    bool peek(char& ch) const override;
private:
    bool m_eof;
    std::list<const PdfObject*> m_contents;
    charbuff m_buffer;
    std::unique_ptr<InputStreamDevice> m_currDevice;
    bool m_deviceSwitchOccurred;
};

}

#endif // PDF_CANVAS_INPUT_DEVICE_H

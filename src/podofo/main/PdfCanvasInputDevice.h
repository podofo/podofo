// SPDX-FileCopyrightText: 2022 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#ifndef PDF_CANVAS_INPUT_DEVICE_H
#define PDF_CANVAS_INPUT_DEVICE_H

#include <podofo/auxiliary/InputDevice.h>

#include <list>

namespace PoDoFo {

class PdfCanvas;
class PdfObject;

/// There are Pdfs spanning delimiters or begin/end tags into
/// contents streams. Let's create a device correctly spanning
/// I/O reads into these
class PODOFO_API PdfCanvasInputDevice final : public InputStreamDevice
{
public:
    PdfCanvasInputDevice(const PdfCanvas& canvas);
public:
    size_t GetLength() const override;
    size_t GetPosition() const override;
    bool Eof() const override { return m_eof; }
private:
    bool tryGetNextBuffer();
    bool tryPopNextBuffer();
    void setEOF();
    /// Enable the read window over the current buffer
    /// @remarks It must not be called while a device switch is owed
    void enableBuffer();
    /// Read/set the position within the current buffer, which
    /// lives in the read window. It's armed whenever no device
    /// switch is owed and the device is not EOF
    size_t getPos() const;
    void setPos(size_t pos);
protected:
    size_t readBuffer(char* buffer, size_t size, bool& eof) override;
    bool readChar(char& ch) override;
    bool peek(char& ch) const override;
private:
    bool m_eof;
    std::list<const PdfObject*> m_contents;
    charbuff m_buffer;
    bool m_deviceSwitchOccurred;
};

}

#endif // PDF_CANVAS_INPUT_DEVICE_H

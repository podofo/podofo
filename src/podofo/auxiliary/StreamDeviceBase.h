/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef AUX_STREAM_DEVICE_BASE_H
#define AUX_STREAM_DEVICE_BASE_H

#include "basedefs.h"
#include <cstdint>
#include "EnumFlags.h"

namespace PoDoFo {

enum class DeviceAccess : uint8_t
{
    Read = 1,
    Write = 2,
    ReadWrite = Read | Write
};

enum class SeekDirection : uint8_t
{
    Begin = 0,
    Current,
    End
};

class PODOFO_API StreamDeviceBase
{
protected:
    StreamDeviceBase();

public:
    /** Seek the device to the position offset from the beginning
     *  \param offset from the beginning of the file
     */
    void Seek(size_t offset);

    /** Seek the device to the position offset from the beginning
     *  \param offset from the beginning of the file
     *  \param direction where to start (Begin, Current, End)
     *
     *  A non-seekable input device will throw an InvalidDeviceOperation.
     */
    void Seek(ssize_t offset, SeekDirection direction);

    void Close();

public:
    DeviceAccess GetAccess() const { return m_Access; }

    /**
     * \return True if the stream is at EOF
     */
    virtual bool Eof() const = 0;

    /** The number of bytes written to this object.
     *  \returns the number of bytes written to this object.
     *
     *  \see Init
     */
    virtual size_t GetLength() const = 0;

    /** Get the current offset from the beginning of the file.
     *  \return the offset form the beginning of the file.
     */
    virtual size_t GetPosition() const = 0;

    virtual bool CanSeek() const;

protected:
    void EnsureAccess(DeviceAccess access) const;
    void SetAccess(DeviceAccess access) { m_Access = access; }

    virtual void seek(ssize_t offset, SeekDirection direction);
    virtual void close();

private:
    DeviceAccess m_Access;
};

}

ENABLE_BITMASK_OPERATORS(PoDoFo::DeviceAccess);

#endif // AUX_STREAM_DEVICE_BASE_H

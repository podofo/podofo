/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef AUX_INPUT_DEVICE_H
#define AUX_INPUT_DEVICE_H

#include <istream>
#include <fstream>

#include "StreamDeviceBase.h"
#include "InputStream.h"

namespace PoDoFo {

/** This class represents an input device
 * It optionally supports peeking
 */
class PODOFO_API InputStreamDevice : virtual public StreamDeviceBase, public InputStream
{
protected:
    InputStreamDevice();
    InputStreamDevice(bool init);

public:
    /** Peek at next char in stream.
     * /returns true if success, false if EOF is encountered
     * before peeking the character
     */
    bool Peek(char& ch) const;

protected:
    /** Peek at next char in stream.
     *  /returns true if success, false if EOF
     */
    virtual bool peek(char& ch) const = 0;

    void checkRead() const override;
};

};

#endif // AUX_INPUT_DEVICE_H

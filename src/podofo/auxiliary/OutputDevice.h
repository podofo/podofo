/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef AUX_OUTPUT_DEVICE_H
#define AUX_OUTPUT_DEVICE_H

#include <ostream>
#include <fstream>

#include "StreamDeviceBase.h"
#include "OutputStream.h"

namespace PoDoFo {

class PODOFO_API OutputStreamDevice : virtual public StreamDeviceBase, public OutputStream
{
protected:
    OutputStreamDevice();
    OutputStreamDevice(bool init);

public:
    /** Truncate the file at this position by discarding subsequent bytes
     */
    void Truncate();

protected:
    virtual void truncate() = 0;
    void checkWrite() const override;
};

};

#endif // AUX_OUTPUT_DEVICE_H

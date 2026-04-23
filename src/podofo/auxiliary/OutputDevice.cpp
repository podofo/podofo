// SPDX-FileCopyrightText: 2006 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "OutputDevice.h"

using namespace PoDoFo;

OutputStreamDevice::OutputStreamDevice()
    : OutputStreamDevice(true) { }

OutputStreamDevice::OutputStreamDevice(bool init)
{
    if (init)
        SetAccess(DeviceAccess::Write);
}

void OutputStreamDevice::Truncate()
{
    checkWrite();
    truncate();
}

void OutputStreamDevice::checkWrite() const
{
    EnsureAccess(DeviceAccess::Write);
}

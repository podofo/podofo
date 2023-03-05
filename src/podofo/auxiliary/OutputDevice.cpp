/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

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

void OutputStreamDevice::checkWrite() const
{
    EnsureAccess(DeviceAccess::Write);
}

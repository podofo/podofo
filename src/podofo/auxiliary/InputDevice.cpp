// SPDX-FileCopyrightText: 2006 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "InputDevice.h"

using namespace std;
using namespace PoDoFo;

InputStreamDevice::InputStreamDevice()
    : InputStreamDevice(true) { }

InputStreamDevice::InputStreamDevice(bool init)
{
    if (init)
        SetAccess(DeviceAccess::Read);
}

bool InputStreamDevice::Peek(char& ch) const
{
    EnsureAccess(DeviceAccess::Read);
    return peek(ch);
}

void InputStreamDevice::checkRead() const
{
    EnsureAccess(DeviceAccess::Read);
}

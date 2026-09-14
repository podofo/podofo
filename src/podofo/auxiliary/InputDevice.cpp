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

PODOFO_INLINE bool InputStreamDevice::Peek(char& ch) const
{
    if (m_head != m_tail)
    {
        ch = *m_head;
        return true;
    }

    return peekSlowPath(ch);
}

bool InputStreamDevice::peekSlowPath(char& ch) const
{
    EnsureAccess(DeviceAccess::Read);
    return peek(ch);
}

void InputStreamDevice::checkRead() const
{
    EnsureAccess(DeviceAccess::Read);
}

void InputStreamDevice::resetBuffers()
{
    m_head = nullptr;
    m_tail = nullptr;
}

void InputStreamDevice::enableReadWindow(const char* head, const char* tail)
{
    PODOFO_INVARIANT((GetAccess() & DeviceAccess::Read) != DeviceAccess{ });

    m_head = head;
    m_tail = tail;
}

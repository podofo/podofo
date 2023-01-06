/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_OUTPUT_DEVICE_H
#define PDF_OUTPUT_DEVICE_H

#include <ostream>
#include <fstream>

#include "PdfStreamDeviceBase.h"
#include "PdfOutputStream.h"

namespace PoDoFo {

class PODOFO_API OutputStreamDevice : virtual public StreamDeviceBase, public OutputStream
{
protected:
    OutputStreamDevice();
    OutputStreamDevice(bool init);

protected:
    void checkWrite() const override;
};

};

#endif // PDF_OUTPUT_DEVICE_H

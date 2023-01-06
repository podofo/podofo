/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfDataProvider.h"
#include "PdfStreamDevice.h"

using namespace std;
using namespace PoDoFo;

PdfDataProvider::PdfDataProvider() { }

PdfDataProvider::~PdfDataProvider() { }

string PdfDataProvider::ToString() const
{
    string ret;
    ToString(ret);
    return ret;
}

void PdfDataProvider::ToString(string& str) const
{
    str.clear();
    StringStreamDevice device(str);
    charbuff buffer;
    Write(device, PdfWriteFlags::None, { }, buffer);
}

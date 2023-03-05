/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "OutputStream.h"

using namespace std;
using namespace PoDoFo;

OutputStream::OutputStream() { }

OutputStream::~OutputStream() { }

void OutputStream::Write(char ch)
{
    checkWrite();
    writeBuffer(&ch, 1);
}

void OutputStream::Write(const string_view& view)
{
    if (view.length() == 0)
        return;

    checkWrite();
    writeBuffer(view.data(), view.size());
}

void OutputStream::Write(const char* buffer, size_t size)
{
    if (size == 0)
        return;

    checkWrite();
    writeBuffer(buffer, size);
}

void OutputStream::Flush()
{
    flush();
}

void OutputStream::WriteBuffer(OutputStream& stream, const char* buffer, size_t size)
{
    stream.writeBuffer(buffer, size);
}

void OutputStream::Flush(OutputStream& stream)
{
    stream.flush();
}

void OutputStream::flush()
{
    // Do nothing
}

void OutputStream::checkWrite() const
{
    // Do nothing
}

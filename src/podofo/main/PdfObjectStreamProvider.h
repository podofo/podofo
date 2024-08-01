/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_OBJECT_STREAM_PROVIDER_H
#define PDF_OBJECT_STREAM_PROVIDER_H

#include "PdfEncrypt.h"
#include <podofo/auxiliary/InputStream.h>
#include <podofo/auxiliary/OutputStream.h>

namespace PoDoFo {

class PdfObject;

class PODOFO_API PdfObjectStreamProvider
{
public:
    virtual ~PdfObjectStreamProvider();

    virtual void Init(PdfObject& obj) = 0;

    virtual void Clear() = 0;

    virtual bool TryCopyFrom(const PdfObjectStreamProvider& rhs) = 0;

    virtual bool TryMoveFrom(PdfObjectStreamProvider&& rhs) = 0;

    virtual std::unique_ptr<InputStream> GetInputStream(PdfObject& obj) = 0;

    virtual std::unique_ptr<OutputStream> GetOutputStream(PdfObject& obj) = 0;

    virtual void Write(OutputStream& stream, const PdfStatefulEncrypt* encrypt) = 0;

    virtual size_t GetLength() const = 0;
};

};

#endif // PDF_OBJECT_STREAM_PROVIDER_H

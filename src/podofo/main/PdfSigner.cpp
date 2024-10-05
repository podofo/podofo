/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfSigner.h"
#include "PdfDictionary.h"
#include "PdfSigningContext.h"
#include <podofo/auxiliary/StreamDevice.h>

using namespace std;
using namespace PoDoFo;

PdfSigner::PdfSigner() { }

PdfSigner::~PdfSigner() { }

void PdfSigner::FetchIntermediateResult(charbuff& result)
{
    (void)result;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

void PdfSigner::ComputeSignatureDeferred(const bufferview& processedResult, charbuff& buffer, bool dryrun)
{
    (void)processedResult;
    (void)buffer;
    (void)dryrun;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

bool PdfSigner::SkipBufferClear() const
{
    return false;
}

string PdfSigner::GetSignatureFilter() const
{
    // Default value
    return "Adobe.PPKLite";
}

void PoDoFo::SignDocument(PdfMemDocument& doc, StreamDevice& device, PdfSigner& signer,
    PdfSignature& signature, PdfSaveOptions saveOptions)
{
    PdfSigningContext ctx;
    ctx.AddSignerUnsafe(signature, signer);
    ctx.Sign(doc, device, saveOptions);
}

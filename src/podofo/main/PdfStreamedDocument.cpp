/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfStreamedDocument.h"
#include "PdfStreamDevice.h"

using namespace std;
using namespace PoDoFo;

PdfStreamedDocument::PdfStreamedDocument(const shared_ptr<OutputStreamDevice>& device, PdfVersion version,
        PdfEncrypt* encrypt, PdfSaveOptions opts) :
    m_Writer(nullptr),
    m_Device(device),
    m_Encrypt(encrypt)
{
    init(version, opts);
}

PdfStreamedDocument::PdfStreamedDocument(const string_view& filename, PdfVersion version,
        PdfEncrypt* encrypt, PdfSaveOptions opts) :
    m_Writer(nullptr),
    m_Device(new FileStreamDevice(filename, FileMode::Create)),
    m_Encrypt(encrypt)
{
    init(version, opts);
}

void PdfStreamedDocument::init(PdfVersion version, PdfSaveOptions opts)
{
    m_Writer.reset(new PdfImmediateWriter(this->GetObjects(), this->GetTrailer().GetObject(), *m_Device, version, m_Encrypt, opts));
}

void PdfStreamedDocument::Close()
{
    GetFonts().EmbedFonts();
    this->GetObjects().Finish();
}

PdfVersion PdfStreamedDocument::GetPdfVersion() const
{
    return m_Writer->GetPdfVersion();
}

void PdfStreamedDocument::SetPdfVersion(PdfVersion version)
{
    (void)version;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

const PdfEncrypt* PdfStreamedDocument::GetEncrypt() const
{
    return m_Encrypt;
}

/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfImmediateWriter.h"

#include "PdfStreamedObjectStream.h"
#include "PdfMemoryObjectStream.h"
#include "PdfObject.h"
#include "PdfXRef.h"
#include "PdfXRefStream.h"

using namespace std;
using namespace PoDoFo;

PdfImmediateWriter::PdfImmediateWriter(PdfIndirectObjectList& objects, const PdfObject& trailer,
        OutputStreamDevice& device, PdfVersion version, PdfEncrypt* encrypt, PdfSaveOptions opts) :
    PdfWriter(objects, trailer),
    m_attached(true),
    m_Device(&device),
    m_Last(nullptr),
    m_OpenStream(false)
{
    // register as observer for PdfIndirectObjectList
    GetObjects().Attach(*this);
    // register as stream factory for PdfIndirectObjectList
    GetObjects().SetStreamFactory(this);

    PdfString identifier;
    this->CreateFileIdentifier(identifier, trailer);
    SetIdentifier(identifier);

    // setup encryption
    if (encrypt != nullptr)
    {
        this->SetEncrypt(*encrypt);
        encrypt->GenerateEncryptionKey(GetIdentifier());
    }

    // start with writing the header
    this->SetPdfVersion(version);
    this->SetSaveOptions(opts);
    this->WritePdfHeader(*m_Device);

    m_xRef.reset(GetUseXRefStream() ? new PdfXRefStream(*this) : new PdfXRef(*this));
}

PdfImmediateWriter::~PdfImmediateWriter()
{
    if (m_attached)
        GetObjects().Detach(*this);
}

PdfWriteFlags PdfImmediateWriter::GetWriteFlags() const
{
    return PdfWriter::GetWriteFlags();
}

PdfVersion PdfImmediateWriter::GetPdfVersion() const
{
    return PdfWriter::GetPdfVersion();
}

void PdfImmediateWriter::WriteObject(const PdfObject& obj)
{
    const int endObjLenght = 7;

    this->FinishLastObject();

    m_xRef->AddInUseObject(obj.GetIndirectReference(), m_Device->GetPosition());
    obj.Write(*m_Device, this->GetWriteFlags(), GetEncrypt(), m_buffer);

    // Let's cheat a bit:
    // obj has written an "endobj\n" as last data to the file.
    // we simply overwrite this string with "stream\n" which 
    // has excatly the same length.
    m_Device->Seek(m_Device->GetPosition() - endObjLenght);
    m_Device->Write("stream\n");
    m_Last = const_cast<PdfObject*>(&obj);
}

void PdfImmediateWriter::Finish()
{
    // write all objects which are still in RAM
    this->FinishLastObject();

    // setup encrypt dictionary
    if (GetEncrypt() != nullptr)
    {
        // Add our own Encryption dictionary
        SetEncryptObj(GetObjects().CreateDictionaryObject());
        GetEncrypt()->CreateEncryptionDictionary(GetEncryptObj()->GetDictionary());
    }

    this->WritePdfObjects(*m_Device, GetObjects(), *m_xRef);

    // write the XRef
    uint64_t lXRefOffset = static_cast<uint64_t>(m_Device->GetPosition());
    m_xRef->Write(*m_Device, m_buffer);

    // FIX-ME: The following is already done by PdfXRef now
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);

    // XRef streams contain the trailer in the XRef
    if (!GetUseXRefStream())
    {
        PdfObject trailer;

        // if we have a dummy offset we write also a prev entry to the trailer
        FillTrailerObject(trailer, m_xRef->GetSize(), false);

        m_Device->Write("trailer\n");
        trailer.Write(*m_Device, this->GetWriteFlags(), nullptr, m_buffer);
    }

    utls::FormatTo(m_buffer, "startxref\n{}\n%%EOF\n", lXRefOffset);
    m_Device->Write(m_buffer);
    m_Device->Flush();

    // we are done now
    GetObjects().Detach(*this);
    m_attached = false;
}

unique_ptr<PdfObjectStreamProvider> PdfImmediateWriter::CreateStream()
{
    return unique_ptr<PdfObjectStreamProvider>(m_OpenStream ?
        static_cast<PdfObjectStreamProvider*>(new PdfMemoryObjectStream()) :
        static_cast<PdfObjectStreamProvider*>(new PdfStreamedObjectStream(*m_Device)));
}

void PdfImmediateWriter::FinishLastObject()
{
    if (m_Last == nullptr)
        return;

    m_Device->Write("\nendstream\n");
    m_Device->Write("endobj\n");

    GetObjects().RemoveObject(m_Last->GetIndirectReference(), false);
    m_Last = nullptr;
}

void PdfImmediateWriter::BeginAppendStream(PdfObjectStream& stream)
{
    auto streamedObjectStream = dynamic_cast<PdfStreamedObjectStream*>(&stream.GetProvider());
    if (streamedObjectStream != nullptr)
    {
        // Only one open file stream is allowed at a time
        PODOFO_ASSERT(!m_OpenStream);
        m_OpenStream = true;

        auto encrypt = GetEncrypt();
        if (encrypt != nullptr)
            streamedObjectStream->SetEncrypted(*encrypt);
    }
}

void PdfImmediateWriter::EndAppendStream(PdfObjectStream& stream)
{
    auto fileStream = dynamic_cast<const PdfStreamedObjectStream*>(&stream);
    if (fileStream != nullptr)
    {
        // A PdfFileStream has to be opened before
        PODOFO_ASSERT(m_OpenStream);
        m_OpenStream = false;
    }
}

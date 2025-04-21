/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2023 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfImmediateWriter.h"

#include <podofo/main/PdfStatefulEncrypt.h>

#include "PdfXRefStream.h"
#include "PdfStreamedObjectStream.h"

using namespace std;
using namespace PoDoFo;

PdfImmediateWriter::PdfImmediateWriter(PdfIndirectObjectList& objects, const PdfObject& trailer,
        OutputStreamDevice& device, PdfVersion version, shared_ptr<PdfEncrypt> encrypt, PdfSaveOptions opts) :
    PdfWriter(objects, trailer),
    m_Device(&device),
    m_OpenStream(false)
{
    SetPdfVersion(version);
    SetSaveOptions(opts);

    // Register as observer for PdfIndirectObjectList
    GetObjects().AttachObserver(*this);
    // Register as stream factory for PdfIndirectObjectList
    GetObjects().SetStreamFactory(this);

    PdfString identifier;
    this->CreateFileIdentifier(identifier, trailer);
    SetIdentifier(identifier);

    // Setup encryption
    if (encrypt != nullptr)
    {
        m_encrypt.reset(new PdfEncryptSession(std::move(encrypt)));
        this->SetEncrypt(*m_encrypt);
        m_encrypt->GetEncrypt().EnsureEncryptionInitialized(GetIdentifier(), m_encrypt->GetContext());
    }

    // Start with writing the header
    this->WritePdfHeader(*m_Device);

    // Manually prepare the cross-reference table/stream
    m_xRef.reset(GetUseXRefStream() ? new PdfXRefStream(*this) : new PdfXRef(*this));
}

PdfImmediateWriter::~PdfImmediateWriter()
{
    finish();
}

void PdfImmediateWriter::finish()
{
    // Before writing remaining objects remove
    // the already handled ones from the collection
    for (unsigned i = 0; i < m_writtenObjects.size(); i++)
        GetObjects().RemoveObject(m_writtenObjects[i]->GetIndirectReference(), false);

    // Eetup encrypt dictionary
    auto encrypt = GetEncrypt();
    if (encrypt != nullptr)
    {
        // Add our own Encryption dictionary
        SetEncryptObj(GetObjects().CreateDictionaryObject());
        encrypt->GetEncrypt().CreateEncryptionDictionary(GetEncryptObj()->GetDictionary());
    }

    // Write all the remaining objects
    this->WritePdfObjects(*m_Device, GetObjects(), *m_xRef);

    // Finally write the XRef
    m_xRef->Write(*m_Device, m_buffer);
}

unique_ptr<PdfObjectStreamProvider> PdfImmediateWriter::CreateStream()
{
    return unique_ptr<PdfObjectStreamProvider>(new PdfStreamedObjectStream(*m_Device));
}

void PdfImmediateWriter::BeginAppendStream(PdfObjectStream& stream)
{
    if (m_OpenStream)
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic,
            "One streaming operation is already opened at the same time");
    }

    m_OpenStream = true;
    auto encrypt = GetEncrypt();
    if (encrypt != nullptr)
    {
        auto& streamedObjectStream = dynamic_cast<PdfStreamedObjectStream&>(stream.GetProvider());
        streamedObjectStream.SetEncrypt(encrypt->GetEncrypt(), encrypt->GetContext());
    }

    auto& obj = stream.GetParent();

    // Manually mark the object as in-use, as it won't be
    // handled by the document object collection
    m_xRef->AddInUseObject(obj.GetIndirectReference(), m_Device->GetPosition());

    // Make sure, no one will add keys now to the object
    obj.SetImmutable();

    // Manually handle writing the object
    unique_ptr<PdfStatefulEncrypt> statefulEncrypt;
    if (encrypt != nullptr)
        statefulEncrypt.reset(new PdfStatefulEncrypt(encrypt->GetEncrypt(), encrypt->GetContext(), obj.GetIndirectReference()));

    obj.WriteHeader(*m_Device, this->GetWriteFlags(), m_buffer);
    obj.GetVariant().Write(*m_Device, this->GetWriteFlags(), statefulEncrypt.get(), m_buffer);
    obj.ResetDirty();
    m_Device->Write("\nstream\n");

    // Already written objects must then be removed
    // from internal document object collection
    m_writtenObjects.push_back(&obj);
}

void PdfImmediateWriter::EndAppendStream(PdfObjectStream& stream)
{
    (void)stream;
    PODOFO_ASSERT(m_OpenStream);
    m_Device->Write("\nendstream\nendobj\n");
    m_Device->Flush();
    m_OpenStream = false;
}

PdfVersion PdfImmediateWriter::GetPdfVersion() const
{
    return PdfWriter::GetPdfVersion();
}

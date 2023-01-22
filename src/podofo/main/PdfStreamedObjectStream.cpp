/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfStreamedObjectStream.h"

#include "PdfDocument.h"
#include "PdfEncrypt.h"
#include "PdfFilter.h"
#include "PdfOutputDevice.h"
#include "PdfObject.h"
#include "PdfDictionary.h"

using namespace std;
using namespace PoDoFo;

class PdfStreamedObjectStream::ObjectOutputStream : public OutputStream
{
public:
    ObjectOutputStream(PdfStreamedObjectStream& stream, OutputStreamDevice& outputStream) :
        m_objectStream(&stream),
        m_outputStream(&outputStream)
    {
    }

    ObjectOutputStream(PdfStreamedObjectStream& stream, unique_ptr<OutputStream> outputStream) :
        m_objectStream(&stream),
        m_outputStream(outputStream.get()),
        m_outputStreamStore(std::move(outputStream))
    {
    }

    ~ObjectOutputStream()
    {
        Flush(*m_outputStream);
        m_objectStream->FinishOutput();
    }

protected:
    virtual void writeBuffer(const char* buffer, size_t size)
    {
        WriteBuffer(*m_outputStream, buffer, size);
        m_objectStream->m_Length += size;
    }

    virtual void flush()
    {
        Flush(*m_outputStream);
    }

private:
    PdfStreamedObjectStream* m_objectStream;
    OutputStream* m_outputStream;
    std::unique_ptr<OutputStream> m_outputStreamStore;
};

PdfStreamedObjectStream::PdfStreamedObjectStream(OutputStreamDevice& device) :
    m_Device(&device),
    m_CurrEncrypt(nullptr),
    m_Length(0),
    m_LengthObj(nullptr)
{
}

void PdfStreamedObjectStream::Init(PdfObject& obj)
{
    m_LengthObj = &obj.GetDocument()->GetObjects().CreateObject(static_cast<int64_t>(0));
    obj.GetDictionary().AddKey(PdfName::KeyLength, m_LengthObj->GetIndirectReference());
}

void PdfStreamedObjectStream::Clear()
{
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Unsupported resetting streamed object stream");
}

bool PdfStreamedObjectStream::TryCopyFrom(const PdfObjectStreamProvider& rhs)
{
    (void)rhs;
    return false;
}

bool PdfStreamedObjectStream::TryMoveFrom(PdfObjectStreamProvider&& rhs)
{
    (void)rhs;
    return false;
}

unique_ptr<InputStream> PdfStreamedObjectStream::GetInputStream(PdfObject& obj)
{
    (void)obj;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Unsupported reading from streamed object stream");
}

unique_ptr<OutputStream> PdfStreamedObjectStream::GetOutputStream(PdfObject& obj)
{
    obj.GetDocument()->GetObjects().WriteObject(obj);
    if (m_CurrEncrypt == nullptr)
    {
        return std::make_unique<ObjectOutputStream>(*this, *m_Device);
    }
    else
    {
        return std::make_unique<ObjectOutputStream>(*this,
            m_CurrEncrypt->CreateEncryptionOutputStream(*m_Device, obj.GetIndirectReference()));
    }
}

void PdfStreamedObjectStream::Write(OutputStream& stream, const PdfStatefulEncrypt& encrypt)
{
    (void)stream;
    (void)encrypt;
    // Do nothing
}

size_t PdfStreamedObjectStream::GetLength() const
{
    return m_Length;
}

bool PdfStreamedObjectStream::IsLengthHandled() const
{
    return true;
}

void PdfStreamedObjectStream::FinishOutput()
{
    if (m_CurrEncrypt != nullptr)
        m_Length = m_CurrEncrypt->CalculateStreamLength(m_Length);

    m_LengthObj->SetNumber(static_cast<int64_t>(m_Length));
}

void PdfStreamedObjectStream::SetEncrypted(PdfEncrypt& encrypt)
{
    m_CurrEncrypt = &encrypt;
}

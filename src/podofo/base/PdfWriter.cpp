/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfWriter.h"

#include "PdfData.h"
#include "PdfDate.h"
#include "PdfDictionary.h"
#include "PdfObject.h"
#include "PdfParser.h"
#include "PdfParserObject.h"
#include "PdfObjectStream.h"
#include "PdfVariant.h"
#include "PdfXRef.h"
#include "PdfXRefStream.h"
#include "PdfStreamDevice.h"

#define PDF_MAGIC           "\xe2\xe3\xcf\xd3\n"
// 10 spaces
#define LINEARIZATION_PADDING "          "

using namespace std;
using namespace PoDoFo;

static PdfWriteFlags ToWriteFlags(PdfSaveOptions opts);

PdfWriter::PdfWriter(PdfIndirectObjectList* objects, const PdfObject& trailer, PdfVersion version) :
    m_Objects(objects),
    m_Trailer(&trailer),
    m_Version(version),
    m_UseXRefStream(false),
    m_EncryptObj(nullptr),
    m_SaveOptions(PdfSaveOptions::None),
    m_WriteFlags(PdfWriteFlags::None),
    m_PrevXRefOffset(0),
    m_IncrementalUpdate(false),
    m_rewriteXRefTable(false)
{
}

PdfWriter::PdfWriter(PdfIndirectObjectList& objects, const PdfObject& trailer)
    : PdfWriter(&objects, trailer, PdfVersionDefault)
{
}

PdfWriter::PdfWriter(PdfIndirectObjectList& objects)
    : PdfWriter(&objects, PdfObject(), PdfVersionDefault)
{
}

void PdfWriter::SetIncrementalUpdate(bool rewriteXRefTable)
{
    m_IncrementalUpdate = true;
    m_rewriteXRefTable = rewriteXRefTable;
}

void PdfWriter::SetSaveOptions(PdfSaveOptions opts)
{
    m_SaveOptions = opts;
    m_WriteFlags = ToWriteFlags(opts);
}

PdfWriter::~PdfWriter()
{
    m_Objects = nullptr;
}

void PdfWriter::Write(OutputStreamDevice& device)
{
    CreateFileIdentifier(m_identifier, *m_Trailer, &m_originalIdentifier);

    // setup encrypt dictionary
    if (m_Encrypt != nullptr)
    {
        m_Encrypt->GenerateEncryptionKey(m_identifier);

        // Add our own Encryption dictionary
        m_EncryptObj = &m_Objects->CreateDictionaryObject();
        m_Encrypt->CreateEncryptionDictionary(m_EncryptObj->GetDictionary());
    }

    unique_ptr<PdfXRef> xRef;
    if (m_UseXRefStream)
        xRef.reset(new PdfXRefStream(*this));
    else
        xRef.reset(new PdfXRef(*this));

    try
    {
        if (!m_IncrementalUpdate)
            WritePdfHeader(device);

        WritePdfObjects(device, *m_Objects, *xRef);

        if (m_IncrementalUpdate)
            xRef->SetFirstEmptyBlock();

        xRef->Write(device, m_buffer);
    }
    catch (PdfError& e)
    {
        // P.Zent: Delete Encryption dictionary (cannot be reused)
        if (m_EncryptObj != nullptr)
        {
            m_Objects->RemoveObject(m_EncryptObj->GetIndirectReference());
            m_EncryptObj = nullptr;
        }

        PODOFO_PUSH_FRAME(e);
        throw e;
    }

    // P.Zent: Delete Encryption dictionary (cannot be reused)
    if (m_EncryptObj != nullptr)
    {
        m_Objects->RemoveObject(m_EncryptObj->GetIndirectReference());
        m_EncryptObj = nullptr;
    }
}

void PdfWriter::WritePdfHeader(OutputStreamDevice& device)
{
    utls::FormatTo(m_buffer, "%PDF-{}\n%{}", PoDoFo::GetPdfVersionName(m_Version), PDF_MAGIC);
    device.Write(m_buffer);
}

void PdfWriter::WritePdfObjects(OutputStreamDevice& device, const PdfIndirectObjectList& objects, PdfXRef& xref)
{
    for (PdfObject* obj : objects)
    {
        if (m_IncrementalUpdate && !obj->IsDirty())
        {
            if (m_rewriteXRefTable)
            {
                PdfParserObject* parserObject = dynamic_cast<PdfParserObject*>(obj);
                if (parserObject != nullptr)
                {
                    // Try to see if we can just write the reference to previous entry
                    // without rewriting the entry

                    // the reference looks like "0 0 R", while the object identifier like "0 0 obj", thus add two letters
                    size_t objRefLength = obj->GetIndirectReference().ToString().length() + 2;

                    // the offset points just after the "0 0 obj" string
                    if (parserObject->GetOffset() - objRefLength > 0)
                    {
                        xref.AddInUseObject(obj->GetIndirectReference(), parserObject->GetOffset() - objRefLength);
                        continue;
                    }
                }
            }
            else
            {
                // The object will not be output in the XRef entries but it will be
                // counted in trailer's /Size
                xref.AddInUseObject(obj->GetIndirectReference(), nullptr);
                continue;
            }
        }

        if (xref.ShouldSkipWrite(obj->GetIndirectReference()))
        {
            // If we skip write of this object, we supply a dummy
            // offset of the object and not retrieve it from the device
            xref.AddInUseObject(obj->GetIndirectReference(), 0xFFFFFFFF);
        }
        else
        {
            xref.AddInUseObject(obj->GetIndirectReference(), device.GetPosition());
            // Also make sure that we do not encrypt the encryption dictionary!
            obj->Write(device, m_WriteFlags, obj == m_EncryptObj ? nullptr : m_Encrypt.get(), m_buffer);
        }
    }

    for (auto& freeObjectRef : objects.GetFreeObjects())
    {
        xref.AddFreeObject(freeObjectRef);
    }
}

void PdfWriter::FillTrailerObject(PdfObject& trailer, size_t size, bool onlySizeKey) const
{
    trailer.GetDictionary().AddKey(PdfName::KeySize, static_cast<int64_t>(size));

    if (!onlySizeKey)
    {
        if (m_Trailer->GetDictionary().HasKey("Root"))
            trailer.GetDictionary().AddKey("Root", *m_Trailer->GetDictionary().GetKey("Root"));
        // It makes no sense to simple copy an encryption key
        // Either we have no encryption or we encrypt again by ourselves
        if (m_Trailer->GetDictionary().HasKey("Info"))
            trailer.GetDictionary().AddKey("Info", *m_Trailer->GetDictionary().GetKey("Info"));

        if (m_EncryptObj != nullptr)
            trailer.GetDictionary().AddKey(PdfName("Encrypt"), m_EncryptObj->GetIndirectReference());

        PdfArray array;
        // The ID is the same unless the PDF was incrementally updated
        if (m_IncrementalUpdate && !m_originalIdentifier.IsEmpty())
            array.Add(m_originalIdentifier);
        else
            array.Add(m_identifier);

        array.Add(m_identifier);

        // finally add the key to the trailer dictionary
        trailer.GetDictionary().AddKey("ID", array);

        if (!m_rewriteXRefTable && m_PrevXRefOffset > 0)
        {
            PdfVariant value(m_PrevXRefOffset);
            trailer.GetDictionary().AddKey("Prev", value);
        }
    }
}

void PdfWriter::CreateFileIdentifier(PdfString& identifier, const PdfObject& trailer, PdfString* originalIdentifier)
{
    NullStreamDevice length;
    unique_ptr<PdfObject> info;
    bool originalIdentifierFound = false;

    const PdfObject* idObj;
    if (originalIdentifier != nullptr &&
        (idObj = trailer.GetDictionary().FindKey("ID")) != nullptr)
    {
        auto it = idObj->GetArray().begin();
        PdfString str;
        if (it != idObj->GetArray().end() && it->TryGetString(str))
        {
            if (str.IsHex())
                *originalIdentifier = it->GetString();
            else
                *originalIdentifier = PdfString::FromRaw(it->GetString().GetRawData());
            originalIdentifierFound = true;
        }
    }

    // create a dictionary with some unique information.
    // This dictionary is based on the PDF files information
    // dictionary if it exists.
    auto infoObj = trailer.GetDictionary().GetKey("Info");
    if (infoObj == nullptr)
    {
        auto now = PdfDate::LocalNow();
        PdfString dateString = now.ToString();

        info.reset(new PdfObject());
        info->GetDictionary().AddKey("CreationDate", dateString);
        info->GetDictionary().AddKey("Creator", PdfString("PoDoFo"));
        info->GetDictionary().AddKey("Producer", PdfString("PoDoFo"));
    }
    else
    {
        PdfReference ref;
        if (infoObj->TryGetReference(ref))
        {
            infoObj = m_Objects->GetObject(ref);

            if (infoObj == nullptr)
            {
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Error while retrieving info dictionary: {} {} R",
                    ref.ObjectNumber(), ref.GenerationNumber());
            }
            else
            {
                info.reset(new PdfObject(*infoObj));
            }
        }
        else if (infoObj->IsDictionary())
        {
            // NOTE: While Table 15, ISO 32000-1:2008, states that Info should be an
            // indirect reference, we found Pdfs, for example produced
            // by pdfjs v0.4.1 (github.com/rkusa/pdfjs) that do otherwise.
            // As usual, Acroat Pro Syntax checker doesn't care about this,
            // so let's just read it
            info.reset(new PdfObject(*infoObj));
        }
        else
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Invalid");
        }
    }

    info->GetDictionary().AddKey("Location", PdfString("SOMEFILENAME"));
    info->Write(length, m_WriteFlags, nullptr, m_buffer);

    charbuff buffer(length.GetLength());
    StringStreamDevice device(buffer);
    info->Write(device, m_WriteFlags, nullptr, m_buffer);

    // calculate the MD5 Sum
    identifier = PdfEncryptMD5Base::GetMD5String(reinterpret_cast<unsigned char*>(buffer.data()),
        static_cast<unsigned>(length.GetLength()));

    if (originalIdentifier != nullptr && !originalIdentifierFound)
        *originalIdentifier = identifier;
}

void PdfWriter::SetEncryptObj(PdfObject& obj)
{
    m_EncryptObj = &obj;
}

// CHECK-ME: Should this accept a mutable reference instead,
// to reflect changes on the source encrypt (see usage on PdfMemDocument)
void PdfWriter::SetEncrypt(const PdfEncrypt& encrypt)
{
    m_Encrypt = PdfEncrypt::CreateFromEncrypt(encrypt);
}

void PdfWriter::SetUseXRefStream(bool useXRefStream)
{
    if (useXRefStream && m_Version < PdfVersion::V1_5)
        this->SetPdfVersion(PdfVersion::V1_5);

    m_UseXRefStream = useXRefStream;
}

PdfWriteFlags ToWriteFlags(PdfSaveOptions opts)
{
    PdfWriteFlags ret = PdfWriteFlags::None;
    if ((opts & PdfSaveOptions::NoFlateCompress) !=
        PdfSaveOptions::None)
    {
        ret |= PdfWriteFlags::NoFlateCompress;
    }

    if ((opts & PdfSaveOptions::Clean) !=
        PdfSaveOptions::None)
    {
        ret |= PdfWriteFlags::Clean;
    }

    return ret;
}

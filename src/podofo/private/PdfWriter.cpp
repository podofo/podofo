/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PdfDeclarationsPrivate.h"
#include "PdfWriter.h"

#include <podofo/auxiliary/StreamDevice.h>
#include <podofo/main/PdfDate.h>
#include <podofo/main/PdfDictionary.h>
#include "PdfParserObject.h"
#include "PdfXRefStream.h"
#include "OpenSSLInternal.h"

#define PDF_MAGIC           "\xe2\xe3\xcf\xd3\n"
// 10 spaces
#define LINEARIZATION_PADDING "          "

using namespace std;
using namespace PoDoFo;

static PdfWriteFlags toWriteFlags(PdfSaveOptions opts, PdfALevel pdfaLevel);

PdfWriter::PdfWriter(PdfIndirectObjectList* objects, const PdfObject& trailer, size_t magicOffset) :
    m_Objects(objects),
    m_Trailer(&trailer),
    m_MagicOffset(magicOffset),
    m_Version(PdfVersionDefault),
    m_PdfALevel(PdfALevel::Unknown),
    m_UseXRefStream(false),
    m_Encrypt(nullptr),
    m_EncryptObj(nullptr),
    m_SaveOptions(PdfSaveOptions::None),
    m_WriteFlags(PdfWriteFlags::None),
    m_PrevXRefOffset(0), // 0 is a sentinel for invalid XRef offset
    m_CurrXRefOffset(0), // 0 is a sentinel for invalid XRef offset
    m_IsIncrementalUpdate(false)
{
}

PdfWriter::PdfWriter(PdfIndirectObjectList& objects, const PdfObject& trailer,
        size_t magicOffset)
    : PdfWriter(&objects, trailer, magicOffset)
{
}

PdfWriter::PdfWriter(PdfIndirectObjectList& objects)
    : PdfWriter(&objects, PdfObject(), 0)
{
}

void PdfWriter::SetIncrementalUpdate(bool enabled)
{
    m_IsIncrementalUpdate = enabled;
}

PdfWriter::~PdfWriter()
{
    m_Objects = nullptr;
}

void PdfWriter::initWriteFlags()
{
    m_WriteFlags = toWriteFlags(m_SaveOptions, m_PdfALevel);
}

void PdfWriter::Write(OutputStreamDevice& device)
{
    CreateFileIdentifier(m_identifier, *m_Trailer, &m_originalIdentifier);

    // setup encrypt dictionary
    if (m_Encrypt != nullptr)
    {
        m_Encrypt->GetEncrypt().EnsureEncryptionInitialized(m_identifier, m_Encrypt->GetContext());

        // Add our own Encryption dictionary
        m_EncryptObj = &m_Objects->CreateDictionaryObject();
        m_Encrypt->GetEncrypt().CreateEncryptionDictionary(m_EncryptObj->GetDictionary());
    }

    unique_ptr<PdfXRef> xRef;
    if (m_UseXRefStream)
        xRef.reset(new PdfXRefStream(*this));
    else
        xRef.reset(new PdfXRef(*this));

    try
    {
        if (m_IsIncrementalUpdate)
        {
            if (m_PrevXRefOffset == 0)
                PoDoFo::LogMessage(PdfLogSeverity::Warning,
                    "Writing an update with previously read invalid XRef sections. "
                    "The cross references will be fully rewritten");
        }
        else
        {
            WritePdfHeader(device);
        }

        WritePdfObjects(device, *m_Objects, *xRef);
        xRef->Write(device, m_buffer);
        m_CurrXRefOffset = xRef->GetOffset();
    }
    catch (PdfError& e)
    {
        // Delete Encryption dictionary (cannot be reused)
        if (m_EncryptObj != nullptr)
        {
            m_Objects->RemoveObject(m_EncryptObj->GetIndirectReference());
            m_EncryptObj = nullptr;
        }

        PODOFO_PUSH_FRAME(e);
        throw;
    }

    // Delete Encryption dictionary (cannot be reused)
    if (m_EncryptObj != nullptr)
    {
        m_Objects->RemoveObject(m_EncryptObj->GetIndirectReference());
        m_EncryptObj = nullptr;
    }

    device.Flush();
    m_Objects->ResetFreeObjectsInvalidated();
}

void PdfWriter::WritePdfHeader(OutputStreamDevice& device)
{
    utls::FormatTo(m_buffer, "%PDF-{}\n%{}", PoDoFo::GetPdfVersionName(m_Version).GetString(), PDF_MAGIC);
    device.Write(m_buffer);
}

void PdfWriter::WritePdfObjects(OutputStreamDevice& device, const PdfIndirectObjectList& objects, PdfXRef& xref)
{
    unique_ptr<PdfStatefulEncrypt> encrypt;
    for (PdfObject* obj : objects)
    {
        if (m_Encrypt != nullptr && obj != m_EncryptObj)
            encrypt.reset(new PdfStatefulEncrypt(m_Encrypt->GetEncrypt(), m_Encrypt->GetContext(), obj->GetIndirectReference()));
        else
            encrypt.reset();

        if (m_IsIncrementalUpdate && !obj->IsDirty())
        {
            if (m_PrevXRefOffset == 0)
            {
                // The previous XRef was not read successfully and needs rewriting,
                // try to see if we can just write the previous object offset in the
                // entry without fully rewriting it
                PdfParserObject* parserObject = dynamic_cast<PdfParserObject*>(obj);
                if (parserObject != nullptr)
                {
                    if (parserObject->GetOffset() > 0)
                    {
                        xref.AddInUseObject(obj->GetIndirectReference(), parserObject->GetOffset() - m_MagicOffset);
                        continue;
                    }
                }
            }
            else
            {
                // It's a regular incremental update, just skip processing the object
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
            xref.AddInUseObject(obj->GetIndirectReference(), device.GetPosition() - m_MagicOffset);
            // Also make sure that we do not encrypt the encryption dictionary!
            obj->WriteFinal(device, m_WriteFlags, encrypt.get(), m_buffer);
        }
    }

    if (!m_IsIncrementalUpdate || m_PrevXRefOffset == 0)
    {
        // It's a regular save, or the previous XRef was not read
        // successfully and needs rewriting: add to the XRef all
        // free/unavailable objects NOTE: It's not necessary to add
        // unavailable objects, they will be handled implicitly as when
        // the object is not defined it's treated as unavailable
        for (auto& freeObjectRef : objects.GetFreeObjects())
            xref.AddFreeObject(freeObjectRef);
    }
    else
    {
        if (objects.AreFreeObjectsInvalidated())
        {
            // Free objects were invalidated, such as when deleting objects
            // or re-using free objects references
            for (auto& freeObjectRef : objects.GetFreeObjects())
                xref.AddFreeObject(freeObjectRef);

            for (auto& freeObjectRef : objects.GetUnavailableObjects())
                xref.AddUnavailableObject(freeObjectRef);
        }
    }
}

void PdfWriter::FillTrailerObject(PdfObject& trailer, size_t size, bool onlySizeKey) const
{
    trailer.GetDictionary().AddKey("Size"_n, static_cast<int64_t>(size));

    if (!onlySizeKey)
    {
        if (m_Trailer->GetDictionary().HasKey("Root"))
            trailer.GetDictionary().AddKey("Root"_n, *m_Trailer->GetDictionary().GetKey("Root"));
        // It makes no sense to simple copy an encryption key
        // Either we have no encryption or we encrypt again by ourselves
        if (m_Trailer->GetDictionary().HasKey("Info"))
            trailer.GetDictionary().AddKey("Info"_n, *m_Trailer->GetDictionary().GetKey("Info"));

        if (m_EncryptObj != nullptr)
            trailer.GetDictionary().AddKey("Encrypt"_n, m_EncryptObj->GetIndirectReference());

        PdfArray array;
        // The ID must stay the same if this is an incremental update
        // or the /Encrypt entry was parsed
        if ((m_IsIncrementalUpdate || (m_Encrypt != nullptr && m_Encrypt->GetEncrypt().IsParsed())) && !m_originalIdentifier.IsEmpty())
            array.Add(m_originalIdentifier);
        else
            array.Add(m_identifier);

        array.Add(m_identifier);

        // finally add the key to the trailer dictionary
        trailer.GetDictionary().AddKey("ID"_n, array);

        // If the previous XRef was read successfully, just make add
        // a /Prev pointer to it
        if (m_PrevXRefOffset > 0)
            trailer.GetDictionary().AddKey("Prev"_n, (int64_t)(m_PrevXRefOffset - m_MagicOffset));
    }
}

void PdfWriter::SetSaveOptions(PdfSaveOptions saveOptions)
{
    m_SaveOptions = saveOptions;
    initWriteFlags();
}

void PdfWriter::SetPdfALevel(PdfALevel level)
{
    m_PdfALevel = level;
    initWriteFlags();
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
        info->GetDictionary().AddKey("CreationDate"_n, dateString);
        info->GetDictionary().AddKey("Creator"_n, PdfString("PoDoFo"));
        info->GetDictionary().AddKey("Producer"_n, PdfString("PoDoFo"));
    }
    else
    {
        PdfReference ref;
        if (infoObj->TryGetReference(ref))
        {
            infoObj = m_Objects->GetObject(ref);

            if (infoObj == nullptr)
            {
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ObjectNotFound, "Error while retrieving info dictionary: {} {} R",
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
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Invalid");
        }
    }

    info->GetDictionary().AddKey("Location"_n, PdfString("SOMEFILENAME"));
    info->WriteFinal(length, m_WriteFlags, nullptr, m_buffer);

    charbuff buffer(length.GetLength());
    StringStreamDevice device(buffer, DeviceAccess::Write, false);
    info->WriteFinal(device, m_WriteFlags, nullptr, m_buffer);

    // calculate the MD5 Sum
    identifier = PdfString(ssl::ComputeMD5(buffer), true);
    if (originalIdentifier != nullptr && !originalIdentifierFound)
        *originalIdentifier = identifier;
}

void PdfWriter::SetEncryptObj(PdfObject& obj)
{
    m_EncryptObj = &obj;
}

void PdfWriter::SetEncrypt(PdfEncryptSession& encrypt)
{
    m_Encrypt = &encrypt;
}

void PdfWriter::SetUseXRefStream(bool useXRefStream)
{
    if (useXRefStream && m_Version < PdfVersion::V1_5)
        m_Version = PdfVersion::V1_5;

    m_UseXRefStream = useXRefStream;
}

PdfWriteFlags toWriteFlags(PdfSaveOptions opts, PdfALevel pdfaLevel)
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

    if (pdfaLevel != PdfALevel::Unknown)
        ret |= PdfWriteFlags::PdfAPreserve;

    return ret;
}

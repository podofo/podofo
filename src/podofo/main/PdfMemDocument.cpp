// SPDX-FileCopyrightText: 2006 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfMemDocument.h"

#include <podofo/auxiliary/StreamDevice.h>
#include <podofo/private/PdfWriter.h>
#include <podofo/private/PdfParser.h>
#include <podofo/private/PdfEncryptSession.h>

#include "PdfCommon.h"

using namespace std;
using namespace PoDoFo;

PdfMemDocument::PdfMemDocument()
    : PdfMemDocument(false) { }

PdfMemDocument::PdfMemDocument(bool empty) :
    PdfDocument(empty),
    m_Version(PdfVersionDefault),
    m_InitialVersion(PdfVersionDefault),
    m_HasXRefStream(false),
    m_HasBrokenXRef(false),
    m_initiallyEncrypted(false),
    m_MagicOffset(0),
    m_PrevXRefOffset(0) // 0 is a sentinel for no or invalid XRef offset
{
}

PdfMemDocument::PdfMemDocument(shared_ptr<InputStreamDevice> device, const string_view& password)
    : PdfMemDocument(true)
{
    if (device == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    loadFromDevice(std::move(device), PdfLoadOptions::None, password);
}

PdfMemDocument::PdfMemDocument(shared_ptr<InputStreamDevice> device, PdfLoadOptions opts, const string_view& password)
    : PdfMemDocument(true)
{
    if (device == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    loadFromDevice(std::move(device), opts, password);
}

PdfMemDocument::PdfMemDocument(const PdfMemDocument& rhs) :
    PdfDocument(rhs),
    m_Version(rhs.m_Version),
    m_InitialVersion(rhs.m_InitialVersion),
    m_HasXRefStream(rhs.m_HasXRefStream),
    m_HasBrokenXRef(rhs.m_HasBrokenXRef),
    m_initiallyEncrypted(rhs.m_initiallyEncrypted),
    m_MagicOffset(rhs.m_MagicOffset),
    m_PrevXRefOffset(rhs.m_PrevXRefOffset)
{
    // Do a full copy of the encrypt session
    if (rhs.m_Encrypt != nullptr)
        m_Encrypt.reset(new PdfEncryptSession(rhs.m_Encrypt->GetEncrypt(), rhs.m_Encrypt->GetContext()));
}

PdfMemDocument::~PdfMemDocument()
{
    // NOTE: This must be defined to avoid exposing PdfEncryptSession
    // in the public header, as it is a private class. Otherwise the
    // compiler will complain about an incomplete type when generating
    // the destructor for PdfMemDocument
}

void PdfMemDocument::clear()
{
    // NOTE: Here we clear only variables that have memory
    // usage. The other variables get initialized by parsing or reset
    m_Encrypt = nullptr;
    m_device = nullptr;
}

void PdfMemDocument::reset()
{
    m_Version = PdfVersionDefault;
    m_InitialVersion = PdfVersionDefault;
    m_HasXRefStream = false;
    m_HasBrokenXRef = false;
    m_initiallyEncrypted = false;
    m_MagicOffset = 0;
    m_PrevXRefOffset = 0;
}

void PdfMemDocument::initFromParser(PdfParser& parser)
{
    m_Version = parser.GetPdfVersion();
    m_InitialVersion = m_Version;
    m_HasXRefStream = parser.HasXRefStream();
    m_HasBrokenXRef = parser.HasCorruptedXRefSections();
    m_PrevXRefOffset = parser.GetXRefOffset();
    m_MagicOffset = parser.GetMagicOffset();
    auto entryPoint = parser.TakeEntryPoints();
    SetEntryPoints(std::move(entryPoint.Trailer), entryPoint.Catalog);

    auto encrypt = parser.GetEncrypt();
    m_initiallyEncrypted = encrypt != nullptr;
    if (encrypt != nullptr)
        m_Encrypt.reset(new PdfEncryptSession(*encrypt));

    Init();
}

void PdfMemDocument::Load(const string_view& filename, const string_view& password)
{
    Load(filename, PdfLoadOptions::None, password);
}

void PdfMemDocument::Load(const string_view& filename, PdfLoadOptions opts, const string_view& password)
{
    if (filename.length() == 0)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    loadFromDevice(std::make_shared<FileStreamDevice>(filename), opts, password);
}

void PdfMemDocument::LoadFromBuffer(const bufferview& buffer, const string_view& password)
{
    LoadFromBuffer(buffer, PdfLoadOptions::None, password);
}

void PdfMemDocument::LoadFromBuffer(const bufferview& buffer, PdfLoadOptions opts, const string_view& password)
{
    if (buffer.size() == 0)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    loadFromDevice(std::make_shared<SpanStreamDevice>(buffer), opts, password);
}

void PdfMemDocument::Load(shared_ptr<InputStreamDevice> device, const string_view& password)
{
    if (device == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    loadFromDevice(std::move(device), PdfLoadOptions::None, password);
}

void PdfMemDocument::Load(shared_ptr<InputStreamDevice> device, PdfLoadOptions opts, const string_view& password)
{
    if (device == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    loadFromDevice(std::move(device), opts, password);
}

void PdfMemDocument::loadFromDevice(shared_ptr<InputStreamDevice>&& device,
    PdfLoadOptions opts, const string_view& password)
{
    this->Clear();
    m_device = std::move(device);

    bool strictParsing = (opts & PdfLoadOptions::StrictParsing) != PdfLoadOptions::None;
    SetStrictParsing(strictParsing);

    // Call parse file instead of using the constructor
    // so that m_Parser is initialized for encrypted documents
    PdfParser parser(PdfDocument::GetObjects());
    parser.SetStrictParsing(strictParsing);
    if ((opts & PdfLoadOptions::SkipXRefRecovery) != PdfLoadOptions::None)
        parser.SetSkipXRefRecovery(true);
    if ((opts & PdfLoadOptions::LoadStreamsEagerly) != PdfLoadOptions::None)
        parser.SetLoadStreamsEagerly(true);

    parser.SetPassword(password);
    parser.Parse(*m_device);
    initFromParser(parser);
}

void PdfMemDocument::Save(const string_view& filename, PdfSaveOptions options)
{
    FileStreamDevice device(filename, FileMode::Create);
    this->Save(device, options);
}

void PdfMemDocument::Save(OutputStreamDevice& device, PdfSaveOptions opts)
{
    beforeWrite(opts, false);

    PdfWriter writer(this->GetObjects(), this->GetTrailer().GetObject(), 0);
    writer.SetPdfVersionHint(GetMetadata().GetPdfVersion());
    writer.SetPdfALevel(GetMetadata().GetPdfALevel());
    writer.SetSaveOptions(opts);
    writer.SetUseXRefStreamHint(m_HasXRefStream);

    if (m_Encrypt != nullptr)
        writer.SetEncrypt(*m_Encrypt);

    try
    {
        writer.Write(device);
    }
    catch (PdfError& e)
    {
        PODOFO_PUSH_FRAME(e);
        throw;
    }

    m_PrevXRefOffset = writer.GetCurrXRefOffset();
    m_HasBrokenXRef = false;
}

void PdfMemDocument::SaveUpdate(const string_view& filename, PdfSaveOptions opts)
{
    FileStreamDevice device(filename, FileMode::Append);
    this->SaveUpdate(device, opts);
}

void PdfMemDocument::SaveUpdate(OutputStreamDevice& device, PdfSaveOptions opts)
{
    // The encryption in effect must be the one the document was parsed with:
    // an update can't re-key the document, as the objects of the previous
    // revisions are not rewritten and would stay encrypted with the previous one
    bool encryptPreserved = m_Encrypt == nullptr
        ? !m_initiallyEncrypted
        : m_Encrypt->GetEncrypt().IsParsed();
    if (!encryptPreserved)
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedOperation,
            "The encryption of a document can't be set, changed or removed when "
            "writing an incremental update. Perform a regular save instead");
    }

    beforeWrite(opts, true);

    PdfWriter writer(this->GetObjects(), this->GetTrailer().GetObject(), m_MagicOffset);
    writer.SetPdfVersionHint(GetMetadata().GetPdfVersion());
    writer.SetPdfALevel(GetMetadata().GetPdfALevel());
    writer.SetSaveOptions(opts);
    writer.SetPrevXRefOffset(m_PrevXRefOffset);
    writer.SetUseXRefStreamHint(m_HasXRefStream);
    writer.SetIncrementalUpdate(true);

    if (m_Encrypt != nullptr)
        writer.SetEncrypt(*m_Encrypt);

    if (m_InitialVersion < this->GetPdfVersion())
    {
        if (this->GetPdfVersion() < PdfVersion::V1_0 || this->GetPdfVersion() > PdfVersion::V2_0)
            PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

        GetCatalog().GetDictionary().AddKey("Version"_n, PoDoFo::GetPdfVersionName(GetPdfVersion()));
    }

    try
    {
        device.Seek(0, SeekDirection::End);
        writer.Write(device);
    }
    catch (PdfError& e)
    {
        PODOFO_PUSH_FRAME(e);
        throw;
    }

    m_PrevXRefOffset = writer.GetCurrXRefOffset();
    m_HasBrokenXRef = false;
}

void PdfMemDocument::beforeWrite(PdfSaveOptions opts, bool isUpdate)
{
    if ((opts & PdfSaveOptions::NoMetadataUpdate) ==
        PdfSaveOptions::None)
    {
        GetMetadata().SetModifyDate(PdfDate::LocalNow());
        (void)GetMetadata().TrySyncXMPMetadata();
    }

    GetFonts().EmbedFonts();

    // After we are done with all operations on objects,
    // we can collect garbage
    if ((opts & PdfSaveOptions::NoCollectGarbage) ==
        PdfSaveOptions::None)
    {
        if (!isUpdate)
        {
            // On a full save the content of the object streams is rewritten as
            // top level objects, so the containers can be collected as well. On an
            // incremental update they must be preserved instead, as previous
            // revisions still reference them for their compressed objects
            GetObjects().ClearCompressedObjectStreams();
        }

        CollectGarbage();
    }
}

void PdfMemDocument::SetEncrypted(const string_view& userPassword, const string_view& ownerPassword,
    PdfPermissions protection, PdfEncryptionAlgorithm algorithm,
    PdfKeyLength keyLength)
{
    m_Encrypt.reset(new PdfEncryptSession(PdfEncrypt::Create(userPassword, ownerPassword, protection, algorithm, keyLength)));
}

void PdfMemDocument::SetEncrypt(unique_ptr<PdfEncrypt>&& encrypt)
{
    if (encrypt == nullptr)
    {
        m_Encrypt = nullptr;
        // Drop the reference to the encryption dictionary of a previously
        // encrypted document, so it can be collected as garbage
        GetTrailer().GetDictionary().RemoveKey("Encrypt");
    }
    else
    {
        m_Encrypt.reset(new PdfEncryptSession(std::move(encrypt)));
    }
}

bool PdfMemDocument::HasOwnerPermissions() const
{
    return m_Encrypt == nullptr || m_Encrypt->HasOwnerPermissions();
}

const PdfEncrypt* PdfMemDocument::GetEncrypt() const
{
    if (m_Encrypt == nullptr)
        return nullptr;

    return &m_Encrypt->GetEncrypt();
}

void PdfMemDocument::SetPdfVersion(PdfVersion version)
{
    m_Version = version;
}

PdfVersion PdfMemDocument::GetPdfVersion() const
{
    return m_Version;
}

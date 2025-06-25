/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfMemDocument.h"

#include <podofo/auxiliary/StreamDevice.h>
#include <podofo/private/PdfWriter.h>
#include <podofo/private/PdfParser.h>

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
    m_PrevXRefOffset(-1)
{
}

PdfMemDocument::PdfMemDocument(shared_ptr<InputStreamDevice> device, const string_view& password)
    : PdfMemDocument(true)
{
    if (device == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    loadFromDevice(std::move(device), password);
}

PdfMemDocument::PdfMemDocument(const PdfMemDocument& rhs) :
    PdfDocument(rhs),
    m_Version(rhs.m_Version),
    m_InitialVersion(rhs.m_InitialVersion),
    m_HasXRefStream(rhs.m_HasXRefStream),
    m_PrevXRefOffset(rhs.m_PrevXRefOffset)
{
    // Do a full copy of the encrypt session
    if (rhs.m_Encrypt != nullptr)
        m_Encrypt.reset(new PdfEncryptSession(rhs.m_Encrypt->GetEncrypt(), rhs.m_Encrypt->GetContext()));
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
    m_PrevXRefOffset = -1;
}

void PdfMemDocument::initFromParser(PdfParser& parser)
{
    m_Version = parser.GetPdfVersion();
    m_InitialVersion = m_Version;
    m_HasXRefStream = parser.HasXRefStream();
    m_PrevXRefOffset = parser.GetXRefOffset();
    this->SetTrailer(parser.TakeTrailer());

    if (PdfCommon::IsLoggingSeverityEnabled(PdfLogSeverity::Debug))
    {
        auto debug = GetTrailer().GetObject().ToString();
        debug.push_back('\n');
        PoDoFo::LogMessage(PdfLogSeverity::Debug, debug);
    }

    auto encrypt = parser.GetEncrypt();
    if (encrypt != nullptr)
        m_Encrypt.reset(new PdfEncryptSession(*encrypt));

    Init();
}

void PdfMemDocument::Load(const string_view& filename, const string_view& password)
{
    if (filename.length() == 0)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    auto device = std::make_shared<FileStreamDevice>(filename);
    Load(device, password);
}

void PdfMemDocument::LoadFromBuffer(const bufferview& buffer, const string_view& password)
{
    if (buffer.size() == 0)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    auto device = std::make_shared<SpanStreamDevice>(buffer);
    Load(device, password);
}

void PdfMemDocument::Load(shared_ptr<InputStreamDevice> device, const string_view& password)
{
    if (device == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    this->Clear();
    loadFromDevice(std::move(device), password);
}

void PdfMemDocument::loadFromDevice(shared_ptr<InputStreamDevice>&& device, const string_view& password)
{
    m_device = std::move(device);

    // Call parse file instead of using the constructor
    // so that m_Parser is initialized for encrypted documents
    PdfParser parser(PdfDocument::GetObjects());
    parser.SetPassword(password);
    parser.Parse(*m_device, true);
    initFromParser(parser);
}

void PdfMemDocument::Save(const string_view& filename, PdfSaveOptions options)
{
    FileStreamDevice device(filename, FileMode::Create);
    this->Save(device, options);
}

void PdfMemDocument::Save(OutputStreamDevice& device, PdfSaveOptions opts)
{
    beforeWrite(opts);

    PdfWriter writer(this->GetObjects(), this->GetTrailer().GetObject());
    writer.SetPdfVersion(GetMetadata().GetPdfVersion());
    writer.SetPdfALevel(GetMetadata().GetPdfALevel());
    writer.SetSaveOptions(opts);

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
}

void PdfMemDocument::SaveUpdate(const string_view& filename, PdfSaveOptions opts)
{
    FileStreamDevice device(filename, FileMode::Append);
    this->SaveUpdate(device, opts);
}

void PdfMemDocument::SaveUpdate(OutputStreamDevice& device, PdfSaveOptions opts)
{
    beforeWrite(opts);

    PdfWriter writer(this->GetObjects(), this->GetTrailer().GetObject());
    writer.SetPdfVersion(GetMetadata().GetPdfVersion());
    writer.SetPdfALevel(GetMetadata().GetPdfALevel());
    writer.SetSaveOptions(opts);
    writer.SetPrevXRefOffset(m_PrevXRefOffset);
    writer.SetUseXRefStream(m_HasXRefStream);
    writer.SetIncrementalUpdate(false);

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
}

void PdfMemDocument::beforeWrite(PdfSaveOptions opts)
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
        m_Encrypt = nullptr;
    else
        m_Encrypt.reset(new PdfEncryptSession(std::move(encrypt)));
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

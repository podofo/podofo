/**
 * SPDX-FileCopyrightText: (C) 2023 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfSigningContext.h"
#include <podofo/auxiliary/StreamDevice.h>

using namespace std;
using namespace PoDoFo;

constexpr const char* ByteRangeBeacon = "[ 0 1234567890 1234567890 1234567890]";
constexpr size_t BufferSize = 65536;

static PdfSignature& getSignature(PdfDocument& doc, int pageIndex, const PdfReference& signatureRef);
static size_t readForSignature(StreamDevice& device,
    size_t conentsBeaconOffset, size_t conentsBeaconSize,
    char* buffer, size_t size);
static void adjustByteRange(StreamDevice& device, size_t byteRangeOffset,
    size_t conentsBeaconOffset, size_t conentsBeaconSize, PdfArray& byteRangeArr, charbuff& buffer);
static void setSignature(StreamDevice& device, const string_view& sigData,
    size_t conentsBeaconOffset, charbuff& buffer);
static void prepareBeaconsData(size_t signatureSize, string& contentsBeacon, string& byteRangeBeacon);

PdfSigningContext::PdfSigningContext()
    : m_doc(nullptr)
{
}

PdfSignerId PdfSigningContext::AddSigner(const PdfSignature& signature, shared_ptr<PdfSigner> signer)
{
    ensureNotStarted();
    if (m_signers.size() != 0)
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Signing multiple signature fields "
            "or signing the same field with multiple signers is currently not implemented");
    }

    return addSigner(signature, signer.get(), std::move(signer));
}

void PdfSigningContext::AddSignerUnsafe(const PdfSignature& signature, PdfSigner& signer)
{
    (void)addSigner(signature, &signer, nullptr);
}

void PdfSigningContext::StartSigning(PdfMemDocument& doc, shared_ptr<StreamDevice> device,
    PdfSigningResults& results, PdfSaveOptions saveOptions)
{
    if (device == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The output device must be not null");

    ensureNotStarted();
    if (m_signers.size() == 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "No signers were configured");

    m_doc = &doc;
    m_device = std::move(device);

    charbuff tmpbuff;
    m_contexts = prepareSignatureContexts(doc, true);
    saveDocForSigning(doc, *m_device, saveOptions);
    appendDataForSigning(m_contexts, *m_device, &results.Intermediate, tmpbuff);
}

void PdfSigningContext::FinishSigning(const PdfSigningResults& processedResults)
{
    if (m_doc == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "A deferred signing has not been started");

    charbuff tmpbuff;
    computeSignatures(m_contexts, *m_doc, *m_device, &processedResults, tmpbuff);

    m_doc = nullptr;
    m_device = nullptr;
    m_contexts.clear();
}

void PdfSigningContext::Sign(PdfMemDocument& doc, StreamDevice& device, PdfSaveOptions saveOptions)
{
    ensureNotStarted();
    if (m_signers.size() == 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "No signers were configured");

    charbuff tmpbuff;

    auto contexts = prepareSignatureContexts(doc, false);
    saveDocForSigning(doc, device, saveOptions);
    appendDataForSigning(contexts, device, nullptr, tmpbuff);
    computeSignatures(contexts, doc, device, nullptr, tmpbuff);
}

PdfSignerId PdfSigningContext::addSigner(const PdfSignature& signature, PdfSigner* signer,
    shared_ptr<PdfSigner>&& storage)
{
    auto reference = signature.GetObject().GetIndirectReference();
    auto found = m_signers.find(reference);
    SignatureAttrs* attrs;
    if (found == m_signers.end())
    {
        auto widget = signature.GetWidget();
        int pageIndex;
        if (widget == nullptr)
            pageIndex = -1;
        else
            pageIndex = (int)widget->MustGetPage().GetIndex();

        attrs = &m_signers[reference];
        attrs->PageIndex = pageIndex;
    }
    else
    {
        attrs = &found->second;
    }

    auto signedIdx = (unsigned)attrs->Signers.size();
    attrs->Signers.push_back(signer);
    attrs->SignersStorage.push_back(std::move(storage));
    return PdfSignerId(reference, signedIdx);
}

void PdfSigningContext::ensureNotStarted() const
{
    if (m_doc != nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "A deferred signing has already been started");
}

// Prepare signature contexts, running dry-run signature computation
unordered_map<PdfSignerId, PdfSigningContext::SignatureCtx> PdfSigningContext::prepareSignatureContexts(
    PdfDocument& doc, bool deferredSigning)
{
    unordered_map<PdfSignerId, SignatureCtx> ret;
    for (auto& pair : m_signers)
    {
        auto& attrs = pair.second;
        auto& signature = getSignature(doc, attrs.PageIndex, pair.first);
        for (unsigned i = 0; i < attrs.Signers.size(); i++)
        {
            auto& ctx = ret[PdfSignerId(pair.first, i)];
            auto& signer = attrs.Signers[i];
            signer->Reset();
            if (deferredSigning)
                signer->ComputeSignatureDeferred({ }, ctx.Contents, true);
            else
                signer->ComputeSignature(ctx.Contents, true);
            ctx.BeaconSize = ctx.Contents.size();
            prepareBeaconsData(ctx.BeaconSize, ctx.Beacons.ContentsBeacon, ctx.Beacons.ByteRangeBeacon);
            signature.PrepareForSigning(signer->GetSignatureFilter(), signer->GetSignatureSubFilter(),
                signer->GetSignatureType(), ctx.Beacons);
        }
    }
    return ret;
}

void PdfSigningContext::saveDocForSigning(PdfMemDocument& doc, StreamDevice& device, PdfSaveOptions saveOptions)
{
    auto& form = doc.GetOrCreateAcroForm();
    auto sigFlags = form.GetSigFlags();
    if ((sigFlags & (PdfAcroFormSigFlags::SignaturesExist | PdfAcroFormSigFlags::AppendOnly))
        != (PdfAcroFormSigFlags::SignaturesExist | PdfAcroFormSigFlags::AppendOnly))
    {
        // TABLE 8.68 Signature flags: SignaturesExist (1) | AppendOnly (2)
        form.SetSigFlags(sigFlags | PdfAcroFormSigFlags::SignaturesExist | PdfAcroFormSigFlags::AppendOnly);
    }

    auto acroForm = doc.GetAcroForm();
    if (acroForm != nullptr)
    {
        // NOTE: Adobe is crazy and if the /NeedAppearances is set to true,
        // it will not show up the signature upon signing. Just
        // remove the key just in case it's present (defaults to false)
        acroForm->GetDictionary().RemoveKey("NeedAppearances");
    }

    if ((saveOptions & PdfSaveOptions::SaveOnSigning) != PdfSaveOptions::None)
        doc.Save(device, saveOptions);
    else
        doc.SaveUpdate(device, saveOptions);

    device.Flush();
}

void PdfSigningContext::appendDataForSigning(unordered_map<PdfSignerId, SignatureCtx>& contexts, StreamDevice& device,
    std::unordered_map<PdfSignerId, charbuff>* intermediateResults, charbuff& tmpbuff)
{
    for (auto& pair : m_signers)
    {
        auto& attrs = pair.second;
        for (unsigned i = 0; i < attrs.Signers.size(); i++)
        {
            PdfSignerId signerId(pair.first, i);
            auto& signer = attrs.Signers[i];
            auto& ctx = contexts[signerId];

            adjustByteRange(device, *ctx.Beacons.ByteRangeOffset, *ctx.Beacons.ContentsOffset,
                ctx.Beacons.ContentsBeacon.size(), ctx.ByteRangeArr, tmpbuff);
            device.Flush();

            // Read data from the device to prepare the signature
            signer->Reset();
            device.Seek(0);
            size_t readBytes;
            tmpbuff.resize(BufferSize);
            while ((readBytes = readForSignature(device, *ctx.Beacons.ContentsOffset, ctx.Beacons.ContentsBeacon.size(),
                tmpbuff.data(), BufferSize)) != 0)
            {
                signer->AppendData({ tmpbuff.data(), readBytes });
            }

            if (intermediateResults != nullptr)
                signer->FetchIntermediateResult((*intermediateResults)[signerId]);
        }
    }
}

void PdfSigningContext::computeSignatures(unordered_map<PdfSignerId, SignatureCtx>& contexts,
    PdfDocument& doc, StreamDevice& device,
    const PdfSigningResults* processedResults, charbuff& tmpbuff)
{
    for (auto& pair : m_signers)
    {
        auto& attrs = pair.second;
        auto& signature = getSignature(doc, attrs.PageIndex, pair.first);
        for (unsigned i = 0; i < attrs.Signers.size(); i++)
        {
            PdfSignerId signerId(pair.first, i);
            auto& signer = attrs.Signers[i];
            auto& ctx = contexts[signerId];

            if (!signer->SkipBufferClear())
                ctx.Contents.clear();

            if (processedResults == nullptr)
                signer->ComputeSignature(ctx.Contents, false);
            else
                signer->ComputeSignatureDeferred(processedResults->Intermediate.at(signerId), ctx.Contents, false);

            if (ctx.Contents.size() > ctx.BeaconSize)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Actual signature size bigger than beacon size");

            // Ensure the signature will be as big as the
            // beacon size previously cached to fill all
            // available reserved space for the /Contents
            ctx.Contents.resize(ctx.BeaconSize);
            setSignature(device, ctx.Contents, *ctx.Beacons.ContentsOffset, tmpbuff);
            device.Flush();

            // Finally set actual /ByteRange on the signature without dirty set
            signature.SetContentsByteRangeNoDirtySet(ctx.Contents, std::move(ctx.ByteRangeArr));
        }
    }
}

size_t readForSignature(StreamDevice& device, size_t conentsBeaconOffset, size_t conentsBeaconSize,
    char* buffer, size_t bufferSize)
{
    if (device.Eof())
        return 0;

    size_t pos = device.GetPosition();
    size_t readSize = 0;
    // Check if we are before beacon
    if (pos < conentsBeaconOffset)
    {
        readSize = std::min(bufferSize, conentsBeaconOffset - pos);
        if (readSize > 0)
        {
            device.Read(buffer, readSize);
            buffer += readSize;
            bufferSize -= readSize;
            if (bufferSize == 0)
                return readSize;
        }
    }

    // shift at the end of beacon
    if ((pos + readSize) >= conentsBeaconOffset
        && pos < (conentsBeaconOffset + conentsBeaconSize))
    {
        device.Seek(conentsBeaconOffset + conentsBeaconSize);
    }

    // read after beacon
    bufferSize = std::min(bufferSize, device.GetLength() - device.GetPosition());
    if (bufferSize == 0)
        return readSize;

    device.Read(buffer, bufferSize);
    return readSize + bufferSize;
}

void adjustByteRange(StreamDevice& device, size_t byteRangeOffset,
    size_t conentsBeaconOffset, size_t conentsBeaconSize, PdfArray& byteRangeArr, charbuff& buffer)
{
    // Get final position
    size_t fileEnd = device.GetLength();
    byteRangeArr.Add(PdfObject(static_cast<int64_t>(0)));
    byteRangeArr.Add(PdfObject(static_cast<int64_t>(conentsBeaconOffset)));
    byteRangeArr.Add(PdfObject(static_cast<int64_t>(conentsBeaconOffset + conentsBeaconSize)));
    byteRangeArr.Add(PdfObject(static_cast<int64_t>(fileEnd - (conentsBeaconOffset + conentsBeaconSize))));

    device.Seek(byteRangeOffset);
    byteRangeArr.Write(device, PdfWriteFlags::None, { }, buffer);
}

void setSignature(StreamDevice& device, const string_view& contentsData,
    size_t conentsBeaconOffset, charbuff& buffer)
{
    auto sig = PdfString::FromRaw(contentsData);

    // Position at contents beacon after '<'
    device.Seek(conentsBeaconOffset);
    // Write the beacon data
    sig.Write(device, PdfWriteFlags::None, { }, buffer);
}

void prepareBeaconsData(size_t signatureSize, string& contentsBeacon, string& byteRangeBeacon)
{
    // Just prepare strings with spaces, for easy writing later
    contentsBeacon.resize((signatureSize * 2) + 2, ' '); // Signature bytes will be encoded
    // as an hex string
    byteRangeBeacon.resize(char_traits<char>::length(ByteRangeBeacon), ' ');
}

PdfSignerId::PdfSignerId()
    : m_SignerIndex(0) { }

PdfSignerId::PdfSignerId(const PdfReference& ref, unsigned signerIndex)
    : m_SignatureRef(ref), m_SignerIndex(signerIndex) { }

PdfSignature& getSignature(PdfDocument& doc, int pageIndex, const PdfReference& signatureRef)
{
    if (pageIndex >= 0)
    {
        auto& page = doc.GetPages().GetPageAt((unsigned)pageIndex);
        return static_cast<PdfSignature&>(static_cast<PdfAnnotationWidget&>(page.GetAnnotations().GetAnnot(signatureRef)).GetField());
    }
    else
    {
        return  static_cast<PdfSignature&>(doc.MustGetAcroForm().GetField(signatureRef));
    }
}

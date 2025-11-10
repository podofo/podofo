/**
 * SPDX-FileCopyrightText: (C) 2023 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfSigningContext.h"
#include <podofo/auxiliary/StreamDevice.h>
#include <podofo/private/XmlUtils.h>
#include "PdfSignerCms.h"

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

constexpr uint16_t DumpFooterMagic = 0x5343; // "SC" -> Signing Context

namespace
{
    /// <summary>
    /// A footer that identifies the a signing context dump
    /// </summary>
    struct SigningContextDumpFooter
    {
        uint16_t Magic = DumpFooterMagic;
        uint8_t Version = 1;
        uint8_t _Unused = 0;
        uint32_t XMLFragmentSize = 0;
    };
}

PdfSigningContext::PdfSigningContext()
    : m_doc(nullptr), m_status(Status::Config)
{
}

unique_ptr<PdfMemDocument> PdfSigningContext::Restore(shared_ptr<StreamDevice> device)
{
    if (m_status != Status::Config)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedOperation, "Restore a deferred context is not allowed");

    void InitXml();

    device->Seek(-(ssize_t)sizeof(SigningContextDumpFooter), SeekDirection::End);
    SigningContextDumpFooter footer;
    device->Read((char*)&footer, sizeof(SigningContextDumpFooter));
    footer.Magic = AS_BIG_ENDIAN(footer.Magic);
    footer.XMLFragmentSize = AS_BIG_ENDIAN(footer.XMLFragmentSize);

    if (footer.Magic != DumpFooterMagic)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidInput, "Invalid PdfSigningContext footer");

    device->Seek(-(ssize_t)(sizeof(SigningContextDumpFooter) + footer.XMLFragmentSize), SeekDirection::End);

    charbuff temp(footer.XMLFragmentSize);
    device->Read(temp.data(), footer.XMLFragmentSize);

    unique_ptr<xmlDoc, decltype(&xmlFreeDoc)> doc(xmlReadMemory(temp.data(), (int)temp.size(), nullptr, nullptr, XML_PARSE_NOBLANKS), xmlFreeDoc);
    xmlNodePtr sigCtxElem;
    if (doc == nullptr
        || (sigCtxElem = xmlDocGetRootElement(doc.get())) == nullptr)
    {
    DeserializationFailed:
        THROW_LIBXML_EXCEPTION("PdfSigningContext deserialization failed");
    }

    m_signatures.clear();
    m_contexts.clear();

    PdfLoadOptions loadOptions = PdfLoadOptions::None;
    auto node = utls::FindChildElement(sigCtxElem, "PdfLoadOptions");
    unsigned num1;
    int num2;
    int64_t num3;
    if (node == nullptr || node->children == nullptr || node->children->content == nullptr
            || !utls::TryParse((const char*)node->children->content, num1))
        goto DeserializationFailed;

    loadOptions = (PdfLoadOptions)num1;

    node = utls::FindChildElement(sigCtxElem, "Signatures");
    if (node == nullptr)
        goto DeserializationFailed;

    for (auto child = node->children; child != nullptr; child = child->next)
    {
        auto key = utls::FindChildElement(child, "Key");
        if (key == nullptr)
            goto DeserializationFailed;

        node = utls::FindChildElement(key, "ObjNum");
        if (node == nullptr || node->children == nullptr || node->children->content == nullptr
                || !utls::TryParse((const char*)node->children->content, num1))
            goto DeserializationFailed;

        node = utls::FindChildElement(key, "GenNum");
        if (node == nullptr || node->children == nullptr || node->children->content == nullptr
                || !utls::TryParse((const char*)node->children->content, num2))
            goto DeserializationFailed;

        PdfReference ref(num1, (uint16_t)num2);

        auto& descs = m_signatures[ref];
        auto value = utls::FindChildElement(child, "Value");
        if (value == nullptr)
            goto DeserializationFailed;

        node = utls::FindChildElement(value, "FullName");
        if (node == nullptr || node->children == nullptr || node->children->content == nullptr)
            goto DeserializationFailed;
        descs.FullName = (const char*)node->children->content;

        node = utls::FindChildElement(value, "PageIndex");
        if (node == nullptr || node->children == nullptr || node->children->content == nullptr
                || !utls::TryParse((const char*)node->children->content, num2))
            goto DeserializationFailed;
        descs.PageIndex = num2;

        node = utls::FindChildElement(value, "Signer");
        // TODO: Check Type="PdfSignerCMS"
        if (node == nullptr)
            goto DeserializationFailed;

        descs.SignerStorage.reset(new PdfSignerCms());
        static_cast<PdfSignerCms&>(*descs.SignerStorage).Restore(node, temp);
        descs.Signer = descs.SignerStorage.get();
    }

    node = utls::FindChildElement(sigCtxElem, "Contexts");
    if (node == nullptr)
        goto DeserializationFailed;

    for (auto child = node->children; child != nullptr; child = child->next)
    {
        auto key = utls::FindChildElement(child, "Key");
        if (key == nullptr)
            goto DeserializationFailed;

        node = utls::FindChildElement(key, "ObjNum");
        if (node == nullptr || node->children == nullptr || node->children->content == nullptr
                || !utls::TryParse((const char*)node->children->content, num1))
            goto DeserializationFailed;

        node = utls::FindChildElement(key, "GenNum");
        if (node == nullptr || node->children == nullptr || node->children->content == nullptr
                || !utls::TryParse((const char*)node->children->content, num2))
            goto DeserializationFailed;

        PdfReference ref(num1, (uint16_t)num2);

        auto& ctx = m_contexts[ref];

        auto value = utls::FindChildElement(child, "Value");
        if (value == nullptr)
            goto DeserializationFailed;

        node = utls::FindChildElement(value, "BeaconSize");
        if (node == nullptr || node->children == nullptr || node->children->content == nullptr
                || !utls::TryParse((const char*)node->children->content, num1))
            goto DeserializationFailed;
        ctx.BeaconSize = num1;

        auto byteRangeArrElem = utls::FindChildElement(value, "ByteRangeArr");
        if (node == nullptr)
            goto DeserializationFailed;

        node = utls::FindChildElement(byteRangeArrElem, "Range1Offset");
        if (node == nullptr || node->children == nullptr || node->children->content == nullptr
                || !utls::TryParse((const char*)node->children->content, num3))
            goto DeserializationFailed;
        ctx.ByteRangeArr.Add(num3);

        node = utls::FindChildElement(byteRangeArrElem, "Range1Length");
        if (node == nullptr || node->children == nullptr || node->children->content == nullptr
                || !utls::TryParse((const char*)node->children->content, num3))
            goto DeserializationFailed;
        ctx.ByteRangeArr.Add(num3);

        node = utls::FindChildElement(byteRangeArrElem, "Range2Offset");
        if (node == nullptr || node->children == nullptr || node->children->content == nullptr
                || !utls::TryParse((const char*)node->children->content, num3))
            goto DeserializationFailed;
        ctx.ByteRangeArr.Add(num3);

        node = utls::FindChildElement(byteRangeArrElem, "Range2Length");
        if (node == nullptr || node->children == nullptr || node->children->content == nullptr
                || !utls::TryParse((const char*)node->children->content, num3))
            goto DeserializationFailed;
        ctx.ByteRangeArr.Add(num3);

        auto beaconsArrElem = utls::FindChildElement(value, "Beacons");
        if (node == nullptr)
            goto DeserializationFailed;

        node = utls::FindChildElement(beaconsArrElem, "ContentsOffset");
        if (node == nullptr || node->children == nullptr || node->children->content == nullptr
                || !utls::TryParse((const char*)node->children->content, num3))
            goto DeserializationFailed;
        *ctx.Beacons.ContentsOffset = (size_t)num3;

        node = utls::FindChildElement(beaconsArrElem, "ByteRangeOffset");
        if (node == nullptr || node->children == nullptr || node->children->content == nullptr
                || !utls::TryParse((const char*)node->children->content, num3))
            goto DeserializationFailed;
        *ctx.Beacons.ByteRangeOffset = (size_t)num3;
    }

    // Truncate the stream just before the XML fragment
    device->Seek(-(ssize_t)(sizeof(SigningContextDumpFooter) + footer.XMLFragmentSize), SeekDirection::End);
    device->Truncate();

    unique_ptr<PdfMemDocument> ret(new PdfMemDocument());
    ret->Load(device, loadOptions);
    m_doc = ret.get();
    m_device = device;
    m_status = Status::Restored;
    return ret;
}

PdfSignerId PdfSigningContext::AddSigner(const PdfSignature& signature, shared_ptr<PdfSigner> signer)
{
    ensureNotStarted();
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
    if (m_signatures.size() == 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "No signers were configured");

    m_doc = &doc;
    m_device = std::move(device);

    charbuff tmpbuff;
    m_contexts = prepareSignatureContexts(doc, true);
    saveDocForSigning(doc, *m_device, saveOptions);
    appendDataForSigning(m_contexts, *m_device, &results.Intermediate, tmpbuff);
    m_status = Status::Started;
}

void PdfSigningContext::FinishSigning(const PdfSigningResults& processedResults)
{
    if (!(m_status == Status::Started || m_status == Status::Restored))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "A deferred signing has not been started");

    charbuff tmpbuff;
    computeSignatures(m_contexts, *m_doc, *m_device, &processedResults, tmpbuff);

    m_doc = nullptr;
    m_device = nullptr;
    m_contexts.clear();
    m_status = Status::Finished;
}

void PdfSigningContext::DumpInPlace()
{
    if (m_status != Status::Started)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedOperation, "Only a non-restored deferred context can be dumped");

    void InitXml();

    m_device->Seek(0, SeekDirection::End);

    string temp;

    unique_ptr<xmlDoc, decltype(&xmlFreeDoc)> fragment(xmlNewDoc(nullptr), xmlFreeDoc);
    auto sigCtxElem = xmlNewChild((xmlNodePtr)fragment.get(), nullptr, XMLCHAR "SigningContext", nullptr);
    if (sigCtxElem == nullptr)
    {
    SerializationFailed:
        THROW_LIBXML_EXCEPTION("PdfSigningContext serialization failed");
    }

    // TODO: Persist and obtain load options from PdfMemDocument
    if (xmlNewChild(sigCtxElem, nullptr, XMLCHAR "PdfLoadOptions", XMLCHAR "0") == nullptr)
        goto SerializationFailed;

    auto signaturesElem = xmlNewChild(sigCtxElem, nullptr, XMLCHAR "Signatures", nullptr);
    if (signaturesElem == nullptr)
        goto SerializationFailed;

    for (auto& pair : m_signatures)
    {
        auto& ref = pair.first;
        auto& descs = pair.second;
        auto signer = dynamic_cast<PdfSignerCms*>(descs.Signer);
        if (signer == nullptr)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedOperation, "Dumping context is supported only for PdfSignerCMS signers");

        auto signatureElem = xmlNewChild(signaturesElem, nullptr, XMLCHAR "Signature", nullptr);
        if (signatureElem == nullptr)
            goto SerializationFailed;

        auto keyElem = xmlNewChild(signatureElem, nullptr, XMLCHAR "Key", nullptr);
        if (signatureElem == nullptr)
            goto SerializationFailed;

        utls::FormatTo(temp, ref.ObjectNumber());
        if (xmlNewChild(keyElem, nullptr, XMLCHAR "ObjNum", XMLCHAR temp.data()) == nullptr)
            goto SerializationFailed;

        utls::FormatTo(temp, ref.GenerationNumber());
        if (xmlNewChild(keyElem, nullptr, XMLCHAR "GenNum", XMLCHAR temp.data()) == nullptr)
            goto SerializationFailed;

        auto valueElem = xmlNewChild(signatureElem, nullptr, XMLCHAR "Value", nullptr);
        if (valueElem == nullptr)
            goto SerializationFailed;

        if (xmlNewChild(valueElem, nullptr, XMLCHAR "FullName", XMLCHAR descs.FullName.data()) == nullptr)
            goto SerializationFailed;

        utls::FormatTo(temp, descs.PageIndex);
        if (xmlNewChild(valueElem, nullptr, XMLCHAR "PageIndex", XMLCHAR temp.data()) == nullptr)
            goto SerializationFailed;

        auto signerElem = xmlNewChild(valueElem, nullptr, XMLCHAR "Signer", nullptr);
        if (signerElem == nullptr)
            goto SerializationFailed;

        // NOTE: This is a hard code as the only serializable signer is PdfSignerCMS
        if (xmlSetProp(signerElem, XMLCHAR "Type", XMLCHAR "PdfSignerCMS") == nullptr)
            goto SerializationFailed;

        signer->Dump(signerElem, temp);
    }

    auto contextsElem = xmlNewChild(sigCtxElem, nullptr, XMLCHAR "Contexts", nullptr);
    for (auto& pair : m_contexts)
    {
        auto& id = pair.first;
        auto& ctx = pair.second;
        auto contextElem = xmlNewChild(contextsElem, nullptr, XMLCHAR "Context", nullptr);
        if (contextElem == nullptr)
            goto SerializationFailed;

        auto keyElem = xmlNewChild(contextElem, nullptr, XMLCHAR "Key", nullptr);
        if (keyElem == nullptr)
            goto SerializationFailed;

        utls::FormatTo(temp, id.ObjectNumber());
        if (xmlNewChild(keyElem, nullptr, XMLCHAR "ObjNum", XMLCHAR temp.data()) == nullptr)
            goto SerializationFailed;

        utls::FormatTo(temp, id.GenerationNumber());
        if (xmlNewChild(keyElem, nullptr, XMLCHAR "GenNum", XMLCHAR temp.data()) == nullptr)
            goto SerializationFailed;

        auto valueElem = xmlNewChild(contextElem, nullptr, XMLCHAR "Value", nullptr);
        if (valueElem == nullptr)
            goto SerializationFailed;

        // NOTE: Ignore SignatureCtx.Contents. This is set during signature computing

        utls::FormatTo(temp, ctx.BeaconSize);
        if (xmlNewChild(valueElem, nullptr, XMLCHAR "BeaconSize", XMLCHAR temp.data()) == nullptr)
            goto SerializationFailed;

        auto byteRangeArrElem = xmlNewChild(valueElem, nullptr, XMLCHAR "ByteRangeArr", nullptr);
        if (byteRangeArrElem == nullptr)
            goto SerializationFailed;

        utls::FormatTo(temp, ctx.ByteRangeArr[0].GetNumber());
        if (xmlNewChild(byteRangeArrElem, nullptr, XMLCHAR "Range1Offset", XMLCHAR temp.data()) == nullptr)
            goto SerializationFailed;

        utls::FormatTo(temp, ctx.ByteRangeArr[1].GetNumber());
        if (xmlNewChild(byteRangeArrElem, nullptr, XMLCHAR "Range1Length", XMLCHAR temp.data()) == nullptr)
            goto SerializationFailed;

        utls::FormatTo(temp, ctx.ByteRangeArr[2].GetNumber());
        if (xmlNewChild(byteRangeArrElem, nullptr, XMLCHAR "Range2Offset", XMLCHAR temp.data()) == nullptr)
            goto SerializationFailed;

        utls::FormatTo(temp, ctx.ByteRangeArr[3].GetNumber());
        if (xmlNewChild(byteRangeArrElem, nullptr, XMLCHAR "Range2Length", XMLCHAR temp.data()) == nullptr)
            goto SerializationFailed;

        auto beaconsElem = xmlNewChild(valueElem, nullptr, XMLCHAR "Beacons", nullptr);
        if (beaconsElem == nullptr)
            goto SerializationFailed;

        // NOTE: Ignore PdfSignatureBeacons::ContentsBeacon and PdfSignatureBeacons::ByteRangeBeacon:
        // These are used during signature context preparation and append

        utls::FormatTo(temp, *ctx.Beacons.ContentsOffset);
        if (xmlNewChild(beaconsElem, nullptr, XMLCHAR "ContentsOffset", XMLCHAR temp.data()) == nullptr)
            goto SerializationFailed;

        utls::FormatTo(temp, *ctx.Beacons.ByteRangeOffset);
        if (xmlNewChild(beaconsElem, nullptr, XMLCHAR "ByteRangeOffset", XMLCHAR temp.data()) == nullptr)
            goto SerializationFailed;
    }

    string serialized;
    if (!utls::TrySerializeXmlDocTo(serialized, fragment.get()))
        THROW_LIBXML_EXCEPTION("Can't serialize signing context");

    m_device->Write(serialized);

    // Finally write the footer
    SigningContextDumpFooter footer;
    footer.XMLFragmentSize = (uint32_t)serialized.size();
    utls::WriteUInt16BE(*m_device, footer.Magic);
    m_device->Write((char)footer.Version);
    m_device->Write(0);
    utls::WriteUInt32BE(*m_device, footer.XMLFragmentSize);
    m_device->Flush();
    m_status = Status::Dumped;
}

shared_ptr<PdfSigner> PdfSigningContext::GetSignerEntry(const PdfReference& signatureRef)
{
    return m_signatures.at(signatureRef).SignerStorage;
}

shared_ptr<PdfSigner> PdfSigningContext::GetSignerEntry(const string_view& fullName,
    PdfReference& signatureRef)
{
    for (auto& pair : m_signatures)
    {
        if (pair.second.FullName == fullName)
        {
            signatureRef = pair.first;
            return pair.second.SignerStorage;
        }
    }

    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ObjectNotFound, "Not found a signature with name\"{}\"", fullName);
}

bool PdfSigningContext::IsEmpty() const
{
    return m_signatures.empty();
}

void PdfSigningContext::Sign(PdfMemDocument& doc, StreamDevice& device, PdfSaveOptions saveOptions)
{
    ensureNotStarted();
    if (m_signatures.size() == 0)
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
    if (m_signatures.size() != 0)
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Signing multiple signature fields "
            "or signing the same field with multiple signers is currently not implemented");
    }

    auto reference = signature.GetObject().GetIndirectReference();
    auto found = m_signatures.find(reference);
    SignatureDescriptors* descs;
    if (found == m_signatures.end())
    {
        auto widget = signature.GetWidget();
        int pageIndex;
        if (widget == nullptr)
            pageIndex = -1;
        else
            pageIndex = (int)widget->MustGetPage().GetIndex();

        descs = &m_signatures[reference];
        descs->FullName = signature.GetFullName();
        descs->PageIndex = pageIndex;
    }
    else
    {
        descs = &found->second;
    }

    descs->Signer = signer;
    descs->SignerStorage = std::move(storage);
    return PdfSignerId(reference, 0);
}

void PdfSigningContext::ensureNotStarted() const
{
    if (m_status != Status::Config)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "A deferred signing has already been started");
}

// Prepare signature contexts, running dry-run signature computation
unordered_map<PdfReference, PdfSigningContext::SignatureCtx> PdfSigningContext::prepareSignatureContexts(
    PdfDocument& doc, bool deferredSigning)
{
    unordered_map<PdfReference, SignatureCtx> ret;
    for (auto& pair : m_signatures)
    {
        auto& descs = pair.second;
        auto& signature = getSignature(doc, descs.PageIndex, pair.first);
        auto& ctx = ret[pair.first];
        auto& signer = descs.Signer;
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

void PdfSigningContext::appendDataForSigning(unordered_map<PdfReference, SignatureCtx>& contexts, StreamDevice& device,
    std::unordered_map<PdfSignerId, charbuff>* intermediateResults, charbuff& tmpbuff)
{
    for (auto& pair : m_signatures)
    {
        auto& descs = pair.second;
        auto& signer = *descs.Signer;
        auto& ctx = contexts[pair.first];

        adjustByteRange(device, *ctx.Beacons.ByteRangeOffset, *ctx.Beacons.ContentsOffset,
            ctx.Beacons.ContentsBeacon.size(), ctx.ByteRangeArr, tmpbuff);
        device.Flush();

        // Read data from the device to prepare the signature
        signer.Reset();
        device.Seek(0);
        size_t readBytes;
        tmpbuff.resize(BufferSize);
        while ((readBytes = readForSignature(device, *ctx.Beacons.ContentsOffset, ctx.Beacons.ContentsBeacon.size(),
            tmpbuff.data(), BufferSize)) != 0)
        {
            signer.AppendData({ tmpbuff.data(), readBytes });
        }

        if (intermediateResults != nullptr)
        {
            signer.FetchIntermediateResult(tmpbuff);
            for (unsigned i = 0, count = signer.GetSignerIdentityCount(); i < count; i++)
                signer.UnpackIntermediateResult(tmpbuff, i, (*intermediateResults)[PdfSignerId(pair.first, i)]);
        }
    }
}

void PdfSigningContext::computeSignatures(unordered_map<PdfReference, SignatureCtx>& contexts,
    PdfDocument& doc, StreamDevice& device,
    const PdfSigningResults* processedResults, charbuff& tmpbuff)
{
    for (auto& pair : m_signatures)
    {
        auto& descs = pair.second;
        auto& signature = getSignature(doc, descs.PageIndex, pair.first);
        auto& signer = *descs.Signer;
        auto& ctx = contexts[pair.first];

        if (!signer.SkipBufferClear())
            ctx.Contents.clear();

        if (processedResults == nullptr)
        {
            signer.ComputeSignature(ctx.Contents, false);
        }
        else
        {
            for (unsigned i = 0, count = signer.GetSignerIdentityCount(); i < count; i++)
                signer.AssembleProcessedResult(processedResults->Intermediate.at((PdfSignerId(pair.first, i))), i, tmpbuff);

            signer.ComputeSignatureDeferred(tmpbuff, ctx.Contents, false);
        }

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

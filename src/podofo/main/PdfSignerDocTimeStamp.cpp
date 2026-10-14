// SPDX-FileCopyrightText: 2026 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfSignerDocTimeStamp.h"
#include <podofo/private/OpenSSLInternal.h>
#include <podofo/private/XmlUtils.h>

using namespace std;
using namespace PoDoFo;

PdfSignerDocTimeStamp::PdfSignerDocTimeStamp(const PdfSignerDocTimeStampParams& parameters) :
    m_parameters(parameters),
    m_digestCtx(nullptr)
{
}

PdfSignerDocTimeStamp::~PdfSignerDocTimeStamp()
{
    EVP_MD_CTX_free(m_digestCtx);
}

void PdfSignerDocTimeStamp::Reset()
{
    if (m_digestCtx != nullptr)
        resetDigest();

    m_hash.clear();

    // Reset also deferred timestamping if it was started
    m_deferredTimestamping = nullptr;
}

void PdfSignerDocTimeStamp::AppendData(const bufferview& data)
{
    ensureDigestInitialized();
    if (EVP_DigestUpdate(m_digestCtx, data.data(), data.size()) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "Error EVP_DigestUpdate");
}

void PdfSignerDocTimeStamp::ComputeSignature(charbuff& contents, bool dryrun)
{
    ensureEventBasedTimestamping();
    if (dryrun)
    {
        if (m_parameters.ReservedSize == 0)
        {
            charbuff fakeHash(ssl::GetEVP_Size(m_parameters.Hashing));
            m_parameters.TimeStampService(fakeHash, true, m_timeStamp);
            contents.resize(m_timeStamp.size());
        }
        else
        {
            contents.resize(m_parameters.ReservedSize);
        }
    }
    else
    {
        ensureHashComputed();
        m_parameters.TimeStampService(m_hash, false, m_timeStamp);
        setContents(m_timeStamp, contents);
    }
}

void PdfSignerDocTimeStamp::FetchIntermediateResult(charbuff& result)
{
    ensureDeferredTimestamping();
    ensureHashComputed();
    result = m_hash;
}

void PdfSignerDocTimeStamp::ComputeSignatureDeferred(const bufferview& processedResult, charbuff& contents, bool dryrun)
{
    ensureDeferredTimestamping();
    if (dryrun)
        contents.resize(m_parameters.ReservedSize == 0 ? DefaultReservedSize : m_parameters.ReservedSize);
    else
        setContents(processedResult, contents);
}

string PdfSignerDocTimeStamp::GetSignatureSubFilter() const
{
    return "ETSI.RFC3161";
}

string PdfSignerDocTimeStamp::GetSignatureType() const
{
    return "DocTimeStamp";
}

void PdfSignerDocTimeStamp::Dump(xmlNodePtr signerElem, string& temp)
{
    PODOFO_ASSERT(m_deferredTimestamping == true);

    // NOTE: The digest is already computed when fetching the intermediate
    // result, so we just need to serialize it for the verification
    utls::WriteHexStringTo(temp, m_hash);
    if (xmlNewChild(signerElem, nullptr, XMLCHAR "Hash", XMLCHAR temp.data()) == nullptr)
    {
    SerializationFailed:
        THROW_LIBXML_EXCEPTION("PdfSignerDocTimeStamp serialization failed");
    }

    auto parametersElem = xmlNewChild(signerElem, nullptr, XMLCHAR "Parameters", nullptr);
    if (parametersElem == nullptr)
        goto SerializationFailed;

    if (xmlNewChild(parametersElem, nullptr, XMLCHAR "Hashing", XMLCHAR PoDoFo::ToString(m_parameters.Hashing).data()) == nullptr)
        goto SerializationFailed;

    utls::FormatTo(temp, m_parameters.ReservedSize);
    if (xmlNewChild(parametersElem, nullptr, XMLCHAR "ReservedSize", XMLCHAR temp.data()) == nullptr)
        goto SerializationFailed;

    utls::FormatTo(temp, (uint32_t)m_parameters.Flags);
    if (xmlNewChild(parametersElem, nullptr, XMLCHAR "Flags", XMLCHAR temp.data()) == nullptr)
        goto SerializationFailed;
}

void PdfSignerDocTimeStamp::Restore(xmlNodePtr signerElem, charbuff& temp)
{
    (void)temp;
    unsigned num;

    // By design only deferred timestamping signers can be serialized
    m_deferredTimestamping = true;

    auto node = utls::FindChildElement(signerElem, "Hash");
    if (node == nullptr || node->children == nullptr || node->children->content == nullptr)
    {
    DeserializationFailed:
        THROW_LIBXML_EXCEPTION("PdfSignerDocTimeStamp deserialization failed");
    }
    utls::DecodeHexStringTo(m_hash, (const char*)node->children->content);

    auto parametersNode = utls::FindChildElement(signerElem, "Parameters");
    if (parametersNode == nullptr)
        goto DeserializationFailed;

    node = utls::FindChildElement(parametersNode, "Hashing");
    if (node == nullptr || node->children == nullptr || node->children->content == nullptr)
        goto DeserializationFailed;
    m_parameters.Hashing = PoDoFo::ConvertTo<PdfHashingAlgorithm>((const char*)node->children->content);

    node = utls::FindChildElement(parametersNode, "ReservedSize");
    if (node == nullptr || node->children == nullptr || node->children->content == nullptr
            || !utls::TryParse((const char*)node->children->content, num))
        goto DeserializationFailed;
    m_parameters.ReservedSize = num;

    node = utls::FindChildElement(parametersNode, "Flags");
    if (node == nullptr || node->children == nullptr || node->children->content == nullptr
            || !utls::TryParse((const char*)node->children->content, num))
        goto DeserializationFailed;
    m_parameters.Flags = (PdfSignerDocTimeStampFlags)num;
}

void PdfSignerDocTimeStamp::ensureEventBasedTimestamping()
{
    if (m_deferredTimestamping.has_value())
    {
        if (*m_deferredTimestamping)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "The signer is enabled for deferred timestamping");
    }
    else
    {
        if (m_parameters.TimeStampService == nullptr)
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic,
                "The signer can't perform event based timestamping without a timestamp service");
        }

        m_deferredTimestamping = false;
    }
}

void PdfSignerDocTimeStamp::ensureDeferredTimestamping()
{
    if (m_deferredTimestamping.has_value())
    {
        if (!*m_deferredTimestamping)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "The signer is not enabled for deferred timestamping");
    }
    else
    {
        m_deferredTimestamping = true;
    }
}

void PdfSignerDocTimeStamp::ensureDigestInitialized()
{
    if (m_digestCtx != nullptr)
        return;

    m_digestCtx = EVP_MD_CTX_new();
    if (m_digestCtx == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "EVP_MD_CTX_new");

    resetDigest();
}

void PdfSignerDocTimeStamp::resetDigest()
{
    if (EVP_MD_CTX_reset(m_digestCtx) != 1
            || EVP_DigestInit_ex(m_digestCtx, ssl::GetEVP_MD(m_parameters.Hashing), nullptr) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "Error EVP_DigestInit_ex");
}

void PdfSignerDocTimeStamp::ensureHashComputed()
{
    if (m_hash.size() != 0)
        return;

    ensureDigestInitialized();
    charbuff hash(ssl::GetEVP_Size(m_parameters.Hashing));
    if (EVP_DigestFinal_ex(m_digestCtx, (unsigned char*)hash.data(), nullptr) != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "Error EVP_DigestFinal_ex");

    m_hash = std::move(hash);
}

void PdfSignerDocTimeStamp::setContents(const bufferview& timeStamp, charbuff& contents)
{
    auto token = ssl::GetTimeStampToken(timeStamp);
    if (shouldVerify())
    {
        ensureHashComputed();
        PdfHashingAlgorithm imprintHashing;
        charbuff imprintHash;
        ssl::GetTimeStampImprint(token, imprintHashing, imprintHash);
        if (imprintHashing != m_parameters.Hashing || imprintHash != m_hash)
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::SignatureVerificationError,
                "The timestamp message imprint doesn't match the document digest");
        }
    }

    contents = token;
}

bool PdfSignerDocTimeStamp::shouldVerify() const
{
    return (m_parameters.Flags & PdfSignerDocTimeStampFlags::SkipVerification) == PdfSignerDocTimeStampFlags::None;
}

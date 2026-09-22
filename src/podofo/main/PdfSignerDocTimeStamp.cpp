// SPDX-FileCopyrightText: 2026 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfSignerDocTimeStamp.h"
#include <podofo/private/OpenSSLInternal.h>
#include <openssl/ts.h>
#include <podofo/private/XmlUtils.h>

using namespace std;
using namespace PoDoFo;

static bufferview getTimeStampToken(const bufferview& timeStamp);
static void verifyMessageImprint(const bufferview& token, const bufferview& hash, PdfHashingAlgorithm hashing);

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
    auto token = getTimeStampToken(timeStamp);
    if (shouldVerify())
    {
        ensureHashComputed();
        verifyMessageImprint(token, m_hash, m_parameters.Hashing);
    }

    contents = token;
}

bool PdfSignerDocTimeStamp::shouldVerify() const
{
    return (m_parameters.Flags & PdfSignerDocTimeStampFlags::SkipVerification) == PdfSignerDocTimeStampFlags::None;
}

// Get the TimeStampToken from the input, being either a RFC 3161 TimeStampResp with granted status
// or the token itself. Trailing bytes are ignored and any other input throws. The token is sliced
// from the input, as re-encoding it may break the token signature
bufferview getTimeStampToken(const bufferview& timeStamp)
{
    // A TimeStampResp has the following grammar:
    // 
    // TimeStampResp ::= SEQUENCE {
    //     status PKIStatusInfo,
    //     timeStampToken TimeStampToken OPTIONAL }
    // 
    // a TimeStampToken is also a SEQUENCE:
    // 
    // TimeStampToken ::= ContentInfo ::= SEQUENCE {
    //     contentType OBJECT IDENTIFIER,
    //     content [0] EXPLICIT ANY }
    auto begin = (const unsigned char*)timeStamp.data();
    auto curr = begin;
    long length;
    int tag;
    int xclass;
    // Check if the input is sequence-like and it's not custom
    if (ASN1_get_object(&curr, &length, &tag, &xclass, (long)timeStamp.size()) != V_ASN1_CONSTRUCTED
        || tag != V_ASN1_SEQUENCE || xclass != V_ASN1_UNIVERSAL)
    {
    InvalidTimeStamp:
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidInput, "Invalid RFC 3161 timestamp response or token");
    }

    auto end = curr + length;
    int ret = ASN1_get_object(&curr, &length, &tag, &xclass, (long)(end - curr));
    // Check if the return flags has the error 0x80 bit set, or the tag is custom
    if ((ret & 0x80) != 0 || xclass != V_ASN1_UNIVERSAL)
        goto InvalidTimeStamp;

    if (tag == V_ASN1_OBJECT)
    {
        // It's an object: assume the response is already a token and return it as is
        return bufferview((const char*)begin, (size_t)(end - begin));
    }

    // It's not an object: assume it's a TimeStampResp
    if (ret != V_ASN1_CONSTRUCTED || tag != V_ASN1_SEQUENCE)
        goto InvalidTimeStamp;

    // Try to decode the PKIStatusInfo:
    // 
    // PKIStatusInfo ::= SEQUENCE {
    //     status PKIStatus,
    //     statusString PKIFreeText OPTIONAL,
    //     failInfo PKIFailureInfo OPTIONAL }
    auto statusInfoEnd = curr + length;
    unique_ptr<ASN1_INTEGER, decltype(&ASN1_INTEGER_free)> status(
        d2i_ASN1_INTEGER(nullptr, &curr, (long)(statusInfoEnd - curr)), ASN1_INTEGER_free);
    if (status == nullptr)
        goto InvalidTimeStamp;

    // Read the status value, which must be either granted (0) or grantedWithMods (1):
    // 
    // PKIStatus ::= INTEGER { granted (0), grantedWithMods (1), ... }
    long statusValue = ASN1_INTEGER_get(status.get());
    if (statusValue != 0 && statusValue != 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidInput, "The timestamp response status {} is not granted", statusValue);

    if (statusInfoEnd == end)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidInput, "The timestamp response has no token");

    // Return the remaining bytes as the token
    return bufferview((const char*)statusInfoEnd, (size_t)(end - statusInfoEnd));
}

void verifyMessageImprint(const bufferview& token, const bufferview& hash, PdfHashingAlgorithm hashing)
{
    auto curr = (const unsigned char*)token.data();
    unique_ptr<CMS_ContentInfo, decltype(&CMS_ContentInfo_free)> cms(
        d2i_CMS_ContentInfo(nullptr, &curr, (long)token.size()), CMS_ContentInfo_free);
    ASN1_OCTET_STRING** content;
    if (cms == nullptr
        || OBJ_obj2nid(CMS_get0_eContentType(cms.get())) != NID_id_smime_ct_TSTInfo
        || (content = CMS_get0_content(cms.get())) == nullptr || *content == nullptr)
    {
        // The token is not a valid CMS ContentInfo or the content is not TSTInfo
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidInput, "Invalid RFC 3161 timestamp token");
    }

    // Parse the TSTInfo from the content
    curr = ASN1_STRING_get0_data(*content);
    unique_ptr<TS_TST_INFO, decltype(&TS_TST_INFO_free)> tstInfo(
        d2i_TS_TST_INFO(nullptr, &curr, ASN1_STRING_length(*content)), TS_TST_INFO_free);
    if (tstInfo == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidInput, "Invalid RFC 3161 timestamp token info");

    // Fetch the message imprint from the TSTInfo and verify it against the document digest
    auto imprint = TS_TST_INFO_get_msg_imprint(tstInfo.get());
    const ASN1_OBJECT* algorithm;
    X509_ALGOR_get0(&algorithm, nullptr, nullptr, TS_MSG_IMPRINT_get_algo(imprint));
    auto imprintHash = TS_MSG_IMPRINT_get_msg(imprint);
    if (OBJ_obj2nid(algorithm) != EVP_MD_get_type(ssl::GetEVP_MD(hashing))
        || (size_t)ASN1_STRING_length(imprintHash) != hash.size()
        || std::memcmp(ASN1_STRING_get0_data(imprintHash), hash.data(), hash.size()) != 0)
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::SignatureVerificationError,
            "The timestamp message imprint doesn't match the document digest");
    }
}

/**
 * SPDX-FileCopyrightText: (C) 2023 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "CmsContext.h"

#include <date/date.h>

#include <podofo/private/OpenSSLInternal.h>

using namespace std;
using namespace PoDoFo;

// The following flags allow for streaming and editing of the attributes
#define CMS_FLAGS CMS_DETACHED | CMS_BINARY | CMS_PARTIAL | CMS_STREAM

static void addAttribute(CMS_SignerInfo* si, int(*addAttributeFun)(CMS_SignerInfo*, const char*, int, const void*, int),
    const string_view& nid, const bufferview& attr, bool octet);

CmsContext::CmsContext() :
    m_status(CmsContextStatus::Uninitialized),
    m_encryption(PdfSignatureEncryption::Unknown),
    m_cert(nullptr),
    m_cms(nullptr),
    m_signer(nullptr),
    m_databio(nullptr),
    m_out(nullptr)
{
}

void CmsContext::Reset(const bufferview& cert, const CmsContextParams& parameters)
{
    clear();

    m_parameters = parameters,

    loadX509Certificate(cert);
    computeCertificateHash();

    reset();
    m_status = CmsContextStatus::Initialized;
}

CmsContext::~CmsContext()
{
    clear();
}

void CmsContext::AppendData(const bufferview& data)
{
    checkAppendStarted();

    if (m_out != nullptr)
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle,
            "The signer must be reset before appening new data");
    }

    auto mem = BIO_new_mem_buf(data.data(), (int)data.size());
    if (mem == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "BIO_new_mem_buf");

    // Append data to the internal CMS buffer and elaborate
    // See also CMS_final implementation for reference
    if (!SMIME_crlf_copy(mem, m_databio, CMS_FLAGS))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "SMIME_crlf_copy");

    (void)BIO_flush(m_databio);
    BIO_free(mem);
}

void CmsContext::ComputeHashToSign(charbuff& hashToSign)
{
    checkAppendStarted();
    if (!m_parameters.SkipWriteSigningTime)
    {
        date::sys_seconds seconds;
        if (m_parameters.SigningTimeUTC.has_value())
            seconds = (date::sys_seconds)*m_parameters.SigningTimeUTC;
        else
            seconds = chrono::time_point_cast<chrono::seconds>(chrono::system_clock::now());

        ssl::cmsAddSigningTime(m_signer, seconds);
    }

    // Sign with external encryption
    // NOTE: Using openssl code would be CMS_dataFinal(m_cms, m_databio),
    // but we can't do that since in OpenSSL 1.1 there's not truly
    // easy way to plug an external encrypion, so we just ripped much
    // OpenSSL code to accomplish the task
    ssl::ComputeHashToSign(m_signer, m_databio, m_parameters.DoWrapDigest, hashToSign);
    m_status = CmsContextStatus::ComputedHash;
}

void CmsContext::ComputeSignature(const bufferview& signedHash, charbuff& signature)
{
    if (m_status != CmsContextStatus::ComputedHash)
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic,
            "The signature can't be computed at this moment");
    }

    auto buf = (unsigned char*)OPENSSL_malloc(signedHash.size());
    if (buf == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "Error while setting encrypted hash");

    std::memcpy(buf, signedHash.data(), signedHash.size());
    auto signatuerAsn1 = CMS_SignerInfo_get0_signature(m_signer);

    // Directly set the signature memory in the SignerInfo
    ASN1_STRING_set0(signatuerAsn1, buf, (int)signedHash.size());

    m_out = BIO_new(BIO_s_mem());
    if (m_out == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "BIO_new");

    // Output CMS as DER format
    i2d_CMS_bio(m_out, m_cms);

    char* signatureData;
    size_t length = (size_t)BIO_get_mem_data(m_out, &signatureData);
    signature.assign(signatureData, signatureData + length);
    m_status = CmsContextStatus::ComputedSignature;
}

void CmsContext::AddAttribute(const string_view& nid, const bufferview& attr, bool signedAttr, bool asOctetString)
{
    if (signedAttr)
    {
        checkEnabledAddSignedAttributes();
        addAttribute(m_signer, CMS_signed_add1_attr_by_txt, nid, attr, asOctetString);
    }
    else
    {
        checkEnabledAddUnsignedAttributes();
        addAttribute(m_signer, CMS_unsigned_add1_attr_by_txt, nid, attr, asOctetString);
    }
}

void CmsContext::loadX509Certificate(const bufferview& cert)
{
    auto in = (const unsigned char*)cert.data();
    m_cert = d2i_X509(nullptr, &in, (int)cert.size());
    if (m_cert == nullptr)
    {
        unique_ptr<BIO, decltype(&BIO_free)> bio(BIO_new_mem_buf(cert.data(), (int)cert.size()), BIO_free);
        if (bio == nullptr)
            goto Fail;

        m_cert = PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr);
        if (m_cert == nullptr)
        {
        Fail:
            string err("Certificate loading failed. Internal OpenSSL error:\n");
            ssl::GetOpenSSLError(err);
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, err);
        }
    }

    auto pubkey = X509_get0_pubkey(m_cert);
    if (pubkey == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "Invalid public key");

    m_encryption = PdfSignatureEncryption::Unknown;
    if (EVP_PKEY_base_id(pubkey) == EVP_PKEY_RSA)
        m_encryption = PdfSignatureEncryption::RSA;
    else if (EVP_PKEY_base_id(pubkey) == EVP_PKEY_EC)
        m_encryption = PdfSignatureEncryption::ECDSA;
}

void CmsContext::computeCertificateHash()
{
    int len;
    unsigned char* buf = nullptr;

    len = i2d_X509(m_cert, &buf);
    if (len < 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "i2d_X509");

    auto clean = [&]() {
        OPENSSL_free(buf);
    };

    try
    {
        m_certHash = ssl::ComputeHash({(const char*) buf, (size_t)len }, m_parameters.Hashing);
    }
    catch (...)
    {
        clean();
        throw;
    }
    clean();
}

void CmsContext::clear()
{
    if (m_cert != nullptr)
    {
        X509_free(m_cert);
        m_cert = nullptr;
    }

    if (m_cms != nullptr)
    {
        CMS_ContentInfo_free(m_cms);
        m_cms = nullptr;
    }

    if (m_databio != nullptr)
    {
        BIO_free(m_databio);
        m_databio = nullptr;
    }

    if (m_out != nullptr)
    {
        BIO_free(m_out);
        m_out = nullptr;
    }
}

void CmsContext::reset()
{
    // By default CMS_sign uses SHA1, so create a partial context with streaming enabled
    m_cms = CMS_sign(nullptr, nullptr, nullptr, nullptr, CMS_FLAGS);
    if (m_cms == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "CMS_sign");

    // Set a signer with a SHA56 digest. Since CMS_PARTIAL is *not* passed,
    // the CMS structure is sealed
    auto sign_md = ssl::GetEVP_MD(m_parameters.Hashing);

    // Fake private key using public key from certificate
    // This allows to pass internal checks of CMS_add1_signer
    // since parameter "pk" can't be nullptr
    auto fakePrivKey = X509_get0_pubkey(m_cert);

    // NOTE: CAdES signatures don't want unneeded attributes
    m_signer = CMS_add1_signer(m_cms, m_cert, fakePrivKey, sign_md,
        m_parameters.SkipWriteMIMECapabilities ? CMS_NOSMIMECAP : 0);
    if (m_signer == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "CMS_add1_signer");

    if (m_parameters.AddSigningCertificateV2)
        ssl::AddSigningCertificateV2(m_signer, m_certHash, m_parameters.Hashing);
}

void CmsContext::checkAppendStarted()
{
    switch (m_status)
    {
        case CmsContextStatus::Initialized:
        {
            // Initialize the internal cms buffer for streaming
            // See also CMS_final implementation for reference
            m_databio = CMS_dataInit(m_cms, nullptr);
            if (m_databio == nullptr)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "CMS_dataInit");

            m_status = CmsContextStatus::AppendingData;
            break;
        }
        case CmsContextStatus::AppendingData:
        {
            // Do nothing
            break;
        }
        default:
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic,
                "The cms context is not initialized or signature was already computed");
        }
    }
}

void CmsContext::checkEnabledAddSignedAttributes()
{
    if (m_status != CmsContextStatus::Initialized)
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic,
            "Signed attributes can be added only before data adding is started");
    }
}

void CmsContext::checkEnabledAddUnsignedAttributes()
{
    switch (m_status)
    {
        case CmsContextStatus::Initialized:
        case CmsContextStatus::AppendingData:
        case CmsContextStatus::ComputedHash:
            break;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic,
                "Unsigned attributes can be added only after initialization and before signature computation");
    }
}

// CHECK-ME: Untested!
void addAttribute(CMS_SignerInfo* si, int(*addAttributeFun)(CMS_SignerInfo*, const char*, int, const void*, int),
    const string_view& nid, const bufferview& attr, bool octet)
{
    int type;
    const void* bytes;
    int len;
    ASN1_TYPE* asn1type;
    if (octet)
    {
        type = V_ASN1_OCTET_STRING;
        bytes = attr.data();
        len = (int)attr.size();
        asn1type = nullptr;
    }
    else
    {
        auto data = (const unsigned char*)attr.data();
        asn1type = d2i_ASN1_TYPE(nullptr, &data, (long)attr.size());
        if (asn1type == nullptr)
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError,
                "Unable to parse an ASN.1 object");
        }

        type = asn1type->type;
        bytes = asn1type->value.ptr;
        len = -1;
    }

    int rc = addAttributeFun(si, nid.data(), type, bytes, len);
    if (asn1type != nullptr)
        ASN1_TYPE_free(asn1type);

    if (rc < 0)
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError,
            "Unable to insert an attribute to the signer");
    }
}

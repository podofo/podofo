// SPDX-FileCopyrightText: 2026 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "CmsVerifyContext.h"

#include <podofo/private/OpenSSLInternal.h>

using namespace std;
using namespace PoDoFo;

CmsVerifyContext::CmsVerifyContext() :
    m_cms(nullptr),
    m_signers(nullptr),
    m_signer(nullptr),
    m_databio(nullptr)
{
}

CmsVerifyContext::~CmsVerifyContext()
{
    clear();
}

bool CmsVerifyContext::TryReset(const bufferview& cms)
{
    clear();

    auto in = (const unsigned char*)cms.data();
    m_cms = d2i_CMS_ContentInfo(nullptr, &in, (long)cms.size());
    if (m_cms == nullptr)
        goto Fail;

    // Resolve the signer certificates from the ones embedded in the CMS
    // NOTE: This is required, as the certificates are not automatically
    // assigned to the signers when parsing the structure
    if (CMS_set1_signers_certs(m_cms, nullptr, 0) <= 0)
        goto Fail;

    // NOTE: The signers are missing when the content is not a "SignedData"
    m_signers = CMS_get0_SignerInfos(m_cms);
    if (m_signers == nullptr)
        goto Fail;

    // Initialize the BIO chain that will digest the appended content. It holds
    // a context for each digest algorithm found in the structure, so a single
    // streaming of the content serves all the signers
    m_databio = CMS_dataInit(m_cms, nullptr);
    if (m_databio == nullptr)
        goto Fail;

    return true;

Fail:
    ERR_clear_error();
    clear();
    return false;
}

unsigned CmsVerifyContext::GetSignerCount() const
{
    checkReset();
    return (unsigned)sk_CMS_SignerInfo_num(m_signers);
}

bool CmsVerifyContext::TryLoadSigner(unsigned signerIndex)
{
    if (signerIndex >= GetSignerCount())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "The signer index is out of range");

    auto signer = sk_CMS_SignerInfo_value(m_signers, (int)signerIndex);
    if (signer == nullptr)
        return false;

    // Signed attributes are mandatory in a PDF signature. Without them the
    // signature would be computed on the content, which we don't support
    if (CMS_signed_get_attr_count(signer) <= 0)
        return false;

    m_signer = signer;
    return true;
}

void CmsVerifyContext::AppendData(const bufferview& data)
{
    checkSignerLoaded();

    if (data.size() == 0)
        return;

    if (BIO_write(m_databio, data.data(), (int)data.size()) != (int)data.size())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "BIO_write");
}

bool CmsVerifyContext::VerifySignature()
{
    checkSignerLoaded();

    // Verify the signature over the signed attributes, as they were parsed
    if (CMS_SignerInfo_verify(m_signer) <= 0)
        goto Fail;

    // Compare the digest of the appended content with the "messageDigest"
    // signed attribute. This is what binds the content to the signature
    if (CMS_SignerInfo_verify_content(m_signer, m_databio) <= 0)
        goto Fail;

    return true;

Fail:
    // NOTE: A verification failure also pushes entries in the error queue
    ERR_clear_error();
    return false;
}

// Compare the hash stored in the "signingCertificateV2" signed attribute
// (RFC 5035) with the actual signer certificate
bool CmsVerifyContext::TryVerifySigningCertificateV2(bool& attrMissing)
{
    checkSignerLoaded();

    attrMissing = false;
    int index = CMS_signed_get_attr_by_NID(m_signer, NID_id_smime_aa_signingCertificateV2, -1);
    if (index < 0)
    {
        attrMissing = true;
        return true;
    }

    auto attr = CMS_signed_get_attr(m_signer, index);
    if (attr == nullptr || X509_ATTRIBUTE_count(attr) != 1)
        return false;

    auto value = X509_ATTRIBUTE_get0_type(attr, 0);
    if (value == nullptr || value->type != V_ASN1_SEQUENCE)
        return false;

    auto in = (const unsigned char*)value->value.sequence->data;
    auto certV2 = (MY_ESS_SIGNING_CERT_V2*)ASN1_item_d2i(nullptr, &in,
        value->value.sequence->length, ASN1_ITEM_rptr(MY_ESS_SIGNING_CERT_V2));
    if (certV2 == nullptr)
        return false;

    bool matches = false;
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned hashLen;
    X509* cert;
    const EVP_MD* md;
    const ASN1_OBJECT* hashOid;

    // NOTE: Only the first identifier refers to the signer certificate,
    // the following ones are hints for the rest of the chain
    if (sk_MY_ESS_CERT_ID_V2_num(certV2->cert_ids) < 1)
        goto Exit;

    {
        auto certId = sk_MY_ESS_CERT_ID_V2_value(certV2->cert_ids, 0);
        if (certId->hash_alg == nullptr)
        {
            // The hashing algorithm defaults to SHA-256, as per RFC 5035
            md = ssl::GetEVP_MD(PdfHashingAlgorithm::SHA256);
        }
        else
        {
            X509_ALGOR_get0(&hashOid, nullptr, nullptr, certId->hash_alg);
            md = EVP_get_digestbyobj(hashOid);
            if (md == nullptr)
                goto Exit;
        }

        CMS_SignerInfo_get0_algs(m_signer, nullptr, &cert, nullptr, nullptr);
        if (cert == nullptr)
            goto Exit;

        // The hash is computed on the ASN.1 encoded certificate
        if (X509_digest(cert, md, hash, &hashLen) <= 0)
            goto Exit;

        matches = (unsigned)certId->hash->length == hashLen
            && std::memcmp(certId->hash->data, hash, hashLen) == 0;
    }

Exit:
    ASN1_item_free((ASN1_VALUE*)certV2, ASN1_ITEM_rptr(MY_ESS_SIGNING_CERT_V2));
    return matches;
}

void CmsVerifyContext::checkReset() const
{
    if (m_cms == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "The context is not initialized");
}

void CmsVerifyContext::checkSignerLoaded() const
{
    if (m_signer == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "A signer must be loaded first");
}

void CmsVerifyContext::clear()
{
    if (m_cms != nullptr)
    {
        CMS_ContentInfo_free(m_cms);
        m_cms = nullptr;
    }

    if (m_databio != nullptr)
    {
        BIO_free_all(m_databio);
        m_databio = nullptr;
    }

    // NOTE: The signers are owned by the CMS structure
    m_signers = nullptr;
    m_signer = nullptr;
}

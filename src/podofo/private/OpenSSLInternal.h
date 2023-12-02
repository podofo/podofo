/**
 * SPDX-FileCopyrightText: (C) 2023 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_OPENSSL_INTERNAL_H
#define PDF_OPENSSL_INTERNAL_H

#include <podofo/main/PdfSigningCommon.h>

#include <date/date.h>
#include <openssl/ssl.h>
#include <openssl/cms.h>
#include <openssl/asn1t.h>

// This is a recreation of ESS_CERT_ID_V2 structure
struct MY_ESS_CERT_ID_V2
{
    X509_ALGOR* hash_alg;
    ASN1_OCTET_STRING* hash;
};

// This is a recreation of MY_ESS_SIGNING_CERT_V2 structure
struct MY_ESS_SIGNING_CERT_V2
{
    STACK_OF(MY_ESS_CERT_ID_V2)* cert_ids;
    STACK_OF(POLICYINFO)* policy_info;
};

DECLARE_ASN1_ITEM(MY_ESS_SIGNING_CERT_V2)

// Used for the serialization of a digest for signing
DEFINE_STACK_OF(MY_ESS_CERT_ID_V2);

namespace ssl
{
    const EVP_MD* GetEVP_MD(PoDoFo::PdfHashingAlgorithm hashing);
    unsigned GetEVP_Size(PoDoFo::PdfHashingAlgorithm hashing);
    void AddSigningCertificateV2(CMS_SignerInfo* signer, const PoDoFo::bufferview& hash);
    void ComputeHashToSign(CMS_SignerInfo* si, BIO* chain, bool doWrapDigest, PoDoFo::charbuff& hashToSign);
    void RsaRawEncrypt(const PoDoFo::bufferview& input, PoDoFo::charbuff& output, EVP_PKEY* pkey);

    // Returns ASN.1 encoded X509 certificate
    PoDoFo::charbuff GetEncoded(const X509* cert);

    // Returns ASN.1 encoded private key
    PoDoFo::charbuff GetEncoded(const EVP_PKEY* pkey);

    PoDoFo::charbuff ComputeHash(const PoDoFo::bufferview& data, PoDoFo::PdfHashingAlgorithm hashing);
    PoDoFo::charbuff ComputeMD5(const PoDoFo::bufferview& data);
    PoDoFo::charbuff ComputeSHA1(const PoDoFo::bufferview& data);

    std::string ComputeHashStr(const PoDoFo::bufferview& data, PoDoFo::PdfHashingAlgorithm hashing);
    std::string ComputeMD5Str(const PoDoFo::bufferview& data);
    std::string ComputeSHA1Str(const PoDoFo::bufferview& data);

    const EVP_CIPHER* Rc4();
    const EVP_CIPHER* Aes128();
    const EVP_CIPHER* Aes256();
    const EVP_MD* MD5();
    const EVP_MD* SHA256();
    const EVP_MD* SHA384();
    const EVP_MD* SHA512();

    void cmsAddSigningTime(CMS_SignerInfo* si, const date::sys_seconds& timestamp);
    void cmsComputeHashToSign(CMS_SignerInfo* si, BIO* chain, PoDoFo::charbuff& hashToSign);
}

#endif // PDF_OPENSSL_INTERNAL_H

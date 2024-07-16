/**
 *
 SPDX-FileCopyrightText: (C) 2023 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_OPENSSL_INTERNAL_H
#define PDF_OPENSSL_INTERNAL_H

#include <podofo/main/PdfDeclarations.h>

#include <openssl/opensslv.h>

// See https://github.com/openssl/openssl/blob/51caffb5c187bb2c633e0da9f5928fced4fae7ed/include/openssl/e_ostime.h#L32:
// It appears that recent versions of OpenSSL now unconditionally includes
// <WinSock2.h>, which includes <Windows.h>. This will cause issues with
// GetObject() and possibly other macros, so we early workaround them
#if defined(_WIN32) && (OPENSSL_VERSION_MAJOR > 3 || (OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR >= 2))
#include "WindowsLeanMean.h"
#endif

#include <openssl/ssl.h>
#include <openssl/cms.h>
#include <openssl/asn1t.h>
#include <openssl/err.h>

#include <date/date.h>

#if OPENSSL_VERSION_MAJOR < 3
 // Fixes warning when compiling with OpenSSL 3
#define EVP_MD_CTX_get0_md EVP_MD_CTX_md
#define EVP_MD_get_type EVP_MD_type
#define EVP_PKEY_get_id EVP_PKEY_id
#endif // OPENSSL_VERSION_MAJOR < 3

// This is a recreation of ESS_CERT_ID_V2 structure
struct MY_ESS_CERT_ID_V2
{
    X509_ALGOR* hash_alg;
    ASN1_OCTET_STRING* hash;
};

// This is a recreation of ESS_SIGNING_CERT_V2 structure
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
    void AddSigningCertificateV2(CMS_SignerInfo* signer, const PoDoFo::bufferview& hash, PoDoFo::PdfHashingAlgorithm hashing);
    void ComputeHashToSign(CMS_SignerInfo* si, BIO* chain, bool doWrapDigest, PoDoFo::charbuff& hashToSign);

    // Load a ASN.1 encoded private key (PKCS#1 or PKCS#8 formats supported)
    EVP_PKEY* LoadPrivateKey(const PoDoFo::bufferview& input);

    // Sign a buffer with the supplied pkey, no encapsulation and deterministic padding
    void DoSign(const PoDoFo::bufferview& input, const PoDoFo::bufferview& pkey,
        PoDoFo::PdfHashingAlgorithm hashing, PoDoFo::charbuff& output);
    void DoSign(const PoDoFo::bufferview& input, EVP_PKEY* pkey,
        PoDoFo::PdfHashingAlgorithm hashing, PoDoFo::charbuff& output);

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

    void WrapDigestPKCS1(const PoDoFo::bufferview& hash, PoDoFo::PdfHashingAlgorithm hashing, PoDoFo::charbuff& output);

    void ComputeHash(const PoDoFo::bufferview& data, PoDoFo::PdfHashingAlgorithm hashing,
        unsigned char* hash, unsigned& length);
    void ComputeMD5(const PoDoFo::bufferview& data, unsigned char* hash);
    void ComputeSHA1(const PoDoFo::bufferview& data, unsigned char* hash);

    void GetOpenSSLError(std::string& err);

    const EVP_CIPHER* Rc4();
    const EVP_CIPHER* Aes128();
    const EVP_CIPHER* Aes256();
    const EVP_MD* MD5();
    const EVP_MD* SHA1();
    const EVP_MD* SHA256();
    const EVP_MD* SHA384();
    const EVP_MD* SHA512();

    void cmsAddSigningTime(CMS_SignerInfo* si, const date::sys_seconds& timestamp);
    void cmsComputeHashToSign(CMS_SignerInfo* si, BIO* chain, PoDoFo::charbuff& hashToSign);

    /** Init the OpenSSL engine. NOTE: To be called by OpenSSLInternal only
     */
    PODOFO_EXPORT void Init();

    /** Class to be initialized only once as a singleton
     */
    class OpenSSLMain
    {
    public:
        OpenSSLMain();
        void Init();
        ~OpenSSLMain();
    public:
        const EVP_CIPHER* GetRc4() const { return m_Rc4; }
        const EVP_CIPHER* GetAes128() const { return m_Aes128; }
        const EVP_CIPHER* GetAes256() const { return m_Aes256; }
        const EVP_MD* GetMD5() const { return m_MD5; }
        const EVP_MD* GetSHA1() const { return m_SHA1; }
        const EVP_MD* GetSHA256() const { return m_SHA256; }
        const EVP_MD* GetSHA384() const { return m_SHA384; }
        const EVP_MD* GetSHA512() const { return m_SHA512; }
    private:
#if OPENSSL_VERSION_MAJOR >= 3
        OSSL_LIB_CTX* m_libCtx;
        OSSL_PROVIDER* m_legacyProvider;
        OSSL_PROVIDER* m_defaultProvider;
#endif // OPENSSL_VERSION_MAJOR >= 3
        const EVP_CIPHER* m_Rc4;
        const EVP_CIPHER* m_Aes128;
        const EVP_CIPHER* m_Aes256;
        const EVP_MD* m_MD5;
        const EVP_MD* m_SHA1;
        const EVP_MD* m_SHA256;
        const EVP_MD* m_SHA384;
        const EVP_MD* m_SHA512;
    };
}

#endif // PDF_OPENSSL_INTERNAL_H

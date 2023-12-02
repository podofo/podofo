/**
 * SPDX-FileCopyrightText: (C) 2023 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "OpenSSLInternal.h"

#if OPENSSL_VERSION_MAJOR >= 3
#include <openssl/provider.h>
#endif // OPENSSL_VERSION_MAJOR >= 3

using namespace std;
using namespace PoDoFo;

static class OpenSSLInit
{
public:
    OpenSSLInit()
    {
#if OPENSSL_VERSION_MAJOR >= 3
        m_libCtx = OSSL_LIB_CTX_new();
        if (m_libCtx == nullptr)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Unable to create OpenSSL library context");

        // NOTE: Load required legacy providers, such as RC4, together regular ones,
        // as explained in https://wiki.openssl.org/index.php/OpenSSL_3.0#Providers
        m_legacyProvider = OSSL_PROVIDER_load(m_libCtx, "legacy");
        if (m_legacyProvider == nullptr)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Unable to load legacy providers in OpenSSL >= 3.x.x");

        m_defaultProvider = OSSL_PROVIDER_load(m_libCtx, "default");
        if (m_defaultProvider == nullptr)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Unable to load default providers in OpenSSL >= 3.x.x");

        // https://www.openssl.org/docs/man3.0/man7/crypto.html#FETCHING-EXAMPLES
        Rc4 = EVP_CIPHER_fetch(m_libCtx, "RC4", "provider=legacy");
        Aes128 = EVP_CIPHER_fetch(m_libCtx, "AES-128-CBC", "provider=default");
        Aes256 = EVP_CIPHER_fetch(m_libCtx, "AES-256-CBC", "provider=default");
        MD5 = EVP_MD_fetch(m_libCtx, "MD5", "provider=default");
        SHA1 = EVP_MD_fetch(m_libCtx, "SHA1", "provider=default");
        SHA256 = EVP_MD_fetch(m_libCtx, "SHA2-256", "provider=default");
        SHA384 = EVP_MD_fetch(m_libCtx, "SHA2-384", "provider=default");
        SHA512 = EVP_MD_fetch(m_libCtx, "SHA2-512", "provider=default");

#else // OPENSSL_VERSION_MAJOR < 3
        Rc4 = EVP_rc4();
        Aes128 = EVP_aes_128_cbc();
        Aes256 = EVP_aes_256_cbc();
        MD5 = EVP_md5();
        SHA1 = EVP_sha1();
        SHA256 = EVP_sha256();
        SHA384 = EVP_sha384();
        SHA512 = EVP_sha512();
#endif // OPENSSL_VERSION_MAJOR >= 3
    }

    ~OpenSSLInit()
    {
#if OPENSSL_VERSION_MAJOR >= 3
        OSSL_PROVIDER_unload(m_legacyProvider);
        OSSL_PROVIDER_unload(m_defaultProvider);
        OSSL_LIB_CTX_free(m_libCtx);
#endif // OPENSSL_VERSION_MAJOR >= 3
    }
public:
    const EVP_CIPHER* Rc4;
    const EVP_CIPHER* Aes128;
    const EVP_CIPHER* Aes256;
    const EVP_MD* MD5;
    const EVP_MD* SHA1;
    const EVP_MD* SHA256;
    const EVP_MD* SHA384;
    const EVP_MD* SHA512;
private:
#if OPENSSL_VERSION_MAJOR >= 3
    OSSL_LIB_CTX* m_libCtx;
    OSSL_PROVIDER* m_legacyProvider;
    OSSL_PROVIDER* m_defaultProvider;
#endif // OPENSSL_VERSION_MAJOR >= 3
} s_SSL;

static void computeHash(const bufferview& data, const EVP_MD* type, charbuff& hash);
static string computeHashStr(const bufferview& data, const EVP_MD* type);

// Add signing-certificate-v2 attribute as defined in rfc5035
// https://tools.ietf.org/html/rfc5035
void ssl::AddSigningCertificateV2(CMS_SignerInfo* signer, const bufferview& hash)
{
    unsigned char* buf = nullptr;
    MY_ESS_SIGNING_CERT_V2 certV2{ };
    ASN1_OCTET_STRING hashstr{ };
    ASN1_OCTET_STRING_set(&hashstr, (const unsigned char*)hash.data(), (int)hash.size());
    MY_ESS_CERT_ID_V2 certIdV2{ };
    certIdV2.hash = &hashstr;
    certV2.cert_ids = sk_MY_ESS_CERT_ID_V2_new_null();
    if (!sk_MY_ESS_CERT_ID_V2_push(certV2.cert_ids, &certIdV2))
        throw runtime_error("Unable to add attribute");

    auto clean = [&]()
    {
        sk_MY_ESS_CERT_ID_V2_free(certV2.cert_ids);
        OPENSSL_free(buf);
    };

    int len = ASN1_item_i2d((ASN1_VALUE*)&certV2, &buf, ASN1_ITEM_rptr(MY_ESS_SIGNING_CERT_V2));
    if (CMS_signed_add1_attr_by_NID(signer, NID_id_smime_aa_signingCertificateV2,
        V_ASN1_SEQUENCE, buf, len) <= 0)
    {
        clean();
        throw runtime_error("Unable to add attribute");
    }

    clean();
}

void ssl::cmsAddSigningTime(CMS_SignerInfo* si, const date::sys_seconds& timestamp)
{
    auto time = chrono::system_clock::to_time_t(timestamp);
    auto ans1time = X509_time_adj(nullptr, 0, &time);
    if (CMS_signed_add1_attr_by_NID(si, NID_pkcs9_signingTime,
        ans1time->type, ans1time, -1) <= 0)
    {
        ASN1_TIME_free(ans1time);
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSL, "Error setting SigningTime");
    }
}

void ssl::RsaRawEncrypt(const bufferview& input, charbuff& output, EVP_PKEY* pkey)
{
    // This is raw RSA encryption with deterministic padding
    auto rsa = EVP_PKEY_get1_RSA(pkey);
    int rsalen = RSA_size(rsa);

    output.resize(rsalen);
    (void)RSA_private_encrypt((int)input.size(), (const unsigned char*)input.data(),
        (unsigned char*)output.data(), rsa, RSA_PKCS1_PADDING);
}

charbuff ssl::GetEncoded(const X509* cert)
{
    unique_ptr<BIO, decltype(&BIO_free)> bio(BIO_new(BIO_s_mem()), BIO_free);
    if (bio == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "BIO_new failed");

    if (i2d_X509_bio(bio.get(), const_cast<X509*>(cert)) == 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSL, "i2d_X509_bio failed");

    char* signatureData;
    size_t length = (size_t)BIO_get_mem_data(bio.get(), &signatureData);

    charbuff ret;
    ret.assign(signatureData, signatureData + length);
    return ret;
}

charbuff ssl::GetEncoded(const EVP_PKEY* pkey)
{
    unique_ptr<BIO, decltype(&BIO_free)> bio(BIO_new(BIO_s_mem()), BIO_free);
    if (bio == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "BIO_new failed");

    if (i2d_PrivateKey_bio(bio.get(), const_cast<EVP_PKEY*>(pkey)) == 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSL, "i2d_PrivateKey_bio failed");

    char* signatureData;
    size_t length = (size_t)BIO_get_mem_data(bio.get(), &signatureData);

    charbuff ret;
    ret.assign(signatureData, signatureData + length);
    return ret;
}

unsigned ssl::GetEVP_Size(PdfHashingAlgorithm hashing)
{
    switch (hashing)
    {
        case PdfHashingAlgorithm::SHA256:
            return 32;
        case PdfHashingAlgorithm::SHA384:
            return 48;
        case PdfHashingAlgorithm::SHA512:
            return 64;
        case PdfHashingAlgorithm::Unknown:
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Unsupported hashing");
    }
}

const EVP_MD* ssl::GetEVP_MD(PdfHashingAlgorithm hashing)
{
    switch (hashing)
    {
        case PdfHashingAlgorithm::SHA256:
            return s_SSL.SHA256;
        case PdfHashingAlgorithm::SHA384:
            return s_SSL.SHA384;
        case PdfHashingAlgorithm::SHA512:
            return s_SSL.SHA512;
        case PdfHashingAlgorithm::Unknown:
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Unsupported hashing");
    }
}

charbuff ssl::ComputeHash(const bufferview& data, PdfHashingAlgorithm hashing)
{
    charbuff hash;
    computeHash(data, ssl::GetEVP_MD(hashing), hash);
    return hash;
}

charbuff ssl::ComputeMD5(const bufferview& data)
{
    charbuff hash;
    computeHash(data, s_SSL.MD5, hash);
    return hash;
}

charbuff ssl::ComputeSHA1(const bufferview& data)
{
    charbuff hash;
    computeHash(data, s_SSL.SHA1, hash);
    return hash;
}

string ssl::ComputeHashStr(const bufferview& data, PdfHashingAlgorithm hashing)
{
    return computeHashStr(data, ssl::GetEVP_MD(hashing));
}

string ssl::ComputeMD5Str(const bufferview& data)
{
    return computeHashStr(data, s_SSL.MD5);
}

string ssl::ComputeSHA1Str(const bufferview& data)
{
    return computeHashStr(data, s_SSL.SHA1);
}

void computeHash(const bufferview& data, const EVP_MD* type, charbuff& hashdst)
{
    unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned len;
    if (EVP_DigestInit(ctx.get(), type) == 0)
        goto Error;

    // Compute the hash to be signed
    if (EVP_DigestUpdate(ctx.get(), data.data(), data.size()) == 0)
        goto Error;

    if (EVP_DigestFinal(ctx.get(), hash, &len) == 0)
        goto Error;

    hashdst.resize(len);
    std::memcpy(hashdst.data(), hash, len);
    return;

Error:
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSL, "Error while computing hash");
}

string computeHashStr(const bufferview& data, const EVP_MD* type)
{
    charbuff hash;
    computeHash(data, type, hash);
    return utls::GetCharHexString(hash);
}

const EVP_CIPHER* ssl::Rc4()
{
    return s_SSL.Rc4;
}

const EVP_CIPHER* ssl::Aes128()
{
    return s_SSL.Aes128;
}

const EVP_CIPHER* ssl::Aes256()
{
    return s_SSL.Aes256;
}

const EVP_MD* ssl::MD5()
{
    return s_SSL.MD5;
}

const EVP_MD* ssl::SHA256()
{
    return s_SSL.SHA256;
}

const EVP_MD* ssl::SHA384()
{
    return s_SSL.SHA384;
}

const EVP_MD* ssl::SHA512()
{
    return s_SSL.SHA512;
}

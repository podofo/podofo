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
using namespace ssl;

// The entry points are defined in the PoDoFo public module. See PdfCommon.cpp
extern PODOFO_IMPORT OpenSSLMain s_SSL;

static void computeHash(const bufferview& data, const EVP_MD* type,
    unsigned char* hash, unsigned& length);
static string computeHashStr(const bufferview& data, const EVP_MD* type);

OpenSSLMain::OpenSSLMain() :
#if OPENSSL_VERSION_MAJOR >= 3
    m_libCtx{ }, m_legacyProvider{ }, m_defaultProvider{ },
#endif // OPENSSL_VERSION_MAJOR >= 3
    m_Rc4{ }, m_Aes128{ }, m_Aes256{ }, m_MD5{ },
    m_SHA1{ }, m_SHA256{ }, m_SHA384{ }, m_SHA512{ }
{
}

void OpenSSLMain::Init()
{
    PODOFO_ASSERT(m_Rc4 == nullptr);
#if OPENSSL_VERSION_MAJOR >= 3
    m_libCtx = OSSL_LIB_CTX_new();
    if (m_libCtx == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Unable to create OpenSSL library context");

    // NOTE: Try to load required legacy providers, such as RC4, together regular ones,
    // as explained in https://wiki.openssl.org/index.php/OpenSSL_3.0#Providers
    m_legacyProvider = OSSL_PROVIDER_load(m_libCtx, "legacy");
    m_defaultProvider = OSSL_PROVIDER_load(m_libCtx, "default");
    if (m_defaultProvider == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Unable to load default providers in OpenSSL >= 3.x.x");

    // https://www.openssl.org/docs/man3.0/man7/crypto.html#FETCHING-EXAMPLES
    if (m_legacyProvider != nullptr)
        m_Rc4 = EVP_CIPHER_fetch(m_libCtx, "RC4", "provider=legacy");
    m_Aes128 = EVP_CIPHER_fetch(m_libCtx, "AES-128-CBC", "provider=default");
    m_Aes256 = EVP_CIPHER_fetch(m_libCtx, "AES-256-CBC", "provider=default");
    m_MD5 = EVP_MD_fetch(m_libCtx, "MD5", "provider=default");
    m_SHA1 = EVP_MD_fetch(m_libCtx, "SHA1", "provider=default");
    m_SHA256 = EVP_MD_fetch(m_libCtx, "SHA2-256", "provider=default");
    m_SHA384 = EVP_MD_fetch(m_libCtx, "SHA2-384", "provider=default");
    m_SHA512 = EVP_MD_fetch(m_libCtx, "SHA2-512", "provider=default");
#else // OPENSSL_VERSION_MAJOR < 3
    m_Rc4 = EVP_rc4();
    m_Aes128 = EVP_aes_128_cbc();
    m_Aes256 = EVP_aes_256_cbc();
    m_MD5 = EVP_md5();
    m_SHA1 = EVP_sha1();
    m_SHA256 = EVP_sha256();
    m_SHA384 = EVP_sha384();
    m_SHA512 = EVP_sha512();
#endif // OPENSSL_VERSION_MAJOR >= 3
}

OpenSSLMain::~OpenSSLMain()
{
#if OPENSSL_VERSION_MAJOR >= 3
    if (m_libCtx == nullptr)
        return;

    OSSL_PROVIDER_unload(m_legacyProvider);
    OSSL_PROVIDER_unload(m_defaultProvider);
    OSSL_LIB_CTX_free(m_libCtx);
#endif // OPENSSL_VERSION_MAJOR >= 3
}

// Add signing-certificate-v2 attribute as defined in rfc5035
// https://tools.ietf.org/html/rfc5035
void ssl::AddSigningCertificateV2(CMS_SignerInfo* signer, const bufferview& hash, PdfHashingAlgorithm hashing)
{
    unique_ptr<X509_ALGOR, decltype(&X509_ALGOR_free)> x509Algor(X509_ALGOR_new(), X509_ALGOR_free);
    if (x509Algor == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "Error X509_ALGOR_new");

    X509_ALGOR_set_md(x509Algor.get(), ssl::GetEVP_MD(hashing));

    unsigned char* buf = nullptr;
    MY_ESS_SIGNING_CERT_V2 certV2{ };
    ASN1_OCTET_STRING hashstr{ };
    ASN1_OCTET_STRING_set(&hashstr, (const unsigned char*)hash.data(), (int)hash.size());
    MY_ESS_CERT_ID_V2 certIdV2{ };

    certIdV2.hash_alg = x509Algor.get();
    certIdV2.hash = &hashstr;
    certV2.cert_ids = sk_MY_ESS_CERT_ID_V2_new_null();
    if (!sk_MY_ESS_CERT_ID_V2_push(certV2.cert_ids, &certIdV2))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "Unable to add attribute");

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
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "Unable to add attribute");
    }

    clean();
}

EVP_PKEY* ssl::LoadPrivateKey(const bufferview& input)
{
    // Try to load a RSA DER private key first
    const unsigned char* data = (const unsigned char*)input.data();
    auto ret = d2i_PrivateKey(EVP_PKEY_RSA, nullptr, &data, (long)input.size());
    if (ret != nullptr)
        return ret;

    // Then try to load a ECDSA DER private
    ret = d2i_PrivateKey(EVP_PKEY_EC, nullptr, &data, (long)input.size());
    if (ret != nullptr)
        return ret;

    // Finally try to load a PEM key
    unique_ptr<BIO, decltype(&BIO_free)> bio(BIO_new_mem_buf(input.data(), (int)input.size()), BIO_free);
    if (bio == nullptr)
        goto Fail;

    ret = PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr);
    if (ret != nullptr)
        return ret;

Fail:
    string err("Private key loading failed. Internal OpenSSL error:\n");
    ssl::GetOpenSSLError(err);
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, err);
}

void ssl::cmsAddSigningTime(CMS_SignerInfo* si, const date::sys_seconds& timestamp)
{
    auto time = chrono::system_clock::to_time_t(timestamp);
    auto ans1time = X509_time_adj(nullptr, 0, &time);
    if (CMS_signed_add1_attr_by_NID(si, NID_pkcs9_signingTime,
        ans1time->type, ans1time, -1) <= 0)
    {
        ASN1_TIME_free(ans1time);
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "Error setting SigningTime");
    }
}

void ssl::DoSign(const bufferview& input, const bufferview& pkey,
    PoDoFo::PdfHashingAlgorithm hashing, charbuff& output)
{
    unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> pkeyssl(ssl::LoadPrivateKey(pkey), EVP_PKEY_free);
    ssl::DoSign(input, pkeyssl.get(), hashing, output);
}

// Note that signing is really encryption with the private key
// and a deterministic padding
void ssl::DoSign(const bufferview& input, EVP_PKEY* pkey,
    PdfHashingAlgorithm hashing, charbuff& output)
{
    size_t siglen;
    unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> ctx(EVP_PKEY_CTX_new(pkey, nullptr), EVP_PKEY_CTX_free);
    if (ctx == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "Error EVP_PKEY_CTX_new");

    if (EVP_PKEY_sign_init(ctx.get()) <= 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "Error EVP_PKEY_sign_init");

    if (EVP_PKEY_base_id(pkey) == EVP_PKEY_RSA)
    {
        // Set deterministic PKCS1 padding
        if (EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_PADDING) <= 0)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "Error EVP_PKEY_CTX_set_rsa_padding");
    }

    auto actualInput = input;
    charbuff tempWrapped;
    if (hashing != PdfHashingAlgorithm::Unknown)
    {
        ssl::WrapDigestPKCS1(input, hashing, tempWrapped);
        actualInput = tempWrapped;
    }
    
    if (EVP_PKEY_sign(ctx.get(), nullptr, &siglen, (const unsigned char*)actualInput.data(), actualInput.size()) <= 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "Error determining output size");

    output.resize(siglen);
    if (EVP_PKEY_sign(ctx.get(), (unsigned char*)output.data(), &siglen,
        (const unsigned char*)actualInput.data(), actualInput.size()) <= 0)
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "Error signing input buffer");
    }

    // NOTE: This is required for ECDSA encryption, as the
    // first determined length is just an upper bound
    output.resize(siglen);
}

charbuff ssl::GetEncoded(const X509* cert)
{
    unique_ptr<BIO, decltype(&BIO_free)> bio(BIO_new(BIO_s_mem()), BIO_free);
    if (bio == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "BIO_new failed");

    if (i2d_X509_bio(bio.get(), const_cast<X509*>(cert)) == 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "i2d_X509_bio failed");

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
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "i2d_PrivateKey_bio failed");

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
            return SHA256();
        case PdfHashingAlgorithm::SHA384:
            return SHA384();
        case PdfHashingAlgorithm::SHA512:
            return SHA512();
        case PdfHashingAlgorithm::Unknown:
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Unsupported hashing");
    }
}

charbuff ssl::ComputeHash(const bufferview& data, PdfHashingAlgorithm hashing)
{
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned length;
    computeHash(data, ssl::GetEVP_MD(hashing), hash, length);
    return charbuff((const char*)hash, length);
}

charbuff ssl::ComputeMD5(const bufferview& data)
{
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned length;
    computeHash(data, MD5(), hash, length);
    return charbuff((const char*)hash, length);
}

charbuff ssl::ComputeSHA1(const bufferview& data)
{
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned length;
    computeHash(data, SHA1(), hash, length);
    return charbuff((const char*)hash, length);
}

string ssl::ComputeHashStr(const bufferview& data, PdfHashingAlgorithm hashing)
{
    return computeHashStr(data, ssl::GetEVP_MD(hashing));
}

string ssl::ComputeMD5Str(const bufferview& data)
{
    return computeHashStr(data, MD5());
}

string ssl::ComputeSHA1Str(const bufferview& data)
{
    return computeHashStr(data, SHA1());
}

void ssl::ComputeHash(const bufferview& data, PdfHashingAlgorithm hashing,
    unsigned char* hash, unsigned& length)
{
    computeHash(data, ssl::GetEVP_MD(hashing), hash, length);
}

void ssl::ComputeMD5(const bufferview& data, unsigned char* hash)
{
    unsigned length;
    computeHash(data, ssl::MD5(), hash, length);
}

void ssl::ComputeSHA1(const bufferview& data, unsigned char* hash)
{
    unsigned length;
    computeHash(data, ssl::SHA1(), hash, length);
}

void ssl::GetOpenSSLError(string& err)
{
    string ret;
    BIO* bio = BIO_new(BIO_s_mem());
    if (bio == nullptr)
        return;

    ERR_print_errors(bio);
    char* buf;
    size_t len = BIO_get_mem_data(bio, &buf);
    err.append(buf, len);
    BIO_free(bio);
}

const EVP_CIPHER* ssl::Rc4()
{
    ssl::Init();
    return s_SSL.GetRc4();
}

const EVP_CIPHER* ssl::Aes128()
{
    ssl::Init();
    return s_SSL.GetAes128();
}

const EVP_CIPHER* ssl::Aes256()
{
    ssl::Init();
    return s_SSL.GetAes256();
}

const EVP_MD* ssl::MD5()
{
    ssl::Init();
    return s_SSL.GetMD5();
}

const EVP_MD* ssl::SHA1()
{
    ssl::Init();
    return s_SSL.GetSHA1();
}

const EVP_MD* ssl::SHA256()
{
    ssl::Init();
    return s_SSL.GetSHA256();
}

const EVP_MD* ssl::SHA384()
{
    ssl::Init();
    return s_SSL.GetSHA384();
}

const EVP_MD* ssl::SHA512()
{
    ssl::Init();
    return s_SSL.GetSHA512();
}

void computeHash(const bufferview& data, const EVP_MD* type,
    unsigned char* hash, unsigned& length)
{
    unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (EVP_DigestInit(ctx.get(), type) == 0)
        goto Error;

    // Compute the hash to be signed
    if (EVP_DigestUpdate(ctx.get(), data.data(), data.size()) == 0)
        goto Error;

    if (EVP_DigestFinal(ctx.get(), hash, &length) == 0)
        goto Error;

    return;

Error:
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "Error while computing hash");
}

string computeHashStr(const bufferview& data, const EVP_MD* type)
{
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned length;
    computeHash(data, type, hash, length);
    return utls::GetCharHexString({ (const char*)hash, length });
}

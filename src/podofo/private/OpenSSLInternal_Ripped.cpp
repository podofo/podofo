/**
 * SPDX-FileCopyrightText: (C) 2023 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: OpenSSL
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "OpenSSLInternal.h"

using namespace std;
using namespace PoDoFo;

// The following functions include software developed by
// the OpenSSL Project for use in the OpenSSL Toolkit(http://www.openssl.org/)
// License: https://www.openssl.org/source/license-openssl-ssleay.txt

// This is a recreation of X509_SIG structure
struct MY_X509_SIG
{
    X509_ALGOR* algor;
    ASN1_OCTET_STRING* digest;
};

static void compute_hash_to_sign(CMS_SignerInfo* si, unsigned char hash[], unsigned& len);
static int cms_DigestAlgorithm_find_ctx(EVP_MD_CTX* mctx, BIO* chain, X509_ALGOR* mdalg);
static void encode_pkcs1(X509_ALGOR* digestAlg,
    const unsigned char* m, unsigned int m_len, charbuff& outbuff);

static X509_ALGOR* getDigestAlgorithm(CMS_SignerInfo* si);
static const ASN1_OBJECT* getASN1Object(X509_ALGOR* alg);
static STACK_OF(X509_ATTRIBUTE)* getSignedAttributesCopy(CMS_SignerInfo* si);

// The following is using for reordering attributes during serialization
ASN1_ITEM_TEMPLATE(CMS_Attributes_Sign) =
ASN1_EX_TEMPLATE_TYPE(ASN1_TFLG_SET_ORDER, 0, CMS_ATTRIBUTES, X509_ATTRIBUTE)
ASN1_ITEM_TEMPLATE_END(CMS_Attributes_Sign)

ASN1_SEQUENCE(MY_X509_SIG) = {
        ASN1_SIMPLE(MY_X509_SIG, algor, X509_ALGOR),
        ASN1_SIMPLE(MY_X509_SIG, digest, ASN1_OCTET_STRING)
} ASN1_SEQUENCE_END(MY_X509_SIG);

ASN1_SEQUENCE(MY_ESS_CERT_ID_V2) = {
        ASN1_OPT(MY_ESS_CERT_ID_V2, hash_alg, X509_ALGOR),
        ASN1_SIMPLE(MY_ESS_CERT_ID_V2, hash, ASN1_OCTET_STRING),
} ASN1_SEQUENCE_END(MY_ESS_CERT_ID_V2)

ASN1_SEQUENCE(MY_ESS_SIGNING_CERT_V2) = {
        ASN1_SEQUENCE_OF(MY_ESS_SIGNING_CERT_V2, cert_ids, MY_ESS_CERT_ID_V2),
        ASN1_SEQUENCE_OF_OPT(MY_ESS_SIGNING_CERT_V2, policy_info, POLICYINFO)
} ASN1_SEQUENCE_END(MY_ESS_SIGNING_CERT_V2)

IMPLEMENT_ASN1_FUNCTIONS(MY_ESS_CERT_ID_V2)

IMPLEMENT_ASN1_FUNCTIONS(MY_ESS_SIGNING_CERT_V2)

// Ripped from cms_SignerInfo_content_sign in crypto/cms/cms_sd.c
void ssl::ComputeHashToSign(CMS_SignerInfo* si, BIO* chain, bool doWrapDigest, charbuff& hashToSign)
{
    ASN1_OBJECT* ctype;
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned hashlen;

    EVP_MD_CTX* mctx = EVP_MD_CTX_new();
    if (!cms_DigestAlgorithm_find_ctx(mctx, chain, getDigestAlgorithm(si)))
        goto Error;

    if (EVP_DigestFinal_ex(mctx, hash, &hashlen) <= 0)
        goto Error;

    if (!CMS_signed_add1_attr_by_NID(si, NID_pkcs9_messageDigest,
        V_ASN1_OCTET_STRING, hash, hashlen))
        goto Error;

    ctype = OBJ_nid2obj(NID_pkcs7_data);
    if (CMS_signed_add1_attr_by_NID(si, NID_pkcs9_contentType,
        V_ASN1_OBJECT, ctype, -1) <= 0)
        goto Error;

    compute_hash_to_sign(si, hash, hashlen);
    if (doWrapDigest)
    {
        // We also need to encode the digest in ANS1 structure
        encode_pkcs1(getDigestAlgorithm(si), hash, hashlen, hashToSign);
    }
    else
    {
        hashToSign.resize(hashlen);
        std::memcpy(hashToSign.data(), hash, hashlen);
    }

    return;

Error:
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "Error while computing the MessageDigest");
}

void ssl::WrapDigestPKCS1(const bufferview& hash, PdfHashingAlgorithm hashing, charbuff& output)
{
    unique_ptr<X509_ALGOR, decltype(&X509_ALGOR_free)> x509Algor(X509_ALGOR_new(), X509_ALGOR_free);
    if (x509Algor == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "Error X509_ALGOR_new");

    X509_ALGOR_set_md(x509Algor.get(), ssl::GetEVP_MD(hashing));
    encode_pkcs1(x509Algor.get(), (const unsigned char*)hash.data(), (unsigned)hash.size(), output);
}


// Ripped from cms_DigestAlgorithm_find_ctx in crypto/cms/cms_lib.c
int cms_DigestAlgorithm_find_ctx(EVP_MD_CTX* mctx, BIO* chain, X509_ALGOR* mdalg)
{
    int nid;
    auto mdoid = getASN1Object(mdalg);
    nid = OBJ_obj2nid(mdoid);
    // Look for digest type to match signature
    for (;;)
    {
        EVP_MD_CTX* mtmp;
        chain = BIO_find_type(chain, BIO_TYPE_MD);
        if (chain == nullptr)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "CMS_NO_MATCHING_DIGEST");

        BIO_get_md_ctx(chain, &mtmp);
        if (EVP_MD_CTX_type(mtmp) == nid
            // Workaround for broken implementations that use signature
            // algorithm OID instead of digest.
            || EVP_MD_pkey_type(EVP_MD_CTX_get0_md(mtmp)) == nid)
        {
            return EVP_MD_CTX_copy_ex(mctx, mtmp);
        }

        chain = BIO_next(chain);
    }
}

// Ripped from CMS_SignerInfo_sign in crypto/cms/cms_sd.c
void compute_hash_to_sign(CMS_SignerInfo* si, unsigned char hash[], unsigned& hashlen)
{
    EVP_MD_CTX* mctx = CMS_SignerInfo_get0_md_ctx(si);
    STACK_OF(X509_ATTRIBUTE)* signedAttrs = nullptr;
    unsigned char* buf = nullptr;
    unsigned len;
    const EVP_MD* sign_md;

    sign_md = EVP_get_digestbyobj(getDigestAlgorithm(si)->algorithm);
    if (EVP_DigestInit(mctx, sign_md) <= 0)
        goto Error;

    // Prepare the DER structure to sign, reordering attributes
    signedAttrs = getSignedAttributesCopy(si);
    len = (unsigned)ASN1_item_i2d((ASN1_VALUE*)signedAttrs, &buf,
        ASN1_ITEM_rptr(CMS_Attributes_Sign));
    sk_X509_ATTRIBUTE_free(signedAttrs);
    if (buf == nullptr)
        goto Error;

    // Compute the hash to be signed
    if (EVP_DigestUpdate(mctx, buf, len) <= 0)
        goto Error;
    OPENSSL_free(buf);

    if (EVP_DigestFinal(mctx, hash, &hashlen) <= 0)
        goto Error;

    EVP_MD_CTX_reset(mctx);
    return;

Error:
    OPENSSL_free(buf);
    EVP_MD_CTX_reset(mctx);
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OpenSSLError, "Error while computing the MessageDigest");
}

/* Ripped/adapted from crypto/rsa/rsa_sign.c
 * encode_pkcs1 encodes a DigestInfo prefix of hash |type| and digest |m|, as
 * described in EMSA-PKCS1-v1_5-ENCODE, RFC 3447 section 9.2 step 2. This
 * encodes the DigestInfo (T and tLen) but does not add the padding.
 */
void encode_pkcs1(X509_ALGOR* digestAlg,
    const unsigned char* m, unsigned int m_len, charbuff& outbuff)
{
    MY_X509_SIG sig;
    X509_ALGOR algor{ };
    ASN1_TYPE parameter{ };
    ASN1_OCTET_STRING digest{ };
    unsigned char* buf = nullptr;
    int len;

    sig.algor = digestAlg;
    sig.algor = &algor;
    sig.algor->algorithm = const_cast<ASN1_OBJECT*>(getASN1Object(digestAlg));
    parameter.type = V_ASN1_NULL;
    parameter.value.ptr = NULL;
    sig.algor->parameter = &parameter;

    sig.digest = &digest;
    sig.digest->data = const_cast<unsigned char*>(m);
    sig.digest->length = m_len;

    // This is the expansion of IMPLEMENT_ASN1_FUNCTIONS
    // NOTE: buf must be a local null pointer otherwise
    // the function will try to reuse the memory
    len = ASN1_item_i2d((ASN1_VALUE*)&sig, &buf, ASN1_ITEM_rptr(MY_X509_SIG));
    if (len < 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "EncodeDigestPKCS1: Out of memory");

    try
    {
        outbuff.resize(len);
    }
    catch (...)
    {
        OPENSSL_free(buf);
        throw;
    }

    std::memcpy(outbuff.data(), buf, len);
    OPENSSL_free(buf);
}

X509_ALGOR* getDigestAlgorithm(CMS_SignerInfo* si)
{
    EVP_PKEY* pkey;
    X509* cert;
    X509_ALGOR* digestAlgorithm;
    X509_ALGOR* signingAlgorithm;
    CMS_SignerInfo_get0_algs(si, &pkey, &cert, &digestAlgorithm, &signingAlgorithm);
    return digestAlgorithm;
}

const ASN1_OBJECT* getASN1Object(X509_ALGOR* alg)
{
    const ASN1_OBJECT* obj;
    X509_ALGOR_get0(&obj, nullptr, nullptr, alg);
    return obj;
}

STACK_OF(X509_ATTRIBUTE)* getSignedAttributesCopy(CMS_SignerInfo* si)
{
    STACK_OF(X509_ATTRIBUTE)* ret = nullptr;
    int count = CMS_signed_get_attr_count(si);
    for (int i = 0; i < count; i++)
    {
        auto attr = CMS_signed_get_attr(si, i);
        if (X509at_add1_attr(&ret, attr) == nullptr)
            goto Error;
    }

    return ret;

Error:
    sk_X509_ATTRIBUTE_free(ret);
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "GetSignedAttributes: Out of memory");
}

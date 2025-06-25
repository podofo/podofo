/**
 * SPDX-FileCopyrightText: (C) 2023 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef CMS_CONTEXT_H
#define CMS_CONTEXT_H

#include <chrono>

#include <podofo/main/PdfDeclarations.h>

extern "C"
{
    // Openssl forward declations
    struct x509_st;
    struct evp_pkey_st;
    struct CMS_ContentInfo_st;
    struct CMS_SignerInfo_st;
    struct bio_st;
}

namespace PoDoFo
{
    struct CmsContextParams
    {
        PdfHashingAlgorithm Hashing = PdfHashingAlgorithm::SHA256;
        bool SkipWriteMIMECapabilities = false;
        bool SkipWriteSigningTime = false;
        bool AddSigningCertificateV2 = false;
        bool DoWrapDigest = false;
        nullable<std::chrono::seconds> SigningTimeUTC;
    };

    /** This class computes a CMS signature according to RFC 5652
     */
    class CmsContext final
    {
    public:
        CmsContext();
        ~CmsContext();
    public:
        void Reset(const bufferview& cert, const CmsContextParams& parameters);
        void AppendData(const bufferview& data);
        void ComputeHashToSign(charbuff& hashToSign);
        void ComputeSignature(const bufferview& signedHash, charbuff& signature);
        void AddAttribute(const std::string_view& nid, const bufferview& attr, bool signedAttr, bool octetString);
    public:
        PdfSignatureEncryption GetEncryption() const { return m_encryption; }
    private:
        void loadX509Certificate(const bufferview& cert);
        void computeCertificateHash();
        void clear();
        void reset();
        void checkAppendStarted();
        void checkEnabledAddSignedAttributes();
        void checkEnabledAddUnsignedAttributes();
    private:
        CmsContext(const CmsContext&) = delete;
        CmsContext& operator=(const CmsContext&) = delete;
    private:
        enum class CmsContextStatus
        {
            Uninitialized = 0,
            Initialized,
            AppendingData,
            ComputedHash,
            ComputedSignature
        };
    private:
        CmsContextStatus m_status;
        CmsContextParams m_parameters;
        PdfSignatureEncryption m_encryption;
        struct x509_st* m_cert;
        charbuff m_certHash;
        struct CMS_ContentInfo_st* m_cms;
        struct CMS_SignerInfo_st* m_signer;
        struct bio_st* m_databio;
        struct bio_st* m_out;
    };
}

#endif // CMS_CONTEXT_H

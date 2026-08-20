// SPDX-FileCopyrightText: 2023 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#ifndef CMS_CONTEXT_H
#define CMS_CONTEXT_H

#include <chrono>

#include <podofo/main/PdfDeclarations.h>

extern "C"
{
    // OpenSSL forward declarations
    struct x509_st;
    struct evp_pkey_st;
    struct CMS_ContentInfo_st;
    struct CMS_SignerInfo_st;
    struct bio_st;
    // libxml2 forward declarations
    typedef struct _xmlNode xmlNode;
    typedef xmlNode* xmlNodePtr;
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

    /// This class computes a CMS signature according to RFC 5652
    class CmsContext final
    {
    public:
        CmsContext();
        ~CmsContext();
    public:
        void Reset(const bufferview& cert, const CmsContextParams& parameters);
        void AppendData(const bufferview& data);
        void ComputeHashToSign(charbuff& hashToSign);
        /// @param verify cross-check the supplied signed hash against the cached hash to sign
        void ComputeSignature(const bufferview& signedHash, charbuff& signature, bool verify);
        /// Validate the given date is within the certificate validity period
        void ValidateSigningDate(const std::chrono::seconds& date) const;
        void AddAttribute(const std::string_view& nid, const bufferview& attr, bool signedAttr, bool octetString);
        void Dump(xmlNodePtr elem, std::string& temp);
        void Restore(xmlNodePtr elem, charbuff& temp);
        unsigned GetSignedHashSize() const;
    public:
        PdfSigningAlgorithm GetSigningAlgorithm() const { return m_signingAlgorithm; }
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
        PdfSigningAlgorithm m_signingAlgorithm;
        struct x509_st* m_cert;
        charbuff m_certHash;
        // Cached hash to sign, used to verify the supplied signed hash
        charbuff m_hashToSign;
        struct CMS_ContentInfo_st* m_cms;
        struct CMS_SignerInfo_st* m_signer;
        struct bio_st* m_databio;
    };
}

#endif // CMS_CONTEXT_H

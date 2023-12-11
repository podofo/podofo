/**
 * SPDX-FileCopyrightText: (C) 2023 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_SIGNER_CMS_H
#define PDF_SIGNER_CMS_H

#include <chrono>
#include "PdfSigner.h"

extern "C"
{
    // OpenSSL forward 
    struct evp_pkey_st;
}

namespace PoDoFo
{
    class CmsContext;

    using PdfSigningService = std::function<void(bufferview hashToSign, bool dryrun, charbuff& signedHash)>;
    using PdfSignedHashHandler = std::function<void(bufferview signedhHash, bool dryrun)>;

    enum class PdfSignerCmsFlags
    {
        None = 0,
        ///< When supplying a PdfSigningService, specify if the service
        ///< expects a bare digest (the default), or if should be wrapped
        ///< in a ASN.1 structure with encryption and hashing type (PKCS#1 v1.5
        ///< encpasulation), and the signing service will just perform an
        ///< encryption with private key
        ServiceDoWrapDigest = 1,
        ///< When supplying an external PdfSigningService, specify if
        ///< the service should be called for a dry run
        ServiceDoDryRun = 2,
    };

    struct PdfSignerCmsParams
    {
        PdfSignatureType SignatureType = PdfSignatureType::PAdES_B;
        PdfEncryptionAlgorithm Encryption = PdfEncryptionAlgorithm::RSA;
        PdfHashingAlgorithm Hashing = PdfHashingAlgorithm::SHA256;
        PdfSigningService SigningService;
        nullable<std::chrono::seconds> SigningTimeUTC;
        PdfSignedHashHandler SignedHashHandler;
        PdfSignerCmsFlags Flags = PdfSignerCmsFlags::None;
    };

    /** This class computes a CMS signature according to RFC 5652
     */
    class PODOFO_API PdfSignerCms : public PdfSigner
    {
    public:
        /** Load X509 certificate and supply a ASN.1 encoded private key
         * \param cert x509 certificate
         * \param pkey asn.1 encoded private key. Can be empty. In that case
         * signing can be supplied by a signing service, or performing a sequential signing
         */
        PdfSignerCms(const bufferview& cert, const bufferview& pkey,
            const PdfSignerCmsParams& parameters = { });

        /** Load X509 certificate without supplying a private key
         * \param cert x509 certificate
         * \remarks signing can be supplied by a signing service, or performing a sequential signing
         */
        PdfSignerCms(const bufferview& cert, const PdfSignerCmsParams& parameters = { });

        ~PdfSignerCms();

    public:
        void AppendData(const bufferview& data) override;
        void ComputeSignature(charbuff& buffer, bool dryrun) override;
        void FetchIntermediateResult(charbuff& result) override;
        void ComputeSignatureSequential(const bufferview& processedResult, charbuff& contents, bool dryrun) override;
        void Reset() override;
        std::string GetSignatureFilter() const override;
        std::string GetSignatureSubFilter() const override;
        std::string GetSignatureType() const override;
        bool SkipBufferClear() const override;

        /** Add signed attribute from asn.1 encoded bytes
         * \remarks bytes will be parsed
         */
        void AddSignedAttribute(const std::string_view& nid, const bufferview& attr);
        /** Add unsigned attribute from asn.1 encoded bytes
         * \remarks bytes will be parsed
         */
        void AddUnsignedAttribute(const std::string_view& nid, const bufferview& attr);
        /** Add signed attribute as octet bytes
         */
        void AddSignedAttributeBytes(const std::string_view& nid, const bufferview& attr);
        /** Add unsigned attribute as octet bytes
          */
        void AddUnsignedAttributeBytes(const std::string_view& nid, const bufferview& attr);

    public:
        const PdfSignerCmsParams& GetParameters() const { return m_parameters; }

    private:
        void ensureEventBasedSigning();
        void ensureSequentialSigning();
        void checkContextInitialized();
        void ensureContextInitialized();
        void resetContext();
        void doSign(const bufferview& input, charbuff& output);
    private:
        nullable<bool> m_sequentialSigning;
        charbuff m_certificate;
        std::unique_ptr<CmsContext> m_cmsContext;
        struct evp_pkey_st* m_privKey;
        PdfSignerCmsParams m_parameters;

        // Temporary buffer variables
        // NOTE: Don't clear it in Reset() override
        charbuff m_encryptedHash;
    };
}

ENABLE_BITMASK_OPERATORS(PoDoFo::PdfSignerCmsFlags);

#endif // PDF_SIGNER_CMS_H

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

    struct PdfSignerCmsParams
    {
        PdfSignatureType SignatureType = PdfSignatureType::PAdES_B;
        PdfEncryptionAlgorithm Encryption = PdfEncryptionAlgorithm::RSA;
        PdfHashingAlgorithm Hashing = PdfHashingAlgorithm::SHA256;
        ///< When supplying a PdfSigningService, specify if the service
        ///< expects a bare digest (the default), or if should be wrapped
        ///< in the ASN.1 structure with encryption and hashing type, and
        ///< the signing service will just perform an encryption with private key
        bool DoWrapDigest = false;
        nullable<std::chrono::seconds> SigningTimeUTC;
    };

    using PdfSigningService = std::function<void(bufferview, bool, charbuff&)>;

    /** This class computes a CMS signature according to RFC 5652
     */
    class PODOFO_API PdfSignerCms : public PdfSigner
    {
    public:
        /** Load X509 certificate and provide a ASN.1 encoded private key
         */
        PdfSignerCms(const bufferview& cert, const bufferview& pkey,
            const PdfSignerCmsParams& parameters = { });

        /** Load X509 certificate and provide an external signing service
         */
        PdfSignerCms(const bufferview& cert, const PdfSigningService& signing,
            const PdfSignerCmsParams& parameters = { });

        ~PdfSignerCms();

    public:
        void AppendData(const bufferview& data) override;
        void ComputeSignature(charbuff& buffer, bool dryrun) override;
        void Reset() override;
        std::string GetSignatureFilter() const override;
        std::string GetSignatureSubFilter() const override;
        std::string GetSignatureType() const override;
        bool SkipBufferClear() const override;

    protected:
        virtual void OnSignedHashReady(const bufferview& signedhHash, bool dryrun);

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
    private:
        void checkContextInitialized();
        void ensureContextInitialized();
        void resetContext();
        void loadPrivateKey(const bufferview& pkey);
        void doEncrypt(const bufferview& input, charbuff& output, bool dryrun);
    private:
        charbuff m_certificate;
        PdfSigningService signingService;
        std::unique_ptr<CmsContext> m_cmsContext;
        struct evp_pkey_st* m_privKey;
        PdfSignerCmsParams m_parameters;

        // Temporary buffer variables
        // NOTE: Don't clear it in Reset() override
        charbuff m_encryptedHash;
    };
}

#endif // PDF_SIGNER_CMS_H

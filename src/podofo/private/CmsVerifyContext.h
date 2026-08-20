// SPDX-FileCopyrightText: 2026 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#ifndef CMS_VERIFY_CONTEXT_H
#define CMS_VERIFY_CONTEXT_H

#include <podofo/main/PdfDeclarations.h>

extern "C"
{
    // OpenSSL forward declarations
    struct CMS_ContentInfo_st;
    struct CMS_SignerInfo_st;
    struct stack_st_CMS_SignerInfo;
    struct bio_st;
}

namespace PoDoFo
{
    /// This class verifies a CMS signature according to RFC 5652 against
    /// the content that is streamed to it. It performs no certificate
    /// trust validation, which is a concern of the caller
    class CmsVerifyContext final
    {
    public:
        CmsVerifyContext();
        ~CmsVerifyContext();
    public:
        /// Parse the CMS structure and resolve the signer certificates
        /// @returns false if the input is not a valid CMS structure
        bool TryReset(const bufferview& cms);

        /// Load the signer that will be verified. Signers are independent
        /// signatures on the same content and are verified one by one
        /// @returns false if the layout of the signer is not supported
        bool TryLoadSigner(unsigned signerIndex);

        /// Append the signed content, as delimited by the /ByteRange
        void AppendData(const bufferview& data);

        /// Compare the digest of the appended content with the "messageDigest"
        /// signed attribute of the loaded signer, then verify its signature
        /// over the signed attributes
        /// @returns false if either check doesn't pass
        bool VerifySignature();

        /// Verify the "signingCertificateV2" signed attribute (RFC 5035) of the
        /// loaded signer against its certificate. The signer identifier and the
        /// certificates are not covered by the signature, so this attribute is
        /// the only thing that binds the signature to the certificate
        /// @param attrMissing set to true if the attribute is not present. It's
        /// not mandatory, eg. it's absent in legacy PKCS#7 signatures
        /// @returns true if the check passes, or if the attribute is missing
        bool TryVerifySigningCertificateV2(bool& attrMissing);
    public:
        /// The count of the signers in the CMS structure
        unsigned GetSignerCount() const;
    private:
        void checkReset() const;
        void checkSignerLoaded() const;
        void clear();
    private:
        CmsVerifyContext(const CmsVerifyContext&) = delete;
        CmsVerifyContext& operator=(const CmsVerifyContext&) = delete;
    private:
        struct CMS_ContentInfo_st* m_cms;
        struct stack_st_CMS_SignerInfo* m_signers;
        struct CMS_SignerInfo_st* m_signer;
        struct bio_st* m_databio;
    };
}

#endif // CMS_VERIFY_CONTEXT_H

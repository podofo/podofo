// SPDX-FileCopyrightText: 2026 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#ifndef PDF_SIGNER_DOC_TIMESTAMP_H
#define PDF_SIGNER_DOC_TIMESTAMP_H

#include "PdfSigner.h"

extern "C"
{
    // OpenSSL forward declaration
    struct evp_md_ctx_st;
    // libxml2 forward declaration
    typedef struct _xmlNode xmlNode;
    typedef xmlNode* xmlNodePtr;
}

namespace PoDoFo
{
    /// A service that obtains a RFC 3161 timestamp for the given document digest
    /// @param hash the document digest, to be used as the message imprint of the timestamp request
    /// @param dryrun if true, the output is not required to be a valid timestamp: it's
    ///     just used to infer its size. The hash is a dummy with the expected size
    /// @param timeStamp the DER encoded TimeStampResp or TimeStampToken
    using PdfTimeStampService = std::function<void(bufferview hash, bool dryrun, charbuff& timeStamp)>;

    enum class PdfSignerDocTimeStampFlags : uint32_t
    {
        None = 0,
        ///< Skip the verification of the message imprint of the supplied timestamp
        ///< against the document digest. The verification is performed by default,
        ///< and it's never performed during dry runs
        SkipVerification = 1,
    };

    struct PODOFO_API PdfSignerDocTimeStampParams final
    {
        PdfHashingAlgorithm Hashing = PdfHashingAlgorithm::SHA256;
        /// Service used in event based timestamping. It's not used in deferred timestamping
        PdfTimeStampService TimeStampService;
        /// The size reserved for the timestamp token in the signature /Contents.
        /// If 0, in event based timestamping the size is inferred by a dry run call to the
        /// timestamp service, in deferred timestamping PdfSignerDocTimeStamp::DefaultReservedSize is used
        unsigned ReservedSize = 0;
        PdfSignerDocTimeStampFlags Flags = PdfSignerDocTimeStampFlags::None;
    };

    /// This class computes a document timestamp (/Type /DocTimeStamp) with
    /// a RFC 3161 timestamp token, as described in ISO 32000-2:2020 12.8.5
    /// @remarks In deferred timestamping the intermediate result is the document digest,
    /// and the processed result is the DER encoded TimeStampResp or TimeStampToken
    class PODOFO_API PdfSignerDocTimeStamp : public PdfSigner
    {
        friend class PdfSigningContext;
    public:
        /// The size reserved for the timestamp token in deferred timestamping, when not specified
        static constexpr unsigned DefaultReservedSize = 8192;

    public:
        PdfSignerDocTimeStamp(const PdfSignerDocTimeStampParams& parameters = { });

        ~PdfSignerDocTimeStamp();

    public:
        void Reset() override;
        void AppendData(const bufferview& data) override;
        void ComputeSignature(charbuff& contents, bool dryrun) override;
        void FetchIntermediateResult(charbuff& result) override;
        void ComputeSignatureDeferred(const bufferview& processedResult, charbuff& contents, bool dryrun) override;
        std::string GetSignatureSubFilter() const override;
        std::string GetSignatureType() const override;

    public:
        const PdfSignerDocTimeStampParams& GetParameters() const { return m_parameters; }

    private:
        // Called by PdfSigningContext
        void Dump(xmlNodePtr signerElem, std::string& temp);
        void Restore(xmlNodePtr signerElem, charbuff& temp);
    private:
        void ensureEventBasedTimestamping();
        void ensureDeferredTimestamping();
        void ensureDigestInitialized();
        void resetDigest();
        void ensureHashComputed();
        void setContents(const bufferview& timeStamp, charbuff& contents);
        bool shouldVerify() const;
    private:
        nullable<bool> m_deferredTimestamping;
        PdfSignerDocTimeStampParams m_parameters;
        struct evp_md_ctx_st* m_digestCtx;
        // The document digest, empty until computed
        charbuff m_hash;

        // Temporary buffer variables
        // NOTE: Don't clear it in Reset() override
        charbuff m_timeStamp;
    };
}

ENABLE_BITMASK_OPERATORS(PoDoFo::PdfSignerDocTimeStampFlags);

#endif // PDF_SIGNER_DOC_TIMESTAMP_H

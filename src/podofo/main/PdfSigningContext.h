/**
 * SPDX-FileCopyrightText: (C) 2023 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_SIGNING_CONTEXT_H
#define PDF_SIGNING_CONTEXT_H

#include "PdfSigner.h"

namespace PoDoFo
{
    class PODOFO_API PdfSignerId final
    {
    public:
        PdfSignerId();
        PdfSignerId(const PdfReference& ref, unsigned signerIndex);

        const PdfReference& GetSignatureRef() const { return m_SignatureRef; }
        unsigned GetSignerIndex() const { return m_SignerIndex; }

        bool operator==(const PdfSignerId& rhs) const
        {
            return m_SignatureRef == rhs.m_SignatureRef && m_SignerIndex == rhs.m_SignerIndex;
        }

        bool operator!=(const PdfSignerId& rhs) const
        {
            return m_SignatureRef != rhs.m_SignatureRef || m_SignerIndex != rhs.m_SignerIndex;
        }

    private:
        PdfReference m_SignatureRef;
        unsigned m_SignerIndex;
    };
}

namespace std
{
    /** Overload hasher for PdfSignerId
     */
    template<>
    struct hash<PoDoFo::PdfSignerId>
    {
        std::size_t operator()(const PoDoFo::PdfSignerId& id) const noexcept
        {
            return id.GetSignatureRef().ObjectNumber() ^ (id.GetSignatureRef().GenerationNumber() << 16)
                ^ ((size_t)id.GetSignerIndex() << 24);
        }
    };
}

namespace PoDoFo
{
    /**
     * Interchange signing procedure results. Used when starting and finishing a deferred (aka "async") signing
     */
    struct PODOFO_API PdfSigningResults final
    {
        std::unordered_map<PdfSignerId, charbuff> Intermediate;
    };

    /**
     * A context that can be used to customize the signing process.
     * It also enables the deferred (aka "async") signing, which is a mean to separately process
     * the intermediate results of signing (normally a hash to sign) that doesn't
     * require a streamlined event based processing. It can be issued by starting
     * the process with StartSigning() and finishing it with FinishSigning()
     */
    class PODOFO_API PdfSigningContext final
    {
        friend PODOFO_API void SignDocument(PdfMemDocument& doc, StreamDevice& device, PdfSigner& signer,
            PdfSignature& signature, PdfSaveOptions saveOptions);
    public:
        PdfSigningContext();

        /** Restore a dumped signing context from an input stream device
         */
        std::unique_ptr<PdfMemDocument> Restore(std::shared_ptr<StreamDevice> device);

        /** Configure a signer on the specific signature field
         */
        PdfSignerId AddSigner(const PdfSignature& signature, std::shared_ptr<PdfSigner> signer);

        /** Start a blocking event-driven signing procedure
         */
        void Sign(PdfMemDocument& doc, StreamDevice& device, PdfSaveOptions options = PdfSaveOptions::None);

        /** Start a deferred (aka "async") signing procedure
         * \param results instance where intermediate results will be stored
         */
        void StartSigning(PdfMemDocument& doc, std::shared_ptr<StreamDevice> device, PdfSigningResults& results,
            PdfSaveOptions saveOptions = PdfSaveOptions::None);

        /** Finish a deferred (aka "async") signing procedure
         * \param processedResults results that will be used to finalize the signatures
         */
        void FinishSigning(const PdfSigningResults& processedResults);

        /** Dump the signing context so it can be resumed later
         * \remarks Can be used only after starting a deferred (aka "async") signing
         * operation. This will effectively disable further operations on this context
         */
        void DumpInPlace();

        /** Get the fist signer entry from the context for the given input signature
         */
        std::shared_ptr<PdfSigner> GetSignerEntry(const PdfReference& signatureRef);

        /** Get the fist signer entry from the context for the given input signature
         */
        std::shared_ptr<PdfSigner> GetSignerEntry(const std::string_view& fullName,
            PdfReference& signatureRef);

        bool IsEmpty() const;

    private:
        struct SignatureDescriptors
        {
            std::string FullName;
            unsigned PageIndex = 0u - 1u; // Necessary to correctly recover the PdfSignature field
            PdfSigner* Signer;
            std::shared_ptr<PdfSigner> SignerStorage; // Unnecessary for PoDoFo::SignDocument()
        };

        struct SignatureCtx
        {
            // Buffer for the final signature /Contents
            charbuff Contents;
            size_t BeaconSize = 0;
            PdfSignatureBeacons Beacons;
            PdfArray ByteRangeArr;
        };

        enum class Status
        {
            Config,     ///< The context is still configuring signers
            Started,    ///< A deferred signing operation has been started 
            Finished,   ///< A deferred signing operation has been finished 
            Dumped,     ///< The context has been dumped
            Restored,   ///< The context has been restored
        };

    private:
        // Used by PoDoFo::SignDocument
        void AddSignerUnsafe(const PdfSignature& signature, PdfSigner& signer);

    private:
        PdfSignerId addSigner(const PdfSignature& signature, PdfSigner* signer,
            std::shared_ptr<PdfSigner>&& storage);
        void ensureNotStarted() const;
        std::unordered_map<PdfReference, SignatureCtx> prepareSignatureContexts(PdfDocument& doc, bool deferredSigning);
        void saveDocForSigning(PdfMemDocument& doc, StreamDevice& device, PdfSaveOptions saveOptions);
        void appendDataForSigning(std::unordered_map<PdfReference, SignatureCtx>& contexts, StreamDevice& device,
            std::unordered_map<PdfSignerId, charbuff>* intermediateResults, charbuff& tmpbuff);
        void computeSignatures(std::unordered_map<PdfReference, SignatureCtx>& contexts,
            PdfDocument& doc, StreamDevice& device,
            const PdfSigningResults* processedResults, charbuff& tmpbuff);

    private:
        PdfSigningContext(const PdfSigningContext&) = delete;
        PdfSigningContext& operator==(const PdfSigningContext&) = delete;

    private:
        std::unordered_map<PdfReference, SignatureDescriptors> m_signatures;
        // Used during deferred signing
        PdfMemDocument* m_doc;
        std::shared_ptr<StreamDevice> m_device;
        std::unordered_map<PdfReference, SignatureCtx> m_contexts;
        Status m_status;
    };
}

#endif // PDF_SIGNING_CONTEXT_H

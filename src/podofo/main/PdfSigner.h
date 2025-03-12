/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_SIGNER_H
#define PDF_SIGNER_H

#include "PdfDeclarations.h"

#include "PdfMemDocument.h"
#include "PdfSignature.h"

namespace PoDoFo
{
    class StreamDevice;

    class PODOFO_API PdfSigner
    {
    public:
        PdfSigner();
        virtual ~PdfSigner();

        /**
         * Prepare the signer for being used/re-used
         * Called before computing the signature with ComputeSignature(buffer, false)
         * \remarks It's not meant to clear parameters that have been set
         *      on this signer
         */
        virtual void Reset() = 0;

        /**
         * Called incrementally with document raw data to compute
         * the signature with
         * \param data incremental raw data 
         */
        virtual void AppendData(const bufferview& data) = 0;

        /**
         * Called to compute the signature 
         * \param contents the buffer that will hold the signature /Contents
         * \param dryrun if true the buffer is not required to
         *   hold the signature, the call is just performed to
         *   infer the signature size
         * \remarks it must support working without prior calls to AppendData()
         */
        virtual void ComputeSignature(charbuff& contents, bool dryrun) = 0;

        /**
         * Retrieve the intermediate result of a signature computation,
         * most probably a hash to sign. Called on deferred (aka "async")
         * signature computation
         * \param buffer the buffer that will hold the intermediate result
         * \remarks by default it throws with PdfErrorCode::NotImplemented
         */
        virtual void FetchIntermediateResult(charbuff& result);

        /**
         * Called when computing the signature in deferred (aka "async") mode
         * \param processedResult the processed intermediate result, for example a signed hash
         * \param buffer the buffer that will hold the signature /Contents
         * \param dryrun if true the buffer is not required to
         *   hold the signature, the call is just performed to
         *   infer the signature size
         * \remarks by default it throws with PdfErrorCode::NotImplemented
         * \remarks it must support working without prior calls to AppendData() and/or FetchIntermediateResult() 
         */
        virtual void ComputeSignatureDeferred(const bufferview& processedResult, charbuff& contents, bool dryrun);

        /**
         * Determines if the buffer should not be cleared amid
         * ComputeSignature(contents, dryrun) calls. The default is false
         */
        virtual bool SkipBufferClear() const;

        /**
         * Should return the signature /Filter, for example "Adobe.PPKLite"
         */
        virtual std::string GetSignatureFilter() const;

        /**
         * Should return the signature /SubFilter, for example "ETSI.CAdES.detached"
         */
        virtual std::string GetSignatureSubFilter() const = 0;

        /**
         * Should return the signature /Type. It can be "Sig" or "DocTimeStamp"
         */
        virtual std::string GetSignatureType() const = 0;

    private:
        PdfSigner(const PdfSigner&) = delete;
        PdfSigner& operator=(const PdfSigner&) = delete;
    };

    /** Sign the document on the given signature field
     * \param doc the document to be signed
     * \param device the input/output device where the document will be saved
     * \param signer the signer implementation that will compute the signature
     * \param signature the signature field where the signature will be applied
     * \param options document saving options
     */
    PODOFO_API void SignDocument(PdfMemDocument& doc, StreamDevice& device, PdfSigner& signer,
        PdfSignature& signature, PdfSaveOptions saveOptions = PdfSaveOptions::None);
}

#endif // PDF_SIGNER_H

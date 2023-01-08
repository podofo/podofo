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
        virtual ~PdfSigner();

        /**
         * Called before computing the signature with ComputeSignature(buffer, false)
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
         * \param buffer The buffer that will hold the signature
         * \param dryrun If true the buffer is not required to
         *   hold the signature, the call is just performed to
         *   infer the signature size
         */
        virtual void ComputeSignature(charbuff& buffer, bool dryrun) = 0;

        /**
         * Determines if the buffer should not be cleared amid
         * ComputeSignature(buffer, dryrun) calls. The default is false
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

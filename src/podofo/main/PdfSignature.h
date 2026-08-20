// SPDX-FileCopyrightText: 2011 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2011 Petr Pytelka
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#ifndef PDF_SIGNATURE_H
#define PDF_SIGNATURE_H

#include "PdfDeclarations.h"

#include "PdfAnnotation.h"
#include "PdfField.h"
#include "PdfDate.h"
#include "PdfData.h"
#include <podofo/auxiliary/InputDevice.h>
#include <podofo/auxiliary/OutputDevice.h>

namespace PoDoFo {

class PdfAcroForm;

enum class PdfCertPermission : uint8_t
{
    NoPerms = 1,
    FormFill = 2,
    Annotations = 3,
};

struct PdfSignatureBeacons final
{
    PdfSignatureBeacons();
    charbuff ContentsBeacon;
    charbuff ByteRangeBeacon;
    std::shared_ptr<size_t> ContentsOffset;
    std::shared_ptr<size_t> ByteRangeOffset;
};

enum class PdfSignatureVerifyStatus : uint8_t
{
    ///< The signature could not be checked. Missing or invalid PKCS7/CMS contents,
    ///< unsupported signer info layout, or a malformed /ByteRange
    Indeterminate = 0,

    ///< The signature is cryptographically invalid: the signed bytes were modified,
    ///< or the signature doesn't correspond to the certificate
    Invalid,

    ///< The CMS signature is cryptographically valid over the signed bytes, but the
    ///< /ByteRange doesn't reach the end of the input: the document has content that
    ///< is not covered by the signature. The certificate trust status is indeterminate
    CryptoVerifiedPartialCoverage,

    ///< The CMS signature is cryptographically valid and the /ByteRange covers the
    ///< whole input. The certificate trust status is indeterminate
    CryptoVerified,
};

class PODOFO_API PdfSignature final : public PdfField
{
    friend class PdfField;
    friend class PdfSigningContext;

private:
    PdfSignature(PdfAcroForm& acroform, std::shared_ptr<PdfField>&& parent);

    PdfSignature(PdfAnnotationWidget& widget, std::shared_ptr<PdfField>&& parent);

    PdfSignature(PdfObject& obj, PdfAcroForm* acroform);

public:
    /// Verify the CMS signature in /Contents against the bytes delimited by the /ByteRange
    ///
    /// It performs no certificate trust validation, and it doesn't inspect the document
    /// beyond the signed revision: a complete verification is a document wide concern
    /// @param input the input device for the document that holds the signature
    /// @param status what was actually verified, assigned on return
    /// @returns true if the signature is cryptographically valid over the signed bytes,
    /// regardless of the /ByteRange coverage. Inspect the status to tell them apart
    bool TryVerifySignature(InputStreamDevice& input, PdfSignatureVerifyStatus& status) const;

    /// Set the signer name
    ///
    /// @param text the signer name
    void SetSignerName(nullable<const PdfString&> text);

    /// Set reason of the signature
    ///
    /// @param text the reason of signature
    void SetSignatureReason(nullable<const PdfString&> text);

    /// Set location of the signature
    ///
    /// @param text the location of signature
    void SetSignatureLocation(nullable<const PdfString&> text);

    [[deprecated("Use the SetCreatingApplication method instead")]]
    void SetSignatureCreator(nullable<const PdfString&> creator);

    /// Set the creator of the signature
    ///
    /// @param application the creator of the signature
    void SetCreatingApplication(nullable<const PdfName&> application);

    /// Date of signature
    void SetSignatureDate(nullable<const PdfDate&> sigDate);

    /// Add certification dictionaries and references to document catalog.
    ///
    /// @param perm document modification permission
    void AddCertificationReference(PdfCertPermission perm = PdfCertPermission::NoPerms);

    /// Get the signer name
    ///
    /// @returns the found signer object
    nullable<const PdfString&> GetSignerName() const;

    /// Get the reason of the signature
    ///
    /// @returns the found reason object
    nullable<const PdfString&> GetSignatureReason() const;

    /// Get the location of the signature
    ///
    /// @returns the found location object
    nullable<const PdfString&> GetSignatureLocation() const;

    /// Get the date of the signature
    ///
    /// @returns the found date object
    nullable<PdfDate> GetSignatureDate() const;

    /// Ensures that the signature field has set a signature object.
    /// The function does nothing, if the signature object is already
    /// set. This is useful for cases when the signature field had been
    /// created from an existing annotation, which didn't have it set.
    void EnsureValueObject();

    PdfSignature* GetParent();
    const PdfSignature* GetParent() const;

    /// Try retrieve the previous revision of the document before signing (if it occurred)
    /// @param input the input device for the document where to search
    /// @param output the output device that will hold the previous revision
    bool TryGetPreviousRevision(InputStreamDevice& input, OutputStreamDevice& output) const;

protected:
    PdfObject* getValueObject() const override;

private:
    /// Create space for signature
    ///
    /// Structure of the PDF file - before signing:
    /// <</ByteRange[ 0 1234567890 1234567890 1234567890]/Contents<signatureData>
    /// Have to be replaced with the following structure:
    /// <</ByteRange[ 0 count pos count]/Contents<real signature ...0-padding>
    ///
    /// @param filter /Filter for this signature
    /// @param subFilter /SubFilter for this signature
    /// @param type /Type for this signature
    /// @param beacons Shared sentinels that will updated
    ///                during writing of the document
    void PrepareForSigning(const std::string_view& filter,
        const std::string_view& subFilter,
        const std::string_view& type,
        const PdfSignatureBeacons& beacons);

    // To be called by SignDocument()
    void SetContentsByteRangeNoDirtySet(const bufferview& contents, PdfArray&& byteRange);

    void ensureValueObject();
private:
    void init(PdfAcroForm& acroForm);

private:
    PdfObject* m_ValueObj;
};

}

#endif // PDF_SIGNATURE_H

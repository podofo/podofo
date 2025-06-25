/**
 * SPDX-FileCopyrightText: (C) 2023 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfSignerCms.h"
#include <podofo/private/OpenSSLInternal.h>
#include <podofo/private/CmsContext.h>

using namespace std;
using namespace PoDoFo;

constexpr unsigned RSASignedHashSize = 256;

PdfSignerCms::PdfSignerCms(const bufferview& cert, const PdfSignerCmsParams& parameters) :
    PdfSignerCms(cert, { }, parameters)
{
}

PdfSignerCms::PdfSignerCms(const bufferview& cert, const bufferview& pkey,
        const PdfSignerCmsParams& parameters) :
    m_certificate(cert),
    m_privKey(nullptr),
    m_parameters(parameters),
    m_reservedSize(0)
{
    if (pkey.size() != 0)
        m_privKey = ssl::LoadPrivateKey(pkey);
}

PdfSignerCms::~PdfSignerCms()
{
    if (m_privKey != nullptr)
    {
        EVP_PKEY_free(m_privKey);
        m_privKey = nullptr;
    }
}

void PdfSignerCms::AppendData(const bufferview& data)
{
    ensureContextInitialized();
    m_cmsContext->AppendData(data);
}

void PdfSignerCms::ComputeSignature(charbuff& contents, bool dryrun)
{
    ensureEventBasedSigning();
    ensureContextInitialized();
    charbuff hashToSign;
    m_cmsContext->ComputeHashToSign(hashToSign);
    if (m_parameters.SigningService == nullptr)
    {
        // Do default signing
        doSign(hashToSign, m_encryptedHash);
    }
    else if (!dryrun || (m_parameters.Flags & PdfSignerCmsFlags::ServiceDoDryRun) != PdfSignerCmsFlags::None)
    {
        m_parameters.SigningService(hashToSign, dryrun, m_encryptedHash);
    }
    else
    {
        // Just prepare a fake result with the size of RSA block
        m_encryptedHash.resize(RSASignedHashSize);
    }

    if (m_parameters.SignedHashHandler != nullptr)
        m_parameters.SignedHashHandler(m_encryptedHash, dryrun);

    m_cmsContext->ComputeSignature(m_encryptedHash, contents);
    if (dryrun)
        tryEnlargeSignatureContents(contents);
}

void PdfSignerCms::FetchIntermediateResult(charbuff& result)
{
    ensureDeferredSigning();
    ensureContextInitialized();
    m_cmsContext->ComputeHashToSign(result);
}

void PdfSignerCms::ComputeSignatureDeferred(const bufferview& processedResult, charbuff& contents, bool dryrun)
{
    ensureDeferredSigning();
    ensureContextInitialized();

    if (dryrun)
    {
        // Just prepare a fake result with the size of RSA block
        charbuff fakeresult;
        m_cmsContext->ComputeHashToSign(fakeresult);
        fakeresult.resize(RSASignedHashSize);
        m_cmsContext->ComputeSignature(fakeresult, contents);
        tryEnlargeSignatureContents(contents);
    }
    else
    {
        m_cmsContext->ComputeSignature(processedResult, contents);
    }
}

void PdfSignerCms::Reset()
{
    if (m_cmsContext != nullptr)
        resetContext();

    // NOTE: Don't reset the reserved size or any other parameter
    // that has been set. In particular we need the reserved size
    // to determine the final size of the CMS block when we do
    // a dry-run

    // Reset also deferred signing if it was started
    m_deferredSigning = nullptr;
}

string PdfSignerCms::GetSignatureFilter() const
{
    return "Adobe.PPKLite";
}

string PdfSignerCms::GetSignatureSubFilter() const
{
    switch (m_parameters.SignatureType)
    {
        case PdfSignatureType::PAdES_B:
            return "ETSI.CAdES.detached";
        case PdfSignatureType::Pkcs7:
            return "adbe.pkcs7.detached";
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Unsupported signature type");
    }
}

string PdfSignerCms::GetSignatureType() const
{
    return "Sig";
}

bool PdfSignerCms::SkipBufferClear() const
{
    // We do pre-allocation semantics, so don't need to clear the buffer
    return true;
}

void PdfSignerCms::AddAttribute(const string_view& nid, const bufferview& attr, PdfSignatureAttributeFlags flags)
{
    ensureContextInitialized();
    bool signedAttr = false;
    if ((flags & PdfSignatureAttributeFlags::SignedAttribute) != PdfSignatureAttributeFlags::None)
        signedAttr = true;

    bool asOctetString = false;
    if ((flags & PdfSignatureAttributeFlags::AsOctetString) != PdfSignatureAttributeFlags::None)
        asOctetString = true;

    m_cmsContext->AddAttribute(nid, attr, signedAttr, asOctetString);
}

void PdfSignerCms::ReserveAttributeSize(unsigned attrSize)
{
    // Increment the size to reserve size plus some constant
    // necessary for the ASN.1 infrastructure to make room
    // for the attribute
    m_reservedSize += (attrSize + 40);
}

void PdfSignerCms::ensureEventBasedSigning()
{
    if (m_deferredSigning.has_value())
    {
        if (*m_deferredSigning)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "The signer is enabled for deferred signing");
    }
    else
    {
        if (m_parameters.SigningService == nullptr && m_privKey == nullptr)
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic,
                "The signer can't perform event based signing without a signing service or a private pkey");
        }

        m_deferredSigning = false;
    }
}

void PdfSignerCms::ensureDeferredSigning()
{
    if (m_deferredSigning.has_value())
    {
        if (!*m_deferredSigning)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "The signer is not enabled for deferred signing");
    }
    else
    {
        m_deferredSigning = true;
    }
}

void PdfSignerCms::checkContextInitialized()
{
    if (m_cmsContext == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "CMS context is uninitialized");
}

void PdfSignerCms::ensureContextInitialized()
{
    if (m_cmsContext != nullptr)
        return;

    m_cmsContext.reset(new CmsContext());
    resetContext();
}

void PdfSignerCms::resetContext()
{
    CmsContextParams params;
    params.Hashing = m_parameters.Hashing;
    params.SigningTimeUTC = m_parameters.SigningTimeUTC;
    switch (m_parameters.SignatureType)
    {
        case PdfSignatureType::PAdES_B:
            params.AddSigningCertificateV2 = true;
            params.SkipWriteMIMECapabilities = true;
            params.SkipWriteSigningTime = true;
            break;
        case PdfSignatureType::Pkcs7:
            params.AddSigningCertificateV2 = false;
            params.SkipWriteMIMECapabilities = false;
            params.SkipWriteSigningTime = false;
            break;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Unsupported signature type");
    }

    if (m_privKey == nullptr)
    {
        params.DoWrapDigest = (m_parameters.Flags & PdfSignerCmsFlags::ServiceDoWrapDigest) != PdfSignerCmsFlags::None;
    }
    else
    {
        if (EVP_PKEY_base_id(m_privKey) == EVP_PKEY_RSA)
        {
            // An encryption with a private RSA keys always requires the
            // digest to be PKCS#1 wrapped
            params.DoWrapDigest = true;
        }
    }

    m_cmsContext->Reset(m_certificate, params);
}

void PdfSignerCms::doSign(const bufferview& input, charbuff& output)
{
    PODOFO_ASSERT(m_privKey != nullptr);
    return ssl::DoSign(input, m_privKey, PdfHashingAlgorithm::Unknown, output);
}

void PdfSignerCms::tryEnlargeSignatureContents(charbuff& contents)
{
    if (m_cmsContext->GetEncryption() == PdfSignatureEncryption::ECDSA)
    {
        // Unconditionally account for 2 slack bytes due to random nature of ECDSA
        contents.resize(contents.size() + 2 + m_reservedSize);
    }
    else
    {
        if (m_reservedSize != 0)
            contents.resize(contents.size() + m_reservedSize);
    }
}

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

PdfSignerCms::PdfSignerCms(const bufferview& cert, const bufferview& pkey,
    const PdfSignerCmsParams& parameters) :
    PdfSignerCms(cert, [&](bufferview input, bool dryrun, charbuff& output)
        {
            return doEncrypt(input, output, dryrun);
        }, parameters)
{
    loadPrivateKey(pkey);
}

PdfSignerCms::PdfSignerCms(const bufferview& cert, const PdfSigningService& signing,
        const PdfSignerCmsParams& parameters) :
    m_certificate(cert),
    signingService(signing),
    m_privKey(nullptr),
    m_parameters(parameters)
{
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

void PdfSignerCms::ComputeSignature(charbuff& output, bool dryrun)
{
    ensureContextInitialized();
    charbuff hashToSign;
    m_cmsContext->ComputeHashToSign(hashToSign);

    signingService(hashToSign, dryrun, m_encryptedHash);

    OnSignedHashReady(m_encryptedHash, dryrun);

    m_cmsContext->ComputeSignature(m_encryptedHash, output);
}

void PdfSignerCms::Reset()
{
    if (m_cmsContext != nullptr)
        resetContext();
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

void PdfSignerCms::OnSignedHashReady(const bufferview& signedhHash, bool dryrun)
{
    (void)signedhHash;
    (void)dryrun;
    // Do nothing
}

void PdfSignerCms::AddSignedAttribute(const string_view& nid, const bufferview& attr)
{
    checkContextInitialized();
    m_cmsContext->AddSignedAttribute(nid, attr);
}

void PdfSignerCms::AddUnsignedAttribute(const string_view& nid, const bufferview& attr)
{
    checkContextInitialized();
    m_cmsContext->AddUnsignedAttribute(nid, attr);
}

void PdfSignerCms::AddSignedAttributeBytes(const string_view& nid, const bufferview& attr)
{
    checkContextInitialized();
    m_cmsContext->AddSignedAttributeBytes(nid, attr);
}

void PdfSignerCms::AddUnsignedAttributeBytes(const string_view& nid, const bufferview& attr)
{
    checkContextInitialized();
    m_cmsContext->AddUnsignedAttributeBytes(nid, attr);
}

void PdfSignerCms::checkContextInitialized()
{
    if (m_cmsContext == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "CMS context is unitialized");
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
    params.Encryption = m_parameters.Encryption;
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
        params.DoWrapDigest = m_parameters.DoWrapDigest;
    else
        params.DoWrapDigest = true; // We just perform encryption with private key, so we expect the digest wrapped

    m_cmsContext->Reset(m_certificate, params);
}

void PdfSignerCms::loadPrivateKey(const bufferview& pkey)
{
    const unsigned char* data = (const unsigned char*)pkey.data();
    m_privKey = d2i_PrivateKey(EVP_PKEY_RSA, nullptr, &data, (long)pkey.size());
    if (m_privKey == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidKey, "Unable to load private key");
}

void PdfSignerCms::doEncrypt(const bufferview& input, charbuff& output, bool dryrun)
{
    (void)dryrun;
    return ssl::RsaRawEncrypt(input, output, m_privKey);
}

/** NSS signature generator
 *
 * Mozilla has two APIs for generating signatures (older SEC_PKCS7)
 * and newer SMIME (CMS). We are using newer API. 
 *
 * You have to have certificate (CERTCertificate * )which will be 
 * used for signing.
 */

#include "NSSSignatureGenerator.h"

#include <podofo.h>

void NSSSignatureGenerator::sm_write_stream(void *arg, const char *buf, unsigned long len) 
{
    NSSSignatureGenerator* pThis = static_cast<NSSSignatureGenerator*>(arg);
    pThis->signature.append(buf, len);
}

SECOidTag NSSSignatureGenerator::getDigestAlgor(CERTCertificate *pCert)
{
    SECAlgorithmID *algID = &pCert->signature;
    if(algID==NULL) return SEC_OID_MD5;
    SECOidTag algOIDTag = SECOID_FindOIDTag(&algID->algorithm);
    switch(algOIDTag)
    {
		case SEC_OID_MD5:
		case SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION:
		case SEC_OID_PKCS5_PBE_WITH_MD5_AND_DES_CBC:
			return SEC_OID_MD5;
		case SEC_OID_SHA1:
		case SEC_OID_ISO_SHA_WITH_RSA_SIGNATURE:
		case SEC_OID_PKCS1_SHA1_WITH_RSA_ENCRYPTION:
		case SEC_OID_PKCS5_PBE_WITH_SHA1_AND_DES_CBC:
		case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC4:
		case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC4:
		case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_TRIPLE_DES_CBC:
		case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC2_CBC:
		case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC:
		case SEC_OID_PKCS12_RSA_SIGNATURE_WITH_SHA1_DIGEST:
		case SEC_OID_ANSIX9_DSA_SIGNATURE_WITH_SHA1_DIGEST:
		case SEC_OID_BOGUS_DSA_SIGNATURE_WITH_SHA1_DIGEST:
		case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_128_BIT_RC4:
		case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_40_BIT_RC4:
		case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_3KEY_TRIPLE_DES_CBC:
		case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_2KEY_TRIPLE_DES_CBC:
		case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_128_BIT_RC2_CBC:
		case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC:
		case SEC_OID_HMAC_SHA1:
			return SEC_OID_SHA1;
		case SEC_OID_SHA256:
		case SEC_OID_PKCS1_SHA256_WITH_RSA_ENCRYPTION:
		case SEC_OID_ANSIX962_ECDSA_SHA256_SIGNATURE:
		case SEC_OID_HMAC_SHA256:
			return SEC_OID_SHA256;
		case SEC_OID_SHA384:
		case SEC_OID_PKCS1_SHA384_WITH_RSA_ENCRYPTION:
		case SEC_OID_ANSIX962_ECDSA_SHA384_SIGNATURE:
		case SEC_OID_HMAC_SHA384:
			return SEC_OID_SHA384;
		case SEC_OID_SHA512:
		case SEC_OID_PKCS1_SHA512_WITH_RSA_ENCRYPTION:
		case SEC_OID_ANSIX962_ECDSA_SHA512_SIGNATURE:
		case SEC_OID_HMAC_SHA512:
			return SEC_OID_SHA512;
    }
    PODOFO_ASSERT(0);
    return SEC_OID_MD5;
}

// create message with signature
NSSCMSMessage* NSSSignatureGenerator::createSign(CERTCertificate *cert)
{
    NSSCMSSignedData *sigd = NULL;
    NSSCMSContentInfo *cinfo = NULL;
    NSSCMSSignerInfo *signerinfo = NULL;
    
    SECOidTag hash=getDigestAlgor(cert);
    
    NSSCMSMessage *cmsg = NSS_CMSMessage_Create(NULL); // create a message on its own pool
    if (cmsg == NULL) return NULL;
    if ((sigd = NSS_CMSSignedData_Create(cmsg)) == NULL) {
        NSS_CMSMessage_Destroy(cmsg); return NULL;
    }
    cinfo = NSS_CMSMessage_GetContentInfo(cmsg);
    if (NSS_CMSContentInfo_SetContent_SignedData(cmsg, cinfo, sigd) != SECSuccess) {
        NSS_CMSMessage_Destroy(cmsg); return NULL;
    }
    
    // if !detatched, the contentinfo will alloc a data item for us
    cinfo = NSS_CMSSignedData_GetContentInfo(sigd);
    if (NSS_CMSContentInfo_SetContent_Data(cmsg, cinfo, NULL, PR_TRUE ) != SECSuccess) {
        NSS_CMSMessage_Destroy(cmsg); return NULL;
    }
    
    signerinfo = NSS_CMSSignerInfo_Create(cmsg, cert, hash);
    if (signerinfo == NULL) { NSS_CMSMessage_Destroy(cmsg); return NULL; }
    
    // we want the cert chain included for this one
    if (NSS_CMSSignerInfo_IncludeCerts(signerinfo, NSSCMSCM_CertChainWithRoot, certUsageEmailSigner) != SECSuccess) 
    { NSS_CMSMessage_Destroy(cmsg); return NULL; }
    
    // SMIME RFC says signing time should always be added
    if (NSS_CMSSignerInfo_AddSigningTime(signerinfo, PR_Now()) != SECSuccess) { NSS_CMSMessage_Destroy(cmsg); return NULL; }
    
    if (NSS_CMSSignedData_AddSignerInfo(sigd, signerinfo) != SECSuccess) {
        NSS_CMSMessage_Destroy(cmsg); return NULL; }
    return cmsg;
}


NSSSignatureGenerator::NSSSignatureGenerator(CERTCertificate *pCert) 
    :pCert(pCert)
{
    pSignature = NULL;
}

NSSSignatureGenerator::~NSSSignatureGenerator() 
{
    delete pSignature;
    if(enc!=NULL) NSS_CMSEncoder_Cancel(enc);
    if(cmsg!=NULL) NSS_CMSMessage_Destroy(cmsg);
}

bool NSSSignatureGenerator::init()
{
    cmsg = createSign(pCert);
    if(cmsg==NULL) return false;
    // Prepare encoder
    enc = NSS_CMSEncoder_Start(cmsg,
                               sm_write_stream, this, // DER output callback
                               NULL, NULL,     // destination storage
                               NULL, NULL,	   // password callback
                               NULL, NULL,     // decrypt key callback
                               NULL, NULL );   // detached digests
}

bool NSSSignatureGenerator::appendData(const char *pData, unsigned int dataSize)
{
    if(enc == NULL) return false;
    
    if (NSS_CMSEncoder_Update(enc, pData, dataSize) != SECSuccess) {
        NSS_CMSEncoder_Cancel(enc);
        enc = NULL;
        return false;
    }
    return true;
}

bool NSSSignatureGenerator::finishData() 
{
    if(enc == NULL) return false;
    
    if (NSS_CMSEncoder_Finish(enc) != SECSuccess) {
        enc = NULL;
        return false;
    }
    enc = NULL;
    return true;
}

const PoDoFo::PdfData* NSSSignatureGenerator::getSignature() 
{
    if(pSignature==NULL) {
        pSignature = new PoDoFo::PdfData(signature.c_str(), signature.size());
    }
    return pSignature;
}

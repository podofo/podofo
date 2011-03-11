/** NSS signature generator
 *
 * Mozilla has two APIs for generating signatures (older SEC_PKCS7)
 * and newer SMIME (CMS). We are using newer API. 
 *
 * You have to have certificate (CERTCertificate * )which will be 
 * used for signing.
 */

#ifndef _NSS_SIGNATURE_GENERATOR_H_
#define _NSS_SIGNATURE_GENERATOR_H_

#include <cms.h>
#include <cert.h>
#include <secoid.h>
#include <string>

#include "SignatureGenerator.h"

namespace PoDoFo {
    class PdfData;
};

class NSSSignatureGenerator : public SignatureGenerator
{
private:
    PoDoFo::PdfData *pSignature;
	CERTCertificate *pCert;
	NSSCMSMessage *cmsg;
	NSSCMSEncoderContext *enc;
	std::string signature;

	static void sm_write_stream(void *arg, const char *buf, unsigned long len);

protected:
    // get digest algoritm for the signing algoritm
	static SECOidTag getDigestAlgor(CERTCertificate *pCert);

	// create message with signature
	static NSSCMSMessage *createSign(CERTCertificate *cert);

public:
	NSSSignatureGenerator(CERTCertificate *pCert);
	virtual ~NSSSignatureGenerator();

	virtual bool init();

	virtual bool appendData(const char *pData, unsigned int dataSize);
    virtual bool finishData();
    virtual const PoDoFo::PdfData *getSignature();
};

#endif // _NSS_SIGNATURE_GENERATOR_H_


#ifndef _SIMPLE_SIGNATURE_GENERATOR_H_
#define _SIMPLE_SIGNATURE_GENERATOR_H_

#include "SignatureGenerator.h"

#include <podofo.h>

/** Simple signature generator
 */
class SimpleSignatureGenerator
	:public SignatureGenerator
{
    PoDoFo::PdfData *pSignature;
public:
	SimpleSignatureGenerator() {
		pSignature = NULL;
	}
	virtual ~SimpleSignatureGenerator() {
		delete pSignature;
	}
	virtual bool appendData(const char * /*pData*/, unsigned int /*dataSize*/)
	{
		return true;
	}
	virtual bool finishData() {
		pSignature = new PoDoFo::PdfData("My-Test-Signature");
		return true;
	}
	virtual const PoDoFo::PdfData *getSignature() {
		return pSignature;
	}
};

#endif // _SIMPLE_SIGNATURE_GENERATOR_H_

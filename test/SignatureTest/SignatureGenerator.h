
#ifndef _SIGNATURE_GENERATOR_H_
#define _SIGNATURE_GENERATOR_H_

namespace PoDoFo {
    class PdfData;
};

/** Abstract class for signature generator
 */
class SignatureGenerator {
public:
	virtual bool appendData(const char* pData, unsigned int dataSize)=0;
	virtual bool finishData()=0;
	virtual const PoDoFo::PdfData* getSignature()=0;
};

#endif // _SIGNATURE_GENERATOR_H_

#ifndef PODOFO_TEST_CONTENTPARSER_H
#define PODOFO_TEST_CONTENTPARSER_H

#include "PdfTokenizer.h"
#include "PdfVariant.h"

enum EPdfContentsType {
    ePdfContentsType_Keyword,
    ePdfContentsType_Variant
};

class PdfContentsTokenizer : public PoDoFo::PdfTokenizer {
public:
    PdfContentsTokenizer( const char* pBuffer, long lLen )
        : PoDoFo::PdfTokenizer( pBuffer, lLen )
    {
        
    }

    virtual ~PdfContentsTokenizer() { }
    
    /** Read the next keyword or variant
     *
     *  \param peType will be set to either keyword or variant
     *  \param ppszKeyword if pType is set to ePdfContentsType_Keyword this will point to the keyword
     *  \param rVariant if pType is set to ePdfContentsType_Variant this will be set to the read variant
     *
     */
    void ReadNext( EPdfContentsType* peType, const char** ppszKeyword, PoDoFo::PdfVariant & rVariant );
};

#endif //PODOFO_TEST_CONTENTPARSER_H

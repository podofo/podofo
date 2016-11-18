#include "PdfDefines.h"
#include "PdfLocale.h"
#include "PdfError.h"
#include "PdfDefinesPrivate.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

namespace PoDoFo {

void PdfLocaleImbue(std::ios_base& s)
{
#if USE_CXX_LOCALE
    static const std::locale cachedLocale( PdfIOLocale );
    try {
    	s.imbue( cachedLocale );
    } catch (const std::runtime_error & e) {
        std::ostringstream err;
        err << "Failed to set safe locale on stream being used for PDF I/O.";
        err << "Locale set was: \"" << PdfIOLocale << "\".";
        err << "Error reported by STL std::locale: \"" << e.what() << "\"";
        // The info string is copied by PdfError so we're ok to just:
        PODOFO_RAISE_ERROR_INFO(
            ePdfError_InvalidDeviceOperation,
            err.str().c_str()
            );
    }
#endif
}

};

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
        std::ostringstream s;
        s << "Failed to set safe locale on stream being used for PDF I/O.";
        s << "Locale set was: \"" << PdfIOLocale << "\".";
        s << "Error reported by STL std::locale: \"" << e.what() << "\"";
        // The info string is copied by PdfError so we're ok to just:
        PODOFO_RAISE_ERROR_INFO(
            ePdfError_InvalidDeviceOperation,
            s.str().c_str()
            );
    }
#endif
}

};

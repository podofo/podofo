/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_EXTENSION_H
#define PDF_EXTENSION_H

#include "PdfDeclarations.h"

namespace PoDoFo {
    
    /** PdfExtension is a simple class that describes a vendor-specific extension to
     *  the official specifications.
     */
    class PODOFO_API PdfExtension
    {
    public:
        PdfExtension(const std::string_view& ns, PdfVersion baseVersion, int64_t level);
        
        inline const std::string& GetNamespace() const { return m_Ns; }
        PdfVersion GetBaseVersion() const { return m_BaseVersion; }
        int64_t GetLevel() const { return m_Level; }
        
    private:
        std::string m_Ns;
        PdfVersion m_BaseVersion;
        int64_t m_Level;
    };
}

#endif	// PDF_EXTENSION_H

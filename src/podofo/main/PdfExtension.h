/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_EXTENSION_H
#define PDF_EXTENSION_H

#include "PdfName.h"
#include "PdfString.h"

namespace PoDoFo
{    
    /** PdfExtension is a simple class that describes a vendor-specific extension to
     *  the official specifications.
     */
    class PODOFO_API PdfExtension final
    {
    public:
        PdfExtension(const PdfName& ns, int64_t level, PdfVersion baseVersion = PdfVersion::Unknown,
            nullable<const PdfString&> url = { }, nullable<const PdfString&> extensionRevision = { });

    public:
        PdfVersion GetBaseVersion() const { return m_BaseVersion; }
        const PdfName& GetNamespace() const { return m_Namespace; }
        int64_t GetLevel() const { return m_Level; }
        /** A URL that refers to the documentation for this extension (PDF2.0)
         */
        const nullable<PdfString>& GetUrl() const { return m_Url; }
        /** An optional text string that provides additional revision
         * information on the extension level being used (PDF2.0)
         */
        const nullable<PdfString>& GetExtensionRevision() const { return m_ExtensionRevision; }
        
    private:
        PdfVersion m_BaseVersion;
        PdfName m_Namespace;
        int64_t m_Level;
        nullable<PdfString> m_Url;
        nullable<PdfString> m_ExtensionRevision;
    };
}

#endif // PDF_EXTENSION_H

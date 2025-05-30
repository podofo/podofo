/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_METADATA_STORE
#define PDF_METADATA_STORE

#include "PdfString.h"
#include "PdfDate.h"

namespace PoDoFo
{
    class PODOFO_API PdfMetadataStore final
    {
    public:
        PdfMetadataStore();
    public:
        nullable<PdfString> Title;
        nullable<PdfString> Author;
        nullable<PdfString> Subject;
        nullable<PdfString> Keywords;
        nullable<PdfString> Creator;
        nullable<PdfString> Producer;
        nullable<PdfDate> CreationDate;
        nullable<PdfDate> ModDate;
        nullable<bool> Trapped;
        PdfVersion Version;
        PdfALevel PdfaLevel;
        PdfUALevel PdfuaLevel;
    public:
        const PdfString* GetMetadata(PdfAdditionalMetadata prop) const;
        void SetMetadata(PdfAdditionalMetadata prop, const PdfString* value);
        void Reset();
    public:
        const std::unordered_map<PdfAdditionalMetadata, PdfString>* GetAdditionalMetadata() const { return m_additionalMetadata.get(); }
    private:
        std::unique_ptr<std::unordered_map<PdfAdditionalMetadata, PdfString>> m_additionalMetadata;
    };
}

namespace std
{
    template<>
    struct hash<PoDoFo::PdfAdditionalMetadata>
    {
        size_t operator()(PoDoFo::PdfAdditionalMetadata prop) const noexcept
        {
            return (size_t)prop;
        }
    };
}

#endif // PDF_METADATA_STORE

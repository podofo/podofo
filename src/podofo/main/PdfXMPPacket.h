/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_XMP_PACKET
#define PDF_XMP_PACKET

#include "PdfMetadataStore.h"

extern "C"
{
    typedef struct _xmlDoc xmlDoc;
    typedef xmlDoc* xmlDocPtr;
    typedef struct _xmlNode xmlNode;
    typedef xmlNode* xmlNodePtr;
}

namespace PoDoFo
{
    class PdfXMPPacket;

    class PODOFO_API PdfXMPProperty final
    {
        enum PropStatus
        {
            None = 0,
            Invalid = 1,
            Duplicated = 2,
        };

        friend class PdfXMPPacket;
    private:
        PdfXMPProperty();
    public:
        const std::string& GetName() const { return Name; }
        const std::string& GetNamespace() const { return Namespace; }
        const std::string& GetPrefix() const { return Prefix; }
        std::string GetPrefixedName() const;
        bool IsValid() const;
        bool IsDuplicated() const;
    private:
        std::string Name;
        std::string Namespace;
        std::string Prefix;
        PropStatus Status;
    };

    class PODOFO_API PdfXMPPacket final
    {
    public:
        PdfXMPPacket();
        PdfXMPPacket(const PdfXMPPacket&) = delete;
        ~PdfXMPPacket();

        static std::unique_ptr<PdfXMPPacket> Create(const std::string_view& xmpview);

    public:
        PdfMetadataStore GetMetadata() const;
        void GetMetadata(PdfMetadataStore& metadata) const;
        void SetMetadata(const PdfMetadataStore& metadata);
        void ToString(std::string& str) const;
        std::string ToString() const;
        /** Remove invalid properties based on specific PDF/A level
         */
        void PruneInvalidProperties(PdfALevel level, const std::function<void(const PdfXMPProperty& prop)>& warnings = nullptr);
#if PODOFO_3RDPARTY_INTEROP_ENABLED
        void PruneInvalidProperties(PdfALevel level, const std::function<void(const PdfXMPProperty& prop, xmlNodePtr)>& warnings);
        xmlDocPtr GetDoc() { return m_Doc; }
        xmlNodePtr GetOrCreateDescription();
        xmlNodePtr GetDescription() { return m_Description; }
#endif // PODOFO_3RDPARTY_INTEROP_ENABLED

    public:
        PdfXMPPacket& operator=(const PdfXMPPacket&) = delete;

    private:
        PdfXMPPacket(xmlDocPtr doc, xmlNodePtr xmpmeta);

    private:
        xmlDocPtr m_Doc;
        xmlNodePtr m_XMPMeta;
        xmlNodePtr m_Description;
    };
}

#endif // PDF_XMP_PACKET

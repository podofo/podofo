/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "XmlUtils.h"
#include <podofo/main/PdfMetadataStore.h>

namespace PoDoFo
{
    enum class XMPNamespaceKind : uint8_t
    {
        Dc,
        Pdf,
        Xmp,
        PdfAId,
        PdfUAId,
        PdfAExtension,
        PdfASchema,
        PdfAProperty,
        PdfAType,
    };

    /**
     * \remarks the store is not cleared: the function sets only read properties
     */
    void GetXMPMetadata(xmlNodePtr description, PdfMetadataStore& metadata);
    void SetXMPMetadata(xmlDocPtr doc, xmlNodePtr description, const PdfMetadataStore& metadata);
    void PruneInvalidProperties(xmlDocPtr doc, xmlNodePtr description, PdfALevel level,
        const std::function<void(std::string_view, xmlNodePtr)>& reportWarnings);
    void GetXMPNamespacePrefix(XMPNamespaceKind ns, std::string_view& href, std::string_view& prefix);

    constexpr std::string_view operator""_ns(const char* name, size_t length)
    {
        using namespace std;
        if (std::char_traits<char>::compare(name, "dc", length) == 0)
            return "http://purl.org/dc/elements/1.1/"sv;
        else if (std::char_traits<char>::compare(name, "pdf", length) == 0)
            return "http://ns.adobe.com/pdf/1.3/"sv;
        else if (std::char_traits<char>::compare(name, "xmp", length) == 0)
            return "http://ns.adobe.com/xap/1.0/"sv;
        else if (std::char_traits<char>::compare(name, "pdfaid", length) == 0)
            return "http://www.aiim.org/pdfa/ns/id/"sv;
        else if (std::char_traits<char>::compare(name, "pdfuaid", length) == 0)
            return "http://www.aiim.org/pdfua/ns/id/"sv;
        else if (std::char_traits<char>::compare(name, "pdfaExtension", length) == 0)
            return "http://www.aiim.org/pdfa/ns/extension/"sv;
        else if (std::char_traits<char>::compare(name, "pdfaSchema", length) == 0)
            return "http://www.aiim.org/pdfa/ns/schema#"sv;
        else if (std::char_traits<char>::compare(name, "pdfaProperty", length) == 0)
            return "http://www.aiim.org/pdfa/ns/property#"sv;
        else if (std::char_traits<char>::compare(name, "pdfaType", length) == 0)
            return "http://www.aiim.org/pdfa/ns/type#"sv;
        else if (std::char_traits<char>::compare(name, "rng", length) == 0)
            return "http://relaxng.org/ns/structure/1.0"sv;
        else
            return { };
    }
}

// Low level XMP functions
namespace utls
{
    enum class XMPListType
    {
        LangAlt, ///< ISO 16684-1:2019 "8.2.2.4 Language alternative"
        Seq,
        Bag,
    };

    void SetListNodeContent(xmlDocPtr doc, xmlNodePtr node, XMPListType seqType,
        const std::string_view& value, xmlNodePtr& newNode);

    void SetListNodeContent(xmlDocPtr doc, xmlNodePtr node, XMPListType seqType,
        const PoDoFo::cspan<std::string_view>& value, xmlNodePtr& newNode);
}


// SPDX-FileCopyrightText: 2022 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#pragma once

#include "XmlUtils.h"
#include <podofo/main/PdfMetadataStore.h>

namespace PoDoFo
{
    enum class XMPNamespaceKind : uint8_t
    {
        Unknown = 0,
        Rdf,
        Dc,
        Pdf,
        Xmp,
        PdfAId,
        PdfUAId,
        PdfVTId,
        PdfXId,
        PdfEId,
        PdfAExtension,
        PdfASchema,
        PdfAProperty,
        PdfAField,
        PdfAType,
    };

    // CHECK-ME: Consider make this publix and expose to PdfXMPProperty
    enum class XMPPropError : uint32_t
    {
        GenericError = 1,
        Duplicated = 2,
        InvalidPrefix = 4
    };

    /// @remarks the store is not cleared: the function sets only read properties
    void GetXMPMetadata(xmlNodePtr description, PdfMetadataStore& metadata);
    void SetXMPMetadata(xmlDocPtr doc, xmlNodePtr description, const PdfMetadataStore& metadata);
    void PruneAndValidate(xmlDocPtr doc, xmlNodePtr description, PdfALevel level,
        bool skipSetPdfAId, const std::function<void(std::string_view name, std::string_view ns,
            std::string_view prefix, XMPPropError error, xmlNodePtr node)>& reportWarnings);
    void GetXMPNamespacePrefix(XMPNamespaceKind ns, std::string_view& prefix);
    void GetXMPNamespacePrefix(XMPNamespaceKind ns, std::string_view& prefix, std::string_view& href);

    constexpr std::string_view operator""_ns(const char* name, size_t length)
    {
        using namespace std;
        string_view ns(name, length);
        if (ns == "rdf"sv)
            return "http://www.w3.org/1999/02/22-rdf-syntax-ns#"sv;
        else if (ns == "dc"sv)
            return "http://purl.org/dc/elements/1.1/"sv;
        else if (ns == "pdf"sv)
            return "http://ns.adobe.com/pdf/1.3/"sv;
        else if (ns == "xmp"sv)
            return "http://ns.adobe.com/xap/1.0/"sv;
        else if (ns == "pdfaid"sv)
            return "http://www.aiim.org/pdfa/ns/id/"sv;
        else if (ns == "pdfuaid"sv)
            return "http://www.aiim.org/pdfua/ns/id/"sv;
        else if (ns == "pdfvtid"sv)
            return "http://www.npes.org/pdfvt/ns/id/"sv;
        else if (ns == "pdfxid"sv)
            return "http://www.npes.org/pdfx/ns/id/"sv;
        else if (ns == "pdfe"sv)
            return "http://www.aiim.org/pdfe/ns/id/"sv;
        else if (ns == "pdfaExtension"sv)
            return "http://www.aiim.org/pdfa/ns/extension/"sv;
        else if (ns == "pdfaSchema"sv)
            return "http://www.aiim.org/pdfa/ns/schema#"sv;
        else if (ns == "pdfaProperty"sv)
            return "http://www.aiim.org/pdfa/ns/property#"sv;
        else if (ns == "pdfaField"sv)
            return "http://www.aiim.org/pdfa/ns/field#"sv;
        else if (ns == "pdfaType"sv)
            return "http://www.aiim.org/pdfa/ns/type#"sv;
        else if (ns == "rng"sv)
            return "http://relaxng.org/ns/structure/1.0"sv;
        else if (ns == "xml"sv)
            return "http://www.w3.org/XML/1998/namespace"sv;
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


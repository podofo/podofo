/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include "PdfDeclarationsPrivate.h"
#include "XMPUtils.h"

#include <libxml/relaxng.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <podofo/optional/PdfConvert.h>
#include <podofo/private/PdfFilterFactory.h>

#include "XmlUtils.h"

using namespace std;
using namespace PoDoFo;
using namespace utls;

#define ADDITIONAL_METADATA_OFFSET 20
#define ASSERT_STREAMING_RNG(cond) if (!(cond)) PODOFO_RAISE_ERROR_INFO(PdfErrorCode::XmpMetadataError, "Unknown RNG error");

#ifdef PODOFO_HAVE_RNG_VALIDATION_RECOVERY
#define assertHaveRngValidationRecovery()
#else // !PODOFO_HAVE_RNG_VALIDATION_RECOVERY
static void assertHaveRngValidationRecovery()
{
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "The libxml2 version used does not implement RELAX NG validation recovery");
}
#define xmlRelaxNGValidCtxtClearErrors(ctx)
#endif // PODOFO_HAVE_RNG_VALIDATION_RECOVERY

enum class XMPMetadataKind : uint8_t
{
    PDFVersion = 1,     // Available since XMP specification 2004
    Title,
    Author,
    Subject,
    Keywords,
    Creator,
    Producer,
    CreationDate,
    ModDate,
    Trapped,        // Available since XMP specification 2008
    PdfAIdPart,
    PdfAIdConformance,
    PdfUAIdPart,
    PdfAIdAmd = ADDITIONAL_METADATA_OFFSET + 1, // Used up to PDF/A-3
    PdfAIdCorr,         // Used up to PDF/A-3
    PdfAIdRev,          // Used since PDF/A-4
    PdfUAIdAmd,         // Used up to PDF/UA-1
    PdfUAIdCorr,        // Used up to PDF/UA-1
    PdfUAIdRev,         // Used since to PDF/UA-2
};

static void addXMPProperty(xmlDocPtr doc, xmlNodePtr description,
    XMPMetadataKind property, const string_view& value);
static void removeXMPProperty(xmlNodePtr description, XMPMetadataKind property);
static xmlNsPtr findOrCreateNamespace(xmlDocPtr doc, xmlNodePtr description, XMPNamespaceKind nsKind);
static void getPdfALevelComponents(PdfALevel level, string& partStr, string& conformanceStr, string& revision);
static void getPdfUALevelComponents(PdfUALevel version, string& part, string& revision);
static nullable<PdfString> getListElementText(xmlNodePtr elem);
static nullable<PdfString> getElementText(xmlNodePtr elem);
static void addExtension(xmlDocPtr doc, xmlNodePtr description, XMPNamespaceKind extension);
static void addExtension(xmlDocPtr doc, xmlNodePtr description,
    string_view extensionSnippet, string_view extensionNs);
static xmlNodePtr getOrCreateExtensionBag(xmlDocPtr doc, xmlNodePtr description);
static void removeExtension(xmlNodePtr extensionBag, string_view extensionNamespace);
static void makeDeterministicAndCollectRNG(xmlDocPtr doc);
static void collectGarbageRNG(xmlNodePtr element, unordered_set<string>& visitedDefines,
    const unordered_map<string, xmlNodePtr>& defineMap);
static void makeDeterministic(xmlDocPtr doc, xmlNodePtr start, const unordered_map<string, xmlNodePtr>& defineMap);
static xmlRelaxNGPtr createTailoredSchema(const unordered_map<string_view, bool>& vars);
static void preprocessXMPSchemaTemplate(xmlNodePtr element, xmlDocPtr doc, const unordered_map<string_view, bool>& vars);
static xmlXPathObjectPtr resolveVariable(void* ctxt, const xmlChar* name, const xmlChar* ns_uri);
static xmlDocPtr getXMPSchemaTemplate();
static xmlRelaxNGPtr getXMPSchema_PDFA1();
static xmlRelaxNGPtr getXMPSchema_PDFA2_3();
static xmlRelaxNGPtr getXMPSchema_PDFA4();
static bool tryValidateElement(xmlRelaxNGValidCtxtPtr ctxt, xmlDocPtr doc, xmlNodePtr elem);
static const unordered_map<string_view, XMPNamespaceKind>& getXMPMandatoryNSPrefixMap();

namespace PoDoFo
{
    extern string_view GetXMPSchemaTemplateDeflated();
    extern string_view GetPdfUAIdSchema();
    extern string_view GetPdfVTIdSchema();
    extern string_view GetPdfXIdSchema();
}

void PoDoFo::GetXMPMetadata(xmlNodePtr description, PdfMetadataStore& metadata)
{
    xmlNodePtr childElement = nullptr;
    nullable<PdfString> text;

    childElement = utls::FindChildElement(description, "dc"_ns, "title");
    if (childElement != nullptr)
        metadata.Title = getListElementText(childElement);

    childElement = utls::FindChildElement(description, "dc"_ns, "creator");
    if (childElement != nullptr)
        metadata.Author = getListElementText(childElement);

    childElement = utls::FindChildElement(description, "dc"_ns, "description");
    if (childElement != nullptr)
        metadata.Subject = getListElementText(childElement);

    childElement = utls::FindChildElement(description, "pdf"_ns, "Keywords");
    if (childElement != nullptr)
        metadata.Keywords = getElementText(childElement);

    childElement = utls::FindChildElement(description, "xmp"_ns, "CreatorTool");
    if (childElement != nullptr)
        metadata.Creator = getListElementText(childElement);

    childElement = utls::FindChildElement(description, "pdf"_ns, "Producer");
    if (childElement != nullptr)
        metadata.Producer = getElementText(childElement);

    PdfDate date;
    childElement = utls::FindChildElement(description, "xmp"_ns, "CreateDate");
    if (childElement != nullptr && (text = getElementText(childElement)) != nullptr
        && PdfDate::TryParseW3C(*text, date))
    {
        metadata.CreationDate = date;
    }

    childElement = utls::FindChildElement(description, "xmp"_ns, "ModifyDate");
    if (childElement != nullptr && (text = getElementText(childElement)) != nullptr
        && PdfDate::TryParseW3C(*text, date))
    {
        metadata.ModDate = date;
    }

    nullable<string> part;
    nullable<string> conformance;
    string tmp;

    childElement = utls::FindChildElement(description, "pdfaid"_ns, "part");
    if (childElement != nullptr && (part = utls::GetNodeContent(childElement)).has_value())
    {
        childElement = utls::FindChildElement(description, "pdfaid"_ns, "conformance");
        if (childElement != nullptr)
            conformance = utls::GetNodeContent(childElement);

        tmp = "L";
        if (conformance.has_value())
            tmp += *part + *conformance;
        else
            tmp += *part;

        (void)PoDoFo::TryConvertTo(tmp, metadata.PdfaLevel);

        childElement = utls::FindChildElement(description, "pdfaid"_ns, "amd");
        if (childElement != nullptr)
            metadata.SetMetadata(PdfAdditionalMetadata::PdfAIdAmd, getElementText(childElement));

        childElement = utls::FindChildElement(description, "pdfaid"_ns, "corr");
        if (childElement != nullptr)
            metadata.SetMetadata(PdfAdditionalMetadata::PdfAIdCorr, getElementText(childElement));

        childElement = utls::FindChildElement(description, "pdfaid"_ns, "rev");
        if (childElement != nullptr)
            metadata.SetMetadata(PdfAdditionalMetadata::PdfAIdRev, getElementText(childElement));
    }

    childElement = utls::FindChildElement(description, "pdfuaid"_ns, "part");
    if (childElement != nullptr && (part = utls::GetNodeContent(childElement)).has_value())
    {
        tmp = "L";
        tmp += *part;
        (void)PoDoFo::TryConvertTo(tmp, metadata.PdfuaLevel);

        childElement = utls::FindChildElement(description, "pdfuaid"_ns, "amd");
        if (childElement != nullptr)
            metadata.SetMetadata(PdfAdditionalMetadata::PdfUAIdAmd, getElementText(childElement));

        childElement = utls::FindChildElement(description, "pdfuaid"_ns, "corr");
        if (childElement != nullptr)
            metadata.SetMetadata(PdfAdditionalMetadata::PdfUAIdCorr, getElementText(childElement));

        childElement = utls::FindChildElement(description, "pdfuaid"_ns, "rev");
        if (childElement != nullptr)
            metadata.SetMetadata(PdfAdditionalMetadata::PdfUAIdRev, getElementText(childElement));
    }
}

void PoDoFo::SetXMPMetadata(xmlDocPtr doc, xmlNodePtr description, const PdfMetadataStore& metadata)
{
    removeXMPProperty(description, XMPMetadataKind::PDFVersion);
    removeXMPProperty(description, XMPMetadataKind::Title);
    removeXMPProperty(description, XMPMetadataKind::Author);
    removeXMPProperty(description, XMPMetadataKind::Subject);
    removeXMPProperty(description, XMPMetadataKind::Keywords);
    removeXMPProperty(description, XMPMetadataKind::Creator);
    removeXMPProperty(description, XMPMetadataKind::Producer);
    removeXMPProperty(description, XMPMetadataKind::CreationDate);
    removeXMPProperty(description, XMPMetadataKind::ModDate);
    removeXMPProperty(description, XMPMetadataKind::Trapped);
    removeXMPProperty(description, XMPMetadataKind::PdfAIdPart);
    removeXMPProperty(description, XMPMetadataKind::PdfAIdConformance);
    removeXMPProperty(description, XMPMetadataKind::PdfAIdAmd);
    removeXMPProperty(description, XMPMetadataKind::PdfAIdCorr);
    removeXMPProperty(description, XMPMetadataKind::PdfAIdRev);
    removeXMPProperty(description, XMPMetadataKind::PdfUAIdPart);
    removeXMPProperty(description, XMPMetadataKind::PdfUAIdAmd);
    removeXMPProperty(description, XMPMetadataKind::PdfUAIdCorr);
    removeXMPProperty(description, XMPMetadataKind::PdfUAIdRev);
    if (metadata.Title.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::Title, metadata.Title->GetString());
    if (metadata.Author.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::Author, metadata.Author->GetString());
    if (metadata.Subject.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::Subject, metadata.Subject->GetString());
    if (metadata.Keywords.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::Keywords, metadata.Keywords->GetString());
    if (metadata.Creator.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::Creator, metadata.Creator->GetString());
    if (metadata.Producer.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::Producer, metadata.Producer->GetString());
    if (metadata.CreationDate.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::CreationDate, metadata.CreationDate->ToStringW3C().GetString());
    if (metadata.ModDate.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::ModDate, metadata.ModDate->ToStringW3C().GetString());

    // NOTE: Ignore setting PDFVersion (which is better set by
    // the %PDF-X.Y header) and Trapped (which is deprecated in PDF 2.0)

    if (metadata.PdfaLevel != PdfALevel::Unknown)
    {
        // Set actual PdfA level
        string partStr;
        string conformanceStr;
        string revision;
        getPdfALevelComponents(metadata.PdfaLevel, partStr, conformanceStr, revision);
        addXMPProperty(doc, description, XMPMetadataKind::PdfAIdPart, partStr);
        if (conformanceStr.length() != 0)
            addXMPProperty(doc, description, XMPMetadataKind::PdfAIdConformance, conformanceStr);
        if (revision.length() != 0)
            addXMPProperty(doc, description, XMPMetadataKind::PdfAIdRev, revision);
    }

    if (metadata.PdfuaLevel != PdfUALevel::Unknown)
    {
        if (metadata.PdfaLevel != PdfALevel::Unknown
            && metadata.PdfaLevel < PdfALevel::L4)
        {
            // PDF/A up to 3 needs extensions schema for external properties
            addExtension(doc, description, PoDoFo::GetPdfUAIdSchema(), "pdfaid"_ns);
        }

        // Set actual PdfUA version
        string partStr;
        string revision;
        getPdfUALevelComponents(metadata.PdfuaLevel, partStr, revision);
        addXMPProperty(doc, description, XMPMetadataKind::PdfUAIdPart, partStr);
        if (revision.length() != 0)
            addXMPProperty(doc, description, XMPMetadataKind::PdfUAIdRev, revision);
    }

    auto additionalMetadata = metadata.GetAdditionalMetadata();
    if (additionalMetadata != nullptr)
    {
        for (auto& pair : *additionalMetadata)
        {
            addXMPProperty(doc, description,
                (XMPMetadataKind)((unsigned)pair.first + ADDITIONAL_METADATA_OFFSET), pair.second);
        }
    }
}

extern "C"
{
    static void nullValidationErrorHandler(void*, const xmlError*)
    {
        // Ignore errors
    }
}

void PoDoFo::PruneInvalidProperties(xmlDocPtr doc, xmlNodePtr description, PdfALevel level,
    const function<void(string_view name, string_view ns, string_view prefix,
        XMPPropError error, xmlNodePtr node)>& reportWarnings)
{
    assertHaveRngValidationRecovery();

    int rc;
    charbuff output;
    auto filter = PdfFilterFactory::Create(PdfFilterType::FlateDecode);
    auto& restrictedPrefixNsMap = getXMPMandatoryNSPrefixMap();
    xmlRelaxNGPtr schema;
    switch (level)
    {
        case PdfALevel::L1A:
        case PdfALevel::L1B:
            schema = getXMPSchema_PDFA1();
            break;
        case PdfALevel::L2A:
        case PdfALevel::L2B:
        case PdfALevel::L2U:
        case PdfALevel::L3A:
        case PdfALevel::L3B:
        case PdfALevel::L3U:
            schema = getXMPSchema_PDFA2_3();
            break;
        case PdfALevel::L4:
        case PdfALevel::L4E:
        case PdfALevel::L4F:
            schema = getXMPSchema_PDFA4();
            break;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Unsupported PDF/A level");
    }

    unique_ptr<xmlRelaxNGValidCtxt, decltype(&xmlRelaxNGFreeValidCtxt)> validCtx(xmlRelaxNGNewValidCtxt(schema), xmlRelaxNGFreeValidCtxt);
    if (validCtx == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "Out of memory while validating XMP packet");

    // Set a null error handler
    xmlRelaxNGSetValidStructuredErrors(validCtx.get(), (xmlStructuredErrorFunc)nullValidationErrorHandler, nullptr);

    // Push enclosing/preable elements
    rc = xmlRelaxNGValidatePushElement(validCtx.get(), doc, xmlDocGetRootElement(doc)); // <x:xmpmeta>
    ASSERT_STREAMING_RNG(rc == 1);
    rc = xmlRelaxNGValidatePushElement(validCtx.get(), doc, description->parent);       // <rdf:RDF>
    ASSERT_STREAMING_RNG(rc == 1);
    rc = xmlRelaxNGValidatePushElement(validCtx.get(), doc, description);               // <rdf:Description>
    ASSERT_STREAMING_RNG(rc == 1);

    // NOTE: The RELAX NG schema itself is deterministic, meaning that interleaving
    // of properties is unrestricted (there can be repeated properties)
    // TODO1: Enforce non repetition of properties
    // TODO2: Enable pdfuaid, pdfvtid, pdfxid namespaces (pdfuaid with maximum priority)
    unordered_set<string> duplicated;
    vector<pair<xmlNodePtr, XMPPropError>> nodesToRemove;
    XMPNamespaceKind extension;
    vector<XMPNamespaceKind> extensionsToAdd;
    for (auto child = xmlFirstElementChild(description); child != nullptr; child = xmlNextElementSibling(child))
    {
        extension = XMPNamespaceKind::Unknown;
        auto foundNs = restrictedPrefixNsMap.find(utls::GetNodeNamespace(child));
        if (foundNs != restrictedPrefixNsMap.end())
        {
            auto ns = foundNs->second;
            switch (ns)
            {
                case XMPNamespaceKind::PdfUAId:
                case XMPNamespaceKind::PdfXId:
                case XMPNamespaceKind::PdfVTId:
                    extension = ns;
                    break;
                default:
                    break;
            }
            string_view mandatoryPrefix;
            GetXMPNamespacePrefix(ns, mandatoryPrefix);
            if (utls::GetNodePrefix(child) != mandatoryPrefix)
            {
                nodesToRemove.push_back({ child, XMPPropError::InvalidPrefix });
                continue;
            }
        }

        auto inserted = duplicated.emplace(utls::GetNodePrefixedName(child));
        if (inserted.second)
        {
            if (tryValidateElement(validCtx.get(), doc, child))
            {
                // Non duplicate property verified
                if (extension != XMPNamespaceKind::Unknown && (unsigned)level <= (unsigned)PdfALevel::L3U)
                {
                    // Try to register an extension to add in case of PDF/A <= 3
                    auto found = std::find(extensionsToAdd.begin(), extensionsToAdd.end(), extension);
                    if (found == extensionsToAdd.end())
                        extensionsToAdd.push_back(extension);
                }
            }
            else
            {
                // Property failed verification
                nodesToRemove.push_back({ child, XMPPropError::GenericError });
            }
        }
        else
        {
            nodesToRemove.push_back({ child, XMPPropError::Duplicated });
        }
    }

    for (unsigned i = 0; i < nodesToRemove.size(); i++)
    {
        auto& pair = nodesToRemove[i];
        if (reportWarnings != nullptr)
        {
            reportWarnings(utls::GetNodeName(pair.first), utls::GetNodeNamespace(pair.first),
                utls::GetNodePrefix(pair.first), pair.second, pair.first);
        }

        xmlUnlinkNode(pair.first);
        xmlFreeNode(pair.first);
    }

    for (unsigned i = 0; i < extensionsToAdd.size(); i++)
        addExtension(doc, description, extensionsToAdd[i]);

    // Pop enclosing/preable elements
    rc = xmlRelaxNGValidatePopElement(validCtx.get(), doc, description);                // </rdf:Description>
    ASSERT_STREAMING_RNG(rc == 1);
    rc = xmlRelaxNGValidatePopElement(validCtx.get(), doc, description->parent);        // </rdf:RDF>
    ASSERT_STREAMING_RNG(rc == 1);
    rc = xmlRelaxNGValidatePopElement(validCtx.get(), doc, xmlDocGetRootElement(doc));  // </x:xmpmeta>
    ASSERT_STREAMING_RNG(rc == 1);
}

void PoDoFo::GetXMPNamespacePrefix(XMPNamespaceKind ns, string_view& prefix)
{
    string_view href;
    PoDoFo::GetXMPNamespacePrefix(ns, prefix, href);
}

void PoDoFo::GetXMPNamespacePrefix(XMPNamespaceKind ns, string_view& prefix, string_view& href)
{
    switch (ns)
    {
        case XMPNamespaceKind::Rdf:
            prefix = "rdf";
            href = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";
            break;
        case XMPNamespaceKind::Dc:
            prefix = "dc";
            href = "http://purl.org/dc/elements/1.1/";
            break;
        case XMPNamespaceKind::Pdf:
            prefix = "pdf";
            href = "http://ns.adobe.com/pdf/1.3/";
            break;
        case XMPNamespaceKind::Xmp:
            prefix = "xmp";
            href = "http://ns.adobe.com/xap/1.0/";
            break;
        case XMPNamespaceKind::PdfAId:
            prefix = "pdfaid";
            href = "http://www.aiim.org/pdfa/ns/id/";
            break;
        case XMPNamespaceKind::PdfUAId:
            prefix = "pdfuaid";
            href = "http://www.aiim.org/pdfua/ns/id/";
            break;
        case XMPNamespaceKind::PdfVTId:
            prefix = "pdfvtid";
            href = "http://www.npes.org/pdfvt/ns/id/";
            break;
        case XMPNamespaceKind::PdfXId:
            prefix = "pdfxid";
            href = "http://www.npes.org/pdfx/ns/id/";
            break;
        case XMPNamespaceKind::PdfEId:
            prefix = "pdfe";
            href = "http://www.aiim.org/pdfe/ns/id/";
            break;
        case XMPNamespaceKind::PdfAExtension:
            prefix = "pdfaExtension";
            href = "http://www.aiim.org/pdfa/ns/extension/";
            break;
        case XMPNamespaceKind::PdfASchema:
            prefix = "pdfaSchema";
            href = "http://www.aiim.org/pdfa/ns/schema#";
            break;
        case XMPNamespaceKind::PdfAProperty:
            prefix = "pdfaProperty";
            href = "http://www.aiim.org/pdfa/ns/property#";
            break;
        case XMPNamespaceKind::PdfAField:
            prefix = "pdfaField";
            href = "http://www.aiim.org/pdfa/ns/field#";
            break;
        case XMPNamespaceKind::PdfAType:
            prefix = "pdfaType";
            href = "http://www.aiim.org/pdfa/ns/type#";
            break;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unsupported");
    }
}

xmlNsPtr findOrCreateNamespace(xmlDocPtr doc, xmlNodePtr description, XMPNamespaceKind nsKind)
{
    string_view href;
    string_view prefix;
    GetXMPNamespacePrefix(nsKind, prefix, href);

    auto xmlNs = xmlSearchNsByHref(doc, description, XMLCHAR href.data());
    if (xmlNs == nullptr)
        xmlNs = xmlNewNs(description, XMLCHAR href.data(), XMLCHAR prefix.data());

    if (xmlNs == nullptr)
        THROW_LIBXML_EXCEPTION(utls::Format("Can't find or create {} namespace", prefix));

    return xmlNs;
}

void addXMPProperty(xmlDocPtr doc, xmlNodePtr description, XMPMetadataKind property, const string_view& value)
{
    xmlNsPtr xmlNs;
    const char* propName;
    switch (property)
    {
        case XMPMetadataKind::PDFVersion:
            xmlNs = findOrCreateNamespace(doc, description, XMPNamespaceKind::Pdf);
            propName = "PDFVersion";
            break;
        case XMPMetadataKind::Title:
            xmlNs = findOrCreateNamespace(doc, description, XMPNamespaceKind::Dc);
            propName = "title";
            break;
        case XMPMetadataKind::Author:
            xmlNs = findOrCreateNamespace(doc, description, XMPNamespaceKind::Dc);
            propName = "creator";
            break;
        case XMPMetadataKind::Subject:
            xmlNs = findOrCreateNamespace(doc, description, XMPNamespaceKind::Dc);
            propName = "description";
            break;
        case XMPMetadataKind::Keywords:
            xmlNs = findOrCreateNamespace(doc, description, XMPNamespaceKind::Pdf);
            propName = "Keywords";
            break;
        case XMPMetadataKind::Creator:
            xmlNs = findOrCreateNamespace(doc, description, XMPNamespaceKind::Xmp);
            propName = "CreatorTool";
            break;
        case XMPMetadataKind::Producer:
            xmlNs = findOrCreateNamespace(doc, description, XMPNamespaceKind::Pdf);
            propName = "Producer";
            break;
        case XMPMetadataKind::CreationDate:
            xmlNs = findOrCreateNamespace(doc, description, XMPNamespaceKind::Xmp);
            propName = "CreateDate";
            break;
        case XMPMetadataKind::ModDate:
            xmlNs = findOrCreateNamespace(doc, description, XMPNamespaceKind::Xmp);
            propName = "ModifyDate";
            break;
        case XMPMetadataKind::Trapped:
            xmlNs = findOrCreateNamespace(doc, description, XMPNamespaceKind::Pdf);
            propName = "Trapped";
            break;
        case XMPMetadataKind::PdfAIdPart:
            xmlNs = findOrCreateNamespace(doc, description, XMPNamespaceKind::PdfAId);
            propName = "part";
            break;
        case XMPMetadataKind::PdfAIdConformance:
            xmlNs = findOrCreateNamespace(doc, description, XMPNamespaceKind::PdfAId);
            propName = "conformance";
            break;
        case XMPMetadataKind::PdfAIdCorr:
            xmlNs = findOrCreateNamespace(doc, description, XMPNamespaceKind::PdfAId);
            propName = "corr";
            break;
        case XMPMetadataKind::PdfAIdAmd:
            xmlNs = findOrCreateNamespace(doc, description, XMPNamespaceKind::PdfAId);
            propName = "amd";
            break;
        case XMPMetadataKind::PdfAIdRev:
            xmlNs = findOrCreateNamespace(doc, description, XMPNamespaceKind::PdfAId);
            propName = "rev";
            break;
        case XMPMetadataKind::PdfUAIdPart:
            xmlNs = findOrCreateNamespace(doc, description, XMPNamespaceKind::PdfUAId);
            propName = "part";
            break;
        case XMPMetadataKind::PdfUAIdAmd:
            xmlNs = findOrCreateNamespace(doc, description, XMPNamespaceKind::PdfUAId);
            propName = "amd";
            break;
        case XMPMetadataKind::PdfUAIdCorr:
            xmlNs = findOrCreateNamespace(doc, description, XMPNamespaceKind::PdfUAId);
            propName = "corr";
            break;
        case XMPMetadataKind::PdfUAIdRev:
            xmlNs = findOrCreateNamespace(doc, description, XMPNamespaceKind::PdfUAId);
            propName = "rev";
            break;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unsupported");
    }

    auto element = xmlNewChild(description, xmlNs, XMLCHAR propName, nullptr);
    if (element == nullptr)
        THROW_LIBXML_EXCEPTION(utls::Format("Can't create xmp:{} node", propName));

    switch (property)
    {
        case XMPMetadataKind::Title:
        case XMPMetadataKind::Subject:
        {
            xmlNodePtr newNode;
            utls::SetListNodeContent(doc, element, XMPListType::LangAlt, value, newNode);
            break;
        }
        case XMPMetadataKind::Author:
        {
            xmlNodePtr newNode;
            utls::SetListNodeContent(doc, element, XMPListType::Seq, value, newNode);
            break;
        }
        default:
        {
            xmlNodeAddContent(element, XMLCHAR value.data());
            break;
        }
    }
}

void utls::SetListNodeContent(xmlDocPtr doc, xmlNodePtr node, XMPListType seqType,
    const string_view& value, xmlNodePtr& newNode)
{
    SetListNodeContent(doc, node, seqType, cspan<string_view>(&value, 1), newNode);
}

void utls::SetListNodeContent(xmlDocPtr doc, xmlNodePtr node, XMPListType seqType,
    const cspan<string_view>& values, xmlNodePtr& newNode)
{
    const char* elemName;
    switch (seqType)
    {
        case XMPListType::LangAlt:
            elemName = "Alt";
            break;
        case XMPListType::Seq:
            elemName = "Seq";
            break;
        case XMPListType::Bag:
            elemName = "Bag";
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    auto rdfNs = xmlSearchNsByHref(doc, node, XMLCHAR ("rdf"_ns).data());
    PODOFO_ASSERT(rdfNs != nullptr);
    auto innerElem = xmlNewChild(node, rdfNs, XMLCHAR elemName, nullptr);
    if (innerElem == nullptr)
        THROW_LIBXML_EXCEPTION(utls::Format("Can't create rdf:{} node", elemName));

    for (auto& view : values)
    {
        auto liElem = xmlNewChild(innerElem, rdfNs, XMLCHAR "li", nullptr);
        if (liElem == nullptr)
            THROW_LIBXML_EXCEPTION(utls::Format("Can't create rdf:li node"));

        if (seqType == XMPListType::LangAlt)
        {
            // Set a xml:lang "x-default" attribute, accordingly
            // ISO 16684-1:2019 "8.2.2.4 Language alternative"
            auto xmlNs = xmlSearchNs(doc, node, XMLCHAR "xml");
            PODOFO_ASSERT(xmlNs != nullptr);
            if (xmlSetNsProp(liElem, xmlNs, XMLCHAR "lang", XMLCHAR "x-default") == nullptr)
                THROW_LIBXML_EXCEPTION(utls::Format("Can't set xml:lang attribute on rdf:li node"));
        }

        xmlNodeAddContent(liElem, XMLCHAR view.data());
    }

    newNode = innerElem->children;
}

void removeXMPProperty(xmlNodePtr description, XMPMetadataKind property)
{
    string_view propname;
    string_view ns;
    switch (property)
    {
        case XMPMetadataKind::PDFVersion:
            ns = "pdf"_ns;
            propname = "PDFVersion";
            break;
        case XMPMetadataKind::Title:
            ns = "dc"_ns;
            propname = "title";
            break;
        case XMPMetadataKind::Author:
            ns = "dc"_ns;
            propname = "creator";
            break;
        case XMPMetadataKind::Subject:
            ns = "dc"_ns;
            propname = "description";
            break;
        case XMPMetadataKind::Keywords:
            ns = "pdf"_ns;
            propname = "Keywords";
            break;
        case XMPMetadataKind::Creator:
            ns = "xmp"_ns;
            propname = "CreatorTool";
            break;
        case XMPMetadataKind::Producer:
            ns = "pdf"_ns;
            propname = "Producer";
            break;
        case XMPMetadataKind::CreationDate:
            ns = "xmp"_ns;
            propname = "CreateDate";
            break;
        case XMPMetadataKind::ModDate:
            ns = "xmp"_ns;
            propname = "ModifyDate";
            break;
        case XMPMetadataKind::Trapped:
            ns = "pdf"_ns;
            propname = "Trapped";
            break;
        case XMPMetadataKind::PdfAIdPart:
            ns = "pdfaid"_ns;
            propname = "part";
            break;
        case XMPMetadataKind::PdfAIdConformance:
            ns = "pdfaid"_ns;
            propname = "conformance";
            break;
        case XMPMetadataKind::PdfAIdAmd:
            ns = "pdfaid"_ns;
            propname = "amd";
            break;
        case XMPMetadataKind::PdfAIdCorr:
            ns = "pdfaid"_ns;
            propname = "corr";
            break;
        case XMPMetadataKind::PdfAIdRev:
            ns = "pdfaid"_ns;
            propname = "rev";
            break;
        case XMPMetadataKind::PdfUAIdPart:
            ns = "pdfuaid"_ns;
            propname = "part";
            break;
        case XMPMetadataKind::PdfUAIdAmd:
            ns = "pdfuaid"_ns;
            propname = "amd";
            break;
        case XMPMetadataKind::PdfUAIdCorr:
            ns = "pdfuaid"_ns;
            propname = "corr";
            break;
        case XMPMetadataKind::PdfUAIdRev:
            ns = "pdfuaid"_ns;
            propname = "rev";
            break;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unsupported");
    }

    xmlNodePtr elemModDate = nullptr;
    do
    {
        elemModDate = utls::FindChildElement(description, ns, propname);
        if (elemModDate != nullptr)
            break;

        description = utls::FindSiblingElement(description, "rdf"_ns, "Description");
    } while (description != nullptr);

    if (elemModDate != nullptr)
    {
        // Remove the existing ModifyDate. We recreate the element
        xmlUnlinkNode(elemModDate);
        xmlFreeNode(elemModDate);
    }
}

void getPdfALevelComponents(PdfALevel level, string& partStr, string& conformanceStr, string& revision)
{
    switch (level)
    {
        case PdfALevel::L1B:
            partStr = "1";
            conformanceStr = "B";
            revision.clear();
            return;
        case PdfALevel::L1A:
            partStr = "1";
            conformanceStr = "A";
            revision.clear();
            return;
        case PdfALevel::L2B:
            partStr = "2";
            conformanceStr = "B";
            revision.clear();
            return;
        case PdfALevel::L2A:
            partStr = "2";
            conformanceStr = "A";
            revision.clear();
            return;
        case PdfALevel::L2U:
            partStr = "2";
            conformanceStr = "U";
            revision.clear();
            return;
        case PdfALevel::L3B:
            partStr = "3";
            conformanceStr = "B";
            revision.clear();
            return;
        case PdfALevel::L3A:
            partStr = "3";
            conformanceStr = "A";
            revision.clear();
            return;
        case PdfALevel::L3U:
            partStr = "3";
            conformanceStr = "U";
            revision.clear();
            return;
        case PdfALevel::L4:
            partStr = "4";
            conformanceStr.clear();
            revision = "2020";
            return;
        case PdfALevel::L4E:
            partStr = "4";
            conformanceStr = "E";
            revision = "2020";
            return;
        case PdfALevel::L4F:
            partStr = "4";
            conformanceStr = "F";
            revision = "2020";
            return;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unsupported");
    }
}

void getPdfUALevelComponents(PdfUALevel version, string& part, string& revision)
{
    switch (version)
    {
        case PdfUALevel::L1:
        {
            part = "1";
            revision.clear();
            break;
        }
        case PdfUALevel::L2:
        {
            part = "2";
            revision = "2024";
            break;
        }
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unsupported");
    }
}

nullable<PdfString> getListElementText(xmlNodePtr elem)
{
    auto listNode = xmlFirstElementChild(elem);
    if (listNode == nullptr)
        return nullptr;

    auto liNode = xmlFirstElementChild(listNode);
    if (liNode == nullptr)
        return nullptr;

    return getElementText(liNode);
}

nullable<PdfString> getElementText(xmlNodePtr elem)
{
    auto text = utls::GetNodeContent(elem);
    if (text == nullptr)
        return nullptr;
    else
        return PdfString(*text);
}

void addExtension(xmlDocPtr doc, xmlNodePtr description, XMPNamespaceKind extension)
{
    string_view extensionSnippet;
    string_view extensionNs;
    switch (extension)
    {
        case XMPNamespaceKind::PdfUAId:
            extensionSnippet = PoDoFo::GetPdfUAIdSchema();
            extensionNs = "pdfuaid"_ns;
            break;
        case XMPNamespaceKind::PdfVTId:
            extensionSnippet = PoDoFo::GetPdfVTIdSchema();
            extensionNs = "pdfvtid"_ns;
            break;
        case XMPNamespaceKind::PdfXId:
            extensionSnippet = PoDoFo::GetPdfXIdSchema();
            extensionNs = "pdfxid"_ns;
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    addExtension(doc, description, extensionSnippet, extensionNs);
}

void addExtension(xmlDocPtr doc, xmlNodePtr description, string_view extensionSnippet, string_view extensionNs)
{
    auto bag = getOrCreateExtensionBag(doc, description);
    // Remove any existing same namespace extension
    removeExtension(bag, extensionNs);

    xmlNodePtr newNode = NULL;
    auto rc = xmlParseInNodeContext(description, extensionSnippet.data(), (int)extensionSnippet.size(), XML_PARSE_NOBLANKS, &newNode);
    if (rc != XML_ERR_OK)
        THROW_LIBXML_EXCEPTION("Could not parse extension fragment");

    if (xmlAddChild(bag, newNode) == nullptr)
    {
        xmlFreeNode(newNode);
        THROW_LIBXML_EXCEPTION("Can't add element to extension bag");
    }
}

xmlNodePtr getOrCreateExtensionBag(xmlDocPtr doc, xmlNodePtr description)
{
    // Add required namespace to write extensions
    auto pdfaExtNs = findOrCreateNamespace(doc, description, XMPNamespaceKind::PdfAExtension);
    (void)findOrCreateNamespace(doc, description, XMPNamespaceKind::PdfASchema);
    (void)findOrCreateNamespace(doc, description, XMPNamespaceKind::PdfAProperty);
    (void)findOrCreateNamespace(doc, description, XMPNamespaceKind::PdfAType);

    auto pdfaExtension = utls::FindChildElement(description, "pdfaExtension"_ns, "schemas");
    if (pdfaExtension == nullptr)
    {
        pdfaExtension = xmlNewChild(description, nullptr, XMLCHAR "schemas", nullptr);
        if (pdfaExtension == nullptr)
            THROW_LIBXML_EXCEPTION("Can't create pdfaExtension:schemas node");

        xmlSetNs(pdfaExtension, pdfaExtNs);
    }

    auto bag = utls::FindChildElement(pdfaExtension, "rdf"_ns, "Bag");
    if (bag == nullptr)
    {
        bag = xmlNewChild(pdfaExtension, nullptr, XMLCHAR "Bag", nullptr);
        if (bag == nullptr)
            THROW_LIBXML_EXCEPTION("Can't create rdf:Bag node");

        auto rdfNs = xmlSearchNsByHref(doc, description, XMLCHAR ("rdf"_ns).data());
        PODOFO_ASSERT(rdfNs != nullptr);

        xmlSetNs(bag, rdfNs);
    }

    return bag;
}

void removeExtension(xmlNodePtr extensionBag, string_view extensionNamespace)
{
    xmlNodePtr cur = extensionBag->children;
    while (cur != NULL)
    {
        xmlNodePtr next = cur->next;  // Save next node, as we might delete current
        if (cur->type != XML_ELEMENT_NODE
            || !xmlStrEqual(cur->name, XMLCHAR "li")
            || cur->ns == nullptr
            || !xmlStrEqual(cur->ns->href, XMLCHAR "http://www.w3.org/1999/02/22-rdf-syntax-ns#"))
        {
            cur = next;
            continue;
        }

        xmlNodePtr child = cur->children;
        if (child->type == XML_ELEMENT_NODE
            && xmlStrEqual(child->name, XMLCHAR "Description"))
        {
            // Handle the XMP packet according to the normalization algorithm
            // described in ISO 16684-2:2014
            child = child->children;
        }

        // Look for a child named <pdfaSchema:namespaceURI>
        // and check for the actual uri
        while (child != NULL)
        {
            if (child->type == XML_ELEMENT_NODE
                && xmlStrEqual(child->name, XMLCHAR "namespaceURI")
                && child->children != nullptr
                && child->children->type == XML_TEXT_NODE
                && string_view((const char*)child->children->content, xmlStrlen(child->children->content)).find(extensionNamespace) != string_view::npos)
            {
                // Found the node; remove it from tree
                xmlUnlinkNode(cur);
                xmlFreeNode(cur);
                break;
            }

            child = child->next;
        }

        cur = next;
    }
}

void makeDeterministicAndCollectRNG(xmlDocPtr doc)
{
    xmlNodePtr root = xmlDocGetRootElement(doc);
    PODOFO_ASSERT(root != nullptr);
    unordered_map<string, xmlNodePtr> defineMap;
    utls::NavigateDescendantElements(root, "rng"_ns, "define", [&defineMap](xmlNodePtr elem) {
        auto name = xmlHasProp(elem, XMLCHAR "name");
        if (name == nullptr)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::XmpMetadataError, "Missing name attribute in <define>");
        defineMap[(const char*)name->children->content] = elem;
    });

    auto start = utls::FindChildElement(root, "rns"_ns, "start");
    if (start == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::XmpMetadataError, "Missing <start> element");

    makeDeterministic(doc, start, defineMap);

    // Traverse and mark visited defines
    unordered_set<string> visitedDefines;
    collectGarbageRNG(start, visitedDefines, defineMap);

    // Remove unvisited <define> nodes
    for (const auto& pair : defineMap)
    {
        if (visitedDefines.find(pair.first) == visitedDefines.end())
        {
            xmlUnlinkNode(pair.second);
            xmlFreeNode(pair.second);
        }
    }
}

void collectGarbageRNG(xmlNodePtr element, unordered_set<string>& visitedDefines,
    const unordered_map<string, xmlNodePtr>& defineMap)
{
    for (auto child = xmlFirstElementChild(element); child != nullptr; child = xmlNextElementSibling(child))
    {
        if (xmlStrEqual(child->name, XMLCHAR "ref") &&
            xmlStrEqual(child->ns ? child->ns->href : nullptr, XMLCHAR ("rng"_ns).data()))
        {
            auto nameAttr = xmlHasProp(child, XMLCHAR "name");
            if (nameAttr == nullptr)
                continue;

            string name = (const char*)nameAttr->children->content;
            if (!visitedDefines.insert(name).second)
                continue;

            auto it = defineMap.find(name);
            if (it != defineMap.end())
                collectGarbageRNG(it->second, visitedDefines, defineMap);
        }

        // Recurse through children of any node
        collectGarbageRNG(child, visitedDefines, defineMap);
    }
}

void makeDeterministic(xmlDocPtr doc, xmlNodePtr start, const unordered_map<string, xmlNodePtr>& defineMap)
{
    // Find the first <rng:interleave> element
    auto interleave = utls::FindDescendantElement(start, "rng"_ns, "interleave");
    PODOFO_ASSERT(interleave != nullptr);

    auto rngNs = xmlSearchNs(doc, start, XMLCHAR "rng");

    // Create <rng:zeroOrMore> and add it to parent <rng:element>
    xmlNodePtr zeroOrMore = xmlNewNode(nullptr, XMLCHAR "zeroOrMore");
    if (zeroOrMore == nullptr)
    {
    FailOOM:
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "Out of memory during XMP schema creation");
    }

    xmlSetNs(zeroOrMore, rngNs);
    if (xmlAddNextSibling(interleave, zeroOrMore) == nullptr)
        goto FailOOM;

    // Create <rng:choice> and add it to parent <rng:zeroOrMore>
    xmlNodePtr choice = xmlNewNode(nullptr, XMLCHAR "choice");
    if (choice == nullptr)
        goto FailOOM;

    xmlSetNs(choice, rngNs);
    if (xmlAddChild(zeroOrMore, choice) == nullptr)
        goto FailOOM;

    vector<xmlNodePtr> refs;
    utls::NavigateDescendantElements(interleave, "rng"_ns, "ref", [&refs](xmlNodePtr node) {
        refs.push_back(node);
    });

    for (xmlNodePtr refNode : refs)
    {
        auto nameAttr = xmlHasProp(refNode, XMLCHAR "name");
        PODOFO_ASSERT(nameAttr != nullptr);

        string name = (const char*)nameAttr->children->content;
        xmlNodePtr define = defineMap.at(name);

        // Find first <interleave> inside the <define>
        auto innerInterleave = utls::FindDescendantElement(define, "rng"_ns, "interleave");
        PODOFO_ASSERT(interleave != nullptr);

        for (auto child = xmlFirstElementChild(innerInterleave); child != nullptr; child = xmlNextElementSibling(child))
        {
            PODOFO_ASSERT(xmlStrEqual(child->name, XMLCHAR "optional") && child->children != nullptr);
            xmlAddChild(choice, child->children);
        }
    }

    // Remove original <interleave>
    xmlUnlinkNode(interleave);
    xmlFreeNode(interleave);
}

xmlRelaxNGPtr createTailoredSchema(const unordered_map<string_view, bool>& vars)
{
    unique_ptr<xmlDoc, decltype(&xmlFreeDoc)> templ(getXMPSchemaTemplate(), xmlFreeDoc);
    preprocessXMPSchemaTemplate(xmlDocGetRootElement(templ.get()), templ.get(), vars);
    makeDeterministicAndCollectRNG(templ.get());
    unique_ptr<xmlRelaxNGParserCtxt, decltype(&xmlRelaxNGFreeParserCtxt)> parserCtx(xmlRelaxNGNewDocParserCtxt(templ.get()), xmlRelaxNGFreeParserCtxt);
    if (parserCtx == nullptr)
    {
    Fail:
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "Out of memory while creting tailored XMP schema");
    }

    auto ret = xmlRelaxNGParse(parserCtx.get());
    if (ret == nullptr)
        goto Fail;

    return ret;
}

void preprocessXMPSchemaTemplate(xmlNodePtr element, xmlDocPtr doc, const unordered_map<string_view, bool>& vars)
{
    xmlNodePtr next;
    for (auto child = xmlFirstElementChild(element); child != nullptr; child = next)
    {
        next = xmlNextElementSibling(child);
        // Function to evaluate condition expression
        auto evaluate = [&](const xmlChar* expr) -> bool
        {
            xmlXPathContextPtr xpctx = xmlXPathNewContext(doc);
            if (xpctx == nullptr)
                return true;

            // Register variables
            xpctx->varLookupData = (void*)&vars;
            xpctx->varLookupFunc = resolveVariable;

            xmlXPathObjectPtr result = xmlXPathEvalExpression(expr, xpctx);
            bool keep = result ? xmlXPathCastToBoolean(result) == 1 : true;

            if (result != nullptr)
                xmlXPathFreeObject(result);

            xmlXPathFreeContext(xpctx);
            return keep;
        };

        auto cond = xmlHasProp(child, XMLCHAR "condition");
        if (cond != nullptr)
        {
            bool keep = evaluate(cond->children->content);
            xmlUnsetProp(child, XMLCHAR "condition");
            if (keep)
            {
                preprocessXMPSchemaTemplate(child, doc, vars);
            }
            else
            {
                // Collect the node
                xmlUnlinkNode(child);
                xmlFreeNode(child);
            }
        }
        else
        {
            preprocessXMPSchemaTemplate(child, doc, vars);
        }
    }
}

xmlXPathObjectPtr resolveVariable(void* ctxt, const xmlChar* name, const xmlChar* ns_uri)
{
    (void)ns_uri; // unused
    auto vars = static_cast<const unordered_map<string_view, bool>*>(ctxt);
    auto it = vars->find(string_view((const char*)name));
    bool value = it != vars->end() && it->second;
    return xmlXPathNewBoolean(value);
}

xmlRelaxNGPtr getXMPSchema_PDFA1()
{
    static struct Init
    {
        Init()
        {
            m_schema = createTailoredSchema(unordered_map<string_view, bool>{
                { "IncludeExtensions", true },
                { "IsPDFA1", true },
                { "IsPDFA1OrGreater", true },
            });
        }
        ~Init()
        {
            xmlRelaxNGFree(m_schema);
        }
        xmlRelaxNGPtr m_schema;
    } s_init;

    return s_init.m_schema;
}

xmlRelaxNGPtr getXMPSchema_PDFA2_3()
{
    static struct Init
    {
        Init()
        {
            m_schema = createTailoredSchema(unordered_map<string_view, bool>{
                { "IncludeExtensions", true },
                { "IsPDFA1", false },
                { "IsPDFA1OrGreater", true },
                { "IsPDFA2", true },
                { "IsPDFA2OrGreater", true },
                { "IsPDFA3", true },
                { "IsPDFA3OrGreater", true },
            });
        }
        ~Init()
        {
            xmlRelaxNGFree(m_schema);
        }
        xmlRelaxNGPtr m_schema;
    } s_init;

    return s_init.m_schema;
}

xmlRelaxNGPtr getXMPSchema_PDFA4()
{
    static struct Init
    {
        Init()
        {
            m_schema = createTailoredSchema(unordered_map<string_view, bool>{
                { "IncludeExtensions", true },
                { "IsPDFA1", false },
                { "IsPDFA1OrGreater", true },
                { "IsPDFA2", false },
                { "IsPDFA2OrGreater", true },
                { "IsPDFA3", false },
                { "IsPDFA3OrGreater", true },
                { "IsPDFA4", true },
                { "IsPDFA4OrGreater", true },
            });
        }
        ~Init()
        {
            xmlRelaxNGFree(m_schema);
        }
        xmlRelaxNGPtr m_schema;
    } s_init;

    return s_init.m_schema;
}

bool tryValidateElement(xmlRelaxNGValidCtxtPtr ctx, xmlDocPtr doc, xmlNodePtr elem)
{
    int rc = xmlRelaxNGValidatePushElement(ctx, doc, elem);
    if (rc == 0)
    {
        /* Streaming is not possible, validate full element instead */
        rc = xmlRelaxNGValidateFullElement(ctx, doc, elem);
        if (rc == 1)
            return true;

    Fail:
        xmlRelaxNGValidCtxtClearErrors(ctx);
        xmlResetLastError();
        return false;
    }
    else if (rc == 1)
    {
        xmlNodePtr child;
        bool success = true;
        for (child = xmlFirstElementChild(elem); child != NULL; child = xmlNextElementSibling(child))
        {
            /* Validate children elements recursively.
             * NOTE: There may be no children to validate,
             * as for example for <rng:text/> defines */
            success = tryValidateElement(ctx, doc, child);
            if (!success)
                break;
        }

        if (xmlRelaxNGValidatePopElement(ctx, doc, elem) == 0)
            goto Fail;

        return success;
    }
    else
    {
        goto Fail;
    }
}

const unordered_map<string_view, XMPNamespaceKind>& getXMPMandatoryNSPrefixMap()
{
    static struct Init
    {
        Init()
        {
            // These namespaces requires a mandatory prefix
            Map.emplace("http://www.aiim.org/pdfa/ns/id/"sv, XMPNamespaceKind::PdfAId);
            Map.emplace("http://www.aiim.org/pdfua/ns/id/"sv, XMPNamespaceKind::PdfUAId);
            Map.emplace("http://www.npes.org/pdfvt/ns/id/"sv, XMPNamespaceKind::PdfVTId);
            Map.emplace("http://www.npes.org/pdfx/ns/id/"sv, XMPNamespaceKind::PdfXId);
            Map.emplace("http://www.aiim.org/pdfe/ns/id/"sv, XMPNamespaceKind::PdfEId);
            Map.emplace("http://www.aiim.org/pdfa/ns/extension/"sv, XMPNamespaceKind::PdfAExtension);
            Map.emplace("http://www.aiim.org/pdfa/ns/schema#"sv, XMPNamespaceKind::PdfASchema);
            Map.emplace("http://www.aiim.org/pdfa/ns/property#"sv, XMPNamespaceKind::PdfAProperty);
            Map.emplace("http://www.aiim.org/pdfa/ns/field#"sv, XMPNamespaceKind::PdfAField);
            Map.emplace("http://www.aiim.org/pdfa/ns/type#"sv, XMPNamespaceKind::PdfAType);
        }

        unordered_map<string_view, XMPNamespaceKind> Map;
    } s_init;
    return s_init.Map;
}

xmlDocPtr getXMPSchemaTemplate()
{
    auto tempDeflated = PoDoFo::GetXMPSchemaTemplateDeflated();
    auto filter = PdfFilterFactory::Create(PdfFilterType::FlateDecode);
    charbuff templ;
    filter->DecodeTo(templ, tempDeflated);
    auto ret = xmlReadMemory(templ.data(), (int)templ.size(), nullptr, nullptr, XML_PARSE_NOBLANKS);
    if (ret == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "Out of memory while parsing XMP schema template");

    return ret;
}

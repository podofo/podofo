/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include "PdfDeclarationsPrivate.h"

#include <podofo/optional/PdfConvert.h>

#include "XMPUtils.h"
#include "XmlUtils.h"

using namespace std;
using namespace PoDoFo;
using namespace utls;

#define ADDITIONAL_METADATA_OFFSET 20

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

enum class PdfANamespaceKind : uint8_t
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

// Coming from: https://pdfa.org/resource/xmp-extension-schema-templates/
// This snippet is normalized accordingly to ISO 16684-2:2014
constexpr string_view PdfUAIdSchema = R"(<rdf:li>
  <rdf:Description>
    <pdfaSchema:namespaceURI>http://www.aiim.org/pdfua/ns/id/</pdfaSchema:namespaceURI>
    <pdfaSchema:prefix>pdfuaid</pdfaSchema:prefix>
    <pdfaSchema:schema>PDF/UA ID Schema</pdfaSchema:schema>
    <pdfaSchema:property>
      <rdf:Seq>
        <rdf:li>
          <rdf:Description>
            <pdfaProperty:category>internal</pdfaProperty:category>
            <pdfaProperty:description>Part of PDF/UA standard</pdfaProperty:description>
            <pdfaProperty:name>part</pdfaProperty:name>
            <pdfaProperty:valueType>Open Choice of Integer</pdfaProperty:valueType>
          </rdf:Description>
        </rdf:li>
        <rdf:li>
          <rdf:Description>
            <pdfaProperty:category>internal</pdfaProperty:category>
            <pdfaProperty:description>Optional PDF/UA amendment identifier</pdfaProperty:description>
            <pdfaProperty:name>amd</pdfaProperty:name>
            <pdfaProperty:valueType>Open Choice of Text</pdfaProperty:valueType>
          </rdf:Description>
        </rdf:li>
        <rdf:li>
          <rdf:Description>
            <pdfaProperty:category>internal</pdfaProperty:category>
            <pdfaProperty:description>Optional PDF/UA corrigenda identifier</pdfaProperty:description>
            <pdfaProperty:name>corr</pdfaProperty:name>
            <pdfaProperty:valueType>Open Choice of Text</pdfaProperty:valueType>
          </rdf:Description>
        </rdf:li>
      </rdf:Seq>
    </pdfaSchema:property>
  </rdf:Description>
</rdf:li>)";

static void addXMPProperty(xmlDocPtr doc, xmlNodePtr description,
    XMPMetadataKind property, const string_view& value);
static void removeXMPProperty(xmlNodePtr description, XMPMetadataKind property);
static xmlNsPtr findOrCreateNamespace(xmlDocPtr doc, xmlNodePtr description, PdfANamespaceKind nsKind);
static void getPdfALevelComponents(PdfALevel level, string& partStr, string& conformanceStr, string& revision);
static void getPdfUALevelComponents(PdfUALevel version, string& part, string& revision);
static nullable<PdfString> getListElementText(xmlNodePtr elem);
static nullable<PdfString> getElementText(xmlNodePtr elem);
static void addExtension(xmlDocPtr doc, xmlNodePtr description, string_view extension, string_view extensionNs);
static xmlNodePtr getOrCreateExtensionBag(xmlDocPtr doc, xmlNodePtr description);
static void removeExtension(xmlNodePtr extensionBag, string_view extensionNamespace);

void PoDoFo::GetXMPMetadata(xmlNodePtr description, PdfMetadataStore& metadata)
{
    xmlNodePtr childElement = nullptr;
    nullable<PdfString> text;

    childElement = utls::FindChildElement(description, "dc", "title");
    if (childElement != nullptr)
        metadata.Title = getListElementText(childElement);

    childElement = utls::FindChildElement(description, "dc", "creator");
    if (childElement != nullptr)
        metadata.Author = getListElementText(childElement);

    childElement = utls::FindChildElement(description, "dc", "description");
    if (childElement != nullptr)
        metadata.Subject = getListElementText(childElement);

    childElement = utls::FindChildElement(description, "pdf", "Keywords");
    if (childElement != nullptr)
        metadata.Keywords = getElementText(childElement);

    childElement = utls::FindChildElement(description, "xmp", "CreatorTool");
    if (childElement != nullptr)
        metadata.Creator = getListElementText(childElement);

    childElement = utls::FindChildElement(description, "pdf", "Producer");
    if (childElement != nullptr)
        metadata.Producer = getElementText(childElement);

    PdfDate date;
    childElement = utls::FindChildElement(description, "xmp", "CreateDate");
    if (childElement != nullptr && (text = getElementText(childElement)) != nullptr
        && PdfDate::TryParseW3C(*text, date))
    {
        metadata.CreationDate = date;
    }

    childElement = utls::FindChildElement(description, "xmp", "ModifyDate");
    if (childElement != nullptr && (text = getElementText(childElement)) != nullptr
        && PdfDate::TryParseW3C(*text, date))
    {
        metadata.ModDate = date;
    }

    nullable<string> part;
    nullable<string> conformance;
    string tmp;

    childElement = utls::FindChildElement(description, "pdfaid", "part");
    if (childElement != nullptr && (part = utls::GetNodeContent(childElement)).has_value())
    {
        childElement = utls::FindChildElement(description, "pdfaid", "conformance");
        if (childElement != nullptr)
            conformance = utls::GetNodeContent(childElement);

        tmp = "L";
        if (conformance.has_value())
            tmp += *part + *conformance;
        else
            tmp += *part;

        (void)PoDoFo::TryConvertTo(tmp, metadata.PdfaLevel);

        childElement = utls::FindChildElement(description, "pdfaid", "amd");
        if (childElement != nullptr)
            metadata.SetMetadata(PdfAdditionalMetadata::PdfAIdAmd, getElementText(childElement));

        childElement = utls::FindChildElement(description, "pdfaid", "corr");
        if (childElement != nullptr)
            metadata.SetMetadata(PdfAdditionalMetadata::PdfAIdCorr, getElementText(childElement));

        childElement = utls::FindChildElement(description, "pdfaid", "rev");
        if (childElement != nullptr)
            metadata.SetMetadata(PdfAdditionalMetadata::PdfAIdRev, getElementText(childElement));
    }

    childElement = utls::FindChildElement(description, "pdfuaid", "part");
    if (childElement != nullptr && (part = utls::GetNodeContent(childElement)).has_value())
    {
        tmp = "L";
        tmp += *part;
        (void)PoDoFo::TryConvertTo(tmp, metadata.PdfuaLevel);

        childElement = utls::FindChildElement(description, "pdfuaid", "amd");
        if (childElement != nullptr)
            metadata.SetMetadata(PdfAdditionalMetadata::PdfUAIdAmd, getElementText(childElement));

        childElement = utls::FindChildElement(description, "pdfuaid", "corr");
        if (childElement != nullptr)
            metadata.SetMetadata(PdfAdditionalMetadata::PdfUAIdCorr, getElementText(childElement));

        childElement = utls::FindChildElement(description, "pdfuaid", "rev");
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
            addExtension(doc, description, PdfUAIdSchema, "http://www.aiim.org/pdfua/ns/id/");
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

xmlNsPtr findOrCreateNamespace(xmlDocPtr doc, xmlNodePtr description, PdfANamespaceKind nsKind)
{
    const char* prefix;
    const char* href;
    switch (nsKind)
    {
        case PdfANamespaceKind::Dc:
            prefix = "dc";
            href = "http://purl.org/dc/elements/1.1/";
            break;
        case PdfANamespaceKind::Pdf:
            prefix = "pdf";
            href = "http://ns.adobe.com/pdf/1.3/";
            break;
        case PdfANamespaceKind::Xmp:
            prefix = "xmp";
            href = "http://ns.adobe.com/xap/1.0/";
            break;
        case PdfANamespaceKind::PdfAId:
            prefix = "pdfaid";
            href = "http://www.aiim.org/pdfa/ns/id/";
            break;
        case PdfANamespaceKind::PdfUAId:
            prefix = "pdfuaid";
            href = "http://www.aiim.org/pdfua/ns/id/";
            break;
        case PdfANamespaceKind::PdfAExtension:
            prefix = "pdfaExtension";
            href = "http://www.aiim.org/pdfa/ns/extension/";
            break;
        case PdfANamespaceKind::PdfASchema:
            prefix = "pdfaSchema";
            href = "http://www.aiim.org/pdfa/ns/schema#";
            break;
        case PdfANamespaceKind::PdfAProperty:
            prefix = "pdfaProperty";
            href = "http://www.aiim.org/pdfa/ns/property#";
            break;
        case PdfANamespaceKind::PdfAType:
            prefix = "pdfaType";
            href = "http://www.aiim.org/pdfa/ns/type#";
            break;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unsupported");
    }
    auto xmlNs = xmlSearchNs(doc, description, XMLCHAR prefix);
    if (xmlNs == nullptr)
        xmlNs = xmlNewNs(description, XMLCHAR href, XMLCHAR prefix);

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
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Pdf);
            propName = "PDFVersion";
            break;
        case XMPMetadataKind::Title:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Dc);
            propName = "title";
            break;
        case XMPMetadataKind::Author:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Dc);
            propName = "creator";
            break;
        case XMPMetadataKind::Subject:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Dc);
            propName = "description";
            break;
        case XMPMetadataKind::Keywords:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Pdf);
            propName = "Keywords";
            break;
        case XMPMetadataKind::Creator:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Xmp);
            propName = "CreatorTool";
            break;
        case XMPMetadataKind::Producer:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Pdf);
            propName = "Producer";
            break;
        case XMPMetadataKind::CreationDate:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Xmp);
            propName = "CreateDate";
            break;
        case XMPMetadataKind::ModDate:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Xmp);
            propName = "ModifyDate";
            break;
        case XMPMetadataKind::Trapped:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Pdf);
            propName = "Trapped";
            break;
        case XMPMetadataKind::PdfAIdPart:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfAId);
            propName = "part";
            break;
        case XMPMetadataKind::PdfAIdConformance:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfAId);
            propName = "conformance";
            break;
        case XMPMetadataKind::PdfAIdCorr:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfAId);
            propName = "corr";
            break;
        case XMPMetadataKind::PdfAIdAmd:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfAId);
            propName = "amd";
            break;
        case XMPMetadataKind::PdfAIdRev:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfAId);
            propName = "rev";
            break;
        case XMPMetadataKind::PdfUAIdPart:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfUAId);
            propName = "part";
            break;
        case XMPMetadataKind::PdfUAIdAmd:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfUAId);
            propName = "amd";
            break;
        case XMPMetadataKind::PdfUAIdCorr:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfUAId);
            propName = "corr";
            break;
        case XMPMetadataKind::PdfUAIdRev:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfUAId);
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

    auto rdfNs = xmlSearchNs(doc, node, XMLCHAR "rdf");
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
    const char* propname;
    const char* ns;
    switch (property)
    {
        case XMPMetadataKind::PDFVersion:
            ns = "pdf";
            propname = "PDFVersion";
            break;
        case XMPMetadataKind::Title:
            ns = "dc";
            propname = "title";
            break;
        case XMPMetadataKind::Author:
            ns = "dc";
            propname = "creator";
            break;
        case XMPMetadataKind::Subject:
            ns = "dc";
            propname = "description";
            break;
        case XMPMetadataKind::Keywords:
            ns = "pdf";
            propname = "Keywords";
            break;
        case XMPMetadataKind::Creator:
            ns = "xmp";
            propname = "CreatorTool";
            break;
        case XMPMetadataKind::Producer:
            ns = "pdf";
            propname = "Producer";
            break;
        case XMPMetadataKind::CreationDate:
            ns = "xmp";
            propname = "CreateDate";
            break;
        case XMPMetadataKind::ModDate:
            ns = "xmp";
            propname = "ModifyDate";
            break;
        case XMPMetadataKind::Trapped:
            ns = "pdf";
            propname = "Trapped";
            break;
        case XMPMetadataKind::PdfAIdPart:
            ns = "pdfaid";
            propname = "part";
            break;
        case XMPMetadataKind::PdfAIdConformance:
            ns = "pdfaid";
            propname = "conformance";
            break;
        case XMPMetadataKind::PdfAIdAmd:
            ns = "pdfaid";
            propname = "amd";
            break;
        case XMPMetadataKind::PdfAIdCorr:
            ns = "pdfaid";
            propname = "corr";
            break;
        case XMPMetadataKind::PdfAIdRev:
            ns = "pdfaid";
            propname = "rev";
            break;
        case XMPMetadataKind::PdfUAIdPart:
            ns = "pdfuaid";
            propname = "part";
            break;
        case XMPMetadataKind::PdfUAIdAmd:
            ns = "pdfuaid";
            propname = "amd";
            break;
        case XMPMetadataKind::PdfUAIdCorr:
            ns = "pdfuaid";
            propname = "corr";
            break;
        case XMPMetadataKind::PdfUAIdRev:
            ns = "pdfuaid";
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

        description = utls::FindSiblingNode(description, "rdf", "Description");
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

void addExtension(xmlDocPtr doc, xmlNodePtr description, string_view extension, string_view extensionNs)
{
    auto bag = getOrCreateExtensionBag(doc, description);
    // Remove any existing same namespace extension
    removeExtension(bag, extensionNs);

    xmlNodePtr newNode = NULL;
    auto rc = xmlParseInNodeContext(description, extension.data(), (int)extension.size(), 0, &newNode);
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
    auto pdfaExtNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfAExtension);
    (void)findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfASchema);
    (void)findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfAProperty);
    (void)findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfAType);

    auto pdfaExtension = utls::FindChildElement(description, "pdfaExtension", "schemas");
    if (pdfaExtension == nullptr)
    {
        pdfaExtension = xmlNewChild(description, nullptr, XMLCHAR "schemas", nullptr);
        if (pdfaExtension == nullptr)
            THROW_LIBXML_EXCEPTION("Can't create pdfaExtension:schemas node");

        xmlSetNs(pdfaExtension, pdfaExtNs);
    }

    auto bag = utls::FindChildElement(pdfaExtension, "rdf", "Bag");
    if (bag == nullptr)
    {
        bag = xmlNewChild(pdfaExtension, nullptr, XMLCHAR "Bag", nullptr);
        if (bag == nullptr)
            THROW_LIBXML_EXCEPTION("Can't create rdf:Bag node");

        auto rdfNs = xmlSearchNs(doc, description, XMLCHAR "rdf");
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

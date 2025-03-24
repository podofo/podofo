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
constexpr string_view PdfUAIdSchema = R"(
<pdfaExtension:schemas>
   <rdf:Bag>
      <rdf:li rdf:parseType="Resource">
         <pdfaSchema:namespaceURI>http://www.aiim.org/pdfua/ns/id/</pdfaSchema:namespaceURI>
         <pdfaSchema:prefix>pdfuaid</pdfaSchema:prefix>
         <pdfaSchema:schema>PDF/UA ID Schema</pdfaSchema:schema>
         <pdfaSchema:property>
            <rdf:Seq>
               <rdf:li rdf:parseType="Resource">
                  <pdfaProperty:category>internal</pdfaProperty:category>
                  <pdfaProperty:description>Part of PDF/UA standard</pdfaProperty:description>
                  <pdfaProperty:name>part</pdfaProperty:name>
                  <pdfaProperty:valueType>Open Choice of Integer</pdfaProperty:valueType>
               </rdf:li>
               <rdf:li rdf:parseType="Resource">
                  <pdfaProperty:category>internal</pdfaProperty:category>
                  <pdfaProperty:description>Optional PDF/UA amendment identifier</pdfaProperty:description>
                  <pdfaProperty:name>amd</pdfaProperty:name>
                  <pdfaProperty:valueType>Open Choice of Text</pdfaProperty:valueType>
               </rdf:li>
               <rdf:li rdf:parseType="Resource">
                  <pdfaProperty:category>internal</pdfaProperty:category>
                  <pdfaProperty:description>Optional PDF/UA corrigenda identifier</pdfaProperty:description>
                  <pdfaProperty:name>corr</pdfaProperty:name>
                  <pdfaProperty:valueType>Open Choice of Text</pdfaProperty:valueType>
               </rdf:li>
            </rdf:Seq>
         </pdfaSchema:property>
      </rdf:li>
   </rdf:Bag>
</pdfaExtension:schemas>
)";

static void setXMPMetadata(xmlDocPtr doc, xmlNodePtr xmpmeta, const PdfMetadataStore& metatata);
static void addXMPProperty(xmlDocPtr doc, xmlNodePtr description,
    XMPMetadataKind property, const string_view& value);
static void removeXMPProperty(xmlNodePtr description, XMPMetadataKind property);
static xmlNsPtr findOrCreateNamespace(xmlDocPtr doc, xmlNodePtr description, PdfANamespaceKind nsKind);
static void getPdfALevelComponents(PdfALevel level, string& partStr, string& conformanceStr, string& revision);
static void getPdfUALevelComponents(PdfUALevel version, string& part, string& revision);
static nullable<PdfString> getListElementText(xmlNodePtr elem);
static nullable<PdfString> getElementText(xmlNodePtr elem);
static void addExtension(xmlDocPtr doc, xmlNodePtr description, string_view extension);

PdfMetadataStore PoDoFo::GetXMPMetadata(const string_view& xmpview, unique_ptr<PdfXMPPacket>& packet)
{
    utls::InitXml();

    PdfMetadataStore metadata;
    xmlNodePtr description;
    packet = PdfXMPPacket::Create(xmpview);
    if (packet == nullptr || (description = packet->GetDescription()) == nullptr)
    {
        // The the XMP metadata is missing or has insufficient data
        // to determine a PDF/A level
        return metadata;
    }

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

    return metadata;
}

void PoDoFo::CreateXMPMetadata(unique_ptr<PdfXMPPacket>& packet)
{
    utls::InitXml();
    if (packet == nullptr)
        packet.reset(new PdfXMPPacket());
}

void PoDoFo::UpdateOrCreateXMPMetadata(unique_ptr<PdfXMPPacket>& packet, const PdfMetadataStore& metatata)
{
    utls::InitXml();
    if (packet == nullptr)
        packet.reset(new PdfXMPPacket());

    setXMPMetadata(packet->GetDoc(), packet->GetOrCreateDescription(), metatata);
}

void setXMPMetadata(xmlDocPtr doc, xmlNodePtr description, const PdfMetadataStore& metatata)
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
    if (metatata.Title.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::Title, metatata.Title->GetString());
    if (metatata.Author.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::Author, metatata.Author->GetString());
    if (metatata.Subject.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::Subject, metatata.Subject->GetString());
    if (metatata.Keywords.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::Keywords, metatata.Keywords->GetString());
    if (metatata.Creator.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::Creator, metatata.Creator->GetString());
    if (metatata.Producer.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::Producer, metatata.Producer->GetString());
    if (metatata.CreationDate.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::CreationDate, metatata.CreationDate->ToStringW3C().GetString());
    if (metatata.ModDate.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::ModDate, metatata.ModDate->ToStringW3C().GetString());

    // NOTE: Ignore setting PDFVersion (which is better set by
    // the %PDF-X.Y header) and Trapped (which is deprecated in PDF 2.0)

    if (metatata.PdfaLevel != PdfALevel::Unknown)
    {
        // Set actual PdfA level
        string partStr;
        string conformanceStr;
        string revision;
        getPdfALevelComponents(metatata.PdfaLevel, partStr, conformanceStr, revision);
        addXMPProperty(doc, description, XMPMetadataKind::PdfAIdPart, partStr);
        if (conformanceStr.length() != 0)
            addXMPProperty(doc, description, XMPMetadataKind::PdfAIdConformance, conformanceStr);
        if (revision.length() != 0)
            addXMPProperty(doc, description, XMPMetadataKind::PdfAIdRev, revision);
    }

    if (metatata.PdfuaLevel != PdfUALevel::Unknown)
    {
        if (metatata.PdfaLevel != PdfALevel::Unknown
            && metatata.PdfaLevel < PdfALevel::L4)
        {
            // PDF/A up to 3 needs extensions schema for external properties
            addExtension(doc, description, PdfUAIdSchema);
        }

        // Set actual PdfUA version
        string partStr;
        string revision;
        getPdfUALevelComponents(metatata.PdfuaLevel, partStr, revision);
        addXMPProperty(doc, description, XMPMetadataKind::PdfUAIdPart, partStr);
        if (revision.length() != 0)
            addXMPProperty(doc, description, XMPMetadataKind::PdfUAIdRev, revision);
    }

    auto additionalMetadata = metatata.GetAdditionalMetadata();
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

void addExtension(xmlDocPtr doc, xmlNodePtr description, string_view extension)
{
    // Add required namespace to write extensions
    (void)findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfAExtension);
    (void)findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfASchema);
    (void)findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfAProperty);
    (void)findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfAType);

    xmlNodePtr fragParent = xmlNewNode(NULL, XMLCHAR "temp");
    int res = xmlParseBalancedChunkMemory(doc, nullptr, nullptr, 0, XMLCHAR extension.data(), &fragParent->children);
    if (res == 0)
    {
        xmlNodePtr child = fragParent->children;
        while (child)
        {
            xmlNodePtr next = child->next; // Save next before moving
            xmlUnlinkNode(child);
            xmlAddChild(description, child);
            child = next;
        }
    }
}

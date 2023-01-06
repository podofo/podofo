/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include "PdfDeclarationsPrivate.h"
#include "XMPUtils.h"
#include "XmlUtils.h"

using namespace std;
using namespace PoDoFo;
using namespace utls;

enum class XMPMetadataKind
{
    Title,
    Author,
    Subject,
    Keywords,
    Creator,
    Producer,
    CreationDate,
    ModDate,
    PdfALevel,
    PdfAConformance,
    PdfARevision,
};

enum class PdfANamespaceKind
{
    Dc,
    Pdf,
    Xmp,
    PdfAId,
};

static void setXMPMetadata(xmlDocPtr doc, xmlNodePtr xmpmeta, const PdfXMPMetadata& metatata);
static void addXMPProperty(xmlDocPtr doc, xmlNodePtr description,
    XMPMetadataKind property, const string& value);
static void addXMPProperty(xmlDocPtr doc, xmlNodePtr description,
    XMPMetadataKind property, const cspan<string>& values);
static void removeXMPProperty(xmlNodePtr description, XMPMetadataKind property);
static xmlNsPtr findOrCreateNamespace(xmlDocPtr doc, xmlNodePtr description, PdfANamespaceKind nsKind);
static PdfALevel getPDFALevelFromString(const string_view& level);
static void getPdfALevelComponents(PdfALevel level, string& levelStr, string& conformanceStr, string& revision);
static nullable<PdfString> getListElementText(xmlNodePtr elem);
static nullable<PdfString> getElementText(xmlNodePtr elem);

PdfXMPMetadata PoDoFo::GetXMPMetadata(const string_view& xmpview, unique_ptr<PdfXMPPacket>& packet)
{
    utls::InitXml();

    PdfXMPMetadata metadata;
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

    {
        nullable<string> pdfaid_part;
        nullable<string> pdfaid_conformance;

        childElement = utls::FindChildElement(description, "pdfaid", "part");
        if (childElement != nullptr)
            pdfaid_part = utls::GetNodeContent(childElement);
        childElement = utls::FindChildElement(description, "pdfaid", "conformance");
        if (childElement != nullptr)
            pdfaid_conformance = utls::GetNodeContent(childElement);

        if (pdfaid_part.has_value() && pdfaid_conformance.has_value())
            metadata.PdfaLevel = getPDFALevelFromString(*pdfaid_part + *pdfaid_conformance);
    }

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

    childElement = utls::FindChildElement(description, "xmp", "CreateDate");
    if (childElement != nullptr && (text = getElementText(childElement)) != nullptr)
        metadata.CreationDate = PdfDate::ParseW3C(*text);

    childElement = utls::FindChildElement(description, "xmp", "ModifyDate");
    if (childElement != nullptr && (text = getElementText(childElement)) != nullptr)
        metadata.ModDate = PdfDate::ParseW3C(*text);

    return metadata;
}

void PoDoFo::UpdateOrCreateXMPMetadata(unique_ptr<PdfXMPPacket>& packet, const PdfXMPMetadata& metatata)
{
    utls::InitXml();
    if (packet == nullptr)
        packet.reset(new PdfXMPPacket());

    setXMPMetadata(packet->GetDoc(), packet->GetOrCreateDescription(), metatata);
}

void setXMPMetadata(xmlDocPtr doc, xmlNodePtr description, const PdfXMPMetadata& metatata)
{
    removeXMPProperty(description, XMPMetadataKind::Title);
    removeXMPProperty(description, XMPMetadataKind::Author);
    removeXMPProperty(description, XMPMetadataKind::Subject);
    removeXMPProperty(description, XMPMetadataKind::Keywords);
    removeXMPProperty(description, XMPMetadataKind::Creator);
    removeXMPProperty(description, XMPMetadataKind::Producer);
    removeXMPProperty(description, XMPMetadataKind::CreationDate);
    removeXMPProperty(description, XMPMetadataKind::ModDate);
    removeXMPProperty(description, XMPMetadataKind::PdfALevel);
    removeXMPProperty(description, XMPMetadataKind::PdfAConformance);
    removeXMPProperty(description, XMPMetadataKind::PdfARevision);
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

    if (metatata.PdfaLevel != PdfALevel::Unknown)
    {
        // Set actual PdfA level
        string levelStr;
        string conformanceStr;
        string revision;
        getPdfALevelComponents(metatata.PdfaLevel, levelStr, conformanceStr, revision);
        addXMPProperty(doc, description, XMPMetadataKind::PdfALevel, levelStr);
        addXMPProperty(doc, description, XMPMetadataKind::PdfAConformance, conformanceStr);
        if (revision.length() != 0)
            addXMPProperty(doc, description, XMPMetadataKind::PdfARevision, revision);
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
        default:
            throw runtime_error("Unsupported");
    }
    auto xmlNs = xmlSearchNs(doc, description, XMLCHAR prefix);
    if (xmlNs == nullptr)
        xmlNs = xmlNewNs(description, XMLCHAR href, XMLCHAR prefix);

    if (xmlNs == nullptr)
        THROW_LIBXML_EXCEPTION(utls::Format("Can't find or create {} namespace", prefix));

    return xmlNs;
}

void addXMPProperty(xmlDocPtr doc, xmlNodePtr description, XMPMetadataKind prop, const string& value)
{
    addXMPProperty(doc, description, prop, cspan<string>(&value, 1));
}

void addXMPProperty(xmlDocPtr doc, xmlNodePtr description,
    XMPMetadataKind property, const cspan<string>& values)
{
    xmlNsPtr xmlNs;
    const char* propName;
    switch (property)
    {
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
        case XMPMetadataKind::PdfALevel:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfAId);
            propName = "part";
            break;
        case XMPMetadataKind::PdfAConformance:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfAId);
            propName = "conformance";
            break;
        case XMPMetadataKind::PdfARevision:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfAId);
            propName = "rev";
            break;
        default:
            throw runtime_error("Unsupported");
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
            utls::SetListNodeContent(doc, element, XMPListType::LangAlt, values, newNode);
            break;
        }
        case XMPMetadataKind::Author:
        {
            xmlNodePtr newNode;
            utls::SetListNodeContent(doc, element, XMPListType::Seq, values, newNode);
            break;
        }
        default:
        {
            xmlNodeSetContent(element, XMLCHAR values[0].data());
            break;
        }
    }
}

void utls::SetListNodeContent(xmlDocPtr doc, xmlNodePtr node, XMPListType seqType,
    const cspan<string>& values, xmlNodePtr& newNode)
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

        xmlNodeSetContent(liElem, XMLCHAR view.data());
    }

    newNode = innerElem->children;
}

void removeXMPProperty(xmlNodePtr description, XMPMetadataKind property)
{
    const char* propname;
    const char* ns;
    switch (property)
    {
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
        case XMPMetadataKind::PdfALevel:
            ns = "pdfaid";
            propname = "part";
            break;
        case XMPMetadataKind::PdfAConformance:
            ns = "pdfaid";
            propname = "conformance";
            break;
        case XMPMetadataKind::PdfARevision:
            ns = "pdfaid";
            propname = "rev";
            break;
        default:
            throw runtime_error("Unsupported");
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

PdfALevel getPDFALevelFromString(const string_view& pdfaid)
{
    if (pdfaid == "1B")
        return PdfALevel::L1B;
    else if (pdfaid == "1A")
        return PdfALevel::L1A;
    else if (pdfaid == "2B")
        return PdfALevel::L2B;
    else if (pdfaid == "2A")
        return PdfALevel::L2A;
    else if (pdfaid == "2U")
        return PdfALevel::L2U;
    else if (pdfaid == "3B")
        return PdfALevel::L3B;
    else if (pdfaid == "3A")
        return PdfALevel::L3A;
    else if (pdfaid == "3U")
        return PdfALevel::L3U;
    else if (pdfaid == "4E")
        return PdfALevel::L4E;
    else if (pdfaid == "4F")
        return PdfALevel::L4F;
    else
        return PdfALevel::Unknown;
}

void getPdfALevelComponents(PdfALevel level, string& levelStr, string& conformanceStr, string& revision)
{
    switch (level)
    {
        case PdfALevel::L1B:
            levelStr = "1";
            conformanceStr = "B";
            revision.clear();
            return;
        case PdfALevel::L1A:
            levelStr = "1";
            conformanceStr = "A";
            revision.clear();
            return;
        case PdfALevel::L2B:
            levelStr = "2";
            conformanceStr = "B";
            revision.clear();
            return;
        case PdfALevel::L2A:
            levelStr = "2";
            conformanceStr = "A";
            revision.clear();
            return;
        case PdfALevel::L2U:
            levelStr = "2";
            conformanceStr = "U";
            revision.clear();
            return;
        case PdfALevel::L3B:
            levelStr = "3";
            conformanceStr = "B";
            revision.clear();
            return;
        case PdfALevel::L3A:
            levelStr = "3";
            conformanceStr = "A";
            revision.clear();
            return;
        case PdfALevel::L3U:
            levelStr = "3";
            conformanceStr = "U";
            revision.clear();
            return;
        case PdfALevel::L4E:
            levelStr = "4";
            conformanceStr = "E";
            revision = "2020";
            return;
        case PdfALevel::L4F:
            levelStr = "4";
            conformanceStr = "F";
            revision = "2020";
            return;
        default:
            throw runtime_error("Unsupported");
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

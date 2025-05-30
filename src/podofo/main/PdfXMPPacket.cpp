/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfXMPPacket.h"
#include <podofo/private/XmlUtils.h>
#include <libxml/xmlsave.h>
#include <libxml/parser.h>
#include <podofo/private/XMPUtils.h>

using namespace std;
using namespace PoDoFo;
using namespace utls;

static xmlDocPtr createXMPDoc(xmlNodePtr& root);
static xmlNodePtr findRootXMPMeta(xmlDocPtr doc);
static void normalizeXMPMetadata(xmlDocPtr doc, xmlNodePtr xmpmeta, xmlNodePtr& description);
static void normalizeQualifiersAndValues(xmlDocPtr doc, xmlNsPtr rdfNs, xmlNodePtr node);
static void normalizeElement(xmlDocPtr doc, xmlNodePtr elem);
static void tryFixArrayElement(xmlDocPtr doc, xmlNodePtr& node, const string_view& nodeContent);
static bool shouldSkipAttribute(xmlAttrPtr attr);
static xmlNodePtr createRDFElement(xmlNodePtr xmpmeta);
static void createRDFNamespace(xmlNodePtr rdf);
static xmlNodePtr createDescriptionElement(xmlNodePtr xmpmeta);
static void serializeXMPMetadataTo(string& str, xmlDocPtr xmpMeta);
static int xmlOutputStringWriter(void* context, const char* buffer, int len);
static int xmlOutputStringWriterClose(void* context);

static unordered_map<string, XMPListType> s_knownListNodes = {
    { "dc:date", XMPListType::Seq },
    { "dc:language", XMPListType::Bag },
};

PdfXMPPacket::PdfXMPPacket()
    : m_Description(nullptr)
{
    utls::InitXml();
    m_Doc = createXMPDoc(m_XMPMeta);
}

PdfXMPPacket::PdfXMPPacket(xmlDocPtr doc, xmlNodePtr xmpmeta)
    : m_Doc(doc), m_XMPMeta(xmpmeta), m_Description(nullptr) { }

PdfXMPPacket::~PdfXMPPacket()
{
    xmlFreeDoc(m_Doc);
}

unique_ptr<PdfXMPPacket> PdfXMPPacket::Create(const string_view& xmpview)
{
    if (xmpview.size() == 0)
        return nullptr;

    utls::InitXml();
    auto doc = xmlReadMemory(xmpview.data(), (int)xmpview.size(), nullptr, nullptr, XML_PARSE_NOBLANKS);
    xmlNodePtr xmpmeta;
    if (doc == nullptr
        || (xmpmeta = findRootXMPMeta(doc)) == nullptr)
    {
        xmlFreeDoc(doc);
        return nullptr;
    }

    unique_ptr<PdfXMPPacket> ret(new PdfXMPPacket(doc, xmpmeta));
    normalizeXMPMetadata(doc, xmpmeta, ret->m_Description);
    return ret;
}

PdfMetadataStore PdfXMPPacket::GetMetadata() const
{
    if (m_Description == nullptr)
    {
        // The the XMP metadata is missing or has insufficient data
        // to determine a PDF/A level
        return { };
    }

    PdfMetadataStore metadata;
    PoDoFo::GetXMPMetadata(m_Description, metadata);
    return metadata;
}

void PdfXMPPacket::GetMetadata(PdfMetadataStore& metadata) const
{
    metadata.Reset();
    PoDoFo::GetXMPMetadata(m_Description, metadata);
}

void PdfXMPPacket::SetMetadata(const PdfMetadataStore& metadata)
{
    PoDoFo::SetXMPMetadata(m_Doc, GetOrCreateDescription(), metadata);
}

xmlNodePtr PdfXMPPacket::GetOrCreateDescription()
{
    if (m_Description != nullptr)
        return m_Description;

    auto rdf = utls::FindChildElement(m_XMPMeta, "rdf", "RDF");
    if (rdf == nullptr)
        rdf = createRDFElement(m_XMPMeta);

    auto description = utls::FindChildElement(rdf, "rdf", "Description");
    if (description == nullptr)
        description = createDescriptionElement(rdf);

    m_Description = description;
    return description;
}

void PdfXMPPacket::ToString(string& str) const
{
    serializeXMPMetadataTo(str, m_Doc);
}

string PdfXMPPacket::ToString() const
{
    string ret;
    ToString(ret);
    return ret;
}

// Normalize XMP accordingly to ISO 16684-2:2014
void normalizeXMPMetadata(xmlDocPtr doc, xmlNodePtr xmpmeta, xmlNodePtr& description)
{
    auto rdf = utls::FindChildElement(xmpmeta, "rdf", "RDF");
    if (rdf == nullptr)
    {
        description = nullptr;
        return;
    }

    normalizeQualifiersAndValues(doc, rdf->ns, rdf);

    description = utls::FindChildElement(rdf, "rdf", "Description");
    if (description == nullptr)
        return;

    // Merge top level rdf:Description elements
    vector<xmlNodePtr> descriptionsToRemove;
    auto element = description;
    while (true)
    {
        element = utls::FindSiblingNode(element, "rdf", "Description");
        if (element == nullptr)
            break;
        else
            descriptionsToRemove.push_back(element);

        vector<xmlNodePtr> childrenToMove;
        for (auto child = xmlFirstElementChild(element); child != nullptr; child = xmlNextElementSibling(child))
            childrenToMove.push_back(child);

        for (auto child : childrenToMove)
        {
            xmlUnlinkNode(child);
            xmlAddChild(description, child);
        }
    }

    if (xmlReconciliateNs(doc, description) == -1)
        THROW_LIBXML_EXCEPTION("Error fixing namespaces");

    // Finally remove spurious rdf:Description elements
    for (auto descToRemove : descriptionsToRemove)
    {
        xmlUnlinkNode(descToRemove);
        xmlFreeNode(descToRemove);
    }
}

void normalizeQualifiersAndValues(xmlDocPtr doc, xmlNsPtr rdfNs, xmlNodePtr elem)
{
    // TODO: Convert RDF Typed nodes from rdf:type notation with rdf:resource
    // As specified in 16684-2:2014:
    // "The RDF TypedNode notation defined in ISO 16684-1:2012,
    // 7.9.2.5 shall not be used for an rdf:type qualifier".
    // Eg. from:
    //      <rdf:Description>
    //          <rdf:type rdf:resource="http://ns.adobe.com/xmp-example/myType"/>
    //          <xe:Field>value</xe:Field>
    //      </rdf:Description>
    //
    // To:
    //     <xe:MyType>
    //         <xe:Field>value</xe:Field>
    //     </xe:MyType>

    auto child = xmlFirstElementChild(elem);
    nullable<string> content;
    if (child == nullptr
        && (elem->children == nullptr || elem->children->type != XML_COMMENT_NODE)
        && (content = utls::GetNodeContent(elem)) != nullptr
        && !utls::IsStringEmptyOrWhiteSpace(*content))
    {
        // Some elements are arrays but they don't use
        // proper array notation
        tryFixArrayElement(doc, elem, *content);
        normalizeElement(doc, elem);
        return;
    }

    normalizeElement(doc, elem);
    for (; child != nullptr; child = xmlNextElementSibling(child))
        normalizeQualifiersAndValues(doc, rdfNs, child);
}

void normalizeElement(xmlDocPtr doc, xmlNodePtr elem)
{
    xmlAttrPtr found;
    auto parseType = utls::FindAttribute(elem, "rdf", "parseType", found);
    if (parseType != nullptr && *parseType == "Resource")
    {
        // ISO 16684-2:2014 "5.6 Qualifier serialization"
        auto descElem = xmlNewDocNode(doc, found->ns, XMLCHAR "Description", nullptr);
        if (descElem == nullptr)
            THROW_LIBXML_EXCEPTION("Can't rdf:Description node");

        vector<xmlAttrPtr> attribsToMove;
        for (xmlAttrPtr attr = elem->properties; attr != nullptr; attr = attr->next)
        {
            if (attr == found)
                continue;

            attribsToMove.push_back(attr);
        }

        for (auto attr : attribsToMove)
        {
            xmlUnlinkNode((xmlNodePtr)attr);
            if (xmlAddChild(descElem, (xmlNodePtr)attr) == nullptr)
                THROW_LIBXML_EXCEPTION("Can't add attribute to new node");
        }

        // Finally remove the found rdf:parseType attribute
        xmlRemoveProp(found);

        vector<xmlNodePtr> elementsToMove;
        for (auto child = xmlFirstElementChild(elem); child != nullptr; child = xmlNextElementSibling(child))
            elementsToMove.push_back(child);

        for (auto childElement : elementsToMove)
        {
            xmlUnlinkNode(childElement);
            if (xmlAddChild(descElem, childElement) == nullptr)
                THROW_LIBXML_EXCEPTION("Can't add children to rdf:Description");
        }

        if (xmlAddChild(elem, descElem) == nullptr)
            THROW_LIBXML_EXCEPTION("Can't add rdf:Description to existing node");

        return;
    }

    // ISO 16684-2:2014 "5.3 Property serialization"
    // ISO 16684-2:2014 "5.6 Qualifier serialization".
    // Try to convert XMP simple properties and qualifiers to elements
    vector<xmlAttrPtr> attribsToRemove;
    for (xmlAttrPtr attr = elem->properties; attr != nullptr; attr = attr->next)
    {
        if (shouldSkipAttribute(attr))
            continue;

        auto value = utls::GetAttributeValue(attr);
        if (xmlNewChild(elem, attr->ns, attr->name, XMLCHAR value.data()) == nullptr)
            THROW_LIBXML_EXCEPTION("Can't create value replacement node");

        attribsToRemove.push_back(attr);
    }

    for (auto attr : attribsToRemove)
        xmlRemoveProp(attr);
}

// ISO 16684-2:2014 "6.3.3 Array value data types"
void tryFixArrayElement(xmlDocPtr doc, xmlNodePtr& node, const string_view& nodeContent)
{
    if (node->ns == nullptr)
        return;

    auto nodename = utls::GetNodeName(node);
    auto found = s_knownListNodes.find(nodename);
    if (found == s_knownListNodes.end())
        return;

    // Delete existing content
    xmlNodeSetContent(node, nullptr);

    xmlNodePtr newNode;
    utls::SetListNodeContent(doc, node, found->second, nodeContent, newNode);
    node = newNode;
}

bool shouldSkipAttribute(xmlAttrPtr attr)
{
    auto attrname = utls::GetAttributeName(attr);
    if (attrname == "xml:lang")
    {
        return true;
    }
    else if (attrname == "rdf:about")
    {
        return true;
    }
    else if (attrname == "rdf:resource")
    {
        // ISO 16684-1:2019 "7.5 Simple valued XMP properties"
        // The element content for an XMP property with a
        // URI simple value shall be empty. The value shall be
        // provided as the value of an rdf : resource attribute
        // attached to the XML element.
        return true;
    }
    else
    {
        return false;
    }
}

xmlDocPtr createXMPDoc(xmlNodePtr& root)
{
    auto doc = xmlNewDoc(nullptr);

    // https://wwwimages2.adobe.com/content/dam/acom/en/devnet/xmp/pdfs/XMP%20SDK%20Release%20cc-2016-08/XMPSpecificationPart1.pdf
    // See 7.3.2 XMP Packet Wrapper
    auto xpacketBegin = xmlNewPI(XMLCHAR "xpacket", XMLCHAR "begin=\"\357\273\277\" id=\"W5M0MpCehiHzreSzNTczkc9d\"");
    if (xpacketBegin == nullptr || xmlAddChild((xmlNodePtr)doc, xpacketBegin) == nullptr)
    {
        xmlFreeNode(xpacketBegin);
        THROW_LIBXML_EXCEPTION("Can't create xpacket begin node");
    }

    // NOTE: x:xmpmeta element doesn't define any attribute
    // but other attributes can be defined (eg. x:xmptk)
    // and should be ignored by processors
    auto xmpmeta = xmlNewChild((xmlNodePtr)doc, nullptr, XMLCHAR "xmpmeta", nullptr);
    if (xmpmeta == nullptr)
        THROW_LIBXML_EXCEPTION("Can't create x:xmpmeta node");

    auto nsAdobeMeta = xmlNewNs(xmpmeta, XMLCHAR "adobe:ns:meta/", XMLCHAR "x");
    if (nsAdobeMeta == nullptr)
        THROW_LIBXML_EXCEPTION("Can't find or create x namespace");
    xmlSetNs(xmpmeta, nsAdobeMeta);

    auto xpacketEnd = xmlNewPI(XMLCHAR "xpacket", XMLCHAR "end=\"w\"");
    if (xpacketEnd == nullptr || xmlAddChild((xmlNodePtr)doc, xpacketEnd) == nullptr)
    {
        xmlFreeNode(xpacketEnd);
        THROW_LIBXML_EXCEPTION("Can't create xpacket end node");
    }

    root = xmpmeta;
    return doc;
}

xmlNodePtr findRootXMPMeta(xmlDocPtr doc)
{
    auto root = xmlDocGetRootElement(doc);
    if (root == nullptr || (const char*)root->name != "xmpmeta"sv)
        return nullptr;

    return root;
}

xmlNodePtr createRDFElement(xmlNodePtr xmpmeta)
{
    auto rdf = xmlNewChild(xmpmeta, nullptr, XMLCHAR "RDF", nullptr);
    if (rdf == nullptr)
        THROW_LIBXML_EXCEPTION("Can't create rdf:RDF node");

    createRDFNamespace(rdf);
    return rdf;
}

void createRDFNamespace(xmlNodePtr rdf)
{
    auto rdfNs = xmlNewNs(rdf, XMLCHAR "http://www.w3.org/1999/02/22-rdf-syntax-ns#", XMLCHAR "rdf");
    if (rdfNs == nullptr)
        THROW_LIBXML_EXCEPTION("Can't find or create rdf namespace");
    xmlSetNs(rdf, rdfNs);
}

xmlNodePtr createDescriptionElement(xmlNodePtr rdf)
{
    auto description = xmlNewChild(rdf, nullptr, XMLCHAR "Description", nullptr);
    if (description == nullptr)
        THROW_LIBXML_EXCEPTION("Can't create rdf:Description node");

    auto nsRDF = xmlNewNs(description, XMLCHAR "http://www.w3.org/1999/02/22-rdf-syntax-ns#", XMLCHAR "rdf");
    if (nsRDF == nullptr)
        THROW_LIBXML_EXCEPTION("Can't find or create rdf namespace");

    xmlSetNs(description, nsRDF);
    if (xmlSetNsProp(description, nsRDF, XMLCHAR "about", XMLCHAR "") == nullptr)
        THROW_LIBXML_EXCEPTION(utls::Format("Can't set rdf:about attribute on rdf:Description node"));

    return description;
}

void serializeXMPMetadataTo(string& str, xmlDocPtr xmpMeta)
{
    auto ctx = xmlSaveToIO(xmlOutputStringWriter, xmlOutputStringWriterClose, &str, nullptr, XML_SAVE_NO_DECL | XML_SAVE_FORMAT);
    if (ctx == nullptr || xmlSaveDoc(ctx, xmpMeta) == -1 || xmlSaveClose(ctx) == -1)
        THROW_LIBXML_EXCEPTION("Can't save XPM fragment");
}

int xmlOutputStringWriter(void* context, const char* buffer, int len)
{
    auto str = (string*)context;
    str->append(buffer, (size_t)len);
    return len;
}

int xmlOutputStringWriterClose(void* context)
{
    (void)context;
    // Do nothing
    return 0;
}

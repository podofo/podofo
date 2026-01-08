#include "XmlUtils.h"
/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include "PdfDeclarationsPrivate.h"
#include "XmlUtils.h"
#include <libxml/xmlsave.h>

using namespace std;
using namespace PoDoFo;

static int xmlOutputStringWriter(void* context, const char* buffer, int len);
static int xmlOutputStringWriterClose(void* context);

void utls::InitXml()
{
    LIBXML_TEST_VERSION;
}

xmlNodePtr utls::FindDescendantElement(xmlNodePtr element, const string_view& name)
{
    return FindDescendantElement(element, { }, name);
}

xmlNodePtr utls::FindDescendantElement(xmlNodePtr element, const string_view& ns, const string_view& name)
{
    for (auto child = xmlFirstElementChild(element); child != nullptr; child = xmlNextElementSibling(child))
    {
        if ((ns.length() == 0 || (child->ns != nullptr
            && ns == (const char*)child->ns->href))
            && name == (const char*)child->name)
        {
            return child;
        }

        auto ret = FindDescendantElement(child, ns, name);
        if (ret != nullptr)
            return ret;
    }

    return nullptr;
}

xmlNodePtr utls::FindChildElement(xmlNodePtr element, const string_view& name)
{
    return FindChildElement(element, { }, name);
}

xmlNodePtr utls::FindChildElement(xmlNodePtr element, const string_view& ns, const string_view& name)
{
    for (auto child = xmlFirstElementChild(element); child != nullptr; child = xmlNextElementSibling(child))
    {
        if ((ns.length() == 0 || (child->ns != nullptr
            && ns == (const char*)child->ns->href))
            && name == (const char*)child->name)
        {
            return child;
        }
    }

    return nullptr;
}

xmlNodePtr utls::FindSiblingElement(xmlNodePtr element, const string_view& name)
{
    return FindSiblingElement(element, { }, name);
}

xmlNodePtr utls::FindSiblingElement(xmlNodePtr element, const string_view& ns, const string_view& name)
{
    for (auto sibling = xmlNextElementSibling(element); sibling; sibling = xmlNextElementSibling(sibling))
    {
        if ((ns.length() == 0 || (sibling->ns != nullptr
                && ns == (const char*)sibling->ns->href))
            && name == (const char*)sibling->name)
        {
            return sibling;
        }
    }

    return nullptr;
}

nullable<string> utls::FindAttribute(xmlNodePtr element, const string_view& name)
{
    xmlAttrPtr attr;
    return FindAttribute(element, { }, name, attr);
}

nullable<std::string> utls::FindAttribute(xmlNodePtr element, const string_view& ns, const string_view& name)
{
    xmlAttrPtr attr;
    return FindAttribute(element, ns, name, attr);
}

nullable<string> utls::FindAttribute(xmlNodePtr element, const string_view& name, xmlAttrPtr& attr)
{
    return FindAttribute(element, { }, name, attr);
}

nullable<string> utls::FindAttribute(xmlNodePtr element, const string_view& ns, const string_view& name, xmlAttrPtr& found)
{
    for (xmlAttrPtr attr = element->properties; attr != nullptr; attr = attr->next)
    {
        if ((ns.length() == 0 || (attr->ns != nullptr
                && ns == (const char*)attr->ns->href))
            && name == (const char*)attr->name)
        {
            found = attr;
            return GetNodeContent((xmlNodePtr)attr);
        }
    }

    found = nullptr;
    return { };
}

nullable<string> utls::GetNodeContent(xmlNodePtr node)
{
    PODOFO_ASSERT(node != nullptr);
    xmlChar* content = xmlNodeGetContent(node);
    if (content == nullptr)
        return { };

    unique_ptr<xmlChar, decltype(xmlFree)> contentFree(content, xmlFree);
    return string((const char*)content);
}

string utls::GetAttributeValue(xmlAttrPtr attr)
{
    PODOFO_ASSERT(attr != nullptr);
    xmlChar* content = xmlNodeGetContent((xmlNodePtr)attr);
    unique_ptr<xmlChar, decltype(xmlFree)> contentFree(content, xmlFree);
    return string((const char*)content);
}

string utls::GetAttributeName(xmlAttrPtr attr)
{
    return GetNodePrefixedName((xmlNodePtr)attr);
}

string utls::GetNodePrefixedName(xmlNodePtr node)
{
    if (node->ns == nullptr)
    {
        return (const char*)node->name;
    }
    else
    {
        string nodename((const char*)node->ns->prefix);
        nodename.push_back(':');
        nodename.append((const char*)node->name);
        return nodename;
    }
}

string_view utls::GetNodeName(xmlNodePtr node)
{
    return (const char*)node->name;
}

string_view utls::GetNodePrefix(xmlNodePtr node)
{
    return node->ns == nullptr ? string_view() : string_view((const char*)node->ns->prefix);
}

string_view utls::GetNodeNamespace(xmlNodePtr node)
{
    return node->ns == nullptr ? string_view() : string_view((const char*)node->ns->href);
}

void utls::NavigateDescendantElements(xmlNodePtr element, const string_view& name, const function<void(xmlNodePtr)>& action)
{
    NavigateDescendantElements(element, { }, name, action);
}

void utls::NavigateDescendantElements(xmlNodePtr element, const string_view& ns, const string_view& name, const function<void(xmlNodePtr)>& action)
{
    for (auto child = xmlFirstElementChild(element); child != nullptr; child = xmlNextElementSibling(child))
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;

        if ((ns.length() == 0 || (child->ns != nullptr
            && ns == (const char*)child->ns->href))
            && name == (const char*)child->name)
        {
            action(child);
        }
        else
        {
            NavigateDescendantElements(child, ns, name, action);
        }
    }
}

bool utls::TrySerializeXmlDocTo(string& str, xmlDocPtr doc)
{
    auto ctx = xmlSaveToIO(xmlOutputStringWriter, xmlOutputStringWriterClose, &str, nullptr, XML_SAVE_NO_DECL | XML_SAVE_FORMAT);
    if (ctx == nullptr || xmlSaveDoc(ctx, doc) == -1 || xmlSaveClose(ctx) == -1)
        return false;

    return true;
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

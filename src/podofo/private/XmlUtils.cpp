/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include "PdfDeclarationsPrivate.h"
#include "XmlUtils.h"

using namespace std;
using namespace PoDoFo;

xmlNodePtr utls::FindChildElement(xmlNodePtr element, const std::string_view& name)
{
    return FindChildElement(element, { }, name);
}

xmlNodePtr utls::FindChildElement(xmlNodePtr element, const string_view& prefix, const string_view& name)
{
    for (auto child = xmlFirstElementChild(element); child != nullptr; child = xmlNextElementSibling(child))
    {
        if (child->ns != nullptr
            && prefix == (const char*)child->ns->prefix
            && name == (const char*)child->name)
        {
            return child;
        }
    }

    return nullptr;
}

xmlNodePtr utls::FindSiblingNode(xmlNodePtr element, const std::string_view& name)
{
    return FindSiblingNode(element, { }, name);
}

xmlNodePtr utls::FindSiblingNode(xmlNodePtr element, const string_view& prefix, const string_view& name)
{
    for (auto sibling = xmlNextElementSibling(element); sibling; sibling = xmlNextElementSibling(sibling))
    {
        if ((prefix.length() == 0 || (sibling->ns != nullptr
                && prefix == (const char*)sibling->ns->prefix))
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

nullable<std::string> utls::FindAttribute(xmlNodePtr element, const string_view& prefix, const string_view& name)
{
    xmlAttrPtr attr;
    return FindAttribute(element, prefix, name, attr);
}

nullable<string> utls::FindAttribute(xmlNodePtr element, const string_view& name, xmlAttrPtr& attr)
{
    return FindAttribute(element, { }, name, attr);
}

nullable<string> utls::FindAttribute(xmlNodePtr element, const string_view& prefix, const string_view& name, xmlAttrPtr& found)
{
    for (xmlAttrPtr attr = element->properties; attr != nullptr; attr = attr->next)
    {
        if ((prefix.length() == 0 || (attr->ns != nullptr
                && prefix == (const char*)attr->ns->prefix))
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
    return GetNodeName((xmlNodePtr)attr);
}

string utls::GetNodeName(xmlNodePtr node)
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

void utls::InitXml()
{
    LIBXML_TEST_VERSION;
}

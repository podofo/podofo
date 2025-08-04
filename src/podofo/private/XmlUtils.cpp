/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include "PdfDeclarationsPrivate.h"
#include "XmlUtils.h"

using namespace std;
using namespace PoDoFo;

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

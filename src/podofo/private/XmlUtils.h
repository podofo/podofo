/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include <string>
#include <libxml/tree.h>
#include <libxml/xmlerror.h>
#include <podofo/auxiliary/nullable.h>

// Cast macro that keep or enforce const to use
// instead of BAD_CAST
#define XMLCHAR (const xmlChar*)

#define THROW_LIBXML_EXCEPTION(msg)\
{\
    const xmlError* error_ = xmlGetLastError();\
    if (error_ == nullptr)\
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::XmpMetadataError, msg);\
    else\
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::XmpMetadataError, "{}, internal error: {}", msg, error_->message);\
}

namespace utls
{
    void InitXml();
    xmlNodePtr FindDescendantElement(xmlNodePtr element, const std::string_view& name);
    xmlNodePtr FindDescendantElement(xmlNodePtr element, const std::string_view& ns, const std::string_view& name);
    xmlNodePtr FindChildElement(xmlNodePtr element, const std::string_view& name);
    xmlNodePtr FindChildElement(xmlNodePtr element, const std::string_view& ns, const std::string_view& name);
    xmlNodePtr FindSiblingElement(xmlNodePtr element, const std::string_view& name);
    xmlNodePtr FindSiblingElement(xmlNodePtr element, const std::string_view& ns, const std::string_view& name);
    void NavigateDescendantElements(xmlNodePtr element, const std::string_view& name,
        const std::function<void(xmlNodePtr)>& action);
    void NavigateDescendantElements(xmlNodePtr element, const std::string_view& ns,
        const std::string_view& name, const std::function<void(xmlNodePtr)>& action);
    PoDoFo::nullable<std::string> FindAttribute(xmlNodePtr element, const std::string_view& name);
    PoDoFo::nullable<std::string> FindAttribute(xmlNodePtr element, const std::string_view& ns, const std::string_view& name);
    PoDoFo::nullable<std::string> FindAttribute(xmlNodePtr element, const std::string_view& name, xmlAttrPtr& ptr);
    PoDoFo::nullable<std::string> FindAttribute(xmlNodePtr element, const std::string_view& ns, const std::string_view& name, xmlAttrPtr& ptr);
    PoDoFo::nullable<std::string> GetNodeContent(xmlNodePtr element);
    std::string GetAttributeValue(const xmlAttrPtr attr);
    std::string GetNodePrefixedName(xmlNodePtr node);
    std::string_view GetNodeName(xmlNodePtr node);
    std::string_view GetNodePrefix(xmlNodePtr node);
    std::string_view GetNodeNamespace(xmlNodePtr node);
    std::string GetAttributeName(xmlAttrPtr attr);
}

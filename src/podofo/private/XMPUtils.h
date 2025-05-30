/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "XmlUtils.h"
#include <podofo/main/PdfMetadataStore.h>

namespace PoDoFo
{
    /**
     * \remarks the store is not cleared: the function sets only read properties
     */
    void GetXMPMetadata(xmlNodePtr description, PdfMetadataStore& metadata);
    void SetXMPMetadata(xmlDocPtr doc, xmlNodePtr description, const PdfMetadataStore& metadata);
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


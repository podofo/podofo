/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfColorSpace.h"
#include "PdfDocument.h"

using namespace std;
using namespace PoDoFo;

PdfColorSpace::PdfColorSpace(PdfDocument& doc, const PdfColorSpaceFilterPtr& filter)
    : PdfDictionaryElement(doc), m_Filter(filter)
{
    GetObject() = filter->GetExportObject(doc.GetObjects());
}

PdfColorSpaceInitializer::PdfColorSpaceInitializer()
    : m_Element(nullptr)
{
}

PdfColorSpaceInitializer::PdfColorSpaceInitializer(const PdfColorSpaceFilterPtr& filter)
    : m_Filter(filter), m_Element(nullptr)
{
    if (filter == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The input filter must not be nullptr");
}

PdfColorSpaceInitializer::PdfColorSpaceInitializer(const PdfColorSpace& colorSpace)
    : m_Filter(colorSpace.GetFilterPtr()), m_Element(&colorSpace)
{
}

PdfColorSpaceInitializer::PdfColorSpaceInitializer(PdfColorSpaceType colorSpace)
    : m_Filter(PdfColorSpaceFilterFactory::GetTrivialFilter(colorSpace)), m_Element(nullptr)
{
}

PdfObject PdfColorSpaceInitializer::GetExportObject(PdfIndirectObjectList& objects) const
{
    if (m_Element != nullptr)
        return m_Element->GetObject().GetIndirectReference();
    else
        return m_Filter->GetExportObject(objects);
}

bool PdfColorSpaceInitializer::IsNull() const
{
    return m_Filter == nullptr;
}

PdfColorSpaceFilterPtr PdfColorSpaceInitializer::Take(const PdfColorSpace*& element)
{
    element = m_Element;
    m_Element = nullptr;
    return std::move(m_Filter);
}

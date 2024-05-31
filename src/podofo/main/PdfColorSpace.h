/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_COLOR_SPACE_H
#define PDF_COLOR_SPACE_H

#include "PdfElement.h"
#include "PdfColorSpaceFilter.h"

namespace PoDoFo {

class PdfDocument;

class PODOFO_API PdfColorSpace final : public PdfDictionaryElement
{
    friend class PdfDocument;
private:
    PdfColorSpace(PdfDocument& doc, const PdfColorSpaceFilterPtr& filter);

    PdfColorSpace(const PdfColorSpace&) = default;
public:
    const PdfColorSpaceFilter& GetFilter() const { return *m_Filter; }
    PdfColorSpaceFilterPtr GetFilterPtr() const { return m_Filter; }
private:
    PdfColorSpaceFilterPtr m_Filter;
};

}

#endif // PDF_COLOR_SPACE_H

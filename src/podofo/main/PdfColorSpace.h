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

/**
 * A proxy class that can used to identify a color space choosing
 * from several input types
 */
class PODOFO_API PdfColorSpaceInitializer final
{
    friend class PdfGraphicsStateWrapper;

    PODOFO_STACK_ONLY

public:
    /** A null color space
     */
    PdfColorSpaceInitializer();

    /** Identify a color space from a filter
     */
    PdfColorSpaceInitializer(const PdfColorSpaceFilterPtr& filter);
    /** Identify a color space from color space document element
     */
    PdfColorSpaceInitializer(const PdfColorSpace& colorSpace);
    /** Identify a trivial colorspace from its enum type (DeviceGray, DeviceRGB or DeviceCYMC)
     */
    PdfColorSpaceInitializer(PdfColorSpaceType colorSpace);

public:
    PdfObject GetExportObject(PdfIndirectObjectList& objects) const;

    bool IsNull() const;

    PdfColorSpaceInitializer(const PdfColorSpaceInitializer&) = default;
    PdfColorSpaceInitializer& operator=(const PdfColorSpaceInitializer&) = default;

public:
    const PdfColorSpaceFilter& GetFilter() const { return *m_Filter; }
    PdfColorSpaceFilterPtr GetFilterPtr() const { return m_Filter; }

private:
    PdfColorSpaceFilterPtr Take(const PdfColorSpace*& element);

private:
    PdfColorSpaceFilterPtr m_Filter;
    const PdfColorSpace* m_Element;
};

}

#endif // PDF_COLOR_SPACE_H

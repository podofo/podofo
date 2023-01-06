/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_FONT_OBJECT_H
#define PDF_FONT_OBJECT_H

#include "PdfFont.h"

namespace PoDoFo {

class PODOFO_API PdfFontObject final : public PdfFont
{
    friend class PdfFont;

private:
    /** Create a PdfFontObject based on an existing PdfObject
     *  To be used by PdfFontFactory
     */
    PdfFontObject(PdfObject& obj, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding);

public:
    static std::unique_ptr<PdfFontObject> Create(PdfObject& obj, PdfObject& descendantObj,
        const PdfFontMetricsConstPtr& metrics, const PdfEncoding& encoding);

    static std::unique_ptr<PdfFontObject> Create(PdfObject& obj,
        const PdfFontMetricsConstPtr& metrics, const PdfEncoding& encoding);

public:
    bool tryMapCIDToGID(unsigned cid, unsigned& gid) const override;

public:
    bool IsObjectLoaded() const override;
    PdfFontType GetType() const override;
};

}

#endif // PDF_FONT_OBJECT_H

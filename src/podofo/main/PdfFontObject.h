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
    PdfFontObject(PdfObject& obj, PdfFontMetricsConstPtr&& metrics,
        const PdfEncoding& encoding);

private:
    // To be used by PdfFont
    static std::unique_ptr<PdfFontObject> Create(PdfObject& obj, PdfObject& descendantObj,
        PdfFontMetricsConstPtr&& metrics, const PdfEncoding& encoding);

    static std::unique_ptr<PdfFontObject> Create(PdfObject& obj,
        PdfFontMetricsConstPtr&& metrics, const PdfEncoding& encoding);

public:
    bool IsObjectLoaded() const override;
};

}

#endif // PDF_FONT_OBJECT_H

/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_FONT_CID_TYPE1_H
#define PDF_FONT_CID_TYPE1_H

#include "PdfFontCID.h"

namespace PoDoFo {

/** A PdfFont that represents a CID-keyed font that has a CFF font backend (aka "CIDFontType0")
 */
class PODOFO_API PdfFontCIDCFF final : public PdfFontCID
{
    friend class PdfFont;

private:
    /** Create a new CID font.
     *
     *  \param parent parent of the font object
     *  \param metrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is
     *         deleted along with the font.
     *  \param encoding the encoding of this font
     */
    PdfFontCIDCFF(PdfDocument& doc, PdfFontMetricsConstPtr&& metrics,
        const PdfEncoding& encoding);

public:
    bool SupportsSubsetting() const override;

protected:
    void embedFontFileSubset(const std::vector<PdfCharGIDInfo>& infos,
        const PdfCIDSystemInfo& cidInfo) override;
};

};

#endif // PDF_FONT_CID_TYPE1_H

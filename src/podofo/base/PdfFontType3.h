/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FONT_TYPE3_H
#define PDF_FONT_TYPE3_H

#include "PdfDeclarations.h"

#include "PdfFontSimple.h"

namespace PoDoFo {

/** A PdfFont implementation that can be used
 *  to embedd type3 fonts into a PDF file
 *  or to draw with type3 fonts.
 *
 *  Type3 fonts are always embedded.
 */
class PdfFontType3 final : public PdfFontSimple
{
    friend class PdfFont;
private:

    /** Create a new Type3 font.
     *
     *  It will get embedded automatically.
     *
     *  \param doc parent of the font object
     *  \param metrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is
     *         deleted along with the font.
     *  \param encoding the encoding of this font. The font will take ownership of this object
     *                   depending on pEncoding->IsAutoDelete()
     *  \param embed if true the font will get embedded.
     *
     */
    PdfFontType3(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding);

public:
    PdfFontType GetType() const override;
};

};

#endif // PDF_FONT_TYPE3_H

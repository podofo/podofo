/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FONT_TYPE1_H
#define PDF_FONT_TYPE1_H

#include "PdfDeclarations.h"

#include <set>

#include "PdfFontSimple.h"

namespace PoDoFo {

/** A PdfFont implementation that can be used
 *  to embed type1 fonts into a PDF file
 *  or to draw with type1 fonts.
 */
class PODOFO_API PdfFontType1 final : public PdfFontSimple
{
    friend class PdfFont;

private:

    /** Create a new Type1 font object.
     *
     *  \param doc parent of the font object
     *  \param metrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is
     *         deleted along with the font.
     *  \param encoding the encoding of this font
     *
     */
    PdfFontType1(PdfDocument& doc, PdfFontMetricsConstPtr&& metrics,
        const PdfEncoding& encoding);
};

};

#endif // PDF_FONT_TYPE1_H


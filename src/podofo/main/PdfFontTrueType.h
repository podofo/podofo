/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FONT_TRUE_TYPE_H
#define PDF_FONT_TRUE_TYPE_H

#include "PdfDeclarations.h"

#include "PdfFontSimple.h"

namespace PoDoFo {

/** A PdfFont implementation that can be used
 *  to embed truetype fonts into a PDF file
 *  or to draw with truetype fonts.
 *
 *  TrueType fonts are always embedded as suggested in the PDF reference.
 */
class PODOFO_API PdfFontTrueType final : public PdfFontSimple
{
    friend class PdfFont;

private:

    /** Create a new TrueType font.
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
    PdfFontTrueType(PdfDocument& doc, PdfFontMetricsConstPtr&& metrics,
        const PdfEncoding& encoding);
};

};

#endif // PDF_FONT_TRUE_TYPE_H


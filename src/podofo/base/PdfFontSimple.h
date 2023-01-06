/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FONT_SIMPLE_H
#define PDF_FONT_SIMPLE_H

#include "PdfDeclarations.h"

#include "PdfFont.h"

namespace PoDoFo {

/** This is a common base class for simple, non CID-keyed fonts
 * like Type1, TrueType and Type3
 */
class PdfFontSimple : public PdfFont
{
    friend class PdfFontStandard14;

protected:
    /** Create a new PdfFont object which will introduce itself
     *  automatically to every page object it is used on.
     *
     *  \param doc parent of the font object
     *  \param metrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is
     *         deleted along with the font.
     *  \param encoding the encoding of this font. The font will take ownership of this object
     *                   depending on pEncoding->IsAutoDelete()
     *
     */
    PdfFontSimple(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding);

protected:
    void Init();

    void embedFont() override final;

    void initImported() override;

private:
    void getWidthsArray(PdfArray& widths) const;
    void getFontMatrixArray(PdfArray& fontMatrix) const;

protected:
    PdfObject* m_Descriptor;
};

};

#endif // PDF_FONT_SIMPLE_H

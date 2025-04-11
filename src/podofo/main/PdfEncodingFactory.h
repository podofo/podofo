/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_ENCODING_FACTORY_H
#define PDF_ENCODING_FACTORY_H

#include "PdfEncoding.h"

namespace PoDoFo {

class PdfFontMetrics;

/** This factory creates a PdfEncoding
 *  from an existing object in the PDF.
 */
class PODOFO_API PdfEncodingFactory final
{
    friend class PdfFont;

public:
    /** Create a new PdfEncoding from either an
     *  encoding name or an encoding dictionary.
     *
     *  \param fontObj font object
     *  \param toUnicode the optional ToUnicode dictionary
     *
     *  \returns a PdfEncoding or nullptr
     */
    static PdfEncoding CreateEncoding(
        const PdfObject& fontObj, const PdfFontMetrics& metrics);

public:
    /** Singleton method which returns a global instance
     *  of WinAnsiEncoding.
     *
     *  \returns global instance of WinAnsiEncoding
     *
     *  \see GlobalWin1250EncodingInstance, GlobalIso88592EncodingInstance
     */
    static PdfEncoding CreateWinAnsiEncoding();

    /** Singleton method which returns a global instance
     *  of MacRomanEncoding.
     *
     *  \returns global instance of MacRomanEncoding
     */
    static PdfEncoding CreateMacRomanEncoding();

    /** Singleton method which returns a global instance
     *  of MacExpertEncoding.
     *
     *  \returns global instance of MacExpertEncoding
     */
    static PdfEncoding CreateMacExpertEncoding();

private:
    static PdfEncoding CreateEncoding(const PdfDictionary& fontDict, const PdfFontMetrics& metrics,
        const PdfObject* descendantFont);

    static PdfEncodingMapConstPtr createEncodingMap(const PdfObject& obj,
        const PdfFontMetrics& metrics);

private:
    PdfEncodingFactory() = delete;
};

}

#endif // PDF_ENCODING_FACTORY_H

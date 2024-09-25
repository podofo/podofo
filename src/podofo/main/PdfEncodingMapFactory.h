/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_ENCODING_MAP_FACTORY_H
#define PDF_ENCODING_MAP_FACTORY_H

#include "PdfEncodingMap.h"
#include "PdfCMapEncoding.h"

namespace PoDoFo {

/** This factory creates a PdfEncodingMap
 */
class PODOFO_API PdfEncodingMapFactory final
{
    friend class PdfEncoding;
    friend class PdfEncodingFactory;
    friend class PdfDifferenceEncoding;
    friend class PdfFontMetricsFreetype;

public:
    /** Try to parse a CMap encoding from an object
     * \remarks The results may be a PdfCMapEncoding or PdfIdentityEncoding
     */
    static bool TryParseCMapEncoding(const PdfObject& cmapObj, std::unique_ptr<PdfEncodingMap>& encoding);

    /** Parse a CMap encoding from an object
     * \remarks Throws if parse failed
     * \returns The results may be a non null PdfCMapEncoding or PdfIdentityEncoding on succces
     */
    static std::unique_ptr<PdfEncodingMap> ParseCMapEncoding(const PdfObject& cmapObj);

    /** Singleton method which returns a global instance
     *  of WinAnsiEncoding.
     *
     *  \returns global instance of WinAnsiEncoding
     *
     *  \see Win1250EncodingInstance
     */
    static PdfBuiltInEncodingConstPtr WinAnsiEncodingInstance();

    /** Singleton method which returns a global instance
     *  of MacRomanEncoding.
     *
     *  \returns global instance of MacRomanEncoding
     */
    static PdfBuiltInEncodingConstPtr MacRomanEncodingInstance();

    /** Singleton method which returns a global instance
     *  of MacExpertEncoding.
     *
     *  \returns global instance of MacExpertEncoding
     */
    static PdfBuiltInEncodingConstPtr MacExpertEncodingInstance();

    /** Singleton method which returns a global instance
     *  of Horizontal IndentityEncoding
     *
     *  \returns global instance of Horizontal IdentityEncoding
     */
    static PdfEncodingMapConstPtr TwoBytesHorizontalIdentityEncodingInstance();

    /** Singleton method which returns a global instance
     *  of Vertical IndentityEncoding
     *
     *  \returns global instance of Vertical IdentityEncoding
     */
    static PdfEncodingMapConstPtr TwoBytesVerticalIdentityEncodingInstance();

    /** Return the encoding map for the given standard font type or nullptr for unknown
     */
    static PdfEncodingMapConstPtr GetStandard14FontEncodingMap(PdfStandard14FontType stdFont);

    /** Get a predefined CMap
     * \returns The found map or nullptr if absent 
     */
    static PdfCMapEncodingConstPtr GetPredefinedCMap(const std::string_view& cmapName);

private:
    PdfEncodingMapFactory() = delete;

    // The following encodings are for internal use only

    static PdfBuiltInEncodingConstPtr StandardEncodingInstance();

    static PdfBuiltInEncodingConstPtr SymbolEncodingInstance();

    static PdfBuiltInEncodingConstPtr ZapfDingbatsEncodingInstance();

    static PdfBuiltInEncodingConstPtr AppleLatin1EncodingInstance();

    static PdfEncodingMapConstPtr GetNullEncodingMap();
};

}

#endif // PDF_ENCODING_MAP_FACTORY_H

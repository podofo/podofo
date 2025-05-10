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
    friend class PdfFontMetrics;

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
    static PdfBuiltInEncodingConstPtr GetWinAnsiEncodingInstancePtr();
    static const PdfBuiltInEncoding& GetWinAnsiEncodingInstance();

    /** Singleton method which returns a global instance
     *  of MacRomanEncoding.
     *
     * \returns global instance of MacRomanEncoding
     * \remarks the encoding here also defines the entries specified in
     * ISO 32000-2:2020 "Table 113 — Additional entries in Mac OS Roman
     * encoding not in MacRomanEncoding", other than the ones specified
     * in "Table D.2 — Latin character set and encodings"
     */
    static PdfBuiltInEncodingConstPtr GetMacRomanEncodingInstancePtr();
    static const PdfBuiltInEncoding& GetMacRomanEncodingInstance();

    /** Singleton method which returns a global instance
     *  of MacExpertEncoding.
     *
     *  \returns global instance of MacExpertEncoding
     */
    static PdfBuiltInEncodingConstPtr GetMacExpertEncodingInstancePtr();
    static const PdfBuiltInEncoding& GetMacExpertEncodingInstance();

    /** Singleton method which returns a global instance
     *  of StandardEncdoing.
     *
     *  \returns global instance of StandardEncdoing
     */
    static PdfBuiltInEncodingConstPtr GetStandardEncodingInstancePtr();
    static const PdfBuiltInEncoding& GetStandardEncodingInstance();

    /** Singleton method which returns a global instance
     *  of the 2 bytes /Identity-H horizontal identity encoding
     *
     *  \returns global instance of Horizontal IdentityEncoding
     */
    static PdfEncodingMapConstPtr GetHorizontalIdentityEncodingInstancePtr();
    static const PdfEncodingMap& GetHorizontalIdentityEncodingInstance();

    /** Singleton method which returns a global instance
     *  of the 2 bytes /Identity-V vertical identity encoding
     *
     *  \returns global instance of Vertical IdentityEncoding
     */
    static PdfEncodingMapConstPtr GetVerticalIdentityEncodingInstancePtr();
    static const PdfEncodingMap& GetVerticalIdentityEncodingInstance();

    /** Return the encoding map for the given standard font type or nullptr for unknown
     */
    static PdfEncodingMapConstPtr GetStandard14FontEncodingInstancePtr(PdfStandard14FontType stdFont);
    static const PdfEncodingMap& GetStandard14FontEncodingInstance(PdfStandard14FontType stdFont);

    /** Get a predefined CMap
     * \returns The found map or nullptr if absent 
     */
    static PdfCMapEncodingConstPtr GetPredefinedCMapInstancePtr(const std::string_view& cmapName);
    static const PdfCMapEncoding& GetPredefinedCMapInstance(const std::string_view& cmapName);
private:
    // The following encodings are for internal use only

    static const PdfBuiltInEncodingConstPtr& GetSymbolEncodingInstancePtr();

    static const PdfBuiltInEncodingConstPtr& GetZapfDingbatsEncodingInstancePtr();

    static const PdfBuiltInEncodingConstPtr& GetAppleLatin1EncodingInstancePtr();

    static const PdfEncodingMapConstPtr& GetNullEncodingInstancePtr();

private:
    PdfEncodingMapFactory() = delete;

    static const PdfBuiltInEncodingConstPtr& getWinAnsiEncodingInstancePtr();
    static const PdfBuiltInEncodingConstPtr& getMacRomanEncodingInstancePtr();
    static const PdfBuiltInEncodingConstPtr& getMacExpertEncodingInstancePtr();
    static const PdfBuiltInEncodingConstPtr& getStandardEncodingInstancePtr();
    static const PdfEncodingMapConstPtr& getHorizontalIdentityEncodingInstancePtr();
    static const PdfEncodingMapConstPtr& getVerticalIdentityEncodingInstancePtr();
    static const PdfBuiltInEncodingConstPtr& getStandard14FontEncodingInstancePtr(PdfStandard14FontType stdFont);
};

}

#endif // PDF_ENCODING_MAP_FACTORY_H

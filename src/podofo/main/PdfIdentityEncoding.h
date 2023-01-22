/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_IDENTITY_ENCODING_H
#define PDF_IDENTITY_ENCODING_H

#include "PdfEncodingMap.h"
#include "PdfObject.h"

namespace PoDoFo {

/** Orientation for predefined CID identity encodings
 */
enum class PdfIdentityOrientation
{
    Unkwnown = 0,
    Horizontal, // Corresponts to /Identity-H
    Vertical,   // Corresponts to /Identity-V
};

/** PdfIdentityEncoding is a two-byte encoding which can be
 *  used with TrueType fonts to represent all characters
 *  present in a font. If the font contains all unicode
 *  glyphs, PdfIdentityEncoding will support all unicode
 *  characters.
 */
class PODOFO_API PdfIdentityEncoding final : public PdfEncodingMap
{
    friend class PdfCMapEncoding;

public:
    /**
     *  Create a new PdfIdentityEncoding.
     *
     *  \param codeSpaceSize size of the codespace size
     */
    PdfIdentityEncoding(unsigned char codeSpaceSize);

    /**
     *  Create a standard 2 bytes CID PdfIdentityEncoding
     */
    PdfIdentityEncoding(PdfIdentityOrientation orientation);

protected:
    bool tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const override;
    bool tryGetCodePoints(const PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const override;
    void getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const override;
    void AppendToUnicodeEntries(OutputStream& stream, charbuff& temp) const override;
    void AppendCIDMappingEntries(OutputStream& stream, const PdfFont& font, charbuff& temp) const override;

public:
    const PdfEncodingLimits& GetLimits() const override;

private:
    PdfIdentityEncoding(PdfEncodingMapType type, const PdfEncodingLimits& limits,
        PdfIdentityOrientation orientation);

private:
    PdfEncodingLimits m_Limits;
    PdfIdentityOrientation m_orientation;
};

};

#endif // PDF_IDENTITY_ENCODING_H

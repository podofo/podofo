/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FONT_STANDARD14_DATA_H
#define PDF_FONT_STANDARD14_DATA_H

#include <podofo/main/PdfFontMetricsStandard14.h>
#include <podofo/auxiliary/Corners.h>

namespace PoDoFo {

struct Standard14FontData final
{
    const unsigned short* Widths;
    unsigned WidthsSize;
    PdfFontDescriptorFlags Flags;
    uint16_t DefaultWidth;
    PdfFontStretch Stretch;
    int16_t Ascent;
    int16_t Descent;
    uint16_t XHeight;
    uint16_t CapHeight;
    int16_t ItalicAngle;
    int16_t Weight;
    int16_t StemV;
    int16_t StemH;
    int16_t StrikeThroughPos;
    int16_t UnderlinePos;
    Corners BBox;
};

using Std14CPToGIDMap = std::unordered_map<unsigned short, unsigned char>;

std::string_view GetStandard14FontName(PdfStandard14FontType stdFont);

std::string_view GetStandard14FontFamilyName(PdfStandard14FontType stdFont);

std::string_view GetStandard14FontBaseName(PdfStandard14FontType stdFont);

bool IsStandard14Font(const std::string_view& fontName, bool useAltNames, PdfStandard14FontType& stdFont);

const unsigned short* GetStd14FontWidths(PdfStandard14FontType stdFont, unsigned& size);

const Std14CPToGIDMap& GetStd14CPToGIDMap(PdfStandard14FontType stdFont);

bufferview GetStandard14FontFileData(PdfStandard14FontType stdFont);

const Standard14FontData& GetStandard14FontData(PdfStandard14FontType std14Font);

};

#endif // PDF_FONT_STANDARD14_DATA_H

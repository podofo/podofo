/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "grayscaleconverter.h"

using namespace PoDoFo;

GrayscaleConverter::GrayscaleConverter()
    : IConverter()
{
}

GrayscaleConverter::~GrayscaleConverter()
{
}

void GrayscaleConverter::StartPage(PdfPage&, int)
{
}

void GrayscaleConverter::EndPage(PdfPage&, int)
{
}

void GrayscaleConverter::StartXObject(PdfXObject&)
{
}

void GrayscaleConverter::EndXObject(PdfXObject&)
{
}

PoDoFo::PdfColor GrayscaleConverter::SetStrokingColorGray(const PdfColor& color)
{
    return color;
}

PoDoFo::PdfColor GrayscaleConverter::SetStrokingColorRGB(const PdfColor& color)
{
    return color.ConvertToGrayScale();
}

PoDoFo::PdfColor GrayscaleConverter::SetStrokingColorCMYK(const PdfColor& color)
{
    return color.ConvertToGrayScale();
}

PoDoFo::PdfColor GrayscaleConverter::SetNonStrokingColorGray(const PdfColor& color)
{
    return color.ConvertToGrayScale();
}

PoDoFo::PdfColor GrayscaleConverter::SetNonStrokingColorRGB(const PdfColor& color)
{
    return color.ConvertToGrayScale();
}

PoDoFo::PdfColor GrayscaleConverter::SetNonStrokingColorCMYK(const PdfColor& color)
{
    return color.ConvertToGrayScale();
}

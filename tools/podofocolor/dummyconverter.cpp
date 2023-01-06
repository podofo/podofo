/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dummyconverter.h"

using namespace PoDoFo;

DummyConverter::DummyConverter()
    : IConverter()
{

}

void DummyConverter::StartPage(PdfPage&, int)
{
}

void DummyConverter::EndPage(PdfPage&, int)
{
}

void DummyConverter::StartXObject(PdfXObject&)
{
}

void DummyConverter::EndXObject(PdfXObject&)
{
}

PdfColor DummyConverter::SetStrokingColorGray(const PdfColor&)
{
    return PdfColor(1.0, 0.0, 0.0);
}

PdfColor DummyConverter::SetStrokingColorRGB(const PdfColor&)
{
    return PdfColor(1.0, 0.0, 0.0);
}

PdfColor DummyConverter::SetStrokingColorCMYK(const PdfColor&)
{
    return PdfColor(1.0, 0.0, 0.0);
}

PdfColor DummyConverter::SetNonStrokingColorGray(const PdfColor&)
{
    return PdfColor(0.0, 1.0, 0.0);
}

PdfColor DummyConverter::SetNonStrokingColorRGB(const PdfColor&)
{
    return PdfColor(0.0, 1.0, 0.0);
}

PdfColor DummyConverter::SetNonStrokingColorCMYK(const PdfColor&)
{
    return PdfColor(0.0, 1.0, 0.0);
}
